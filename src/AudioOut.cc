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

#include "AudioOptions.h"
#include "AudioOut.h"
#include "OutWorker.h"
#include <portaudio.h>

namespace streampunk {

AudioOut::AudioOut(Local<Object> options) {
  outContext = std::make_shared<OutContext>(std::make_shared<AudioOptions>(options));
}

AudioOut::~AudioOut() {}

NAN_MODULE_INIT(AudioOut::Init) {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
  tpl->SetClassName(Nan::New("AudioOut").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  SetPrototypeMethod(tpl, "start", Start);
  SetPrototypeMethod(tpl, "pause", Pause);
  SetPrototypeMethod(tpl, "write", Write);
  SetPrototypeMethod(tpl, "quit", Quit);

  constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());

  Nan::Set(
    target,
    Nan::New("AudioOut").ToLocalChecked(),
    Nan::GetFunction(tpl).ToLocalChecked()
  );
}

NAN_METHOD(AudioOut::New) {
  if (info.IsConstructCall()) {

    if (info.Length() != 1 || !info[0]->IsObject()) {
      return Nan::ThrowError("AudioOut constructor requires a valid options object as the parameter");
    }

    v8::Local<v8::Object> options = v8::Local<v8::Object>::Cast(info[0]);
    AudioOut *audioOut = new AudioOut(options);
    audioOut->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  } else {
    const int argc = 1;
    v8::Local<v8::Value> argv[] = { info[0] };
    v8::Local<v8::Function> cons = Nan::New(constructor());
    info.GetReturnValue().Set(cons->NewInstance(Nan::GetCurrentContext(), argc, argv).ToLocalChecked());
  }
}

NAN_METHOD(AudioOut::Pause) {
  AudioOut* audioOut = Nan::ObjectWrap::Unwrap<AudioOut>(info.Holder());
  audioOut->getContext()->stop();
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(AudioOut::Quit) {
  Local<Function> callback = Local<Function>::Cast(info[0]);

  Pa_Terminate();
  (new Nan::Callback(callback))->Call(0, NULL);

  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(AudioOut::Start) {
  AudioOut* audioOut = Nan::ObjectWrap::Unwrap<AudioOut>(info.Holder());
  audioOut->getContext()->start();
  info.GetReturnValue().SetUndefined();
}

NAN_METHOD(AudioOut::Write) {

  if (info.Length() != 2) {
    return Nan::ThrowError("AudioOut.write requires 2 arguments");
  }

  if (!info[0]->IsObject()) {
    return Nan::ThrowError("AudioOut.write requires a valid chunk buffer as first parameter");
  }

  if (!info[1]->IsFunction()) {
    return Nan::ThrowError("AudioOut.write requires a valid callback as second parameter");
  }

  Local<Object> chunkObj = Local<Object>::Cast(info[0]);
  Local<Function> callback = Local<Function>::Cast(info[1]);
  AudioOut* obj = Nan::ObjectWrap::Unwrap<AudioOut>(info.Holder());

  AsyncQueueWorker(new OutWorker(obj->getContext(), new Nan::Callback(callback), std::make_shared<AudioChunk>(chunkObj)));
  info.GetReturnValue().SetUndefined();
}

} // namespace streampunk
