TEMPLATE = app
TARGET = BupPlay

QT += network \
      xml \
      multimedia \
      multimediawidgets \
      widgets

HEADERS = \
    application.h \
    clickableSlider.h \
    consts.h \
    player.h \
    playercontrols.h \
    playlistmodel.h \
    slider_style.h \
    stable.h \
    stdafx.h \
    videowidget.h \
    histogramwidget.h \
    ../../lsMisc/stdQt/stdQt.h \
    ../../lsMisc/stdQt/inisettings.h

SOURCES = main.cpp \
    application.cpp \
    clickableSlider.cpp \
    player.cpp \
    playercontrols.cpp \
    playlistmodel.cpp \
    videowidget.cpp \
    histogramwidget.cpp \
    ../../lsMisc/stdQt/stdQt.cpp \
    ../../lsMisc/stdQt/inisettings.cpp

PRECOMPILED_HEADER = stable.h

target.path = $$[QT_INSTALL_EXAMPLES]/multimediawidgets/player
INSTALLS += target

RESOURCES += \
    resource.qrc

win32 {
    message("win32")
    HEADERS += \
        ../../lsMisc/GetLastErrorString.h

    SOURCES += \
        ../../lsMisc/stdQt/stdQt_win32.cpp \
        ../../lsMisc/GetLastErrorString.cpp

    win32-g++ {
        message("win32-g++")
        LIBS += -lshlwapi -lPsapi
    }
    win32-msvc* {
        message("win32-msvc*")
        LIBS += User32.lib Shlwapi.lib
        # Ole32.lib
    }

    RC_ICONS += images/icon.ico
}
linux {
    message("linux-g++")

    SOURCES += \
        ../../lsMisc/stdQt/stdQt_linux.cpp

    RC_ICONS += images/icon.ico
}

DISTFILES += \
    ../history.txt
