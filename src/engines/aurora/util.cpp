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

/** @file engines/aurora/util.cpp
 *  Generic Aurora engines utility functions.
 */

#include "common/error.h"
#include "common/stream.h"
#include "common/file.h"
#include "common/configman.h"

#include "../../aurora/util.h"
#include "../../aurora/resman.h"
#include "../../aurora/gfffile.h"
#include "../../aurora/2dafile.h"

#include "graphics/aurora/texture.h"

#include "sound/sound.h"

#include "video/aurora/videoplayer.h"

#include "events/events.h"

#include "engines/aurora/util.h"

namespace Engines {

void playVideo(const Common::UString &video) {
	if (ConfigMan.getBool("skipvideos", false))
		return;

	// Mute other sound sources
	SoundMan.setTypeGain(Sound::kSoundTypeMusic, 0.0);
	SoundMan.setTypeGain(Sound::kSoundTypeSFX  , 0.0);
	SoundMan.setTypeGain(Sound::kSoundTypeVoice, 0.0);

	try {
		Video::Aurora::VideoPlayer videoPlayer(video);

		videoPlayer.play();
	} catch (Common::Exception &e) {
		Common::printException(e, "WARNING: ");
	}

	// Restore volumes
	SoundMan.setTypeGain(Sound::kSoundTypeMusic, ConfigMan.getDouble("volume_music", 1.0));
	SoundMan.setTypeGain(Sound::kSoundTypeSFX  , ConfigMan.getDouble("volume_sfx"  , 1.0));
	SoundMan.setTypeGain(Sound::kSoundTypeVoice, ConfigMan.getDouble("volume_voice", 1.0));
}

Sound::ChannelHandle playSound(const Common::UString &sound, Sound::SoundType soundType,
		bool loop, float volume, bool pitchVariance) {

	Aurora::ResourceType resType =
		(soundType == Sound::kSoundTypeMusic) ? Aurora::kResourceMusic : Aurora::kResourceSound;

	Sound::ChannelHandle channel;

	try {
		Common::SeekableReadStream *soundStream = ResMan.getResource(resType, sound);
		if (!soundStream)
			return channel;

		channel = SoundMan.playSoundFile(soundStream, soundType, loop);

		SoundMan.setChannelGain(channel, volume);

		if (pitchVariance) {
			const float pitch = 1.0 + ((((std::rand() % 1001) / 1000.0) / 5.0) - 0.1);

			SoundMan.setChannelPitch(channel, pitch);
		}

		SoundMan.startChannel(channel);

	} catch (Common::Exception &e) {
		Common::printException(e, "WARNING: ");
	}

	return channel;
}

void checkConfigInt(const Common::UString &key, int min, int max, int def) {
	int value = ConfigMan.getInt(key, def);
	if ((value >= min) && (value <= max))
		return;

	warning("Config \"%s\" has invalid value (%d), resetting to default (%d)",
			key.c_str(), value, def);
	ConfigMan.setInt(key, def);
}

void checkConfigDouble(const Common::UString &key, double min, double max, double def) {
	double value = ConfigMan.getDouble(key, def);
	if ((value >= min) && (value <= max))
		return;

	warning("Config \"%s\" has invalid value (%lf), resetting to default (%lf)",
			key.c_str(), value, def);
	ConfigMan.setDouble(key, def);
}

bool longDelay(uint32 ms) {
	while ((ms > 0) && !EventMan.quitRequested()) {
		uint32 delay = MIN<uint32>(ms, 10);

		EventMan.delay(delay);

		ms -= delay;
	}

	return EventMan.quitRequested();
}

bool dumpResList(const Common::UString &name) {
	try {

		ResMan.dumpResourcesList(name);
		return true;

	} catch (...) {
	}

	return false;
}

bool dumpStream(Common::SeekableReadStream &stream, const Common::UString &fileName) {
	Common::DumpFile file;
	if (!file.open(fileName))
		return false;

	uint32 pos = stream.pos();

	stream.seek(0);

	file.writeStream(stream);
	file.flush();

	bool error = file.err();

	stream.seek(pos);
	file.close();

	return !error;
}

bool dumpResource(const Common::UString &name, Aurora::FileType type, Common::UString file) {
	Common::SeekableReadStream *res = ResMan.getResource(name, type);
	if (!res)
		return false;

	if (file.empty())
		file = Aurora::setFileType(name, type);

	bool success = dumpStream(*res, file);

	delete res;

	return success;
}

bool dumpResource(const Common::UString &name, const Common::UString &file) {
	Aurora::FileType type = Aurora::getFileType(name);

	return dumpResource(Aurora::setFileType(name, Aurora::kFileTypeNone), type, file);
}

bool dumpTGA(const Common::UString &name) {
	try {

		Graphics::Aurora::Texture texture(name);

		return texture.dumpTGA(name + ".tga");

	} catch (...) {
	}

	return false;
}

bool dump2DA(const Common::UString &name) {
	Common::SeekableReadStream *twoDAFile = 0;
	bool success = false;

	try {

		if (!(twoDAFile = ResMan.getResource(name, Aurora::kFileType2DA)))
			return false;

		Aurora::TwoDAFile twoda;

		twoda.load(*twoDAFile);

		success = twoda.dumpASCII(name + ".2da");

	} catch (...) {
	}

	delete twoDAFile;
	return success;
}

} // End of namespace Engines
