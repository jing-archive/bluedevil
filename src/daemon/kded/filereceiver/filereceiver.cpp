/*************************************************************************************
 *  Copyright (C) 2013 by Alejandro Fiestas Fiestas <afiestas@kde.org>               *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "filereceiver.h"
#include "../BlueDevilDaemon.h"
#include "obexagent.h"

#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>

FileReceiver::FileReceiver(QObject* parent)
    : QObject(parent)
{
    qCDebug(BLUEDAEMON);
    qDBusRegisterMetaType<QVariantMap>();

    new ObexAgent(this);
    m_agentManager = new org::bluez::obex::AgentManager1(QStringLiteral("org.bluez.obex"),
                                                         QStringLiteral("/org/bluez/obex"),
                                                         QDBusConnection::sessionBus(),
                                                         this);

    registerAgent();

    // obexd should be set to auto-start by D-Bus (D-Bus activation), so this should restart it in case of crash
    QDBusServiceWatcher *serviceWatcher = new QDBusServiceWatcher(QStringLiteral("org.bluez.obex"), QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(serviceWatcher, SIGNAL(serviceUnregistered(QString)), this, SLOT(registerAgent()));
}

FileReceiver::~FileReceiver()
{

}

void FileReceiver::registerAgent()
{
    QDBusPendingReply <void > r = m_agentManager->RegisterAgent(QDBusObjectPath(QStringLiteral("/BlueDevil_receiveAgent")));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(r, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), SLOT(agentRegistered(QDBusPendingCallWatcher*)));
}

void FileReceiver::agentRegistered(QDBusPendingCallWatcher* call)
{
    QDBusPendingReply <void > r = *call;
    qCDebug(BLUEDAEMON) << "Error: " << r.isError();
    if (r.isError()) {
        qCDebug(BLUEDAEMON) << r.error().message();
    }

    call->deleteLater();
}
