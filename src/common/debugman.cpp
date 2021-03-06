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

// Inspired by ScummVM's debug channels

/** @file common/debugman.cpp
 *  The debug manager, managing debug channels.
 */

#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"

#include "common/maths.h"
#include "common/util.h"
#include "common/filepath.h"
#include "common/debugman.h"

// boost-date_time stuff
using boost::posix_time::ptime;
using boost::posix_time::second_clock;

DECLARE_SINGLETON(Common::DebugManager)

namespace Common {

DebugManager::DebugManager() : _debugLevel(0) {
	for (uint32 i = 0; i < 30; i++)
		_channels[i].enabled = false;

	addDebugChannel(kDebugGraphics, "GGraphics", "Global graphics debug channel");
	addDebugChannel(kDebugSound   , "GSound"   , "Global sound debug channel");
	addDebugChannel(kDebugEvents  , "GEvents"  , "Global events debug channel");
	addDebugChannel(kDebugScripts , "GScripts" , "Global scripts debug channel");
}

DebugManager::~DebugManager() {
	closeLogFile();
}

bool DebugManager::addDebugChannel(uint32 channel, const UString &name,
                                   const UString &description) {

	if ((channel == 0) || name.empty())
		return false;

	int index = intLog2(channel);
	if ((index < 0) || (index > 30))
		return false;

	if (!_channels[index].name.empty())
		return false;

	_channels[index].name        = name;
	_channels[index].description = description;
	_channels[index].enabled     = false;

	_channelMap[name] = channel;

	return true;
}

void DebugManager::getDebugChannels(std::vector<UString> &names,
                                    std::vector<UString> &descriptions,
                                    uint32 &nameLength) const {

	names.clear();
	descriptions.clear();
	nameLength = 0;

	for (uint32 i = 0; i < 30; i++) {
		const Channel &channel = _channels[i];
		if (channel.name.empty())
			continue;

		names.push_back(channel.name);
		descriptions.push_back(channel.description);

		nameLength = MAX<uint32>(nameLength, channel.name.size());
	}
}

void DebugManager::clearEngineChannels() {
	for (uint32 i = 15; i < 30; i++) {
		Channel &channel = _channels[i];

		ChannelMap::iterator c = _channelMap.find(channel.name);
		if (c != _channelMap.end())
			_channelMap.erase(c);

		channel.name.clear();
		channel.description.clear();
		channel.enabled = false;
	}
}

uint32 DebugManager::parseChannelList(const UString &list) const {
	std::vector<UString> channels;
	UString::split(list, ',', channels);

	uint32 mask = 0;
	for (std::vector<UString>::const_iterator c = channels.begin(); c != channels.end(); ++c) {
		if (c->equalsIgnoreCase("all"))
			return 0xFFFFFFFF;

		ChannelMap::const_iterator channel = _channelMap.find(*c);
		if (channel == _channelMap.end()) {
			warning("No such debug channel \"%s\"", c->c_str());
			continue;
		}

		mask |= channel->second;
	}

	return mask;
}

void DebugManager::setEnabled(uint32 mask) {
	for (uint32 i = 0; i < 30; i++, mask >>= 1)
		_channels[i].enabled = (mask & 1) != 0;
}

bool DebugManager::isEnabled(uint32 level, uint32 channel) const {
	if (_debugLevel < level)
		return false;

	if (channel == 0)
		return false;

	int index = intLog2(channel);
	if ((index < 0) || (index > 30))
		return false;

	return _channels[index].enabled;
}

uint32 DebugManager::getDebugLevel() const {
	return _debugLevel;
}

void DebugManager::setDebugLevel(uint32 level) {
	_debugLevel = level;
}

bool DebugManager::openLogFile(const UString &file) {
	closeLogFile();

	_logFileStartLine = true;
	return _logFile.open(FilePath::makeAbsolute(file));
}

void DebugManager::closeLogFile() {
	_logFile.close();
}

void DebugManager::logString(const UString &str) {
	if (!_logFile.isOpen())
		return;

	if (_logFileStartLine) {
		ptime t(second_clock::universal_time());
		const UString tstamp = UString::sprintf("[%04d-%02d-%02dT%02d:%02d:%02d] ",
			(int) t.date().year(), (int) t.date().month(), (int) t.date().day(),
			(int) t.time_of_day().hours(), (int) t.time_of_day().minutes(),
			(int) t.time_of_day().seconds());

		_logFile.writeString(tstamp);
	}

	_logFile.writeString(str);
	_logFileStartLine = !str.empty() && (*--str.end() == '\n');
}

} // End of namespace Common
