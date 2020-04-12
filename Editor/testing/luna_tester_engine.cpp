/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <mainwindow.h>
#include <ui_mainwindow.h>

#include "luna_tester_engine.h"

#include <sstream>
#include <cstring>
#include <signal.h>

#if !defined(_WIN32) && !defined(__APPLE__)
#include <glob.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#endif

#include <QThread>
#include <QEventLoop>
#include <QDesktopWidget>
#include <QDirIterator>
#include <QJsonObject>
#include <QMetaObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QIcon>
#include <QMenu>
#include <QAction>
#include <QtConcurrentRun>
#include <QGridLayout>
#include <QRadioButton>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QGroupBox>
#include <QSpacerItem>

#if !defined(_WIN32)
#include <QStandardPaths>
#include "wine/wine_setup.h"
#endif

#ifdef __APPLE__
#include <QRegExp>
#endif

#include <PGE_File_Formats/file_formats.h>
#include <data_functions/smbx64_validation_messages.h>
#include <common_features/app_path.h>
#include <common_features/logger.h>
#include <dev_console/devconsole.h>
#include <main_window/global_settings.h>



class QThreadPointNuller
{
    QThread **pointer;
public:
    explicit QThreadPointNuller(QThread **ptr) : pointer(ptr) {}
    ~QThreadPointNuller()
    {
        *pointer = nullptr;
    }
};

#ifdef _WIN32
static BOOL  checkProc(DWORD procId, const wchar_t *proc_name_wanted);
static DWORD getPidsByPath(const std::wstring &process_path, DWORD *found_pids, DWORD found_pids_max_size);

/* Checks matching of single process with full path string by PID */
static BOOL checkProc(DWORD procId, const wchar_t *proc_name_wanted)
{
    wchar_t proc_name[MAX_PATH + 1] = L"<unknown>";
    HMODULE h_mod = nullptr;
    DWORD cbNeeded;
    DWORD proc_name_len;
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procId);

    if(nullptr == h_process)
        return FALSE;

    if(!EnumProcessModules(h_process, &h_mod, sizeof(h_mod), &cbNeeded))
        return FALSE;

    proc_name_len = GetModuleFileNameExW(h_process, h_mod, proc_name, MAX_PATH);
    proc_name[proc_name_len] = '\0';

    if(proc_name_len == 0)
    {
        proc_name_len = GetModuleBaseNameW(h_process, h_mod, proc_name, MAX_PATH);
        proc_name[proc_name_len] = '\0';
    }

    CloseHandle(h_process);
    return (lstrcmpiW(proc_name, proc_name_wanted) == 0);
}

/* Get all PIDs are matching given full path */
static DWORD getPidsByPath(const std::wstring &process_path, DWORD *found_pids, DWORD found_pids_max_size)
{
    DWORD proc_id[1024];
    DWORD ret_bytes = 0;
    DWORD proc_count = 0;
    unsigned int i;
    DWORD found_pids_count = 0;

    if(!EnumProcesses(proc_id, sizeof(proc_id), &ret_bytes))
    {
        LogCritical("ERROR: Can not execute EnumProcesses()...");
        return 0;
    }

    proc_count = ret_bytes / sizeof(DWORD);

    for(i = 0; i < proc_count; i++)
    {
        if(proc_id[i] != 0)
        {
            if(checkProc(proc_id[i], process_path.c_str()))
            {
                found_pids[found_pids_count++] = proc_id[i];
                if(found_pids_count >= found_pids_max_size)
                    break;
            }
        }
    }

    return found_pids_count;
}
#endif // _WIN32

#if !defined(_WIN32) && !defined(__APPLE__)
static pid_t find_pid(const QString &proc_name)
{
    const std::string process_name = proc_name.toStdString();
    pid_t pid = -1;
    glob_t pglob;
    char real_path[PATH_MAX] = {0};
    char cmdline_buffer[PATH_MAX] = {0};

    /* Get a list of all comm files. man 5 proc */
    if (glob("/proc/*/exe", 0, nullptr, &pglob) != 0)
        return pid;

    for(size_t i = 0; i < pglob.gl_pathc; ++i)
    {
        std::memset(real_path, 0, PATH_MAX);
        std::memset(cmdline_buffer, 0, PATH_MAX);
        size_t len = std::strlen(pglob.gl_pathv[i]);

        if(nullptr == realpath(pglob.gl_pathv[i], real_path))
            continue;

        if(nullptr == std::strstr(real_path, "wine-preloader"))
            continue;

        std::string cmdline_path = std::string(pglob.gl_pathv[i], len - 3);
        cmdline_path += "cmdline";

        FILE *cline = std::fopen(cmdline_path.c_str(), "rb");
        if(!cline)
            continue;

        std::fread(cmdline_buffer, 1, PATH_MAX, cline);
        std::fclose(cline);
        for(size_t j = 0; j < PATH_MAX; j++)
        {
            char &c = cmdline_buffer[j];
            if(c == '\\')
                c = '/';
        }

        // If exe matches our process path, extract the process ID from the
        // path, convert it to a pid_t, and return it.
        if(nullptr != std::strstr(cmdline_buffer, process_name.c_str()))
        {
            pid = static_cast<pid_t>(std::atoi(pglob.gl_pathv[i] + strlen("/proc/")));
            break;
        }
    }

    /* Clean up. */
    globfree(&pglob);
    return pid;
}
#endif // !_WIN32 && !__APPLE__


void LunaEngineWorker::init()
{
    if(!m_process)
    {
        m_readBuffer.clear();
        m_readPktQueue.clear();
        m_process = new QProcess;
        QObject::connect(m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                         this, &LunaEngineWorker::processFinished);
        QObject::connect(m_process, &QProcess::readyReadStandardOutput,
                         this, &LunaEngineWorker::gotReadReady);
#if QT_VERSION >= 0x050600
        QObject::connect(m_process, &QProcess::errorOccurred, this, &LunaEngineWorker::errorOccurred);
#endif
        m_lastStatus = m_process->state();
    }
}

void LunaEngineWorker::unInit()
{
    emit stopLoop();
    if(m_process)
    {
        QObject::disconnect(m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                         this, &LunaEngineWorker::processFinished);
        QObject::disconnect(m_process, &QProcess::readyReadStandardOutput,
                         this, &LunaEngineWorker::gotReadReady);
#if QT_VERSION >= 0x050600
        QObject::disconnect(m_process, &QProcess::errorOccurred, this, &LunaEngineWorker::errorOccurred);
#endif
        terminate();
        QThread::msleep(1000);
        delete m_process;
        m_process = nullptr;
    }
}

LunaEngineWorker::LunaEngineWorker(QObject *parent) : QObject(parent)
{
    m_lastStatus = QProcess::NotRunning;
}

LunaEngineWorker::~LunaEngineWorker()
{
    unInit();
}

void LunaEngineWorker::setExecPath(const QString &path)
{
    m_lunaExecPath = path;
}

void LunaEngineWorker::setEnv(const QProcessEnvironment &env)
{
    if(!m_process)
        return;
    m_process->setProcessEnvironment(env);
}

void LunaEngineWorker::setWorkPath(const QString &wDir)
{
    m_workingPath = wDir;
    if(!m_process)
        return;
    m_process->setWorkingDirectory(wDir);
}

void LunaEngineWorker::start(const QString &command, const QStringList &args, bool *ok, QString *errString)
{
    init();
    Q_ASSERT(m_process);
    LogDebug(QString("LunaTester: starting command: %1 %2").arg(command).arg(args.join(' ')));
    m_process->setWorkingDirectory(m_workingPath);
    m_process->start(command, args);
    m_lastStatus = m_process->state();
    *ok = m_process->waitForStarted();
    *errString = m_process->errorString();
    if(!*ok)
        LogWarning(QString("LunaTester: startup error: %1").arg(*errString));
    m_lastStatus = m_process->state();
}

