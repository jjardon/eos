/* eos - A reimplementation of BioWare's Aurora engine
 *
 * eos is the legal property of its developers, whose names can be
 * found in the AUTHORS file distributed with this source
 * distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 */

// Largely based on the AudioStream implementation found in ScummVM.

/** @file sound/audiostream.cpp
 *  Streaming audio.
 */

#include <queue>

#include "common/error.h"
#include "common/mutex.h"

#include "sound/audiostream.h"

namespace Sound {

LoopingAudioStream::LoopingAudioStream(RewindableAudioStream *stream, uint loops, bool disposeAfterUse)
    : _parent(stream), _disposeAfterUse(disposeAfterUse), _loops(loops), _completeIterations(0) {
}

LoopingAudioStream::~LoopingAudioStream() {
	if (_disposeAfterUse)
		delete _parent;
}

int LoopingAudioStream::readBuffer(int16 *buffer, const int numSamples) {
	if ((_loops && _completeIterations == _loops) || !numSamples)
		return 0;

	int samplesRead = _parent->readBuffer(buffer, numSamples);

	if (_parent->endOfStream()) {
		++_completeIterations;
		if (_completeIterations == _loops)
			return samplesRead;

		const int remainingSamples = numSamples - samplesRead;

		if (!_parent->rewind()) {
			// TODO: Properly indicate error
			_loops = _completeIterations = 1;
			return samplesRead;
		}

		return samplesRead + readBuffer(buffer + samplesRead, remainingSamples);
	}

	return samplesRead;
}

bool LoopingAudioStream::endOfData() const {
	return (_loops != 0 && (_completeIterations == _loops));
}

AudioStream *makeLoopingAudioStream(RewindableAudioStream *stream, uint loops) {
	if (loops != 1)
		return new LoopingAudioStream(stream, loops);
	else
		return stream;
}

class QueuingAudioStreamImpl : public QueuingAudioStream {
private:
	/**
	 * We queue a number of (pointers to) audio stream objects.
	 * In addition, we need to remember for each stream whether
	 * to dispose it after all data has been read from it.
	 * Hence, we don't store pointers to stream objects directly,
	 * but rather StreamHolder structs.
	 */
	struct StreamHolder {
		AudioStream *_stream;
		bool _disposeAfterUse;
		StreamHolder(AudioStream *stream, bool disposeAfterUse)
		    : _stream(stream),
		      _disposeAfterUse(disposeAfterUse) {}
	};

	/**
	 * The sampling rate of this audio stream.
	 */
	const int _rate;

	/**
	 * The number of channels in this audio stream.
	 */
	const int _channels;

	/**
	 * This flag is set by the finish() method only. See there for more details.
	 */
	bool _finished;

	/**
	 * A mutex to avoid access problems (causing e.g. corruption of
	 * the linked list) in thread aware environments.
	 */
	Common::Mutex _mutex;

	/**
	 * The queue of audio streams.
	 */
	std::queue<StreamHolder> _queue;

public:
	QueuingAudioStreamImpl(int rate, int channels)
	    : _rate(rate), _channels(channels), _finished(false) {}
	~QueuingAudioStreamImpl();

	// Implement the AudioStream API
	virtual int readBuffer(int16 *buffer, const int numSamples);
	virtual int getChannels() const { return _channels; }
	virtual int getRate() const { return _rate; }
	virtual bool endOfData() const {
		//Common::StackLock lock(_mutex);
		return _queue.empty();
	}
	virtual bool endOfStream() const { return _finished && _queue.empty(); }

	// Implement the QueuingAudioStream API
	virtual void queueAudioStream(AudioStream *stream, bool disposeAfterUse);
	virtual void finish() { _finished = true; }

	uint32 numQueuedStreams() const {
		//Common::StackLock lock(_mutex);
		return _queue.size();
	}
};

QueuingAudioStreamImpl::~QueuingAudioStreamImpl() {
	while (!_queue.empty()) {
		StreamHolder tmp = _queue.front();
		_queue.pop();
		if (tmp._disposeAfterUse)
			delete tmp._stream;
	}
}

void QueuingAudioStreamImpl::queueAudioStream(AudioStream *stream, bool disposeAfterUse) {
	if (_finished)
		throw Common::Exception("QueuingAudioStreamImpl::queueAudioStream(): Trying to queue another audio stream, but the QueuingAudioStream is finished.");

	if ((stream->getRate() != getRate()) || (stream->getChannels() != getChannels()))
		throw Common::Exception("QueuingAudioStreamImpl::queueAudioStream(): stream has mismatched parameters");

	Common::StackLock lock(_mutex);
	_queue.push(StreamHolder(stream, disposeAfterUse));
}

int QueuingAudioStreamImpl::readBuffer(int16 *buffer, const int numSamples) {
	Common::StackLock lock(_mutex);
	int samplesDecoded = 0;

	while (samplesDecoded < numSamples && !_queue.empty()) {
		AudioStream *stream = _queue.front()._stream;
		samplesDecoded += stream->readBuffer(buffer + samplesDecoded, numSamples - samplesDecoded);

		if (stream->endOfData()) {
			StreamHolder tmp = _queue.front();
			_queue.pop();
			if (tmp._disposeAfterUse)
				delete stream;
		}
	}

	return samplesDecoded;
}

QueuingAudioStream *makeQueuingAudioStream(int rate, int channels) {
	return new QueuingAudioStreamImpl(rate, channels);
}

} // End of namespace Sound
