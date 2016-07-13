/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2016 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QFile>
#include <QStringList>
#include <QtDebug>

#include <common_features/main_window_ptr.h>
#include <networking/engine_intproc.h>

#include "localserver.h"

/**
 * @brief LocalServer::LocalServer
 *  Constructor
 */
LocalServer::LocalServer() :
    m_sema(PGE_EDITOR_SEMAPHORE, 1),
    m_shmem(PGE_EDITOR_SHARED_MEMORY),
    m_isWorking(false)
{
//    ipServer = new IntProcServer();
//    connect(ipServer, SIGNAL(messageIn(QString)),
//            this, SLOT(slotOnData(QString)));
//    connect(this, SIGNAL(privateDataReceived(QString)),
//            this, SLOT(slotOnData(QString)));

    if(!m_shmem.create(4096, QSharedMemory::ReadWrite))
    {
        qWarning() << m_shmem.errorString();
    } else {
        //! Zero data in the memory
        memset(m_shmem.data(), 0, 4096);
    }
}


/**
 * @brief LocalServer::~LocalServer
 *  Destructor
 */
LocalServer::~LocalServer()
{
    m_sema.release();//Free semaphore
    m_isWorking = false;
}




/**
 * -----------------------
 * QThread requred methods
 * -----------------------
 */

/**
 * @brief run
 *  Initiate the thread.
 */
void LocalServer::run()
{
    m_isWorking = true;
    exec();
}

/**
 * @brief LocalServer::exec
 *  Keeps the thread alive. Waits for incomming connections
 */
void LocalServer::exec()
{
    while(m_isWorking)
    {
        m_sema.acquire();//Avoid races
        typedef char* pchar;
        typedef int*  pint;
        char* inData = pchar(m_shmem.data());
        if(inData[0] == 1) // If any data detected
        {
            char out[4095-sizeof(int)];
            int  size = *pint(inData+1);
            size = qMin(size, int(4095-sizeof(int)));
            memcpy(out, inData+1+sizeof(int), size);
            memset(m_shmem.data(), 0, 4096);
            QMetaObject::invokeMethod(this,
                                      "slotOnData",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, QString::fromUtf8(out, size)));
        }
        m_sema.release();//Free semaphore
        msleep(25);
    }
}


/**
 * -------
 * SLOTS
 * -------
 */
void LocalServer::stopServer()
{
    m_isWorking = false;
}

/**
 * @brief LocalServer::slotOnData
 *  Executed when data is received
 * @param data
 */
void LocalServer::slotOnData(QString data)
{
    qDebug() << data;
    QStringList args = data.split('\n');
    foreach(QString c, args)
    {
        if(c.startsWith("CMD:", Qt::CaseInsensitive))
        {
            onCMD(c);
        }
        else
        {
            emit dataReceived(c);
        }
    }
}



/**
 * -------
 * Helper methods
 * -------
 */
void LocalServer::onCMD(QString data)
{
    //  Trim the leading part from the command
    if(data.startsWith("CMD:"))
    {
        data.remove("CMD:");
        QString cmd      = data;
        QString argsPart = "";

        if(data.contains(':'))
        {
            int splitAt = data.indexOf(':');
            cmd      = data.mid(0, splitAt);
            argsPart = data.mid(splitAt+1, -1);
        }

        qDebug() << "Accepted data: " + data;
        qDebug() << "Command " + cmd;
        qDebug() << "Args " + argsPart;

        QStringList commands;
        commands << "showUp";
        commands << "CONNECT_TO_ENGINE";
        commands << "ENGINE_CLOSED";
        commands << "testSetup";

        int cmdID = commands.indexOf(cmd);

        if( (cmdID==3) || (MainWinConnect::pMainWin->m_isAppInited))
        switch(cmdID)
        {
            case 0:
            {
                emit showUp();
                MainWinConnect::pMainWin->setWindowState((MainWinConnect::pMainWin->windowState()&
                                             (~(MainWinConnect::pMainWin->windowState()&Qt::WindowMinimized)))
                                              |Qt::WindowActive);
                if(MainWinConnect::pMainWin->isMinimized())
                {
                    MainWinConnect::pMainWin->raise();
                    MainWinConnect::pMainWin->activateWindow();
                    MainWinConnect::pMainWin->showNormal();
                }
                qApp->setActiveWindow(MainWinConnect::pMainWin);
                break;
            }
            case 1:
            {
                IntEngine::sendLevelBuffer();
                MainWinConnect::pMainWin->showMinimized();
                break;
            }
            case 2:
            {
                IntEngine::quit();
                break;
            }
            case 3:
            {
                QStringList args = argsPart.split(',');
                SETTINGS_TestSettings& t = GlobalSettings::testing;
                if(args.size()>=5)
                {
                    bool ok;
                    int playerID = args[0].toInt(&ok);
                    if(ok)
                    {
                        if(playerID==0)
                        {
                            if(ok) t.p1_char = args[1].toInt(&ok);
                            if(ok) t.p1_state= args[2].toInt(&ok);
                            if(ok) t.p1_vehicleID= args[3].toInt(&ok);
                            if(ok) t.p1_vehicleType= args[4].toInt(&ok);
                        } else if(playerID==1) {
                            if(ok) t.p2_char = args[1].toInt(&ok);
                            if(ok) t.p2_state= args[2].toInt(&ok);
                            if(ok) t.p2_vehicleID= args[3].toInt(&ok);
                            if(ok) t.p2_vehicleType= args[4].toInt(&ok);
                        }
                    }
                }
                break;
            }
            default:
                emit acceptedCommand(data);
        }
    }
    else
        emit acceptedCommand(data);

}