void LunaEngineWorker::terminate()
{
    if(m_process && (m_process->state() == QProcess::Running))
    {
#ifdef _WIN32
        // Kill everything that has "smbx.exe"
        DWORD proc_id[1024];
        DWORD proc_count = 0;
        QString smbxPath = m_lunaExecPath + ConfStatus::SmbxEXE_Name;
        smbxPath.replace('/', '\\');
        proc_count = getPidsByPath(smbxPath.toStdWString(), proc_id, 1024);
        if(proc_count > 0)
        {
            LogDebugNC(QString("LunaEngineWorker: Found matching PIDs for running %1, going to kill...")
                     .arg(ConfStatus::SmbxEXE_Name));
            for(DWORD i = 0; i < proc_count; i++)
            {
                DWORD f_pid = proc_id[i];
                HANDLE h_process = OpenProcess(PROCESS_TERMINATE, FALSE, f_pid);
                if(nullptr != h_process)
                {
                    BOOL res = TerminateProcess(h_process, 0);
                    if(!res)
                    {
                        LogDebugNC(
                            QString("LunaEngineWorker: Failed to kill %1 with PID %2 by 'TerminateProcess()' with error '%3', possibly it's already terminated.")
                                 .arg(ConfStatus::SmbxEXE_Name)
                                 .arg(f_pid)
                                 .arg(GetLastError())
                        );
                    }
                    CloseHandle(h_process);
                }
            }
        }
        else
            LogDebugNC(QString("LunaEngineWorker: No matching PIDs found for %1:").arg(ConfStatus::SmbxEXE_Name));
#else // _WIN32
#   ifdef __APPLE__
        LogDebugNC(QString("LunaEngineWorker: Killing %1 by 'kill'...").arg(ConfStatus::SmbxEXE_Name));
        QProcess ps;
        ps.start("/bin/ps", {"-A"});
        ps.waitForFinished();
        QString psAll = ps.readAllStandardOutput();

        QStringList psAllList = psAll.split('\n');
        QString smbxExeName = ConfStatus::SmbxEXE_Name;

        QRegExp psReg(QString("(\\d+) .*(wine-preloader).*(%1)").arg(smbxExeName));
        for(QString &psOne : psAllList)
        {
            int pos = 0;
            while((pos = psReg.indexIn(psOne, pos)) != -1)
            {
                pid_t toKill = static_cast<pid_t>(psReg.cap(1).toUInt());
                LogDebugNC(QString("LunaEngineWorker: kill -TERM %1").arg(toKill));
                kill(toKill, SIGTERM);
                pos += psReg.matchedLength();
            }
        }
#   else
        pid_t pid = find_pid(m_lunaExecPath + ConfStatus::SmbxEXE_Name);
        LogDebugNC(QString("LunaEngineWorker: Killing %1 by pid %2...")
            .arg(ConfStatus::SmbxEXE_Name)
            .arg(pid));
        if(pid > 0)
            kill(pid, SIGKILL);
#   endif //__APPLE__
#endif // _WIN32

        LogDebugNC(QString("LunaEngineWorker: Killing by QProcess::kill()..."));
        QMetaObject::invokeMethod(m_process, "kill", Qt::BlockingQueuedConnection);
    }
}

void LunaEngineWorker::write(const QString &out, bool *ok)
{
    if(!m_process)
    {
        *ok = false;
        return;
    }
    QByteArray o = out.toUtf8();
    *ok = m_process->write(o) == qint64(o.size());
    m_process->waitForBytesWritten();
}

void LunaEngineWorker::read(QString *in, bool *ok)
{
    while (m_readPktQueue.isEmpty() && m_process && m_process->isOpen())
    {
        // gotReadReady is called from the same thread as this, so no synchronization, just loop this to run this thread's event queue until we might have something in the queue
        m_process->waitForReadyRead();
    }

    if (m_readPktQueue.isEmpty())
    {
        *ok = false;
        return;
    }

    *in = QString::fromStdString(m_readPktQueue.dequeue());
    *ok = !in->isEmpty();
}

void LunaEngineWorker::writeStd(const std::string &out, bool *ok)
{
    if(!m_process)
    {
        *ok = false;
        return;
    }
    *ok = m_process->write(out.c_str(), qint64(out.size())) == qint64(out.size());
    m_process->waitForBytesWritten();
}

void LunaEngineWorker::readStd(std::string *in, bool *ok)
{
    while (m_readPktQueue.isEmpty() && m_process && m_process->isOpen())
    {
        // gotReadReady is called from the same thread as this, so no synchronization, just loop this to run this thread's event queue until we might have something in the queue
        m_process->waitForReadyRead();
    }

    if (m_readPktQueue.isEmpty())
    {
        *ok = false;
        return;
    }

    *in = m_readPktQueue.dequeue();
    *ok = !in->empty();
}

void LunaEngineWorker::processLoop()
{
    QEventLoop loop;
    QObject::connect(this, &LunaEngineWorker::stopLoop,
                     &loop, &QEventLoop::quit,
                     Qt::BlockingQueuedConnection);
    loop.exec();

    if(m_process) // Interrupt running process
    {
        m_process->kill();
        m_process->waitForFinished(2000);
    }

    emit loopFinished();
}

void LunaEngineWorker::quitLoop()
{
    emit stopLoop();
}

bool LunaEngineWorker::isActive()
{
    return m_lastStatus == QProcess::Running;
}

void LunaEngineWorker::errorOccurred(QProcess::ProcessError err)
{
    QString errString;
    switch(err)
    {
    case QProcess::FailedToStart:
        errString = "Failed to start";
        break;
    case QProcess::Crashed:
        errString = "Crashed";
        break;
    case QProcess::Timedout:
        errString = "Timed out";
        break;
    case QProcess::ReadError:
        errString = "Read error";
        break;
    case QProcess::WriteError:
        errString = "Write error";
        break;
    case QProcess::UnknownError:
        errString = "Unknown error";
        break;
    }
    LogDebug(QString("LunaEngineWorker: Process finished with error: %1").arg(errString));
    m_lastStatus = m_process->state();
}

void LunaEngineWorker::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    LogDebug(QString("LunaEngineWorker: Process finished with code %1 and %2 exit")
        .arg(exitCode)
        .arg(exitStatus == QProcess::NormalExit ? "normal" : "crash")
    );
    m_lastStatus = m_process->state();
}

void LunaEngineWorker::gotReadReady()
{
    // Sanity check
    if (!m_process)
    {
        return;
    }

    // Read the new data
    m_readBuffer += m_process->readAll();

    while (m_readBuffer.size() > 0)
    {
        // Skip any non-digit characters
        int i;
        for (i = 0; i < m_readBuffer.size(); i++)
        {
            if ((m_readBuffer[i] >= '0') && (m_readBuffer[i] <= '9'))
            {
                break;
            }
        }

        // If we skipped non-digits, clear that much from the start of the buffer
        if (i > 0)
        {
            m_readBuffer.remove(0, i);
        }

        // Check how many digits we've read
        for (i = 0; i < m_readBuffer.size(); i++)
        {
            if ((m_readBuffer[i] < '0') || (m_readBuffer[i] > '9'))
            {
                break;
            }
        }

        // If we don't have a following byte, abort for now
        if (i >= m_readBuffer.size())
        {
            break;
        }

        // If the following byte is not a colon, skip past this
        if (m_readBuffer[i] != ':')
        {
            m_readBuffer.remove(0, i+1);
            continue;
        }

        // Decode length string
        bool ok;
        int len = m_readBuffer.left(i).toInt(&ok);

        // If unable to decode length string, skip past this
        if (!ok)
        {
            m_readBuffer.remove(0, i+1);
            continue;
        }

        // Check if we have enough data, otherwise we have to wait till we receive more
        if ((i+1+len) > m_readBuffer.size())
        {
            break;
        }

        // We have enough data, let's get the packet as a string
        std::string pkt = m_readBuffer.mid(i+1, len).toStdString();

        // Discard from read buffer
        m_readBuffer.remove(0, i+1+len);

        gotIPCPacket(pkt);
    }
}

void LunaEngineWorker::gotIPCPacket(const std::string& str)
{
    // Add the IPC packet to a queue
    // In the future this should probably be a signal that is used for event-driven logic?
    m_readPktQueue.enqueue(str);
}

#ifndef _WIN32
void LunaTesterEngine::useWine(QString &command, QStringList &args)
{
    args.push_front(command);
    auto setup = m_wineSetup;
    WineSetup::prepareSetup(setup);
    command = setup.metaWineExec;
}

QString LunaTesterEngine::pathUnixToWine(const QString &unixPath)
{
    auto setup = m_wineSetup;
    WineSetup::prepareSetup(setup);
    auto env = WineSetup::buildEnv(setup);

    QStringList args;
    QString command = QStandardPaths::findExecutable("winepath", {setup.metaWineBinDir});

    QProcess winePathExec;
    // Ask for in-Wine Windows path from in-UNIX native path
    args << "--windows" << unixPath;
    // Use wine custom environment
    winePathExec.setProcessEnvironment(env);

    // Start winepath
    winePathExec.start(command, args);
    winePathExec.waitForFinished();
    // Retrieve converted path
    QString windowsPath = winePathExec.readAllStandardOutput();
    return windowsPath.trimmed();
}

#else // _WIN32
void LunaTesterEngine::useWine(QString &, QStringList &) // Dummy
{}

QString LunaTesterEngine::pathUnixToWine(const QString &unixPath)
{
    // dummy, no need on real Windows
    return unixPath;
}
#endif //_WIN32

QString LunaTesterEngine::getEnginePath()
{
    if(ConfStatus::defaultTestEngine == ConfStatus::ENGINE_LUNA)
        return ConfStatus::configDataPath; // Ignore custom path

    return m_customLunaPath.isEmpty() ?
                ConfStatus::configDataPath :
                m_customLunaPath + "/";
}





