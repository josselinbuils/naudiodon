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

#include "GetAPIList.h"
#include <portaudio.h>

namespace streampunk {

// Public

NAN_METHOD(GetAPIList) {
	PaError errCode = Pa_Initialize();

	if (errCode != paNoError) {
		std::string err = std::string("Could not initialize PortAudio: ") + Pa_GetErrorText(errCode);
		Nan::ThrowError(err.c_str());
	}

	int apiCount = Pa_GetHostApiCount();
	int defaultApiIndex = Pa_GetDefaultHostApi();
	v8::Local<v8::Array> result = Nan::New<v8::Array>(apiCount);

	for (int apiIndex = 0; apiIndex < apiCount; apiIndex++) {
		const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(apiIndex);
		v8::Local<v8::Object> v8ApiInfo = Nan::New<v8::Object>();

		Nan::Set(
			v8ApiInfo,
			Nan::New("id").ToLocalChecked(),
			Nan::New(apiIndex)
			);

		Nan::Set(
			v8ApiInfo,
			Nan::New("default").ToLocalChecked(),
			Nan::New(apiIndex == defaultApiIndex)
			);

		Nan::Set(
			v8ApiInfo,
			Nan::New("name").ToLocalChecked(),
			Nan::New(apiInfo->name).ToLocalChecked()
			);

		Nan::Set(result, apiIndex, v8ApiInfo);
	}

	Pa_Terminate();
	info.GetReturnValue().Set(result);
}

} // namespace streampunk
