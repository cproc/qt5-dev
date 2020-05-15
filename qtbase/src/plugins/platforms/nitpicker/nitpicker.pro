TARGET = qnitpicker

QT += \
    core-private \
    egl_support-private \
    eventdispatcher_support-private \
    fontdatabase_support-private \
    gui-private

CONFIG += exceptions

DEFINES += QT_NO_FOREACH

SOURCES =   main.cpp \
            qgenodeclipboard.cpp \
            qnitpickercursor.cpp \
            qnitpickerglcontext.cpp \
            qnitpickerintegration.cpp \
            qnitpickerplatformwindow.cpp \
            qnitpickerwindowsurface.cpp

HEADERS =   qgenodeclipboard.h \
            qnitpickerintegrationplugin.h \
            qnitpickerplatformwindow.h \ 
            qnitpickerwindowsurface.h

OTHER_FILES += nitpicker.json

qtConfig(freetype): QMAKE_USE_PRIVATE += freetype

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QNitpickerIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