LunaTesterEngine::LunaTesterEngine(QObject *parent) :
    AbstractRuntimeEngine(parent)
{}

LunaTesterEngine::~LunaTesterEngine()
{
    unInitRuntime();
}

void LunaTesterEngine::initRuntime()
{
    if(m_worker.isNull() && m_thread.isNull())
    {
        m_worker.reset(new LunaEngineWorker());
        m_thread.reset(new QThread());
        m_worker->moveToThread(m_thread.data());
        auto *worker_ptr = m_worker.data();
        auto *thread_ptr = m_thread.data();
        QObject::connect(thread_ptr, SIGNAL(started()),
                         worker_ptr, SLOT(processLoop()));
        QObject::connect(worker_ptr, SIGNAL(loopFinished()),
                         thread_ptr, SLOT(quit()));
        QObject::connect(this, &LunaTesterEngine::engineSetEnv, worker_ptr,
                &LunaEngineWorker::setEnv, Qt::BlockingQueuedConnection);
        QObject::connect(this, &LunaTesterEngine::engineSetWorkPath, worker_ptr,
                &LunaEngineWorker::setWorkPath, Qt::BlockingQueuedConnection);
        QObject::connect(this, &LunaTesterEngine::engineSetExecPath, worker_ptr,
                &LunaEngineWorker::setExecPath, Qt::BlockingQueuedConnection);
        QObject::connect(this, &LunaTesterEngine::engineStart, worker_ptr,
                &LunaEngineWorker::start, Qt::BlockingQueuedConnection);
        QObject::connect(this, &LunaTesterEngine::engineWrite, worker_ptr,
                &LunaEngineWorker::write, Qt::BlockingQueuedConnection);
        QObject::connect(this, &LunaTesterEngine::engineRead, worker_ptr,
                &LunaEngineWorker::read, Qt::BlockingQueuedConnection);
        QObject::connect(this, &LunaTesterEngine::engineWriteStd, worker_ptr,
                &LunaEngineWorker::writeStd, Qt::BlockingQueuedConnection);
        QObject::connect(this, &LunaTesterEngine::engineReadStd, worker_ptr,
                &LunaEngineWorker::readStd, Qt::BlockingQueuedConnection);
        m_thread->start();
    }
}

void LunaTesterEngine::unInitRuntime()
{
    if(m_helper.isRunning())
    {
        killEngine();
        m_helper.waitForFinished();
    }
    if(!m_thread.isNull() && !m_worker.isNull())
    {
        m_worker->unInit();
        m_thread->quit();
        m_thread->requestInterruption();
        m_thread->wait();
    }
    m_worker.reset();
    m_thread.reset();
}

bool LunaTesterEngine::updateLunaCaps()
{
    //! Path to SMBX root
    const QString smbxPath = getEnginePath();
    //! Full path to LunaDLL library
    const QString lunaDllPath = smbxPath + "LunaDll.dll";
    return getLunaCapabilities(m_caps, lunaDllPath);
}







void LunaTesterEngine::init()
{
    MainWindow *w = qobject_cast<MainWindow*>(parent());

    Q_ASSERT(w);
    if(!w)
    {
        LogCritical("Can't run test: MainWindow parent is null!");
        return;
    }

    m_w = w;

#if QT_VERSION_CHECK(5, 6, 0)
    QObject::connect(&m_lunaGameIPC, &QProcess::errorOccurred,
                     this, &LunaTesterEngine::gameErrorOccurred);
#endif
    QObject::connect(&m_lunaGameIPC, &QProcess::started,
                     this, &LunaTesterEngine::gameStarted);
    QObject::connect(&m_lunaGameIPC,
                     static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
                     this, &LunaTesterEngine::gameFinished);
    QObject::connect(&m_lunaGameIPC, &QProcess::stateChanged,
                     this, &LunaTesterEngine::gameStateChanged);
    QObject::connect(&m_lunaGameIPC, &QProcess::readyReadStandardError,
                     this, &LunaTesterEngine::gameReadyReadStandardError);
    QObject::connect(&m_lunaGameIPC, &QProcess::readyReadStandardOutput,
                     this, &LunaTesterEngine::gameReadyReadStandardOutput);

    QObject::connect(m_w, &MainWindow::languageSwitched, this, &LunaTesterEngine::retranslateMenu);

    loadSetup();
}

void LunaTesterEngine::unInit()
{
    unInitRuntime();
    saveSetup();
}

void LunaTesterEngine::initMenu(QMenu *lunaMenu)
{
    size_t menuItemId = 0;
    QAction *RunLunaTest;
    {
        RunLunaTest = lunaMenu->addAction("runTesting");
        QObject::connect(RunLunaTest,   &QAction::triggered,
                    this,               &LunaTesterEngine::startTestAction,
                    Qt::QueuedConnection);
        m_menuItems[menuItemId++] = RunLunaTest;
    }
    QAction *ResetCheckPoints = lunaMenu->addAction("resetCheckpoints");
    {
        QObject::connect(ResetCheckPoints,   &QAction::triggered,
                    this,               &LunaTesterEngine::resetCheckPoints,
                    Qt::QueuedConnection);
        m_menuItems[menuItemId++] = ResetCheckPoints;
    }
    {
        QAction *disableOpenGL = lunaMenu->addAction("Disable OpenGL");
        disableOpenGL->setCheckable(true);
        disableOpenGL->setChecked(m_noGL);
        QObject::connect(disableOpenGL,   &QAction::toggled,
                    [this](bool state)
        {
            m_noGL = state;
        });
        m_menuItems[menuItemId++] = disableOpenGL;
    }
    QAction *enableKeepingInBackground;
    {
        enableKeepingInBackground = lunaMenu->addAction("enableKeepingInBackground");
        enableKeepingInBackground->setCheckable(true);
        enableKeepingInBackground->setChecked(!m_killPreviousSession);
        QObject::connect(enableKeepingInBackground,   &QAction::toggled,
                    [this](bool state)
        {
            m_killPreviousSession = !state;
        });
        m_menuItems[menuItemId++] = enableKeepingInBackground;
    }
    {
        QAction *KillFrozenThread = lunaMenu->addAction("Terminate frozen loader");
        QObject::connect(KillFrozenThread,   &QAction::triggered,
                    this,               &LunaTesterEngine::killFrozenThread,
                    Qt::QueuedConnection);
        m_menuItems[menuItemId++] = KillFrozenThread;
    }
    {
        QAction *KillBackgroundInstance = lunaMenu->addAction("KillBackgroundInstance");
        QObject::connect(KillBackgroundInstance,   &QAction::triggered,
                    this,               &LunaTesterEngine::killBackgroundInstance,
                    Qt::QueuedConnection);
        m_menuItems[menuItemId++] = KillBackgroundInstance;
    }

    if(ConfStatus::defaultTestEngine != ConfStatus::ENGINE_LUNA)
        lunaMenu->addSeparator();

    QAction *choosEnginePath;
    {
        choosEnginePath = lunaMenu->addAction("changePath");
        QObject::connect(choosEnginePath,   &QAction::triggered,
                    this,                   &LunaTesterEngine::chooseEnginePath,
                    Qt::QueuedConnection);
        m_menuItems[menuItemId++] = choosEnginePath;
    }

#ifndef _WIN32
    lunaMenu->addSeparator();
    {
        QAction *wineSetup;
        wineSetup = lunaMenu->addAction("wineSetup");
        QObject::connect(wineSetup,   &QAction::triggered,
                    this,                   &LunaTesterEngine::runWineSetup,
                    Qt::QueuedConnection);
        m_menuItems[menuItemId++] = wineSetup;
    }
#endif

    lunaMenu->addSeparator();
    {
        QAction *runLegacyEngine = lunaMenu->addAction("startLegacyEngine");
        QObject::connect(runLegacyEngine,   &QAction::triggered,
                    this,              &LunaTesterEngine::lunaRunGame,
                    Qt::QueuedConnection);
        m_menuItems[menuItemId++] = runLegacyEngine;
    }

    retranslateMenu();
    QObject::connect(m_w, &MainWindow::languageSwitched, this, &LunaTesterEngine::retranslateMenu);

    if(ConfStatus::defaultTestEngine == ConfStatus::ENGINE_LUNA)
    {
        choosEnginePath->setVisible(false); // Don't show this when LunaTester is a default engine
        ResetCheckPoints->setShortcut(QStringLiteral("Ctrl+F5"));
        ResetCheckPoints->setShortcutContext(Qt::WindowShortcut);
    }
}

