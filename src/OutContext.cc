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
#include "Persist.h"
#include <nan.h>
#include <pa_win_wasapi.h>

using namespace v8;

namespace streampunk {

  OutContext::OutContext(std::shared_ptr<AudioOptions> audioOptions, PaStreamCallback *cb)
    : mAudioOptions(audioOptions), mChunkQueue(mAudioOptions->maxQueue()),
      mCurOffset(0), mActive(true), mFinished(false) {

    PaError errCode = Pa_Initialize();

    if (errCode != paNoError) {
      std::string err = std::string("Could not initialize PortAudio: ") + Pa_GetErrorText(errCode);
      Nan::ThrowError(err.c_str());
    }

    printf("Output %s\n", mAudioOptions->toString().c_str());

    PaStreamParameters outParams;
    memset(&outParams, 0, sizeof(PaStreamParameters));

    int32_t deviceID = (int32_t)mAudioOptions->deviceID();

    if (deviceID >= 0 && deviceID < Pa_GetDeviceCount()) {
      outParams.device = (PaDeviceIndex)deviceID;
    } else {
      outParams.device = Pa_GetDefaultOutputDevice();
    }

    if (outParams.device == paNoDevice) {
      Nan::ThrowError("No default output device");
    }

    printf("Output device name is %s\n", Pa_GetDeviceInfo(outParams.device)->name);

    outParams.channelCount = mAudioOptions->channelCount();

    if (outParams.channelCount > Pa_GetDeviceInfo(outParams.device)->maxOutputChannels) {
      Nan::ThrowError("Channel count exceeds maximum number of output channels for device");
    }

    uint32_t sampleFormat = mAudioOptions->sampleFormat();

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
      printf("sapi device detected\n");
      wasapiInfo.size = sizeof(PaWasapiStreamInfo);
      wasapiInfo.hostApiType = paWASAPI;
      wasapiInfo.version = 1;
      wasapiInfo.flags = (paWinWasapiExclusive|paWinWasapiThreadPriority);
      wasapiInfo.threadPriority = eThreadPriorityProAudio;
      outParams.hostApiSpecificStreamInfo = (&wasapiInfo);
    } else {
      outParams.hostApiSpecificStreamInfo = NULL;
    }

    double sampleRate = (double)mAudioOptions->sampleRate();
    uint32_t framesPerBuffer = paFramesPerBufferUnspecified;

    #ifdef __arm__
      framesPerBuffer = 256;
      outParams.suggestedLatency = Pa_GetDeviceInfo(outParams.device)->defaultHighOutputLatency;
    #endif

    errCode = Pa_OpenStream(&mStream, NULL, &outParams, sampleRate, framesPerBuffer, paNoFlag, cb, this);

    if (errCode != paNoError) {
      std::string err = std::string("Could not open stream: ") + Pa_GetErrorText(errCode);
      Nan::ThrowError(err.c_str());
    }
  }

  OutContext::~OutContext() {
    Pa_StopStream(mStream);
    Pa_Terminate();
  }

  void OutContext::start() {
    PaError errCode = Pa_StartStream(mStream);

    if (errCode != paNoError) {
      std::string err = std::string("Could not start output stream: ") + Pa_GetErrorText(errCode);
      return Nan::ThrowError(err.c_str());
    }
  }

  void OutContext::stop() {
    PaError errCode = Pa_StopStream(mStream);

    if (errCode != paNoError) {
      std::string err = std::string("Could not stop output stream: ") + Pa_GetErrorText(errCode);
      return Nan::ThrowError(err.c_str());
    }
  }

  void OutContext::addChunk(std::shared_ptr<AudioChunk> audioChunk) {
    mChunkQueue.enqueue(audioChunk);
  }

  bool OutContext::fillBuffer(void *buf, uint32_t frameCount) {
    uint8_t *dst = (uint8_t *)buf;
    uint32_t bytesRemaining = frameCount * mAudioOptions->channelCount() * mAudioOptions->sampleFormat() / 8;

    uint32_t active = isActive();

    if (
      !active &&
      (0 == mChunkQueue.size()) &&
      (!mCurChunk || (mCurChunk && (bytesRemaining >= mCurChunk->chunk()->numBytes() - mCurOffset)))
    ) {
      if (mCurChunk) {
        uint32_t bytesCopied = doCopy(mCurChunk->chunk(), dst, bytesRemaining);
        uint32_t missingBytes = bytesRemaining - bytesCopied;

        if (missingBytes > 0) {
          printf("Finishing - %d bytes not available for the last output buffer\n", missingBytes);
          memset(dst + bytesCopied, 0, missingBytes);
        }
      }
      std::lock_guard<std::mutex> lk(m);
      mFinished = true;
      cv.notify_one();
    } else {
      while (bytesRemaining) {
        if (!(mCurChunk && (mCurOffset < mCurChunk->chunk()->numBytes()))) {
          mCurChunk = mChunkQueue.dequeue();
          mCurOffset = 0;
        }
        if (mCurChunk) {
          uint32_t bytesCopied = doCopy(mCurChunk->chunk(), dst, bytesRemaining);
          bytesRemaining -= bytesCopied;
          dst += bytesCopied;
          mCurOffset += bytesCopied;
        } else { // Deal with termination case of ChunkQueue being kicked and returning null chunk
          std::lock_guard<std::mutex> lk(m);
          mFinished = true;
          cv.notify_one();
          break;
        }
      }
    }

    return !mFinished;
  }

  void OutContext::checkStatus(uint32_t statusFlags) {
    if (statusFlags) {
      std::string err = std::string("portAudio status - ");

      if (statusFlags & paOutputUnderflow) {
        err += "output underflow ";
      } else if (statusFlags & paOutputOverflow) {
        err += "output overflow ";
      } else if (statusFlags & paPrimingOutput) {
        err += "priming output ";
      }

      std::lock_guard<std::mutex> lk(m);
      mErrStr = err;
    }
  }

  bool OutContext::getErrStr(std::string& errStr) {
    std::lock_guard<std::mutex> lk(m);
    errStr = mErrStr;
    mErrStr = std::string();
    return errStr != std::string();
  }

  bool OutContext::isActive() const {
    std::unique_lock<std::mutex> lk(m);
    return mActive;
  }

  uint32_t OutContext::doCopy(std::shared_ptr<Memory> chunk, void *dst, uint32_t numBytes) {
    uint32_t curChunkBytes = chunk->numBytes() - mCurOffset;
    uint32_t thisChunkBytes = std::min<uint32_t>(curChunkBytes, numBytes);
    memcpy(dst, chunk->buf() + mCurOffset, thisChunkBytes);
    return thisChunkBytes;
  }

} // namespace streampunk
