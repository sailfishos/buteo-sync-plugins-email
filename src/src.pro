TARGET = syncemail-client

CONFIG += link_pkgconfig
PKGCONFIG += buteosyncfw5 QmfClient nemoemail-qt5 accounts-qt5

QT += network dbus
QT -= gui

TEMPLATE = lib
CONFIG += plugin
target.path = $$[QT_INSTALL_LIBS]/buteo-plugins-qt5/oopp

HEADERS += syncemailclient.h

SOURCES += syncemailclient.cpp

sync.path = /etc/buteo/profiles/sync
sync.files = ../xmls/sync/*

client.path = /etc/buteo/profiles/client
client.files = ../xmls/client/*

INSTALLS += target sync client
