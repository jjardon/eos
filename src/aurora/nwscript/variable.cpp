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

/** @file aurora/nwscript/variable.cpp
 *  NWScript variable.
 */

#include "common/error.h"

#include "aurora/nwscript/variable.h"
#include "aurora/nwscript/enginetype.h"

namespace Aurora {

namespace NWScript {

Variable::Variable(Type type) : _type(kTypeVoid) {
	setType(type);
}

Variable::Variable(int32 value) : _type(kTypeVoid) {
	setType(kTypeInt);

	*this = value;
}

Variable::Variable(float value) : _type(kTypeVoid) {
	setType(kTypeFloat);

	*this = value;
}

Variable::Variable(const Common::UString &value) : _type(kTypeVoid) {
	setType(kTypeString);

	*this = value;
}

Variable::Variable(Object *value) : _type(kTypeVoid) {
	setType(kTypeObject);

	*this = value;
}

Variable::Variable(const EngineType *value) : _type(kTypeEngineType) {
	setType(kTypeEngineType);

	*this = value;
}

Variable::Variable(const EngineType &value) : _type(kTypeEngineType) {
	setType(kTypeEngineType);

	*this = value;
}

Variable::Variable(float x, float y, float z) : _type(kTypeVoid) {
	setType(kTypeVector);

	setVector(x, y, z);
}

Variable::Variable(const Variable &var) : _type(kTypeVoid) {
	*this = var;
}

Variable::~Variable() {
	setType(kTypeVoid);
}

void Variable::setType(Type type) {
	if      (_type == kTypeString)
		delete _value._string;
	else if (_type == kTypeEngineType)
		delete _value._engineType;
	else if (_type == kTypeScriptState)
		delete _value._scriptState;

	_type = type;

	switch (_type) {
		case kTypeVoid:
			break;

		case kTypeInt:
			_value._int = 0;
			break;

		case kTypeFloat:
			_value._float = 0.0;
			break;

		case kTypeString:
			_value._string = new Common::UString;
			break;

		case kTypeObject:
			_value._object = 0;
			break;

		case kTypeVector:
			_value._vector[0] = 0.0;
			_value._vector[1] = 0.0;
			_value._vector[2] = 0.0;
			break;

		case kTypeEngineType:
			_value._engineType = 0;
			break;

		case kTypeScriptState:
			_value._scriptState = new ScriptState;
			break;

		default:
			throw Common::Exception("Variable::setType(): Invalid type %d", type);
			break;
	}
}

Variable &Variable::operator=(const Variable &var) {
	if (&var == this)
		return *this;

	setType(var._type);

	if      (_type == kTypeString)
		*_value._string = *var._value._string;
	else if (_type == kTypeEngineType)
		*this = var._value._engineType;
	else if (_type == kTypeScriptState)
		*_value._scriptState = *var._value._scriptState;
	else
		_value = var._value;

	return *this;
}

Variable &Variable::operator=(int32 value) {
	if (_type != kTypeInt)
		throw Common::Exception("Can't assign an int value to a non-int variable");

	_value._int = value;

	return *this;
}

Variable &Variable::operator=(float value) {
	if (_type != kTypeFloat)
		throw Common::Exception("Can't assign a float value to a non-float variable");

	_value._float = value;

	return *this;
}

Variable &Variable::operator=(const Common::UString &value) {
	if (_type != kTypeString)
		throw Common::Exception("Can't assign a string value to a non-string variable");

	*_value._string = value;

	return *this;
}

Variable &Variable::operator=(Object *value) {
	if (_type != kTypeObject)
		throw Common::Exception("Can't assign an object value to a non-object variable");

	_value._object = value;

	return *this;
}

Variable &Variable::operator=(const EngineType *value) {
	if (_type != kTypeEngineType)
		throw Common::Exception("Can't assign an engine-type value to a non-engine-type variable");

	EngineType *engineType = value ? value->clone() : 0;

	delete _value._engineType;

	_value._engineType = engineType;

	return *this;
}

Variable &Variable::operator=(const EngineType &value) {
	*this = &value;

	return *this;
}

bool Variable::operator==(const Variable &var) const {
	if (this == &var)
		return true;

	if (_type != var._type)
		return false;

	switch (_type) {
		case kTypeVoid:
			return true;

		case kTypeInt:
			return _value._int == var._value._int;

		case kTypeFloat:
			return _value._float == var._value._float;

		case kTypeString:
			return *_value._string == *var._value._string;

		case kTypeObject:
			return _value._object == var._value._object;

		case kTypeVector:
			return _value._vector[0] == var._value._vector[0] &&
			       _value._vector[1] == var._value._vector[1] &&
			       _value._vector[2] == var._value._vector[2];

		default:
			break;
	}

	return false;
}

bool Variable::operator!=(const Variable &var) const {
	return !(*this == var);
}

Type Variable::getType() const {
	return _type;
}

int32 Variable::getInt() const {
	if (_type != kTypeInt)
		throw Common::Exception("Can't get an int value from a non-int variable");

	return _value._int;
}

float Variable::getFloat() const {
	if (_type != kTypeFloat)
		throw Common::Exception("Can't get a float value from a non-float variable");

	return _value._float;
}

const Common::UString &Variable::getString() const {
	if (_type != kTypeString)
		throw Common::Exception("Can't get a string value from a non-string variable");

	return *_value._string;
}

Common::UString &Variable::getString() {
	if (_type != kTypeString)
		throw Common::Exception("Can't get a string value from a non-string variable");

	return *_value._string;
}

Object *Variable::getObject() const {
	if (_type != kTypeObject)
		throw Common::Exception("Can't get an object value from a non-object variable");

	return _value._object;
}

EngineType *Variable::getEngineType() const {
	if (_type != kTypeEngineType)
		throw Common::Exception("Can't get an engine-type value from a non-engine-type variable");

	return _value._engineType;
}

void Variable::setVector(float x, float y, float z) {
	if (_type != kTypeVector)
		throw Common::Exception("Can't assign a vector value to a non-vector variable");

	_value._vector[0] = x;
	_value._vector[1] = y;
	_value._vector[2] = z;
}

void Variable::getVector(float &x, float &y, float &z) const {
	if (_type != kTypeVector)
		throw Common::Exception("Can't get a vector value from a non-vector variable");

	x = _value._vector[0];
	y = _value._vector[1];
	z = _value._vector[2];
}

ScriptState &Variable::getScriptState() {
	if (_type != kTypeScriptState)
		throw Common::Exception("Can't get a script state value from a non-script-state variable");

	return *_value._scriptState;
}

const ScriptState &Variable::getScriptState() const {
	if (_type != kTypeScriptState)
		throw Common::Exception("Can't get a script state value from a non-script-state variable");

	return *_value._scriptState;
}

} // End of namespace NWScript

} // End of namespace Aurora
