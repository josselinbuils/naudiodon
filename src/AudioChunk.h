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

#ifndef AUDIOCHUNK_H
#define AUDIOCHUNK_H

#include "Memory.h"
#include "Persist.h"
#include <nan.h>

using namespace v8;

namespace streampunk {

class AudioChunk {
  public:
  AudioChunk (Local<Object> chunk)
    : mPersistentChunk(new Persist(chunk)),
      mChunk(Memory::makeNew((uint8_t *)node::Buffer::Data(chunk), (uint32_t)node::Buffer::Length(chunk))) {}

  ~AudioChunk() {}

  std::shared_ptr<Memory> chunk() const { return mChunk; }

  private:
  std::unique_ptr<Persist> mPersistentChunk;
  std::shared_ptr<Memory> mChunk;
};

} // namespace streampunk

#endif
