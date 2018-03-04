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

const {
  inherits
} = require('util');
const {
  Writable
} = require('stream');
const {
  getAPIList,
  OutContext
} = require('bindings')('naudiodon.node');

inherits(AudioOutput, Writable);

exports.AudioOutput = AudioOutput;
exports.getAPIList = getAPIList;

function AudioOutput(options) {

  if (!(this instanceof AudioOutput)) {
    return new AudioOutput(options);
  }

  Writable.call(this, {
    highWaterMark: 16384,
    decodeStrings: false,
    objectMode: false,
    write: (chunk, encoding, cb) => outContext.write(chunk, cb),
  });

  this.options = options;

  Object.defineProperty(this, 'options', {
    get: () => Object.assign({}, options)
  });

  const outContext = new OutContext();
  outContext.openStream(options);

  this.clear = () => outContext.clear();
  this.close = () => outContext.close();
  this.isActive = () => outContext.isActive();
  this.isStopped = () => outContext.isStopped();
  this.start = () => outContext.start();
  this.stop = () => outContext.stop();
}
