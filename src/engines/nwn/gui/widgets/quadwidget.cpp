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

/** @file engines/nwn/gui/widgets/quadwidget.cpp
 *  A NWN quad widget.
 */

#include "common/util.h"
#include "common/ustring.h"

#include "graphics/aurora/guiquad.h"

#include "engines/nwn/gui/widgets/quadwidget.h"

namespace Engines {

namespace NWN {

QuadWidget::QuadWidget(::Engines::GUI &gui, const Common::UString &tag,
                       const Common::UString &texture,
                       float  x1, float  y1, float  x2, float  y2,
                       float tX1, float tY1, float tX2, float tY2) :
	NWNWidget(gui, tag) {

	_quad = new Graphics::Aurora::GUIQuad(texture, x1, y1, x2, y2, tX1, tY1, tX2, tY2);
	_quad->setTag(tag);
	_quad->setClickable(true);

	_width  = ABS(x2 - x1);
	_height = ABS(y2 - y1);
}

QuadWidget::~QuadWidget() {
	delete _quad;
}

void QuadWidget::show() {
	_quad->show();
}

void QuadWidget::hide() {
	_quad->hide();
}

void QuadWidget::setPosition(float x, float y, float z) {
	NWNWidget::setPosition(x, y, z);

	getPosition(x, y, z);
	_quad->setPosition(x, y, z);
}

void QuadWidget::setColor(float r, float g, float b, float a) {
	_quad->setColor(r, g, b, a);
}

void QuadWidget::setTexture(const Common::UString &texture) {
	_quad->setTexture(texture);
}

void QuadWidget::setWidth(float w) {
	_quad->setWidth(w);
}

void QuadWidget::setHeight(float h) {
	_quad->setHeight(h);
}

float QuadWidget::getWidth() const {
	return _width;
}

float QuadWidget::getHeight() const {
	return _height;
}

} // End of namespace NWN

} // End of namespace Engines
