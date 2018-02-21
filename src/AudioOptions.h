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

#ifndef AUDIOOPTIONS_H
#define AUDIOOPTIONS_H

#include "Params.h"
#include <nan.h>
#include <sstream>

using namespace v8;

namespace streampunk {

class AudioOptions : public Params {
public:
AudioOptions(Local<Object> tags)
	: deviceID(unpackNum(tags, "deviceId", 0xffffffff)),
	sampleRate(unpackNum(tags, "sampleRate", 44100)),
	channelCount(unpackNum(tags, "channelCount", 2)),
	sampleFormat(unpackNum(tags, "sampleFormat", 8)),
	maxQueue(unpackNum(tags, "maxQueue", 2)) {
}

~AudioOptions() {
}

uint32_t getChannelCount() const {
	return channelCount;
}
uint32_t getDeviceID() const {
	return deviceID;
}
uint32_t getMaxQueue() const {
	return maxQueue;
}
uint32_t getSampleFormat() const {
	return sampleFormat;
}
uint32_t getSampleRate() const {
	return sampleRate;
}

std::string toString() const {
	std::stringstream ss;

	ss << "audio options: ";

	if (deviceID == 0xffffffff) {
		ss << "default device, ";
	} else {
		ss << "device " << deviceID << ", ";
	}

	ss << "sample rate " << sampleRate << ", ";
	ss << "channels " << channelCount << ", ";
	ss << "bits per sample " << sampleFormat << ", ";
	ss << "max queue " << maxQueue;

	return ss.str();
}

private:
uint32_t channelCount;
uint32_t deviceID;
uint32_t maxQueue;
uint32_t sampleFormat;
uint32_t sampleRate;
};

} // namespace streampunk

#endif
