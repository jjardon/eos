/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file graphics/graphics.cpp
 *  The global graphics manager.
 */

#include <cmath>

#include "common/util.h"
#include "common/error.h"

#include "graphics/graphics.h"
#include "graphics/renderable.h"
#include "graphics/cube.h"

#include "events/events.h"

DECLARE_SINGLETON(Graphics::GraphicsManager)

namespace Graphics {

GraphicsManager::GraphicsManager() {
	_ready    = false;
	_initedGL = false;

	_screen = 0;
}

void GraphicsManager::init() {
	uint32 sdlInitFlags = SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_VIDEO;

// Might be needed on unixoid OS, but it crashes Windows. Nice.
#ifndef WIN32
	sdlInitFlags |= SDL_INIT_EVENTTHREAD;
#endif

	if (SDL_Init(sdlInitFlags) < 0)
		throw Common::Exception("Failed to initialize SDL: %s", SDL_GetError());

	_ready = true;
}

void GraphicsManager::deinit() {
	if (!_ready)
		return;

	clearRenderQueue();

	SDL_Quit();

	_ready    = false;
	_initedGL = false;
}

bool GraphicsManager::ready() const {
	return _ready;
}

void GraphicsManager::initSize(int width, int height, bool fullscreen) {
	int bpp = SDL_GetVideoInfo()->vfmt->BitsPerPixel;
	if ((bpp != 24) && (bpp != 32))
		throw Common::Exception("Need 24 or 32 bits per pixel");

	uint32 flags = SDL_OPENGL;

	if (fullscreen)
		flags |= SDL_FULLSCREEN;

	if (setupSDLGL(width, height, bpp, flags))
		return;

	// Could not initialize OpenGL, trying a different bpp value

	bpp = (bpp == 32) ? 24 : 32;

	if (setupSDLGL(width, height, bpp, flags))
		return;

	// Still couldn't initialize OpenGL, erroring out
	throw Common::Exception("Failed setting the video mode: %s", SDL_GetError());
}

bool GraphicsManager::setupSDLGL(int width, int height, int bpp, uint32 flags) {
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE    ,   8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE  ,   8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE   ,   8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE  , bpp);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,   1);

	_screen = SDL_SetVideoMode(width, height, bpp, flags);
	if (!_screen)
		return false;

	return true;
}

void GraphicsManager::setWindowTitle(const std::string &title) {
	SDL_WM_SetCaption(title.c_str(), 0);
}

void GraphicsManager::setupScene() {
	if (!_screen)
		throw Common::Exception("No screen initialized");

	glClearColor( 0, 0, 0, 0 );
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, _screen->w, _screen->h);

	perspective(60.0, ((GLdouble) _screen->w) / ((GLdouble) _screen->h), 1.0, 1000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0, 0.0, 0.0, 0.5);
	glClearDepth(1.0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	_initedGL = true;
}

void GraphicsManager::clearRenderQueue() {
	// Notify all objects in the queue that they have been kicked out
	for (RenderQueue::iterator it = _renderQueue.begin(); it != _renderQueue.end(); ++it)
		(*it)->kickedOutOfRenderQueue();

	// Clear the queue
	_renderQueue.clear();
}

GraphicsManager::RenderQueueRef GraphicsManager::addToRenderQueue(Renderable &renderable) {
	_renderQueue.push_back(&renderable);

	return --_renderQueue.end();
}

void GraphicsManager::removeFromRenderQueue(RenderQueueRef &ref) {
	_renderQueue.erase(ref);
}

void GraphicsManager::renderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();

	for (RenderQueue::iterator it = _renderQueue.begin(); it != _renderQueue.end(); ++it) {
		glPushMatrix();

		(*it)->render();

		glPopMatrix();
	}

	glPopMatrix();

	SDL_GL_SwapBuffers();
}

void GraphicsManager::perspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
	// Basically implements gluPerspective

	GLdouble halfHeight = zNear * std::tan(fovy * 0.5 * ((PI * 2) / 360));
	GLdouble halfWidth  = halfHeight * aspect;

	glFrustum(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);
}

} // End of namespace Graphics