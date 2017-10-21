#-------------------------------------------------
#
# Project created by QtCreator 2011-08-22T16:37:18
#
#-------------------------------------------------

QT       += core gui

TARGET = ProgSkeet_Flasher
TEMPLATE = app

win32 {
    TARGET = WinSkeet40000
}

unix {
    TARGET = YASkeet
}

macx {
    TARGET = iSkeet
}

DEFINES += "TARGETNAME=\\\"$$TARGET\\\""

SOURCES += \
    main.cpp \
    mainwindow.cpp\
    Flasher.cpp \
    NorFlashThread.cpp \
    NorDumpThread.cpp \
    FlasherThread.cpp \
    NandDumpThread.cpp \
    NandFlashThread.cpp \
    NandEraseThread.cpp \
    NorEraseThread.cpp \
    NandCommonThread.cpp \
    NorDumpCFIThread.cpp \
    sizespinbox.cpp \
    binsizespinbox.cpp \
    NorCommonThread.cpp \
    FlashUtilities.cpp \
    eraseblockspinbox.cpp \
    async_io.cpp

HEADERS  += \
    mainwindow.h\
    Flasher.h \
    NorFlashThread.h \
    NorDumpThread.h \
    FlasherThread.h \
    Arguments.h \
    NandDumpThread.h \
    NandFlashThread.h \
    NandEraseThread.h \
    NorEraseThread.h \
    NandCommonThread.h \
    NorDumpCFIThread.h \
    sizespinbox.h \
    binsizespinbox.h \
    utilities.h \
    NorCommonThread.h \
    FlashUtilities.h \
    eraseblockspinbox.h \
    async_io.h

FORMS    += mainwindow.ui

win32 {
    LIBS += -L$$PWD/libusb
    INCLUDEPATH += $$PWD/libusb
}

macx {
    LIBS += -lusb-1.0
    ICON = res/progskeet.icns
} else {
    LIBS += -lusb-1.0
}

OTHER_FILES += \
    ProgSkeet.rc \
    doc/build.txt \
    doc/todo.txt \
    doc/changelog.txt \
    doc/credits.txt

win32:RC_FILE = ProgSkeet.rc

RESOURCES += \
    ProgSkeet.qrc

TRANSLATIONS += \
    res/ProgSkeet_de_DE.ts \
    res/ProgSkeet_fr_FR.ts \
    res/ProgSkeet_nl_NL.ts \
    res/ProgSkeet_it_IT.ts \
    res/ProgSkeet_ru_RU.ts \
    res/ProgSkeet_sr_CS.ts \
    res/ProgSkeet_ja_JP.ts \
    res/ProgSkeet_hu_HU.ts \
    res/ProgSkeet_ar_AR.ts

CONFIG(debug, debug|release): DEFINES += DEBUG




