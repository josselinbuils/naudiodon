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

  this.AudioOutAdon = new AudioOut(options);

  Writable.call(this, {
    highWaterMark: 16384,
    decodeStrings: false,
    objectMode: false,
    write: (chunk, encoding, cb) => this.AudioOutAdon.write(chunk, cb)
  });

  this.start = () => this.AudioOutAdon.start();

  // TODO Close only the stream instead of destroying all PortAudio context
  this.stop = () => {
    return new Promise((resolve, reject) => {
      if (active) {
        active = false;
        this.AudioOutAdon.quit(resolve);
      } else {
        reject(new Error('AudioOutput inactive'));
      }
    }).then(() => this.emit('stopped'));
  };

  // Triggered by readable stream
  this.on('finish', () => {
    active && this.stop();
  });
}

inherits(AudioOutput, Writable);
exports.AudioOutput = AudioOutput;
