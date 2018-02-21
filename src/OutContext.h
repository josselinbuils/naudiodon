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

#include "AudioChunk.h"
#include "AudioOptions.h"
#include "ChunkQueue.h"
#include "Memory.h"
#include <mutex>
#include <condition_variable>
#include <portaudio.h>

namespace streampunk {

class OutContext : public Nan::ObjectWrap {
public:
~OutContext();
void addChunk(std::shared_ptr<AudioChunk> audioChunk);
void checkStatus(uint32_t statusFlags);
bool getErrStr(std::string& errStr);
bool fillBuffer(void *buf, uint32_t frameCount);
static NAN_MODULE_INIT(Init);

private:
explicit OutContext();

static inline Nan::Persistent<v8::Function> & constructor() {
	static Nan::Persistent<v8::Function> my_constructor;
	return my_constructor;
}

bool active;
AudioOptions *audioOptions;
ChunkQueue<std::shared_ptr<AudioChunk> > chunkQueue;
std::shared_ptr<AudioChunk> curChunk;
uint32_t curOffset;
std::condition_variable cv;
std::string errorString;
bool finished;
mutable std::mutex m;
PaStream *stream;

uint32_t doCopy(std::shared_ptr<Memory> chunk, void *dst, uint32_t numBytes);
bool isActive() const;
static NAN_METHOD(IsActive);
static NAN_METHOD(New);
void openStream(AudioOptions *audioOptions);
static NAN_METHOD(OpenStream);
void pause();
static NAN_METHOD(Pause);
void start();
static NAN_METHOD(Start);
void stop();
static NAN_METHOD(Stop);
static NAN_METHOD(Write);
};

} // namespace streampunk

#endif
