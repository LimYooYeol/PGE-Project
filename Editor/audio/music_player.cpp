/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifdef Q_OS_MACX
#undef Q_OS_MACX //Workaroud for an AUTOMOC
#endif

#include <ui_mainwindow.h>
#include <mainwindow.h>

#include <common_features/main_window_ptr.h>

#include "sdl_music_player.h"
#include "music_player.h"
#include <main_window/dock/lvl_sctc_props.h>

QString LvlMusPlay::currentCustomMusic;


QString LvlMusPlay::currentMusicPath;
QString LvlMusPlay::lastMusicPath;


long LvlMusPlay::currentMusicId=0;
long LvlMusPlay::currentWldMusicId=0;
long LvlMusPlay::currentSpcMusicId=0;
bool LvlMusPlay::musicButtonChecked;
bool LvlMusPlay::musicForceReset=false;
int LvlMusPlay::musicType=LvlMusPlay::LevelMusic;


void MainWindow::on_actionPlayMusic_triggered(bool checked)
{
    LogDebug("Clicked play music button");
    setMusic(checked);
}

// TODO: Refactor this
void LvlMusPlay::setMusic(LvlMusPlay::MusicType mt, unsigned long id, QString cmus)
{
    QString root = ".";
    MainWindow  *mw = MainWinConnect::pMainWin;
    if(!mw) return;

    if(id==0)
    {
        setNoMusic();
        return;
    }

    if(mw->activeChildWindow() == MainWindow::WND_Level)
    {
        if(mw->activeLvlEditWin() != nullptr)
            root = mw->activeLvlEditWin()->LvlData.meta.path+"/";
    }
    else
    if(mw->activeChildWindow() == MainWindow::WND_World)
    {
        if(mw->activeWldEditWin() != nullptr)
            root = mw->activeWldEditWin()->WldData.meta.path+"/";
    }

    //Force correction of Windows paths into UNIX style
    cmus.replace('\\', '/');

    switch(mt)
    {
        case LevelMusic:
            if(id==mw->configs.music_custom_id)
            {
                LogDebug(QString("get Custom music path"));
                currentMusicPath = root + cmus;
            }
            else
            {
                LogDebug(QString("get standart music path (level)"));
                QString musicFile;
                long j = mw->configs.getMusLvlI(id);
                if(j>=0)
                {
                    if(id==mw->configs.main_music_lvl[j].id)
                    {
                        musicFile = mw->configs.main_music_lvl[j].file;
                    }
                }
                currentMusicPath = mw->configs.dirs.music + musicFile;
            }
            break;
        case WorldMusic:
            if(id==mw->configs.music_w_custom_id)
            {
                LogDebug(QString("get Custom music path"));
                currentMusicPath = root + cmus;
            }
            else
            {
                LogDebug(QString("get standart music path (world)"));
                QString musicFile;
                long j = mw->configs.getMusWldI(id);
                if(j>=0)
                {
                    if(id==mw->configs.main_music_wld[j].id)
                    {
                        musicFile = mw->configs.main_music_wld[j].file;
                    }
                }
                currentMusicPath = mw->configs.dirs.music + musicFile;
            }
            break;
        case SpecialMusic:
            {
                LogDebug(QString("get standart music path (special)"));
                QString musicFile;
                long j = mw->configs.getMusSpcI(id);
                if(j>=0)
                {
                    if(id==mw->configs.main_music_spc[j].id)
                    {
                        musicFile = mw->configs.main_music_spc[j].file;
                    }
                }
                currentMusicPath = mw->configs.dirs.music + musicFile;
            }
            break;
    }

    LogDebug(QString("path is %1").arg(currentMusicPath));

    QString trackNum;
    if(currentMusicPath.contains('|'))
    {
        QStringList x=currentMusicPath.split("|");
        currentMusicPath=x[0];
        trackNum=x[1];
    }
    QFileInfo mus(currentMusicPath);
    if((!mus.exists())||(!mus.isFile()))
    {
        currentMusicPath.clear();
    }
    else
    {
        if(!trackNum.isEmpty())
        {
            currentMusicPath=currentMusicPath+"|"+trackNum;
        }
    }
}

void LvlMusPlay::setNoMusic()
{
    currentMusicPath.clear();
}

// TODO: Refactor this
void LvlMusPlay::updateMusic()
{
    int w_id = MainWinConnect::pMainWin->activeChildWindow();

    switch(w_id)
    {
    case MainWindow::WND_NoWindow:
        setNoMusic();
        updatePlayerState(false);
        break;
    case MainWindow::WND_Level:
        MainWinConnect::pMainWin->dock_LvlSectionProps->loadMusic();
        break;
    case MainWindow::WND_NpcTxt:
        setNoMusic();
        updatePlayerState(false);
        break;
    case MainWindow::WND_World:
        {
            WorldEdit *w = MainWinConnect::pMainWin->activeWldEditWin();
            if(!w)
                return;
            setMusic(LvlMusPlay::WorldMusic, static_cast<unsigned long>(w->currentMusic), w->currentCustomMusic);
            MainWinConnect::pMainWin->setMusic();
        }
        break;
    default: break;
    }

}

void LvlMusPlay::updatePlayerState(bool playing)
{
    if(!LvlMusPlay::musicForceReset)
    {
        if(
            (!lastMusicPath.isNull()) &&
            (lastMusicPath == currentMusicPath)&&
            (musicButtonChecked == playing)
                )
                return;

    }
    else
    {
        musicForceReset=false;
    }

    if((playing==false) || (currentMusicPath.isNull()))
    {
        PGE_MusPlayer::MUS_stopMusic();
    }
    else
    {
        PGE_MusPlayer::MUS_stopMusic();
        PGE_MusPlayer::MUS_openFile( currentMusicPath );
        PGE_MusPlayer::MUS_changeVolume(MainWinConnect::pMainWin->musicVolume());
        PGE_MusPlayer::MUS_playMusic();
    }

    lastMusicPath = currentMusicPath;
    musicButtonChecked  = playing;
}

void LvlMusPlay::stopMusic()
{
    setNoMusic();
    updatePlayerState(false);
}

/*******************************************************************************/
void MainWindow::setMusic(bool checked)
{
    checked = ui->actionPlayMusic->isChecked();

    if( configs.check() )
    {
        LogCritical(QString("Error! *.INI Configs for music not loaded"));
        return;
    }

    LvlMusPlay::updatePlayerState(checked);
}

void MainWindow::setMusicButton(bool checked)
{
    ui->actionPlayMusic->setChecked(checked);
}

int MainWindow::musicVolume()
{
    if(m_ui_musicVolume)
        return m_ui_musicVolume->value();
    else
        return 128;
}

