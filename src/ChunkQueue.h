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

#ifndef CHUNKQUEUE_H
#define CHUNKQUEUE_H

#include <nan.h>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace streampunk {

template <class T>
class ChunkQueue {
  public:
  ChunkQueue(uint32_t inputMaxQueue) : active(true), maxQueue(inputMaxQueue), qu(), m(), cv() {}

  ~ChunkQueue() {}
  
  T dequeue() {
    std::unique_lock<std::mutex> lk(m);
    while(active && qu.empty()) {
      cv.wait(lk);
    }
    T val;
    if (active) {
      val = qu.front();
      qu.pop();
      cv.notify_one();
    }
    return val;
  }

  size_t size() const {
    std::lock_guard<std::mutex> lk(m);
    return qu.size();
  }

  void enqueue(T t) {
    std::unique_lock<std::mutex> lk(m);
    while(active && (qu.size() >= maxQueue)) {
      cv.wait(lk);
    }
    qu.push(t);
    cv.notify_one();
  }

  void quit() {
    std::lock_guard<std::mutex> lk(m);
    if ((0 == qu.size()) || (qu.size() >= maxQueue)) {
      // ensure release of any blocked thread
      active = false;
      cv.notify_all();
    }
  }

  private:
  bool active;
  std::condition_variable cv;
  mutable std::mutex m;
  uint32_t maxQueue;
  std::queue<T> qu;
};

} // namespace streampunk

#endif
