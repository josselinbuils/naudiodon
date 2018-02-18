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

const {inherits} = require('util');
const {Writable} = require('stream');
const {AudioOut, getDevices} = require('bindings')('naudiodon.node');

exports.getDevices = getDevices;

function AudioOutput(options) {

  if (!(this instanceof AudioOutput)) {
    return new AudioOutput(options);
  }

  let active = true;

  try {
    this.AudioOutAdon = new AudioOut(options);
  } catch (error) {
    active = false;
    throw error;
  }

  Writable.call(this, {
    highWaterMark: 16384,
    decodeStrings: false,
    objectMode: false,
    write: (chunk, encoding, cb) => this.AudioOutAdon.write(chunk, cb)
  });

  this.isActive = () => active;

  this.start = () => this.AudioOutAdon.start();

  this.stop = () => {
    active = false;
    return new Promise(resolve => this.AudioOutAdon.quit(resolve));
  };

  this.on('finish', () => active && this.stop());
}

inherits(AudioOutput, Writable);
exports.AudioOutput = AudioOutput;
