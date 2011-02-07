/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010-2011 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file common/foxpro.cpp
 *  A database in FoxPro format.
 */

#include "common/foxpro.h"
#include "common/error.h"
#include "common/stream.h"

// boost-date_time stuff
using boost::gregorian::date;
using boost::gregorian::day_clock;

namespace Common {

FoxPro::FoxPro() : _hasIndex(false), _hasMemo(false), _memoBlockSize(512) {
}

FoxPro::~FoxPro() {
	for (std::list<byte *>::iterator it = _pool.begin(); it != _pool.end(); ++it)
		delete[] *it;
	for (std::vector<byte *>::iterator it = _memos.begin(); it != _memos.end(); ++it)
		delete[] *it;
}

void FoxPro::load(SeekableReadStream *dbf, SeekableReadStream *cdx,
                  SeekableReadStream *fpt) {

	byte version = dbf->readByte();
	if (version != 0xF5)
		throw Exception("Unknown database version 0x%02X", version);

	int lastUpdateYear   = dbf->readByte() + 2000;
	int lastUpdateMonth  = dbf->readByte();
	int lastUpdateDay    = dbf->readByte();

	_lastUpdate = date(lastUpdateYear, lastUpdateMonth, lastUpdateDay);

	uint32 recordCount = dbf->readUint32LE();

	uint32 firstRecordPos = dbf->readUint16LE();
	uint32 recordSize     = dbf->readUint16LE();

	dbf->skip(16); // Reserved

	bool flags = dbf->readByte();

	_hasIndex = flags & 0x01;
	_hasMemo  = flags & 0x02;

	if (flags & 0x04)
		throw Exception("DBC unsupported");

	if (flags & 0xF8)
		throw Exception("Unknown flags 0x%02X", flags);

	if (_hasIndex && !cdx)
		throw Exception("Index needed");

	if (_hasMemo && !fpt)
		throw Exception("Memo needed");

	dbf->skip(1); // Codepage marker
	dbf->skip(2); // Reserved

	// Read all field descriptions, 0x0D is the end marker
	uint32 fieldsLength = 0;
	while (!dbf->eos() && !dbf->err() && (dbf->readByte() != 0x0D)) {
		Field field;

		dbf->seek(-1, SEEK_CUR);

		field.name.readASCII(*dbf, 11);

		field.type     = (Type) dbf->readByte();
		field.offset   = dbf->readUint32LE();
		field.size     = dbf->readByte();
		field.decimals = dbf->readByte();

		field.flags = dbf->readByte();

		field.autoIncNext = dbf->readUint32LE();
		field.autoIncStep = dbf->readByte();

		dbf->skip(8); // Reserved

		if (field.offset != (fieldsLength + 1))
			throw Exception("Field offset makes no sense (%d != %d)",
					field.offset, fieldsLength + 1);

		if (field.type == kTypeMemo) {
			_hasMemo = true;

			if (!fpt)
				throw Exception("Memo needed");
		}

		fieldsLength += field.size;

		_fields.push_back(field);
	}

	if (dbf->eos() || dbf->err())
		throw Exception(kReadError);

	if (recordSize != (fieldsLength + 1))
		throw Exception("Length of all fields does not equal the record size");

	dbf->seek(firstRecordPos);

	_pool.push_back(new byte[recordSize * recordCount]);
	byte *recordData = _pool.back();

	if (dbf->read(recordData, recordSize * recordCount) != (recordSize * recordCount))
		throw Exception(kReadError);

	if (dbf->readByte() != 0x1A)
		throw Exception("Record end marker missing");

	uint32 fieldCount = _fields.size();

	// Create the records array
	_records.resize(recordCount);
	for (uint32 i = 0; i < recordCount; i++) {
		Record &record = _records[i];
		const byte *data = recordData + i * recordSize;

		char status = *data++;
		if ((status != ' ') && (status != '*'))
			throw Exception("Unknown record status '%c'", status);

		record.deleted = status == '*';

		record.fields.resize(fieldCount);
		for (uint32 j = 0; j < fieldCount; j++) {
			record.fields[j] = data;
			data += _fields[j].size;
		}
	}

	// Read memo blocks
	if (_hasMemo) {
		fpt->skip(4); // Next free block
		fpt->skip(2); // Unused

		uint32 _memoBlockSize = fpt->readUint16BE();
		if (_memoBlockSize < 33)
			_memoBlockSize *= 1024;

		fpt->skip(504); // Unused

		while (!fpt->eos() && !fpt->err()) {
			_memos.push_back(new byte[_memoBlockSize]);
			byte *data = _memos.back();

			fpt->read(data, _memoBlockSize);
		}

		if (fpt->err())
			throw Exception(kReadError);
	}

	// TODO: Read the compound index (CDX) file
}

date FoxPro::getLastUpdate() const {
	return _lastUpdate;
}

bool FoxPro::hasIndex() const {
	return _hasIndex;
}

bool FoxPro::hasMemo() const {
	return _hasMemo;
}

uint32 FoxPro::getFieldCount() const {
	return _fields.size();
}

uint32 FoxPro::getRecordCount() const {
	return _records.size();
}

const std::vector<FoxPro::Field> &FoxPro::getFields() const {
	return _fields;
}

const std::vector<FoxPro::Record> &FoxPro::getRecords() const {
	return _records;
}

UString FoxPro::getString(const Record &record, uint32 field) const {
	assert(field < _fields.size());

	const Field &f = _fields[field];
	if (f.type != kTypeString)
		throw Exception("Field is not of string type ('%c')", f.type);

	Common::MemoryReadStream stream(record.fields[field], f.size);

	UString str;
	str.readLatin9(stream, f.size);

	// xBase fields are padded with spaces...
	str.trimRight();

	return str;
}

int32 FoxPro::getInt(const Record &record, uint32 field) const {
	assert(field < _fields.size());

	const Field &f = _fields[field];
	if ((f.type != kTypeInteger) && (f.type != kTypeNumber))
		throw Exception("Field is not of int type ('%c')", f.type);

	int32 i = 0;
	if (!getInt(record.fields[field], f.size, i))
		i = 0;

	return i;
}

bool FoxPro::getBool(const Record &record, uint32 field) const {
	assert(field < _fields.size());

	const Field &f = _fields[field];
	if (f.type != kTypeBool)
		throw Exception("Field is not of bool type ('%c')", f.type);

	if (f.size != 1)
		throw Exception("Bool field size != 1 (%d)", f.size);

	char c = (char) record.fields[field][0];

	if ((c == 't') || (c == 'T') || (c == 'y') || (c == 'Y') || (c == '1'))
		return true;

	return false;
}

double FoxPro::getDouble(const Record &record, uint32 field) const {
	assert(field < _fields.size());

	const Field &f = _fields[field];
	if ((f.type != kTypeFloat) && (f.type != kTypeDouble) && (f.type != kTypeNumber))
		throw Exception("Field is not of int type ('%c')", f.type);

	char n[32];
	double d = 0.0;

	if (f.size > 31)
		throw Exception("Numerical field size > 31 (%d)", f.size);

	strncpy(n, (const char *) record.fields[field], f.size);
	n[f.size] = '\0';

	if (sscanf(n, "%lf", &d) != 1)
		d = 0.0;

	return d;
}

SeekableReadStream *FoxPro::getMemo(const Record &record, uint32 field) const {
	assert(field < _fields.size());

	const Field &f = _fields[field];
	if (f.type != kTypeMemo)
		throw Exception("Field is not of memo type ('%c')", f.type);

	int32 i = 0;
	if (!getInt(record.fields[field], f.size, i) || (i < 1))
		return 0;

	uint32 block = ((uint32) i) - 1;

	if (block >= _memos.size())
		throw Exception("Memo block #%d >= memo block count %d", block, _memos.size());

	uint32 type = READ_BE_UINT32(_memos[block] + 0);
	uint32 size = READ_BE_UINT32(_memos[block] + 4);

	if ((type != 0x00) && (type != 0x01) && (type != 0x02))
		throw Exception("Memo type unknown (%d)", type);

	bool first = true;

	uint32 dataSize = size;

	// Read the data
	byte *data    = new byte[size];
	byte *dataPtr = data;
	while (size > 0) {
		if (block >= _memos.size()) {
			delete data;
			throw Exception("Memo block #%d >= memo block count %d", block, _memos.size());
		}

		uint32 n = MIN<uint32>(size, _memoBlockSize - (first ? 8 : 0));

		memcpy(dataPtr, _memos[block] + (first ? 8 : 0), n);

		dataPtr += n;
		size    -= n;
		block   += 1;

		first = false;
	}

	return new MemoryReadStream(data, dataSize, true);
}

void FoxPro::deleteRecord(uint32 record) {
	assert(record < _records.size());

	_records[record].deleted = true;

	updateUpdate();

	// TODO: Deleting a record should also mark any memo fields in that
	//       record as free. They should be reused when adding a memo
	//       field of equals or less size.
}

uint32 FoxPro::addFieldString(const UString &name, uint8 size) {
	uint32 offset = 1;
	if (!_fields.empty())
		offset = _fields.back().offset + _fields.back().size;

	_fields.push_back(Field());

	Field &field = _fields.back();

	field.name        = name;
	field.type        = kTypeString;
	field.offset      = offset;
	field.size        = size;
	field.decimals    = 0;
	field.flags       = 0;
	field.autoIncNext = 0;
	field.autoIncStep = 0;

	addField(field.size);
	updateUpdate();

	return _fields.size() - 1;
}

uint32 FoxPro::addFieldNumber(const UString &name, uint8 size, uint8 decimals) {
	uint32 offset = 1;
	if (!_fields.empty())
		offset = _fields.back().offset + _fields.back().size;

	_fields.push_back(Field());

	Field &field = _fields.back();

	field.name        = name;
	field.type        = kTypeNumber;
	field.offset      = offset;
	field.size        = size;
	field.decimals    = decimals;
	field.flags       = 0;
	field.autoIncNext = 0;
	field.autoIncStep = 0;

	addField(field.size);
	updateUpdate();

	return _fields.size() - 1;
}

uint32 FoxPro::addFieldInt(const UString &name) {
	uint32 offset = 1;
	if (!_fields.empty())
		offset = _fields.back().offset + _fields.back().size;

	_fields.push_back(Field());

	Field &field = _fields.back();

	field.name        = name;
	field.type        = kTypeInteger;
	field.offset      = offset;
	field.size        = 10;
	field.decimals    = 0;
	field.flags       = 0;
	field.autoIncNext = 0;
	field.autoIncStep = 0;

	addField(field.size);
	updateUpdate();

	return _fields.size() - 1;
}

uint32 FoxPro::addFieldBool(const UString &name) {
	uint32 offset = 1;
	if (!_fields.empty())
		offset = _fields.back().offset + _fields.back().size;

	_fields.push_back(Field());

	Field &field = _fields.back();

	field.name        = name;
	field.type        = kTypeBool;
	field.offset      = offset;
	field.size        = 1;
	field.decimals    = 0;
	field.flags       = 0;
	field.autoIncNext = 0;
	field.autoIncStep = 0;

	addField(field.size);
	updateUpdate();

	return _fields.size() - 1;
}

uint32 FoxPro::addFieldMemo(const UString &name) {
	uint32 offset = 1;
	if (!_fields.empty())
		offset = _fields.back().offset + _fields.back().size;

	_fields.push_back(Field());

	Field &field = _fields.back();

	field.name        = name;
	field.type        = kTypeMemo;
	field.offset      = offset;
	field.size        = 10;
	field.decimals    = 0;
	field.flags       = 0;
	field.autoIncNext = 0;
	field.autoIncStep = 0;

	addField(field.size);
	updateUpdate();

	_hasMemo = true;

	return _fields.size() - 1;
}

void FoxPro::addField(uint8 size) {
	if (_records.empty())
		return;

	uint32 dataSize = size * _records.size();

	_pool.push_back(new byte[dataSize]);

	byte *data = _pool.back();

	for (std::vector<Record>::iterator it = _records.begin(); it != _records.end(); ++it) {
		it->fields.push_back(data);
		data += size;
	}
}

uint32 FoxPro::addRecord() {
	_records.push_back(Record());
	Record &record = _records.back();

	record.deleted = false;

	if (!_fields.empty()) {
		uint32 dataSize = _fields.back().offset + _fields.back().size;

		_pool.push_back(new byte[dataSize]);

		byte *data = _pool.back();

		record.fields.resize(_fields.size());
		for (uint i = 0; i < _fields.size(); i++) {
			record.fields[i] = data;
			data += _fields[i].size;
		}
	}

	updateUpdate();

	return _records.size() - 1;
}

void FoxPro::setString(uint32 record, uint32 field, const Common::UString &value) {
	assert((record < _records.size()) && (field < _fields.size()));

	Record &r = _records[record];
	Field  &f = _fields[field];

	if (f.type != kTypeString)
		throw Exception("Field is not of string type ('%c')", f.type);

	char *data    = (char *) r.fields[field];
	char *dataEnd = (char *) r.fields[field] + f.size;

	strncpy(data, value.c_str(), f.size);

	data += strlen(value.c_str());

	while (data < dataEnd)
		*dataEnd++ = 0x20;

	updateUpdate();
}

void FoxPro::setInt(uint32 record, uint32 field, int32 value) {
	assert((record < _records.size()) && (field < _fields.size()));

	Record &r = _records[record];
	Field  &f = _fields[field];

	if ((f.type != kTypeInteger) && (f.type != kTypeNumber))
		throw Exception("Field is not of int type ('%c')", f.type);

	char *data = (char *) r.fields[field];

	if (f.decimals != 0)
		snprintf(data, f.size, "%*d", f.size, value);
	else
		snprintf(data, f.size, "%*.*f\n", f.size, f.decimals, (double) value);

	updateUpdate();
}

void FoxPro::setBool(uint32 record, uint32 field, bool value) {
	assert((record < _records.size()) && (field < _fields.size()));

	Record &r = _records[record];
	Field  &f = _fields[field];

	if (f.type != kTypeBool)
		throw Exception("Field is not of bool type ('%c')", f.type);

	if (f.size != 1)
		throw Exception("Bool field size != 1 (%d)", f.size);

	char *data = (char *) r.fields[field];

	data[0] = value ? 'T' : 'F';

	updateUpdate();
}

void FoxPro::setDouble(uint32 record, uint32 field, double value) {
	assert((record < _records.size()) && (field < _fields.size()));

	Record &r = _records[record];
	Field  &f = _fields[field];

	if ((f.type != kTypeFloat) && (f.type != kTypeDouble) && (f.type != kTypeNumber))
		throw Exception("Field is not of double type ('%c')", f.type);

	char *data = (char *) r.fields[field];

	if (f.decimals != 0)
		snprintf(data, f.size, "%*d", f.size, (int32) value);
	else
		snprintf(data, f.size, "%*.*f\n", f.size, f.decimals, value);

	updateUpdate();
}

void FoxPro::setMemo(uint32 record, uint32 field, SeekableReadStream *value) {
	assert((record < _records.size()) && (field < _fields.size()));

	Record &r = _records[record];
	Field  &f = _fields[field];

	if (f.type != kTypeMemo)
		throw Exception("Field is not of memo type ('%c')", f.type);

	if (!value) {
		memset((char *) r.fields[field], 0x20, f.size);
		updateUpdate();
		return;
	}

	value->seek(0);

	uint32 size = value->size();

	uint32 block = _memos.size();
	_memos.push_back(new byte[_memoBlockSize]);

	WRITE_BE_UINT32(_memos[block]    , 1);
	WRITE_BE_UINT32(_memos[block] + 4, size);

	bool first = true;
	while (size > 0) {
		uint32 n = MIN<uint32>(size, _memoBlockSize - (first ? 8 : 0));

		if (value->read(_memos[block] + (first ? 8 : 0), n) != n)
			throw Exception(kReadError);

		size  -= n;
		block += 1;

		if (size > 0)
			_memos.push_back(new byte[_memoBlockSize]);

		first = false;
	}

	updateUpdate();
}

bool FoxPro::getInt(const byte *data, uint32 size, int32 &i) {
	char n[32];

	if (size > 31)
		throw Exception("Numerical field size > 31 (%d)", size);

	strncpy(n, (const char *) data, size);
	n[size] = '\0';

	return sscanf(n, "%d", &i) == 1;
}

void FoxPro::updateUpdate() {
	_lastUpdate = day_clock::universal_day();
}

} // End of namespace Common
