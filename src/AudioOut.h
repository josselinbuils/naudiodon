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

#ifndef AUDIOOUT_H
#define AUDIOOUT_H

#include "AudioChunk.h"
#include "OutContext.h"
#include <nan.h>

using namespace v8;

namespace streampunk {

class AudioOut : public Nan::ObjectWrap {
  public:
  std::shared_ptr<OutContext> getContext() const { return outContext; }
  static NAN_MODULE_INIT(Init);

  private:
  explicit AudioOut(v8::Local<v8::Object> options);

  ~AudioOut();

  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> my_constructor;
    return my_constructor;
  }

  static NAN_METHOD(New);
  static NAN_METHOD(Pause);
  static NAN_METHOD(Quit);
  static NAN_METHOD(Start);
  static NAN_METHOD(Write);

  std::shared_ptr<OutContext> outContext;
};

} // namespace streampunk

#endif
