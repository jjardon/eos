/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010-2011 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file engines/kotor/modelobject.cpp
 *  An object associated with a visible model.
 */

#include "engines/kotor/modelobject.h"

namespace Engines {

namespace KotOR {

ModelObject::ModelObject() {
	_position   [0] = 0.0;
	_position   [1] = 0.0;
	_position   [2] = 0.0;
	_orientation[0] = 0.0;
	_orientation[1] = 0.0;
	_orientation[2] = 0.0;
}

ModelObject::~ModelObject() {
}

void ModelObject::setPosition(float x, float y, float z) {
	_position[0] = x;
	_position[1] = y;
	_position[2] = z;

	changedPosition();
}

void ModelObject::setOrientation(float x, float y, float z) {
	_orientation[0] = x;
	_orientation[1] = y;
	_orientation[2] = z;

	changedOrientation();
}

} // End of namespace KotOR

} // End of namespace Engines