void LunaTesterEngine::retranslateMenu()
{
    size_t menuItemId = 0;
    {
        QAction *RunLunaTest = m_menuItems[menuItemId++];
        RunLunaTest->setText(tr("Test level",
                                "Run the LunaTester based level testing."));
        RunLunaTest->setToolTip(tr("Starts level testing in the legacy engine.\n"
                                   "To have this feature work, latest LunaLUA must be installed.\n"
                                   "Otherwise, it will be very limited."));
    }
    {
        QAction *ResetCheckPoints = m_menuItems[menuItemId++];
        ResetCheckPoints->setText(tr("Reset checkpoints"));
        ResetCheckPoints->setToolTip(tr("Reset all checkpoint states to initial state."));
    }
    {
        QAction *disableOpenGL = m_menuItems[menuItemId++];
        disableOpenGL->setText(tr("Disable OpenGL", "Disable OpenGL on LunaTester side"));
        disableOpenGL->setToolTip(tr("Disable OpenGL rendering engine and use the GDI. "
                                     "Useful if your video card does not support OpenGL or "
                                     "LunaLua is crashing on the attempt to use it."));
    }
    {
        QAction *enableKeepingInBackground = m_menuItems[menuItemId++];
        enableKeepingInBackground->setText(tr("Keep running in background",
                                              "Keep Legacy Engine be running in background to speed-up testing starts after first launch."));
        enableKeepingInBackground->setToolTip(tr("Allows to start level testing very fast after first launch.\n"
                                                 "Requires powerful computer, otherwise engine will freeze on next test launch.\n"
                                                 "Suggested to disable this feature on slow machines or if any troubles are happens while attempts to run a testing."));
    }
    {
        QAction *KillFrozenThread = m_menuItems[menuItemId++];
        KillFrozenThread->setText(tr("Terminate frozen loader",
                                     "Terminite frozen LunaTester on the attempt to send any command to LunaLua."));
        KillFrozenThread->setToolTip(tr("Termiates frozen thread to allow you to run a test again."));
    }
    {
        QAction *KillBackgroundInstance = m_menuItems[menuItemId++];
        KillBackgroundInstance->setText(tr("Terminate running process",
                                           "Ends the LunaTester process, regardless of whether it's in \n"
                                           "the background or foreground, so the engine can be loaded from scratch."));
        KillBackgroundInstance->setToolTip(tr("Ends the LunaTester process so the engine can be loaded from scratch."));
    }
    {
        QAction *chooseEnginePath = m_menuItems[menuItemId++];
        chooseEnginePath->setText(tr("Change the path to LunaTester...",
                                     "Open a dialog to choose the location of LunaTester (aka SMBX2 data root directory)."));
        chooseEnginePath->setToolTip(tr("Select the location of LunaTester."));
    }
#ifndef _WIN32
    {
        QAction *runWineSetup = m_menuItems[menuItemId++];
        runWineSetup->setText(tr("Wine settings...",
                                 "Open Wine settings to choose which Wine toolchain use"));
        runWineSetup->setToolTip(tr("Select a Wine toolchain for use."));
    }
#endif
    {
        QAction *runLegacyEngine = m_menuItems[menuItemId++];
        runLegacyEngine->setText(tr("Start Game",
                                    "Launch LunaTester as a normal game."));
        runLegacyEngine->setToolTip(tr("Launch LunaTester as a normal game."));
    }
}

bool LunaTesterEngine::isEngineActive()
{
    return (m_lunaGameIPC.state() == QProcess::Running);
//    if(m_worker.isNull())
//        return false;
//    return m_worker->isActive();
}

bool LunaTesterEngine::isInPipeOpen()
{
    return isEngineActive();
//    if(m_worker.isNull())
//        return false;
//    return m_worker->isActive();
}

bool LunaTesterEngine::isOutPipeOpen()
{
    return isEngineActive();
//    if(m_worker.isNull())
//        return false;
//    return m_worker->isActive();
}

bool LunaTesterEngine::writeToIPC(const std::string &out)
{
//    bool ret = false;
//    emit engineWriteStd(out, &ret);
//    return ret;
    auto ret = m_lunaGameIPC.write(out.c_str(), static_cast<int>(out.size()));
    return (ret == qint64(out.size()));
}

bool LunaTesterEngine::writeToIPC(const QString &out)
{
//    bool ret = false;
//    emit engineWrite(out, &ret);
//    return ret;
    auto outData = out.toUtf8();
    auto ret = m_lunaGameIPC.write(outData);
    return (ret == qint64(outData.size()));
}

std::string LunaTesterEngine::readFromIPC()
{
    std::string out;
    bool ok = false;
    emit engineReadStd(&out, &ok);
    return out;
}

QString LunaTesterEngine::readFromIPCQ()
{
    QString out;
    bool ok = false;
    emit engineRead(&out, &ok);
    return out;
}

#if QT_VERSION_CHECK(5, 6, 0)
void LunaTesterEngine::gameErrorOccurred(QProcess::ProcessError error)
{
    QString title = tr("LunaTester error");
    QString msg;
    switch(error)
    {
    case QProcess::FailedToStart:
        msg = tr("Failed to start: %1").arg(m_lunaGame.errorString());
        break;
    case QProcess::Crashed:
        msg = tr("Crashed: %1").arg(m_lunaGame.errorString());
        break;
    case QProcess::Timedout:
        msg = tr("Timed out: %1").arg(m_lunaGame.errorString());
        break;
    case QProcess::WriteError:
        msg = tr("Write error: %1").arg(m_lunaGame.errorString());
        break;
    case QProcess::ReadError:
        msg = tr("Read error: %1").arg(m_lunaGame.errorString());
        break;
    default:
    case QProcess::UnknownError:
        msg = tr("\nUnknown error: %1").arg(m_lunaGame.errorString());
        break;
    }
    QMessageBox::critical(m_w, title, msg, QMessageBox::Ok);
}
#endif

void LunaTesterEngine::gameStarted()
{
    if(sendLevelData(m_levelTestBuffer, m_levelTestPath, m_levelTestUntitled))
    {
        //GlobalSettings::recentMusicPlayingState = ui->actionPlayMusic->isChecked();
        //Stop music playback in the PGE Editor!
        QMetaObject::invokeMethod(m_w, "setMusicButton", Qt::QueuedConnection, Q_ARG(bool, false));
        QMetaObject::invokeMethod(m_w, "on_actionPlayMusic_triggered", Qt::QueuedConnection, Q_ARG(bool, false));
    }
    else
    {
        QMessageBox::warning(m_w,
                    LunaTesterEngine::tr("LunaTester error"),
                    LunaTesterEngine::tr("Failed to send level into LunaLUA-SMBX!"),
                    QMessageBox::Ok);
    }
}

void LunaTesterEngine::gameFinished(int exitCode, QProcess::ExitStatus exitStatus)
{

}

void LunaTesterEngine::gameStateChanged(QProcess::ProcessState newState)
{

}

void LunaTesterEngine::gameReadyReadStandardError()
{

}

void LunaTesterEngine::gameReadyReadStandardOutput()
{
    if(!isEngineActive())
        return;

    QByteArray strData = m_lunaGameIPC.readAllStandardOutput();
//    readInputStream(strData);

    LogDebugQD(QString("LunaTester: Received from SMBX JSON message: %1").arg(QString::fromUtf8(strData)));

}

void LunaTesterEngine::readInputStream(QByteArray &out)
{
    char c = 0;

    out.clear();
    out.reserve(100);

    while(true)
    {
        qint64 bytesRead = m_lunaGameIPC.read(&c, 1);
        while(bytesRead == 0)
        {
            // No data? Block before trying again.
            m_lunaGameIPC.waitForReadyRead();
            bytesRead = m_lunaGameIPC.read(&c, 1);
        }

        if(bytesRead != 1)
            break; // error

        if(c == '\n')
            break; // complete
        out.push_back(c);
    }
}


static void jSonToNetString(const QJsonDocument &jd, std::string &outString);

void LunaTesterEngine::killEngine()
{
    if(!isEngineActive())
        return;

    if(m_caps.ipcCommands.contains("quit"))
    {
        QJsonDocument jsonOut;
        QJsonObject jsonObj;
        jsonObj["jsonrpc"]  = "2.0";
        jsonObj["method"]   = "quit";
        QJsonObject JSONparams;
        JSONparams["xxx"] = 0;
        jsonObj["params"] = JSONparams;
        jsonObj["id"] = 3;
        jsonOut.setObject(jsonObj);

        //Converting JSON data into net string
        std::string dataToSend;
        jSonToNetString(jsonOut, dataToSend);

        //Send data to SMBX
        writeToIPC(dataToSend);
        m_lunaGameIPC.waitForFinished(1000);
        if(isEngineActive())
            m_lunaGameIPC.kill();
    }
    else
    {
        m_lunaGameIPC.kill();
    }
//    if(!m_thread.isNull())
//        m_worker->terminate();
}

/*****************************************Menu slots*************************************************/

