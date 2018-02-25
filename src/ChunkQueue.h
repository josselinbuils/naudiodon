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
ChunkQueue(uint32_t inputMaxQueue)
	: active(true), maxQueue(inputMaxQueue), queue(), mutex(), conditionVariable() {
}

~ChunkQueue() {
}

void enqueue(T t) {
	std::unique_lock<std::mutex> lk(mutex);

	while(active && queue.size() >= maxQueue) {
		conditionVariable.wait(lk);
	}

	queue.push(t);
	conditionVariable.notify_one();
}

T dequeue() {
	std::unique_lock<std::mutex> lk(mutex);

	while(active && queue.empty()) {
		conditionVariable.wait(lk);
	}

	T val;
	if (active) {
		val = queue.front();
		queue.pop();
		conditionVariable.notify_one();
	}
	return val;
}

void enqueue(T t) {
	std::unique_lock<std::mutex> lk(mutex);

	while(active && queue.size() >= maxQueue) {
		conditionVariable.wait(lk);
	}

	queue.push(t);
	conditionVariable.notify_one();
}

size_t size() const {
	std::lock_guard<std::mutex> lk(mutex);
	return queue.size();
}

private:
bool active;
std::condition_variable conditionVariable;
mutable std::mutex mutex;
uint32_t maxQueue;
std::queue<T> queue;
};

} // namespace streampunk

#endif
