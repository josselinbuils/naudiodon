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

#ifndef OUTCONTEXT_H
#define OUTCONTEXT_H

#include "AudioOptions.h"
#include "ChunkQueue.h"
#include "Memory.h"
#include <mutex>
#include <condition_variable>
#include <portaudio.h>

namespace streampunk {

class OutContext {
  public:

  OutContext(std::shared_ptr<AudioOptions> audioOptions);

  ~OutContext();

  void addChunk(std::shared_ptr<AudioChunk> audioChunk);
  void checkStatus(uint32_t statusFlags);
  bool fillBuffer(void *buf, uint32_t frameCount);
  bool getErrStr(std::string& errStr);
  void start();
  void stop();

  private:
  std::shared_ptr<AudioOptions> audioOptions;
  ChunkQueue<std::shared_ptr<AudioChunk> > chunkQueue;
  std::shared_ptr<AudioChunk> curChunk;
  PaStream *stream;
  uint32_t curOffset;
  bool active;
  bool finished;
  std::string errorString;
  mutable std::mutex m;
  std::condition_variable cv;

  uint32_t doCopy(std::shared_ptr<Memory> chunk, void *dst, uint32_t numBytes);
  bool isActive() const;
};

} // namespace streampunk

#endif