inline void busyThreadBox(MainWindow *mw)
{
    QMessageBox::warning(mw,
                         "LunaTester",
                         LunaTesterEngine::tr("LunaTester loader thread is busy, try again or use the \"terminate frozen loader\" option!"),
                         QMessageBox::Ok);
}

void LunaTesterEngine::startTestAction()
{
    if(m_w->activeChildWindow() == MainWindow::WND_Level)
    {
        LevelEdit *edit = m_w->activeLvlEditWin();
        if(!edit)
            return;
        startLunaTester(edit->LvlData);
    }
}

/*!
 * \brief Starts testing in the hacked with LunaLUA-SMBX Engine if possible (Only for Windows builds)
 */
bool LunaTesterEngine::startLunaTester(const LevelData &d)
{
    initRuntime();
//    if(m_helper.isRunning())
//        busyThreadBox(m_w);
//    else
//    {
        if(m_w->activeChildWindow() == MainWindow::WND_Level)
        {
            LevelEdit *lvl = m_w->activeLvlEditWin();
            if(lvl)
            {
                if(m_killPreviousSession)
                    killEngine();

                LevelData l = d;

                lvl->prepareLevelFile(l);
//                m_helper = QtConcurrent::run(this,
//                                             &LunaTesterEngine::lunaRunnerThread,
//                                             l,
//                                             lvl->curFile,
//                                             lvl->isUntitled());
                lunaRunnerThread(l, lvl->curFile, lvl->isUntitled());
            }
        }
//    }
    return true;
}

void LunaTesterEngine::resetCheckPoints()
{
//    if(m_helper.isRunning())
//        busyThreadBox(m_w);
//    else
//        m_helper = QtConcurrent::run(this, &LunaTesterEngine::lunaChkResetThread);
    lunaChkResetThread();
}

void LunaTesterEngine::killFrozenThread()
{
    if(m_helper.isRunning())
    {
        QMessageBox::StandardButton reply = QMessageBox::warning(m_w,
                                            "LunaTester",
                                            tr("Are you really want to terminate loader thread?"),
                                            QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            killEngine();
        }
    }
    else
    {
        QMessageBox::information(m_w,
                                 "LunaTester",
                                 tr("Loader thread is not running.",
                                    "LunaTester thread now doing nothing.\n"
                                    "This message is shown on request to terminate "
                                    "frozen LunaTester while it is not "
                                    "actually frozen and responds."),
                                 QMessageBox::Ok);
    }
}

void LunaTesterEngine::killBackgroundInstance()
{
    if(isEngineActive())
    {
        QMessageBox::StandardButton reply = QMessageBox::warning(m_w,
                                            "LunaTester",
                                            tr("Are you sure you want to close LunaTester? If you are testing a level, this will immediately end it!"),
                                            QMessageBox::Yes | QMessageBox::No);
        if(reply == QMessageBox::Yes)
        {
            killEngine();
            QMessageBox::information(m_w,
                         "LunaTester",
                         tr("LunaTester has been successfully closed."),
                         QMessageBox::Ok);
        }
    }
    else
    {
        QMessageBox::information(m_w,
                                 "LunaTester",
                                 tr("LunaTester is not running."),
                                 QMessageBox::Ok);
    }
}

void LunaTesterEngine::chooseEnginePath()
{
    QDialog d(m_w);
    d.setWindowTitle(tr("Path to LunaTester", "Title of dialog"));
    d.setModal(true);

    QGridLayout *g = new QGridLayout(&d);
    d.setLayout(g);

    QGroupBox *gr = new QGroupBox(&d);
    gr->setTitle(tr("Please select a path to LunaTester:"));
    g->addWidget(gr, 0, 0, 1, 2);
    QGridLayout *gd = new QGridLayout(gr);
    gr->setLayout(gd);

    g->setColumnStretch(0, 1000);
    g->setColumnStretch(1, 0);

    QRadioButton *useDefault = new QRadioButton(gr);
    useDefault->setText(tr("Use default", "Using default LunaTester path, specified by a config pack"));

    QRadioButton *useCustom = new QRadioButton(gr);
    useCustom->setText(tr("Custom", "Using a user selected LunaTester path"));
    QLineEdit *c = new QLineEdit(gr);
    QPushButton *br = new QPushButton(gr);
    br->setText(tr("Browse..."));

    gd->addWidget(useDefault, 0, 0, 1, 3);
    gd->addWidget(useCustom, 1, 0);
    gd->addWidget(c, 1, 1);
    gd->addWidget(br, 1, 2);

    gd->setColumnStretch(0, 0);
    gd->setColumnStretch(1, 1000);
    gd->setColumnStretch(2, 0);

    if(m_customLunaPath.isEmpty())
    {
        useDefault->setChecked(true);
        c->setText(ConfStatus::configDataPath);
        c->setEnabled(false);
    }
    else
    {
        useCustom->setChecked(true);
        c->setText(m_customLunaPath);
    }

    QPushButton *save = new QPushButton(&d);
    save->setText(tr("Save"));
    g->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0);
    g->addWidget(save, 1, 1);

    save->setFocus(Qt::TabFocusReason);

    QObject::connect(useCustom, &QRadioButton::toggled, c, &QLineEdit::setEnabled);
    QObject::connect(useCustom, &QRadioButton::toggled, br, &QPushButton::setEnabled);

    QObject::connect(save, &QPushButton::clicked, &d, &QDialog::accept);
    QObject::connect(br, &QPushButton::clicked,
                     [&d, c](bool)->void
    {
        QString p = QFileDialog::getExistingDirectory(&d, tr("Select a location of LunaTester", "Directory select dialog title"), c->text());
        if(!p.isEmpty())
            c->setText(p);
    });

    int ret = d.exec();

    if(ret == QDialog::Accepted)
    {
        if(useCustom->isChecked())
            m_customLunaPath = c->text();
        else
            m_customLunaPath.clear();

        if(!updateLunaCaps())
        {
            QMessageBox::critical(m_w,
                                  tr("LunaTester error"),
                                  tr("Unable to recognize capabilities of selected LunaLua path, game may not work. Please select a different path."),
                                  QMessageBox::Ok);
        }
    }
}

#ifndef _WIN32
void LunaTesterEngine::runWineSetup()
{
    WineSetup d(m_w);
    d.setModal(true);
    d.setSetup(m_wineSetup);
    int ret = d.exec();
    if(ret == QDialog::Accepted)
        m_wineSetup = d.getSetup();
}
#endif

/*****************************************Private functions*************************************************/

/**
 * @brief Converting JSON data into net string
 * @param jd Input JSON document
 * @param outString Output raw NET string
 */
static void jSonToNetString(const QJsonDocument &jd, std::string &outString)
{
    QByteArray outputJSON = jd.toJson();
    auto len = static_cast<size_t>(outputJSON.size());
    const char *dataToSend = outputJSON.data();
    std::string len_std = std::to_string(len) + ":";

    outString.clear();
    outString.append(len_std);
    outString.append(dataToSend, len);
    outString.push_back(',');
}

static bool stringToJson(const std::string &message, QJsonDocument &out, QJsonParseError &err)
{
    QByteArray jsonData(message.c_str(), static_cast<int>(message.size()));
    out = QJsonDocument::fromJson(jsonData, &err);
    return (err.error == QJsonParseError::NoError);
}

