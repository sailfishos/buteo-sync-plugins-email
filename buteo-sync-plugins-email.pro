TEMPLATE = lib
TARGET = syncemail-client

CONFIG += link_pkgconfig plugin
PKGCONFIG += buteosyncfw5 qmfclient5 nemoemail-qt5 accounts-qt5

QT += network
QT -= gui

HEADERS += syncemailclient.h

SOURCES += syncemailclient.cpp

OTHER_FILES += \
    rpm/buteo-sync-plugins-email.spec \
    xmls/client/syncemail.xml \
    xmls/sync/syncemail.xml

target.path = /usr/lib/buteo-plugins-qt5

sync.path = /etc/buteo/profiles/sync
sync.files = xmls/sync/*

client.path = /etc/buteo/profiles/client
client.files = xmls/client/*

INSTALLS += target sync client
