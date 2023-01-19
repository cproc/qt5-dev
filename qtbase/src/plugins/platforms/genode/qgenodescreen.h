/*
 * \brief  QGenodeScreen
 * \author Christian Prochaska
 * \date   2013-05-08
 */

/*
 * Copyright (C) 2013-2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */


#ifndef _QGENODESCREEN_H_
#define _QGENODESCREEN_H_

/* Genode includes */
#include <gui_session/connection.h>

/* Qt includes */
#include <qpa/qplatformscreen.h>

#include <QDebug>

#include "qgenodecursor.h"

QT_BEGIN_NAMESPACE

class QGenodeScreen : public QPlatformScreen
{
	private:

		Genode::Env &_env;
		QRect        _geometry;

	public:

		QGenodeScreen(Genode::Env &env) : _env(env)
		{
			Gui::Connection _gui(env);

			Framebuffer::Mode const scr_mode = _gui.mode();

			_geometry.setRect(0, 0, scr_mode.area.w(),
			                        scr_mode.area.h());
		}

		QRect geometry() const override { return _geometry; }
		int depth() const override { return 32; }
		QImage::Format format() const override{ return QImage::Format_RGB32; }
		QDpi logicalDpi() const override { return QDpi(80, 80); };

		QSizeF physicalSize() const override
		{
			/* 'overrideDpi()' takes 'QT_FONT_DPI' into account */
			static const int dpi = overrideDpi(logicalDpi()).first;
			return QSizeF(geometry().size()) / dpi * qreal(25.4);
		}

		QPlatformCursor *cursor() const override
		{
			static QGenodeCursor instance(_env);
			return &instance;
		}
};

QT_END_NAMESPACE

#endif /* _QGENODESCREEN_H_ */