bool LunaTesterEngine::sendLevelData(LevelData &lvl, QString levelPath, bool isUntitled)
{
    //{"jsonrpc": "2.0", "method": "testLevel", "params":
    //   {"filename": "myEpisode/test.lvl", "leveldata": "<RAW SMBX64 LEVEL DATA>", "players": [{"character": 1}], "godMode": false, "showFPS": false },
    //"id": 3}

    //"players": [ { "character": 1, "powerup": 1, "mountType": 1, "mountColor": 1 }, { "character": 2, "powerup": 1, "mountType": 1, "mountColor": 1 } ]

    if(!isOutPipeOpen())
        return false;

    // Ask "Is LVLX"
    bool hasLvlxSupport = m_caps.features.contains("LVLX");
//    {
//        QJsonDocument jsonOut;
//        QJsonObject jsonObj;
//        jsonObj["jsonrpc"] = "2.0";
//        jsonObj["method"] = "getSupportedFeatures";
//        jsonObj["id"] = 3;
//        jsonOut.setObject(jsonObj);
//        std::string dataToSend;
//        LogDebug("LunaTester: -> Checking for LVLX support on LunaLua side...");

//        jSonToNetString(jsonOut, dataToSend);
//        LogDebugQD(QString("Sending message to SMBX %1").arg(QString::fromStdString(dataToSend)));

//        if(writeToIPC(dataToSend))
//        {
//            std::string resultMsg = readFromIPC();
//            LogDebugQD(QString("LunaTester: Received from SMBX JSON message: %1").arg(QString::fromStdString(resultMsg)));
//            QJsonParseError errData;
//            QJsonDocument jsonOutData;
//            if(stringToJson(resultMsg, jsonOutData, errData))
//            {
//                QJsonObject obj = jsonOutData.object();
//                QJsonObject result = obj["result"].toObject();
//                if(!result["LVLX"].isNull())
//                {
//                    hasLvlxSupport = result["LVLX"].toBool();
//                    LogDebug("LunaTester: <- Yes! LVLX is supported!");
//                }
//            }
//            if(errData.error != QJsonParseError::NoError)
//            {
//                LogDebug("LunaTester: <- Oops, fail to parse: " + errData.errorString());
//            }
//        }
//    }

    if(hasLvlxSupport)
        LogDebug("LunaTester: <- Yes! LVLX is supported!");
    else
        LogDebug("LunaTester: <- No! LVLX is NOT supported by this LunaLua build!");

    QJsonDocument jsonOut;
    QJsonObject jsonObj;
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["method"] = "testLevel";

    QJsonObject JSONparams;
    QString levelPathOut;
    QString smbxPath = getEnginePath();

    if(!isUntitled)
    {
        if(!hasLvlxSupport && levelPath.endsWith(".lvlx", Qt::CaseInsensitive))
            levelPath.remove(levelPath.size() - 1, 1);
        levelPathOut = pathUnixToWine(levelPath);
        JSONparams["filename"] = levelPathOut;
    }
    else
    {
        levelPathOut = QStringLiteral("worlds\\untitled.lvl") + (hasLvlxSupport ? "x" : "");
        JSONparams["filename"] = levelPathOut;
    }

//    JSONparams["filename"] = pathUnixToWine(levelPathOut);

    QJsonArray JSONPlayers;
    QJsonObject JSONPlayer1, JSONPlayer2;
    SETTINGS_TestSettings  t = GlobalSettings::testing;
    JSONPlayer1["character"]  = t.p1_char;
    JSONPlayer1["powerup"]    = t.p1_state;
    JSONPlayer1["mountType"]  = t.p1_vehicleID;
    JSONPlayer1["mountColor"] = t.p1_vehicleType;
    JSONPlayer2["character"]  = t.p2_char;
    JSONPlayer2["powerup"]    = t.p2_state;
    JSONPlayer2["mountType"]  = t.p2_vehicleID;
    JSONPlayer2["mountColor"] = t.p2_vehicleType;
    //mountType

    JSONPlayers.push_back(JSONPlayer1);

    if(t.numOfPlayers > 1)
        JSONPlayers.push_back(JSONPlayer2);

    JSONparams["players"] = JSONPlayers;

    // Extra flags
    JSONparams["godMode"] = t.xtra_god;
    JSONparams["showFPS"] = t.xtra_showFPS;

    QString LVLRawData;
    //Generate actual SMBX64 Level file data
    if(hasLvlxSupport)
    {
        FileFormats::WriteExtendedLvlFileRaw(lvl, LVLRawData);
    }
    else
    {
        FileFormats::WriteSMBX64LvlFileRaw(lvl, LVLRawData, 64);
        //Set CRLF
        LVLRawData.replace("\n", "\r\n");
    }
    //Store data into JSON
    JSONparams["leveldata"] = LVLRawData;

    jsonObj["params"] = JSONparams;
    jsonObj["id"] = 3;
    jsonOut.setObject(jsonObj);

    //Converting JSON data into net string
    std::string dataToSend;
    jSonToNetString(jsonOut, dataToSend);

    LogDebug("LunaTester: -> Sending level data and testing request...");
    //Send data to SMBX
    if(writeToIPC(dataToSend))
    {
        //Read result from level testing run
//        std::string resultMsg = readFromIPC();
//        LogDebugQD(QString("LunaTester: Received from SMBX JSON message: %1").arg(QString::fromStdString(resultMsg)));
//        LogDebug("LunaTester: <- Command has been sent!");
        return true;
    }
    LogWarning("<- Fail to send level data and testing request into LunaTester!");
    return false;
}

void LunaTesterEngine::loadSetup()
{
    QString inifile = AppPathManager::settingsFile();
    QSettings settings(inifile, QSettings::IniFormat);

    settings.beginGroup("LunaTester");
    {
        m_noGL = settings.value("nogl", false).toBool();
        m_killPreviousSession = settings.value("kill-engine-on-every-test", false).toBool();
        m_customLunaPath = settings.value("custom-runtime-path", QString()).toString();
#ifndef _WIN32
        m_wineSetup.useCustom = settings.value("wine-custom", false).toBool();
        m_wineSetup.wineRoot = settings.value("wine-root", QString()).toString();
        m_wineSetup.useCustomEnv = settings.value("wine-custom-env", false).toBool();
        m_wineSetup.useWinePrefix = settings.value("wine-use-prefix", false).toBool();
        m_wineSetup.winePrefix = settings.value("wine-prefix", QString()).toString();
#endif
    }
    settings.endGroup();

    if(!updateLunaCaps())
        qWarning() << "Failed to initialize LunaLua capabilities!";
}

void LunaTesterEngine::saveSetup()
{
    QString inifile = AppPathManager::settingsFile();
    QSettings settings(inifile, QSettings::IniFormat);

    settings.beginGroup("LunaTester");
    {
        settings.setValue("nogl", m_noGL);
        settings.setValue("kill-engine-on-every-test", m_killPreviousSession);
        settings.setValue("custom-runtime-path", m_customLunaPath);
#ifndef _WIN32
        settings.setValue("wine-custom", m_wineSetup.useCustom);
        settings.setValue("wine-root", m_wineSetup.wineRoot);
        settings.setValue("wine-custom-env", m_wineSetup.useCustomEnv);
        settings.setValue("wine-use-prefix", m_wineSetup.useWinePrefix);
        settings.setValue("wine-prefix", m_wineSetup.winePrefix);
#endif
    }
    settings.endGroup();
}

void LunaTesterEngine::lunaChkResetThread()
{
    QMutexLocker mlocker(&this->m_engine_mutex);
    Q_UNUSED(mlocker)
    QThreadPointNuller tlocker(&this->m_helperThread);
    Q_UNUSED(tlocker)
    SafeMsgBoxInterface msg(&m_w->m_messageBoxer);
    this->m_helperThread = QThread::currentThread();

    if(isEngineActive())
    {
        if(!isOutPipeOpen() || !isInPipeOpen())
            goto failed;
        QJsonDocument jsonOut;
        QJsonObject jsonObj;
        jsonObj["jsonrpc"]  = "2.0";
        jsonObj["method"]   = "resetCheckPoints";
        QJsonObject JSONparams;
        JSONparams["xxx"] = 0;
        jsonObj["params"] = JSONparams;
        jsonObj["id"] = 3;
        jsonOut.setObject(jsonObj);

        //Converting JSON data into net string
        std::string dataToSend;
        jSonToNetString(jsonOut, dataToSend);

        //Send data to SMBX
        if(writeToIPC(dataToSend))
        {
#if 0
            std::string inMessage = readFromIPC();
            LogDebugQD(QString("Received from SMBX JSON message: %1").arg(QString::fromStdString(inMessage)));
            QJsonParseError err;
            QJsonDocument jsonOut;
            if(stringToJson(inMessage, jsonOut, err))
            {
                QJsonObject obj = jsonOut.object();
                if(obj["error"].isNull())
                {
                    msg.info("LunaTester",
                             tr("Checkpoints successfully reseted!"),
                             QMessageBox::Ok);
                }
                else
                {
                    msg.warning("LunaTester",
                                QString::fromStdString(inMessage),
                                QMessageBox::Ok);
                }
            }
            else
            {
                msg.warning(tr("LunaTester error!"),
                            err.errorString() +
                            "\n\nData:\n" +
                            QString::fromStdString(inMessage),
                            QMessageBox::Warning);
            }
#endif
        }
    }
    else
        goto failed;

    return;
failed:
    msg.warning("LunaTester", tr("LunaLUA tester is not started!"), QMessageBox::Ok);
}

