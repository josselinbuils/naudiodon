/* Copyright 2017 Streampunk Media Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include "AudioChunk.h"
#include "OutContext.h"
#include "OutWorker.h"
#include "Persist.h"
#include <pa_win_wasapi.h>

using namespace v8;

namespace streampunk {

int streamCallback(const void *input, void *output, unsigned long frameCount,
                   const PaStreamCallbackTimeInfo *timeInfo,
                   PaStreamCallbackFlags statusFlags, void *userData) {

	((OutContext *) userData)->fillBuffer(output, frameCount);
	return paContinue;
}

// Public

OutContext::OutContext() : chunkQueue(), curChunk(NULL), curOffset(0) {

	PaError errCode = Pa_Initialize();

	if (errCode != paNoError) {
		std::string err = std::string("Could not initialize PortAudio: ") + Pa_GetErrorText(errCode);
		Nan::ThrowError(err.c_str());
	}
}

OutContext::~OutContext() {
	if (stream != nullptr) {
		Pa_AbortStream(stream);
	}
	Pa_Terminate();
}

void OutContext::addChunk(std::shared_ptr<AudioChunk> audioChunk) {
	chunkQueue.enqueue(audioChunk);
}

void OutContext::fillBuffer(void *buffer, uint32_t frameCount) {
	uint8_t *dst = (uint8_t *)buffer;
	uint32_t bytesRemaining = frameCount * audioOptions->getChannelCount() * audioOptions->getSampleFormat() / 8;

	while (bytesRemaining) {
		if (!curChunk || curOffset >= curChunk->getChunk()->getNumBytes()) {
			curChunk = chunkQueue.dequeue();
			curOffset = 0;

			if (!curChunk) {
				break;
			}
		}

		uint32_t bytesCopied = doCopy(curChunk->getChunk(), dst, bytesRemaining);
		bytesRemaining -= bytesCopied;
		dst += bytesCopied;
		curOffset += bytesCopied;
	}
}

bool OutContext::getErrStr(std::string &errStr) {
	errStr = errorString;
	errorString = std::string();
	return errStr != std::string();
}

NAN_MODULE_INIT(OutContext::Init) {
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
	tpl->SetClassName(Nan::New("OutContext").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	SetPrototypeMethod(tpl, "clear", Clear);
	SetPrototypeMethod(tpl, "close", Close);
	SetPrototypeMethod(tpl, "isActive", IsActive);
	SetPrototypeMethod(tpl, "isStopped", IsStopped);
	SetPrototypeMethod(tpl, "openStream", OpenStream);
	SetPrototypeMethod(tpl, "start", Start);
	SetPrototypeMethod(tpl, "stop", Stop);
	SetPrototypeMethod(tpl, "write", Write);

	constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

	Nan::Set(
		target,
		Nan::New("OutContext").ToLocalChecked(),
		Nan::GetFunction(tpl).ToLocalChecked()
		);
}

// Private

uint32_t OutContext::doCopy(std::shared_ptr<Memory> chunk, void *dst, uint32_t numBytes) {
	uint32_t curChunkBytes = chunk->getNumBytes() - curOffset;
	uint32_t thisChunkBytes = std::min<uint32_t>(curChunkBytes, numBytes);
	memcpy(dst, chunk->getBuffer() + curOffset, thisChunkBytes);
	return thisChunkBytes;
}

void OutContext::clear() {
	chunkQueue.clear();
	curChunk = NULL;
}

NAN_METHOD(OutContext::Clear) {
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());
	outContext->clear();
	info.GetReturnValue().SetUndefined();
}

void OutContext::close() {
	PaError errCode = Pa_CloseStream(stream);

	if (errCode != paNoError) {
		std::string err = std::string("Could not close output stream: ") + Pa_GetErrorText(errCode);
		Nan::ThrowError(err.c_str());
	}
}

NAN_METHOD(OutContext::Close) {
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());
	outContext->close();
	info.GetReturnValue().SetUndefined();
}

bool OutContext::isActive() {
	int res = Pa_IsStreamActive(stream);

	if (res < 0) {
		std::string err = std::string("Could not get stream state: ") + Pa_GetErrorText(res);
		Nan::ThrowError(err.c_str());
		return false;
	} else {
		return res == 1 ? true : false;
	}
}

NAN_METHOD(OutContext::IsActive) {
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());
	info.GetReturnValue().Set(outContext->isActive());
}

bool OutContext::isStopped() {
	int res = Pa_IsStreamStopped(stream);

	if (res < 0) {
		std::string err = std::string("Could not get stream state: ") + Pa_GetErrorText(res);
		Nan::ThrowError(err.c_str());
		return false;
	} else {
		return res == 1 ? true : false;
	}
}

NAN_METHOD(OutContext::IsStopped) {
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());
	info.GetReturnValue().Set(outContext->isStopped());
}

NAN_METHOD(OutContext::New) {
	if (info.IsConstructCall()) {
		OutContext *outContext = new OutContext();
		outContext->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	}
}

void OutContext::openStream(AudioOptions *audioOptions) {
	this->audioOptions = audioOptions;

	printf("Output %s\n", audioOptions->toString().c_str());

	PaStreamParameters outParams;
	memset(&outParams, 0, sizeof(PaStreamParameters));

	int apiId = audioOptions->getApiId();

	if (apiId < 0 || apiId >= Pa_GetHostApiCount()) {
		apiId = Pa_GetDefaultHostApi();
	}

	const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(apiId);
	outParams.device = apiInfo->defaultOutputDevice;

	if (outParams.device == paNoDevice) {
		Nan::ThrowError("No default output device");
	}

	const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(outParams.device);
	printf("Using output device %s with API %s\n", deviceInfo->name, apiInfo->name);

	outParams.channelCount = audioOptions->getChannelCount();

	if (outParams.channelCount > Pa_GetDeviceInfo(outParams.device)->maxOutputChannels) {
		Nan::ThrowError("Channel count exceeds maximum number of output channels for device");
	}

	uint32_t sampleFormat = audioOptions->getSampleFormat();

	switch(sampleFormat) {
	case 8: outParams.sampleFormat = paInt8; break;
	case 16: outParams.sampleFormat = paInt16; break;
	case 24: outParams.sampleFormat = paInt24; break;
	case 32: outParams.sampleFormat = paInt32; break;
	default: Nan::ThrowError("Invalid sampleFormat");
	}

	outParams.suggestedLatency = Pa_GetDeviceInfo(outParams.device)->defaultLowOutputLatency;

	struct PaWasapiStreamInfo wasapiInfo;

	if (Pa_GetHostApiInfo(Pa_GetDeviceInfo(outParams.device)->hostApi)->type == paWASAPI) {
		printf("Wasapi device detected\n");
		wasapiInfo.size = sizeof(PaWasapiStreamInfo);
		wasapiInfo.hostApiType = paWASAPI;
		wasapiInfo.version = 1;
		wasapiInfo.flags = (paWinWasapiExclusive|paWinWasapiThreadPriority);
		wasapiInfo.threadPriority = eThreadPriorityProAudio;
		outParams.hostApiSpecificStreamInfo = (&wasapiInfo);
	} else {
		outParams.hostApiSpecificStreamInfo = NULL;
	}

	double sampleRate = (double)audioOptions->getSampleRate();
	uint32_t framesPerBuffer = paFramesPerBufferUnspecified;

  #ifdef __arm__
	framesPerBuffer = 256;
	outParams.suggestedLatency = Pa_GetDeviceInfo(outParams.device)->defaultHighOutputLatency;
  #endif

	PaError errCode = Pa_OpenStream(
		&stream, NULL, &outParams, sampleRate, framesPerBuffer, paNoFlag,
		&streamCallback, this
		);

	if (errCode != paNoError) {
		std::string err = std::string("Could not open stream: ") + Pa_GetErrorText(errCode);
		Nan::ThrowError(err.c_str());
	}
}

NAN_METHOD(OutContext::OpenStream) {
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());

	if (info.Length() != 1 || !info[0]->IsObject()) {
		return Nan::ThrowError("Invalid options object");
	}

	v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[0]);
	outContext->openStream(new AudioOptions(options));
}

void OutContext::start() {
	PaError errCode = Pa_StartStream(stream);

	if (errCode != paNoError) {
		std::string err = std::string("Could not start output stream: ") + Pa_GetErrorText(errCode);
		Nan::ThrowError(err.c_str());
	}
}

NAN_METHOD(OutContext::Start) {
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());
	outContext->start();
	info.GetReturnValue().SetUndefined();
}

void OutContext::stop() {
	PaError errCode = Pa_StopStream(stream);

	if (errCode != paNoError) {
		std::string err = std::string("Could not stop output stream: ") + Pa_GetErrorText(errCode);
		Nan::ThrowError(err.c_str());
	}
}

NAN_METHOD(OutContext::Stop) {
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());
	outContext->stop();
	info.GetReturnValue().SetUndefined();
}

NAN_METHOD(OutContext::Write) {

	if (info.Length() != 2) {
		return Nan::ThrowError("Requires 2 arguments");
	}

	if (!info[0]->IsObject()) {
		return Nan::ThrowError("First parameter is not a valid chunk buffer");
	}

	if (!info[1]->IsFunction()) {
		return Nan::ThrowError("Second parameter is not a valid callback");
	}

	Local<Object> chunkObj = Local<Object>::Cast(info[0]);
	Local<Function> callback = Local<Function>::Cast(info[1]);
	OutContext *outContext = Nan::ObjectWrap::Unwrap<OutContext>(info.Holder());

	AsyncQueueWorker(new OutWorker(outContext,new Nan::Callback(callback), std::make_shared<AudioChunk>(chunkObj)));
	info.GetReturnValue().SetUndefined();
}

} // namespace streampunk
