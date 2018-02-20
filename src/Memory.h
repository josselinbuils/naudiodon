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

#ifndef MEMORY_H
#define MEMORY_H

#include <memory>

namespace streampunk {

class Memory {
  public:
  static std::shared_ptr<Memory> makeNew(uint32_t srcBytes) {
    return std::make_shared<Memory>(srcBytes);
  }

  static std::shared_ptr<Memory> makeNew(uint8_t *buf, uint32_t srcBytes) {
    return std::make_shared<Memory>(buf, srcBytes);
  }

  Memory(uint32_t nBytes): ownAlloc(true), numBytes(nBytes), buffer(new uint8_t[numBytes]) {}

  Memory(uint8_t *buf, uint32_t nBytes): ownAlloc(false), numBytes(nBytes), buffer(buf) {}

  ~Memory() {
    if (ownAlloc) {
      delete[] buffer;
    }
  }

  uint8_t *getBuffer() const { return buffer; }
  uint32_t getNumBytes() const { return numBytes; }

  private:
  uint8_t *const buffer;
  const uint32_t numBytes;
  const bool ownAlloc;
};

} // namespace streampunk

#endif
