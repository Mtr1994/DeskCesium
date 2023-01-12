QT       += core gui webenginewidgets webchannel

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

lessThan(QT_MAJOR_VERSION, 6): win32: QMAKE_CXXFLAGS += -execution-charset:utf-8

# make the application have authority of running on Ubuntu, and I do not know why it works
unix: QMAKE_LFLAGS += -no-pie

# Qt 5.14 VERSION can only contains numbers, no any others type of character please
# please do not put 0 before any version number, because this will cause a warnning on Qt 5.14
win32:  VERSION = 21.5.9.1140                # major.minor.patch.build
else:   VERSION = 21.5.9                    # major.minor.patch

QMAKE_TARGET_COPYRIGHT = mtr company Co., Ltd

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += PROTOBUF_USE_DLLS

# 禁用 QDebug 输出调试i信息
#DEFINES += QT_NO_DEBUG_OUTPUT


# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += $$PWD/Sdk/gdal303/gdal/include
INCLUDEPATH += $$PWD/Sdk/qxlsx/include
INCLUDEPATH += $$PWD/Sdk/proto/include
INCLUDEPATH += $$PWD/Sdk/hpsocket/include

DESTDIR = ../output

SOURCES += \
    Channel/chartcontext.cpp \
    Channel/jscontext.cpp \
    Channel/surveycontext.cpp \
    Control/Message/messagewidget.cpp \
    Dialog/dialogabout.cpp \
    Dialog/dialogsearch.cpp \
    Dialog/dialogsetting.cpp \
    Dialog/dialogstatistics.cpp \
    Dialog/dialoguploaddata.cpp \
    EntityTreeWidget/entitytreewidget.cpp \
    Ftp/ftpmanager.cpp \
    Net/basesocket.cpp \
    Net/tcpsocket.cpp \
    Net/usernetworker.cpp \
    Proto/sidescansource.pb.cc \
    Protocol/protocolhelper.cpp \
    Public/appconfig.cpp \
    Public/appsignal.cpp \
    Widget/widgetselectparameter.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    Channel/chartcontext.h \
    Channel/jscontext.h \
    Channel/surveycontext.h \
    Common/common.h \
    Control/Message/messagewidget.h \
    Dialog/dialogabout.h \
    Dialog/dialogsearch.h \
    Dialog/dialogsetting.h \
    Dialog/dialogstatistics.h \
    Dialog/dialoguploaddata.h \
    EntityTreeWidget/entitytreewidget.h \
    Ftp/ftpmanager.h \
    Net/basesocket.h \
    Net/tcpsocket.h \
    Net/usernetworker.h \
    Proto/sidescansource.pb.h \
    Protocol/protocolhelper.h \
    Public/appconfig.h \
    Public/appsignal.h \
    Public/treeitemdelegate.h \
    Widget/widgetselectparameter.h \
    mainwindow.h

FORMS += \
    Dialog/dialogabout.ui \
    Dialog/dialogsearch.ui \
    Dialog/dialogsetting.ui \
    Dialog/dialogstatistics.ui \
    Dialog/dialoguploaddata.ui \
    Widget/widgetselectparameter.ui \
    mainwindow.ui

TRANSLATIONS += \
    Translate_zh_CN.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RC_ICONS = ./Resource/icon/icon.ico

RESOURCES += \
    res.qrc

#0x0800代表和系统当前语言一致
RC_LANG = 0x0800

LIBS += -L$$PWD/Sdk/gdal303/gdal/lib \
-lgdal_i

LIBS +=-L$$PWD/Sdk/qxlsx/lib \
-lqxlsx

LIBS +=-L$$PWD/Sdk/proto/lib \
-llibprotobuf

LIBS += -L$$PWD/Sdk/hpsocket/lib \
-lHPSocket_U

