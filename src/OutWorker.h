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

namespace streampunk {

class OutWorker : public Nan::AsyncWorker {
  public:
  OutWorker(std::shared_ptr<OutContext> OutContext, Nan::Callback *callback, std::shared_ptr<AudioChunk> audioChunk)
    : AsyncWorker(callback), outContext(OutContext), audioChunk(audioChunk) {}

  ~OutWorker() {}

  void Execute() {
    outContext->addChunk(audioChunk);
  }

  void HandleOKCallback () {
    Nan::HandleScope scope;
    std::string errStr;

    if (outContext->getErrStr(errStr)) {
      Local<Value> argv[] = { Nan::Error(errStr.c_str()) };
      callback->Call(1, argv);
    } else {
      callback->Call(0, NULL);
    }
  }

  private:
  std::shared_ptr<OutContext> outContext;
  std::shared_ptr<AudioChunk> audioChunk;
};

} // namespace streampunk
