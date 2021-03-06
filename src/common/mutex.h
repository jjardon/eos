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

/** @file common/mutex.h
 *  Thread mutex classes.
 */

#ifndef COMMON_MUTEX_H
#define COMMON_MUTEX_H

#include <SDL_thread.h>

#include "common/types.h"

namespace Common {

/** A mutex. */
class Mutex {
public:
	Mutex();
	~Mutex();

	void lock();
	void unlock();

private:
	SDL_mutex *_mutex;

	friend class Condition;
};

/** A semaphore . */
class Semaphore {
public:
	Semaphore(uint value);
	~Semaphore();

	bool lock(uint32 timeout = 0);
	bool lockTry();
	void unlock();

private:
	SDL_sem *_semaphore;
};

/** Convenience class that locks a mutex on creation and unlocks it on destruction. */
class StackLock {
public:
	StackLock(Mutex &mutex);
	StackLock(Semaphore &semaphore);
	~StackLock();

private:
	Mutex     *_mutex;
	Semaphore *_semaphore;
};

/** A condition. */
class Condition {
public:
	Condition();
	Condition(Mutex &mutex);
	~Condition();

	bool wait(uint32 timeout = 0);
	void signal();

private:
	bool _ownMutex;

	Mutex *_mutex;
	SDL_cond *_condition;
};

} // End of namespace Common

#endif // COMMON_MUTEX_H
