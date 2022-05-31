/*
 * \brief  QGenodeGLContext
 * \author Christian Prochaska
 * \date   2013-11-18
 */

/*
 * Copyright (C) 2013-2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/log.h>

/* EGL includes */
#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>

/* Qt includes */
#include <QtEglSupport/private/qeglconvenience_p.h>
#include <QtEglSupport/private/qeglpbuffer_p.h>
#include <QDebug>

/* local includes */
#include "qgenodeplatformwindow.h"
#include "qgenodeglcontext.h"

static const bool qnglc_verbose = false;

QT_BEGIN_NAMESPACE

QGenodeGLContext::QGenodeGLContext(QOpenGLContext *context, EGLDisplay egl_display)
: QPlatformOpenGLContext(),
  _egl_display(egl_display)
{
	if (qnglc_verbose)
		Genode::log(__func__, "called");

    _egl_config = q_configFromGLFormat(_egl_display, context->format(), false, EGL_PBUFFER_BIT);
    if (_egl_config == 0)
        qFatal("Could not find a matching EGL config");

	_format = q_glFormatFromConfig(_egl_display, _egl_config);

	_egl_context = eglCreateContext(_egl_display, _egl_config, EGL_NO_CONTEXT, 0);
    if (_egl_context == EGL_NO_CONTEXT)
        qFatal("eglCreateContext() failed");
}


bool QGenodeGLContext::makeCurrent(QPlatformSurface *platform_surface)
{
	if (qnglc_verbose)
		Genode::log(__func__, " called");

	doneCurrent();

	EGLSurface egl_surface;

	if (platform_surface->surface()->surfaceClass() == QSurface::Window) {

		QGenodePlatformWindow *w = static_cast<QGenodePlatformWindow*>(platform_surface);
qDebug() << (void*)w << ": QGenodeGLContext::makeCurrent(): " << w->geometry();
		egl_surface = w->egl_surface();

		if (egl_surface == EGL_NO_SURFACE) {

			Genode_egl_window egl_window = { w->geometry().width(),
			                                 w->geometry().height(),
			                                 w->framebuffer(),
			                                 PIXMAP };

			egl_surface = eglCreatePixmapSurface(_egl_display,
			                                     _egl_config,
			                                     &egl_window, 0);

			if (egl_surface == EGL_NO_SURFACE)
				qFatal("eglCreatePixmapSurface() failed");

			w->egl_surface(egl_surface);
		}

	} else {
		egl_surface = static_cast<QEGLPbuffer *>(platform_surface)->pbuffer();
	}

	if (!eglMakeCurrent(_egl_display, egl_surface, egl_surface, _egl_context))
		qFatal("eglMakeCurrent() failed");

	return true;
}


void QGenodeGLContext::doneCurrent()
{
	if (qnglc_verbose)
		Genode::log(__func__, " called");

	if (!eglMakeCurrent(_egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
		qFatal("eglMakeCurrent() failed");
}


void QGenodeGLContext::swapBuffers(QPlatformSurface *surface)
{
	if (qnglc_verbose)
		Genode::log(__func__, " called");

	QGenodePlatformWindow *w = static_cast<QGenodePlatformWindow*>(surface);

	if (!eglSwapBuffers(_egl_display, w->egl_surface()))
		qFatal("eglSwapBuffers() failed");

	w->refresh(0, 0, w->geometry().width(), w->geometry().height());
}


QFunctionPointer QGenodeGLContext::getProcAddress(const char *procName)
{
	if (qnglc_verbose)
		Genode::log("procName=", Genode::Cstring(procName), " , "
		            "pointer=", eglGetProcAddress(procName));

	return static_cast<QFunctionPointer>(eglGetProcAddress(procName));
}


QSurfaceFormat QGenodeGLContext::format() const
{
    return _format;
}

QT_END_NAMESPACE

