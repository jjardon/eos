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

/** @file graphics/images/txitypes.cpp
 *  Texture information types.
 */

#include "common/stringmap.h"
#include "common/ustring.h"

#include "graphics/images/txitypes.h"

static const char *kTXICommands[] = {
	"alphamean",
	"arturoheight",
	"arturowidth",
	"baselineheight",
	"blending",
	"bumpmapscaling",
	"bumpmaptexture",
	"bumpyshinytexture",
	"candownsample",
	"caretindent",
	"channelscale",
	"channeltranslate",
	"clamp",
	"codepage",
	"cols",
	"compresstexture",
	"controllerscript",
	"cube",
	"dbmapping",
	"decal",
	"defaultbpp",
	"defaultheight",
	"defaultwidth",
	"distort",
	"distortangle",
	"distortionamplitude",
	"downsamplefactor",
	"downsamplemax",
	"downsamplemin",
	"envmaptexture",
	"filerange",
	"filter",
	"fontheight",
	"fontwidth",
	"fps",
	"isbumpmap",
	"isdoublebyte",
	"islightmap",
	"lowerrightcoords",
	"maxSizeHQ",
	"maxSizeLQ",
	"minSizeHQ",
	"minSizeLQ",
	"mipmap",
	"numchars",
	"numcharspersheet",
	"numx",
	"numy",
	"ondemand",
	"priority",
	"proceduretype",
	"rows",
	"spacingB",
	"spacingR",
	"speed",
	"temporary",
	"texturewidth",
	"unique",
	"upperleftcoords",
	"waterheight",
	"waterwidth",
	"xbox_downsample"
};

static Common::StringListMap kTXICommandMap(kTXICommands, ARRAYSIZE(kTXICommands), true);

namespace Graphics {

TXICommand parseTXICommand(const Common::UString &str, int &skip) {
	const char *s = str.c_str();

	TXICommand command = (TXICommand) kTXICommandMap.find(s, &s);

	skip = s - str.c_str();

	return command;
}

} // End of namespace Graphics
