/*
 * Copyright (c) 2013 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
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

#include "syncemailclient.h"

// QMF
#include <qmailnamespace.h>

// buteo-syncfw
#include <ProfileEngineDefs.h>

// Qt
#include <QTime>
#include <QDebug>

extern "C" SyncEmailClient* createPlugin(const QString& pluginName,
                                         const Buteo::SyncProfile& profile,
                                         Buteo::PluginCbInterface *cbInterface)
{
    return new SyncEmailClient(pluginName, profile, cbInterface);
}

extern "C" void destroyPlugin(SyncEmailClient *client)
{
    delete client;
}

SyncEmailClient::SyncEmailClient(const QString& pluginName,
                                 const Buteo::SyncProfile& profile,
                                 Buteo::PluginCbInterface *cbInterface)
    : ClientPlugin(pluginName, profile, cbInterface),
      m_emailAgent(nullptr)
{
    connect(&m_ipcTimeout, SIGNAL(timeout()), this, SLOT(ipcTimeout()));
}

SyncEmailClient::~SyncEmailClient()
{
}

static bool isAncestorFolder(const QMailFolder &folder, const QMailFolderId &ancestor)
{
    if (folder.status() & QMailFolder::NonMail) {
        return false;
    }
    QMailFolderId parentId = folder.parentFolderId();
    if (!parentId.isValid()) {
        return false;
    } else {
        return parentId == ancestor
            || isAncestorFolder(QMailFolder(parentId), ancestor);
    }
}

bool SyncEmailClient::init()
{
    m_accountId = QMailAccountId(profile().key(Buteo::KEY_ACCOUNT_ID).toInt());
    if (!m_accountId.isValid()) {
        qWarning() << Q_FUNC_INFO << "Invalid email account, ID: " << m_accountId.toULongLong();
        return false;
    }

    Accounts::Manager accountManager;
    Accounts::Account *account = accountManager.account(m_accountId.toULongLong());
    if (account) {
        account->selectService(accountManager.service(QStringLiteral("email")));
        m_folderSyncPolicy = account->valueAsString(QStringLiteral("folderSyncPolicy"));
        bool all = (m_folderSyncPolicy == QLatin1String("all-folders"));
        if (all || (m_folderSyncPolicy == QLatin1String("inbox-and-subfolders"))) {
            // Ensure that synchronization flag is set
            // for inbox and subfolders or for all.
            QMailAccount account(m_accountId);
            QMailFolderId inboxId = account.standardFolder(QMailFolder::InboxFolder);
            if (inboxId.isValid()) {
                QMailFolderKey key = QMailFolderKey::parentAccountId(m_accountId);
                QList<QMailFolderId> folders = QMailStore::instance()->queryFolders(key);
                for (QList<QMailFolderId>::ConstIterator it = folders.constBegin();
                     it != folders.constEnd(); ++it) {
                    if (it->isValid()) {
                        QMailFolder folder(*it);
                        folder.setStatus(QMailFolder::SynchronizationEnabled,
                                         all || *it == inboxId
                                         || isAncestorFolder(folder, inboxId));
                    }
                }
            } else {
                qWarning() << Q_FUNC_INFO << "Email account has no inbox.";
                // This will trigger inbox creation in email agent.
                m_folderSyncPolicy = QStringLiteral("inbox");
            }
        }
    }

    // if messageserver is not running, EmailAgent will attempt to start it via systemd
    m_emailAgent = new EmailAgent(this);
    connect(m_emailAgent, SIGNAL(ipcConnectionEstablished()), this, SLOT(ipcConnected()));
    return true;
}

bool SyncEmailClient::uninit()
{
    delete m_emailAgent;
    m_emailAgent = 0;
    return true;
}

bool SyncEmailClient::startSync()
{
    if (m_emailAgent->ipcConnected()) {
        triggerSync();
    } else {
        qWarning() << Q_FUNC_INFO << "IPC not connect yet, waiting....";
        // Since the process running this plugin can cause a wakelock we dont want to wait long time for IPC connection
        m_ipcTimeout.start(30000);
    }
    return true;
}

void SyncEmailClient::abortSync(Sync::SyncStatus status)
{
    //TODO: check if there's a use case for this (some place in the UI ?)
    Q_UNUSED(status)
}

Buteo::SyncResults SyncEmailClient::getSyncResults() const
{
    return m_syncResults;
}

bool SyncEmailClient::cleanUp()
{
    return true;
}

void SyncEmailClient::connectivityStateChanged(Sync::ConnectivityType, bool)
{
    // TODO
}

void SyncEmailClient::syncStatusChanged()
{
    if (!m_emailAgent->synchronizing()) {
        disconnect(m_emailAgent, 0, this, 0);
        updateResults(Buteo::SyncResults(QDateTime::currentDateTime(), Buteo::SyncResults::SYNC_RESULT_SUCCESS,
                                         Buteo::SyncResults::NO_ERROR));
        emit success(getProfileName(), "Sync completed");
    }
}

void SyncEmailClient::cancelSync()
{
    disconnect(m_emailAgent, 0, this, 0);
    m_emailAgent->cancelAll();
    updateResults(Buteo::SyncResults(QDateTime::currentDateTime(), Buteo::SyncResults::SYNC_RESULT_FAILED,
                                     Buteo::SyncResults::ABORTED));
    emit error(getProfileName(), "Sync failed", Buteo::SyncResults::SYNC_RESULT_FAILED);
}

void SyncEmailClient::ipcConnected()
{
    m_ipcTimeout.stop();
    // give it a bit more time to make sure all data is properly loaded
    QTimer::singleShot(5000, this, SLOT(triggerSync()));
}

void SyncEmailClient::triggerSync()
{
    qDebug() << Q_FUNC_INFO << "Starting scheduled sync for email account: " << m_accountId.toULongLong() << "policy:" << m_folderSyncPolicy;

    connect(m_emailAgent, SIGNAL(synchronizingChanged()), this, SLOT(syncStatusChanged()));
    connect(m_emailAgent, SIGNAL(networkConnectionRequested()), this, SLOT(cancelSync()));
    if (m_folderSyncPolicy.isEmpty() || m_folderSyncPolicy == QLatin1String("inbox")) {
        m_emailAgent->synchronizeInbox(m_accountId.toULongLong());
    } else {
        m_emailAgent->synchronize(m_accountId.toULongLong());
    }
}

void SyncEmailClient::updateResults(const Buteo::SyncResults &results)
{
    m_syncResults = results;
    m_syncResults.setScheduled(true);
}

void SyncEmailClient::ipcTimeout()
{
    qWarning() << Q_FUNC_INFO << "IPC connection timeout, abording...";
    updateResults(Buteo::SyncResults(QDateTime::currentDateTime(), Buteo::SyncResults::SYNC_RESULT_FAILED, Buteo::SyncResults::ABORTED));
    emit error(getProfileName(), "Sync failed", Buteo::SyncResults::SYNC_RESULT_FAILED);
}
