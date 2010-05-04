/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file common/stringmap.h
 *  A map to quickly match strings from a list.
 */

#ifndef COMMON_STRWORDMAP_H
#define COMMON_STRWORDMAP_H

#include "boost/unordered/unordered_map.hpp"

#include "common/ustring.h"

namespace Common {

class StringMap {
public:
	/** Build a string map to match a list of strings against. */
	StringMap(const char **strings, int count, bool onlyFirstWord = false);

	/** Match a string against the map.
	 *
	 *  @param  str The string to match against.
	 *  @param  match If != 0, the position after the match will be stored here.
	 *  @return The index of the matched string in the original list, or -1 if not found.
	 */
	int find(const char *str, const char **match) const;

	/** Match a string against the map.
	 *
	 *  @param  str The string to match against.
	 *  @param  match If != 0, the position after the match will be stored here.
	 *  @return The index of the matched string in the original list, or -1 if not found.
	 */
	int find(const Common::UString &str, const char **match) const;

private:
	bool _onlyFirstWord;

	typedef boost::unordered_map<Common::UString, int, Common::hashUStringCaseInsensitive> StrMap;

	StrMap _map;

};

} // End of namespace Common

#endif // COMMON_STRWORDMAP_H