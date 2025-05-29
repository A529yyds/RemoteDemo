QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ExampleDemo.cpp \
    OptFFmpeg.cpp \
    Receiver.cpp \
    RecvAnalyzerTask.cpp \
    ScreenShotTask.cpp \
    Sender.cpp \
    main.cpp \
    Remote.cpp

HEADERS += \
    ExampleDemo.h \
    OptFFmpeg.h \
    Receiver.h \
    RecvAnalyzerTask.h \
    Remote.h \
    ScreenShotTask.h \
    Sender.h \
    defines.h

FORMS += \
    Remote.ui

TRANSLATIONS += \
    RemoteDemo_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

INCLUDEPATH += $$PWD
INCLUDEPATH += /usr/local/include
INCLUDEPATH += $$PWD/NdiSdkAdvanced/include
#LIBS += $$PWD/NdiSdkAdvanced/lib/libndi_advanced.so
LIBS += -L$$PWD/NdiSdkAdvanced/lib -lndi_advanced

LIBS += -L/usr/local/lib -lswscale
LIBS += -L/usr/local/lib -lavutil
LIBS += -L/usr/local/lib -lavcodec
LIBS += -L/usr/local/lib -lavformat
# 假设 .so 文件原始位置在项目根目录的 NdiSdkAdvanced/lib/
NDI_LIB_DIR = $$PWD/NdiSdkAdvanced/lib
TARGET_LIB_NAME = libndi_advanced.so.6.1.1

# 构建后复制 .so 文件并创建软链接
QMAKE_POST_LINK += \
    cp $$NDI_LIB_DIR/$$TARGET_LIB_NAME $$DESTDIR/ && \
    cd $$DESTDIR/ && \
    ln -sf $$TARGET_LIB_NAME libndi_advanced.so && \
    ln -sf $$TARGET_LIB_NAME libndi_advanced.so.6


QMAKE_RPATHDIR += $$ORIGIN

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