#if 0
bool LunaTesterEngine::closeSmbxWindow()
{
#ifdef _WIN32 // Windows-only trick
    if(m_killPreviousSession || (!isOutPipeOpen()) || (!isInPipeOpen()))
        return false;

    QJsonDocument jsonOut;
    QJsonObject jsonObj;
    jsonObj["jsonrpc"]  = "2.0";
    jsonObj["method"]   = "getWindowHandle";
    QJsonObject JSONparams;
    JSONparams["xxx"] = 0;
    jsonObj["params"] = JSONparams;
    jsonObj["id"] = 3;
    jsonOut.setObject(jsonObj);

    //Converting JSON data into net string
    std::string dataToSend;
    jSonToNetString(jsonOut, dataToSend);

    //Send data to SMBX
    if(writeToIPC(dataToSend))
    {
        std::string inMessage = readFromIPC();
        LogDebugQD(QString("Received from SMBX JSON message: %1").arg(QString::fromStdString(inMessage)));
        QJsonParseError err;
        QJsonDocument jsonOut;
        if(stringToJson(inMessage, jsonOut, err))
        {
            QJsonObject obj = jsonOut.object();
            if(obj["error"].isNull())
            {
// #ifdef _WIN32 // Windows-only trick
                uintptr_t smbxHwnd = uintptr_t(obj["result"].toVariant().toULongLong());
                HWND wSmbx = HWND(smbxHwnd);
                //Close SMBX's window
                PostMessage(wSmbx, WM_CLOSE, 0, 0);
                //Wait half of second to avoid racings
                Sleep(500);
// #endif
            }
            else
                LogWarningQD("LunaTester (closeSmbxWindow, obj[\"error\"]): "
                             "LunaLua returned error message:\n" +
                             QString::fromStdString(inMessage));
        }
        else
        {
            LogWarningQD("LunaTester (closeSmbxWindow): Error of parsing "
                         "input data from LunaLua:\n" + err.errorString() +
                         "\n\nData:\n" + QString::fromStdString(inMessage));
        }
        return true;
    }
    return false;
#else
    return true; // Windows-only trick
#endif
}
#endif

bool LunaTesterEngine::switchToSmbxWindow(SafeMsgBoxInterface &msg)
{
    if(!isOutPipeOpen() || !isInPipeOpen())
        return false;

    if(m_caps.features.contains("SelfForegrounding"))
        return true; // this workaround is no more needed
#if 0
    QJsonDocument jsonOut;
    QJsonObject jsonObj;
    jsonObj["jsonrpc"]  = "2.0";
    jsonObj["method"]   = "getWindowHandle";
    QJsonObject JSONparams;
    JSONparams["xxx"] = 0;
    jsonObj["params"] = JSONparams;
    jsonObj["id"] = 3;
    jsonOut.setObject(jsonObj);

    //Converting JSON data into net string
    std::string dataToSend;
    jSonToNetString(jsonOut, dataToSend);

    //Send data to SMBX
    if(writeToIPC(dataToSend))
    {
        std::string inMessage = readFromIPC();
        LogDebugQD(QString("Received from SMBX JSON message: %1").arg(QString::fromStdString(inMessage)));
        QJsonParseError err;
        QJsonDocument jsonOut;
        if(stringToJson(inMessage, jsonOut, err))
        {
            QJsonObject obj = jsonOut.object();
            if(obj["error"].isNull())
            {
                uintptr_t smbxHwnd = uintptr_t(obj["result"].toVariant().toULongLong());
#ifdef _WIN32 // Windows-only trick
                HWND wSmbx = HWND(smbxHwnd);
                //msg.warning("Test2", QString("Received: %1, from: %2").arg(smbxHwnd).arg(QString::fromStdString(inMessage)), QMessageBox::Ok);
                //wchar_t title[MAX_PATH];
                //GetWindowTextW(wSmbx, title, MAX_PATH);
                //msg.warning("Test2", QString("Window title is: %1").arg(QString::fromWCharArray(title)), QMessageBox::Ok);
                ShowWindow(wSmbx, SW_SHOW);
                SetForegroundWindow(wSmbx);
                SetActiveWindow(wSmbx);
                SetFocus(wSmbx);
#else
                QStringList args;
                args << QString::number(smbxHwnd);
                QString cmd = ApplicationPath + "/ipc/luna_hwnd_show.exe";
                if(QFile::exists(cmd))
                {
                    useWine(cmd, args);
                    qDebug() << "Starting HWND bring bridge: " << cmd << args;
                    QProcess q;
                    {
                        auto setup = m_wineSetup;
                        WineSetup::prepareSetup(setup);
                        auto env = WineSetup::buildEnv(setup);
                        q.setProcessEnvironment(env);
                    }
                    q.start(cmd, args);
                    if(q.waitForFinished())
                        qDebug() << q.exitCode();
                    else
                        qWarning() << q.exitCode();
                }
#endif // _WIN32
            }
            else
                msg.warning("LunaTester (switchToSmbxWindow, obj[\"error\"])",
                            "LunaLua returned error message:\n" +
                            QString::fromStdString(inMessage),
                            QMessageBox::Ok);
        }
        else
        {
            msg.warning("LunaTester (switchToSmbxWindow)",
                        "Error of parsing input data from LunaLua:\n" +
                        err.errorString() +
                        "\n\nData:\n" + QString::fromStdString(inMessage),
                        QMessageBox::Warning);
        }
        return true;
    }
#endif
    return false;
}

