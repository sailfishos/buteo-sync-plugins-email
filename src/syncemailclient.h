/* Copyright (C) 2013 - 2014 Jolla Ltd.
 *
 * Contributors: Valerio Valerio <valerio.valerio@jollamobile.com>
 *
 * This file is part of buteo-sync-plugins-email
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef SYNCEMAILCLIENT_H
#define SYNCEMAILCLIENT_H

// buteo-syncfw
#include <ClientPlugin.h>
#include <SyncResults.h>
#include <ProfileManager.h>

// nemo-qml-plugins-email
#include <emailagent.h>

// QMF
#include <qmailaccount.h>

// accounts-qt5
#include <Accounts/Manager>
#include <Accounts/Account>

// Qt
#include <QNetworkReply>
#include <QtCore/qglobal.h>
#include <QTimer>

class Q_DECL_EXPORT SyncEmailClient : public Buteo::ClientPlugin
{
    Q_OBJECT

public:
    SyncEmailClient(const QString& pluginName,
                  const Buteo::SyncProfile& profile,
                  Buteo::PluginCbInterface *cbInterface);

    virtual ~SyncEmailClient();
    virtual bool init();
    virtual bool uninit();
    virtual bool startSync();
    virtual void abortSync(Sync::SyncStatus status = Sync::SYNC_ABORTED);
    virtual Buteo::SyncResults getSyncResults() const;
    virtual bool cleanUp();

public slots:
    virtual void connectivityStateChanged(Sync::ConnectivityType type, bool state);

private slots:
    void syncStatusChanged(EmailAgent::Status status);
    void ipcConnected();
    void triggerSync();
    void ipcTimeout();

private:
    Buteo::ProfileManager m_profileManager;
    Buteo::SyncProfile  *m_syncProfile;
    Buteo::SyncResults m_syncResults;
    EmailAgent *m_emailAgent;
    QMailAccountId m_accountId;
    QTimer m_ipcTimeout;

    void updateResults(const Buteo::SyncResults &results);
};

extern "C" SyncEmailClient* createPlugin(const QString& pluginName,
                                       const Buteo::SyncProfile& profile,
                                       Buteo::PluginCbInterface *cbInterface);

extern "C" void destroyPlugin(SyncEmailClient *client);

#endif // SYNCEMAILCLIENT_H