void LunaTesterEngine::lunaRunnerThread(LevelData in_levelData, const QString &levelPath, bool isUntitled)
{
    QMutexLocker mlocker(&this->m_engine_mutex);
    Q_UNUSED(mlocker)
    QThreadPointNuller tlocker(&this->m_helperThread);
    Q_UNUSED(tlocker)

//    SafeMsgBoxInterface msg(&m_w->m_messageBoxer);

    this->m_helperThread = QThread::currentThread();

    // Prepare level file for SMBX-64 format
    FileFormats::smbx64LevelPrepare(in_levelData);

    if(m_caps.features.contains("SMBX64"))
    {
        int smbx64limits = FileFormats::smbx64LevelCheckLimits(in_levelData);
        if(smbx64limits != FileFormats::SMBX64_FINE)
        {
            int reply = QMessageBox::warning(m_w,
                                    tr("SMBX64 limits are exceeded!"),
                                    tr("Violation of SMBX64 standard has been found!\n"
                                       "%1\n"
                                       ", legacy engine may crash!\n"
                                       "Suggested to remove all excess elements.\n"
                                       "Do you want to continue the process?")
                                    .arg(smbx64ErrMsgs(in_levelData, smbx64limits)),
                                    QMessageBox::Yes | QMessageBox::No);
            if(reply != QMessageBox::Yes)
                return;
        }
    }


    //-----------------------------------------------------------------

    //! Path to SMBX root
    const QString smbxPath = getEnginePath();
    //! Full path to SMBX executable
    const QString smbxExePath = smbxPath + ConfStatus::SmbxEXE_Name;
    //! Full path to LunaDLL library
    const QString lunaDllPath = smbxPath + "LunaDll.dll";
    //! Full path to LunaTester proxy executable
    const QString execProxy = smbxPath + "LunaLoader-exec.exe";

    const QStringList smbxFiles =
    {
        smbxExePath,
        execProxy
    };
    //! Extra checks of SMBX path
    const QStringList smbxDirs =
    {
        smbxPath,
        smbxPath + "graphics",
        smbxPath + "graphics/background",
        smbxPath + "graphics/background2",
        smbxPath + "graphics/block",
        smbxPath + "graphics/npc",
        smbxPath + "graphics/tile",
        smbxPath + "graphics/scene",
        smbxPath + "graphics/path",
        smbxPath + "graphics/level",
        smbxPath + "graphics/player"
    };

    const QString notFoundErrorTitle = LunaTesterEngine::tr("LunaTester directory check failed",
                                                            "A title of a message box that shows when some of the files or directories not exist.");
    const QString notFoundErrorText = LunaTesterEngine::tr("Can't start LunaTester because \"%1\" is not found! That might happen due to any of the following reasons:",
                                                           "A text of a message box that shows when some of the files or directories not exist.");

    const QString explanationGeneric =
            LunaTesterEngine::tr("- Incorrect location of LunaTester (or SMBX2 data root) was specified, please check the LunaTester location setup.\n"
                                 "- Possible removal of files by your antivirus (false positive or an infection of the file), "
                                 "please check your antivirus' quarantine or report of recently removed threats.\n"
                                 "- Incorrect installation of SMBX2 has caused missing files, please reinstall SMBX2 to fix your problem.",
                                 "Description of a problem, showing when LunaTester is NOT a default engine in a current config pack.");

    const QString explanationSMBX2 =
            LunaTesterEngine::tr("- Possible removal of files by your antivirus (false positive or an infection of the file), "
                                 "please check your antivirus' quarantine or report of recently removed threats.\n"
                                 "- Incorrect installation of SMBX2 has caused missing files, please reinstall SMBX2 to fix your problem.",
                                 "Description of a problem, showing when LunaTester is a default engine in a current config pack.");

    for(auto &f : smbxFiles)
    {
        if(!QFile::exists(f))
        {
            QMessageBox::warning(m_w,
                        notFoundErrorTitle,
                        QString("%1\n%2")
                        .arg(notFoundErrorText.arg(f))
                        .arg((ConfStatus::defaultTestEngine == ConfStatus::ENGINE_LUNA) ? explanationSMBX2 : explanationGeneric),
                        QMessageBox::Ok);
            return;
        }
    }

    for(auto &d : smbxDirs)
    {
        QFileInfo i(d);
        if(!i.exists() || !i.isDir())
        {
            QMessageBox::warning(m_w,
                        notFoundErrorTitle,
                        QString("%1\n%2")
                        .arg(notFoundErrorText.arg(d))
                        .arg((ConfStatus::defaultTestEngine == ConfStatus::ENGINE_LUNA) ? explanationSMBX2 : explanationGeneric),
                        QMessageBox::Ok);
            return;
        }
    }

    if(!QFile::exists(lunaDllPath))
    {
        QMessageBox::warning(m_w,
                    LunaTesterEngine::tr("Vanilla SMBX detected!"),
                    LunaTesterEngine::tr("\"%1\" not found!\n"
                                         "You have a Vanilla SMBX!\n"
                                         "That means, impossible to launch level testing with a LunaTester. "
                                         "LunaLua is required to run level testing with SMBX Engine.")
                    .arg(lunaDllPath),
                    QMessageBox::Close);
        return;
    }

    if(isEngineActive())
    {
        if(isOutPipeOpen())
        {
#if 0 // Obsolete
            //Workaround to avoid weird crash
            closeSmbxWindow();
#endif
            //Attempt to switch SMBX Window
//            switchToSmbxWindow(msg);
            //Then send level data to SMBX Engine
            if(sendLevelData(in_levelData, levelPath, isUntitled))
            {
                //Stop music playback in the PGE Editor!
                QMetaObject::invokeMethod(m_w, "setMusicButton", Qt::QueuedConnection, Q_ARG(bool, false));
                // not sure how efficient it is
                QMetaObject::invokeMethod(m_w, "on_actionPlayMusic_triggered", Qt::QueuedConnection, Q_ARG(bool, false));
            }
        }
        else if(QMessageBox::warning(m_w,
                            LunaTesterEngine::tr("SMBX Test is already runned"),
                            LunaTesterEngine::tr("SMBX Engine is already testing another level.\n"
                                                 "Do you want to abort current testing process?"),
                            QMessageBox::Abort | QMessageBox::Cancel) == QMessageBox::Abort)
        {
            killEngine();
        }
        return;
    }

    // Store level buffer
    m_levelTestBuffer = in_levelData;
    m_levelTestPath = levelPath;
    m_levelTestUntitled = isUntitled;

    QString command = execProxy;
    QStringList params;

    /**********************************************
     **********Do LunaLUA testing launch!**********
     **********************************************/
    params << (m_killPreviousSession ? "--patch" : "--hideOnCloseIPC");

    if(m_noGL)
        params << "--nogl";

    QString argString;
    for(int i = 0; i < params.length(); i++)
    {
        if(i > 0)
            argString += " ";
        argString += params[i];
    }

//    bool engineStartedSuccess = true;
    QString engineStartupErrorString;
#ifndef _WIN32
    {
        useWine(command, params);
        auto wSetup = m_wineSetup;
        WineSetup::prepareSetup(wSetup);
        auto env = WineSetup::buildEnv(wSetup);
        m_lunaGameIPC.setProcessEnvironment(env);
//        emit engineSetEnv(env);
    }
#endif
//    emit engineSetExecPath(getEnginePath());
    m_lunaGameIPC.setProgram(command);
//    emit engineSetWorkPath(smbxPath);
    m_lunaGameIPC.setWorkingDirectory(smbxPath);
//    /*emit engine*/Start(command, params, &engineStartedSuccess, &engineStartupErrorString);
    m_lunaGameIPC.start(command, params);

//    engineStartedSuccess = m_lunaGame.waitForStarted();

//    if(engineStartedSuccess)
//    {
//        if(sendLevelData(in_levelData, levelPath, isUntitled))
//        {
//            //GlobalSettings::recentMusicPlayingState = ui->actionPlayMusic->isChecked();
//            //Stop music playback in the PGE Editor!
//            QMetaObject::invokeMethod(m_w, "setMusicButton", Qt::QueuedConnection, Q_ARG(bool, false));
//            QMetaObject::invokeMethod(m_w, "on_actionPlayMusic_triggered", Qt::QueuedConnection, Q_ARG(bool, false));
//        }
//        else
//        {
//            QMessageBox::warning(m_w,
//                        LunaTesterEngine::tr("LunaTester error"),
//                        LunaTesterEngine::tr("Failed to send level into LunaLUA-SMBX!"),
//                        QMessageBox::Ok);
//        }
//    }
//    else
//    {
//        QString luna_error = engineStartupErrorString.isEmpty() ? "Unknown error" : engineStartupErrorString;
//        QMessageBox::warning(m_w,
//                    LunaTesterEngine::tr("LunaTester error"),
//                    LunaTesterEngine::tr("Impossible to launch LunaTester, due to %1").arg(luna_error),
//                    QMessageBox::Ok);
//    }
}

void LunaTesterEngine::lunaRunGame()
{
    //! Path to SMBX root
    const QString smbxPath = getEnginePath();
    //! Full path to SMBX executable
    const QString smbxExePath = smbxPath + ConfStatus::SmbxEXE_Name;
    //! Full path to LunaDLL library
    const QString lunaDllPath = smbxPath + "LunaDll.dll";
    //! Full path to LunaTester proxy executable
    const QString execProxy = smbxPath + "LunaLoader-exec.exe";

    if(m_lunaGame.state() == QProcess::Running)
        m_lunaGame.terminate();

    if(!QFile::exists(execProxy))
    {
        QMessageBox::warning(m_w,
                             LunaTesterEngine::tr("LunaTester Directory wasn't configured right"),
                             LunaTesterEngine::tr("%1 not found! To run testing with LunaTester, you should specify the right SMBX location.")
                                .arg(execProxy),
                             QMessageBox::Ok);
        return;
    }

    QString command = execProxy;
    QStringList params;
    params << "--patch";
    params << "--game";

    if(m_noGL)
        params << "--nogl";

#ifndef _WIN32
    useWine(command, params);
    auto wSetup = m_wineSetup;
    WineSetup::prepareSetup(wSetup);
    auto env = WineSetup::buildEnv(wSetup);
    m_lunaGame.setProcessEnvironment(env);
#endif
    m_lunaGame.setProgram(command);
    m_lunaGame.setArguments(params);
    m_lunaGame.setWorkingDirectory(smbxPath);
    m_lunaGame.start();

//    if(!QFile::exists(lunaDllPath))
//    {
//#ifndef _WIN32
//        useWine(command, params);
//        auto wSetup = m_wineSetup;
//        WineSetup::prepareSetup(wSetup);
//        auto env = WineSetup::buildEnv(wSetup);
//        m_lunaGame.setProcessEnvironment(env);
//#endif
//        m_lunaGame.setProgram(command);
//        m_lunaGame.setArguments(params);
//        m_lunaGame.setWorkingDirectory(smbxPath);
//        m_lunaGame.start();
//    }
//    else
//    {
//        initRuntime();

//        bool engineStartedSuccess = true;
//        QString engineStartupErrorString;
//        killEngine();// Kill previously running game
//#ifndef _WIN32
//        useWine(command, params);
//        auto wSetup = m_wineSetup;
//        WineSetup::prepareSetup(wSetup);
//        auto env = WineSetup::buildEnv(wSetup);
//        emit engineSetEnv(env);
//#endif
//        emit engineSetExecPath(getEnginePath());
//        emit engineSetWorkPath(smbxPath);
//        emit engineStart(command, params, &engineStartedSuccess, &engineStartupErrorString);

//        if(!engineStartedSuccess)
//        {
//            QString luna_error = engineStartupErrorString.isEmpty() ? "Unknown error" : engineStartupErrorString;
//            QMessageBox::warning(m_w,
//                                 LunaTesterEngine::tr("LunaTester error"),
//                                 LunaTesterEngine::tr("Impossible to launch LunaTester, due to %1").arg(luna_error),
//                                 QMessageBox::Ok);
//            return;
//        }
//    }

    //Stop music playback in the PGE Editor!
    QMetaObject::invokeMethod(m_w,
                              "setMusicButton",
                              Qt::QueuedConnection,
                              Q_ARG(bool, false));

    // not sure how efficient it is
    QMetaObject::invokeMethod(m_w,
                              "on_actionPlayMusic_triggered",
                              Qt::QueuedConnection,
                              Q_ARG(bool, false));
}









bool LunaTesterEngine::doTestLevelIPC(const LevelData &d)
{
    return startLunaTester(d);
}

bool LunaTesterEngine::runNormalGame()
{
    lunaRunGame();
    return true;
}

void LunaTesterEngine::terminate()
{
    killEngine();
}

bool LunaTesterEngine::isRunning()
{
    return false;
}

int LunaTesterEngine::capabilities()
{
    return CAP_LEVEL_IPC |
           CAP_RUN_GAME |
           CAP_DAEMON |
           CAP_HAS_MENU;
}