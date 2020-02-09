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

#include "musicfilelist.h"

#include <QIcon>

MusicFileList::MusicFileList(QString searchDirectory, QString curFile, QWidget *parent, bool sfxMode) :
    FileListBrowser(searchDirectory, curFile, parent),
    m_sfxMode(sfxMode)
{
    QStringList filters;

    setIcon(QIcon(":/images/playmusic.png"));

    if(m_sfxMode)
    {
        setWindowTitle(tr("Select SFX file"));
        filters // MPEG 1 Layer III (LibMAD)
               << "*.mp3"
               // OGG Vorbis, FLAC, amd Opus (LibOGG, LibVorbis, LibFLAC, libOpus)
               << "*.ogg" << "*.flac" << "*.opus"
               // Uncompressed audio data
               << "*.wav" << "*.voc" << "*.aiff"
               // Some chiptunes can be used as SFX
               << "*.spc";
        setFilters(tr("Supported SFX formats"), filters);
    }
    else
    {
        setWindowTitle(tr("Select Custom music"));
        filters // MPEG 1 Layer III (LibMAD)
                << "*.mp3"
                // OGG Vorbis, FLAC, amd Opus (LibOGG, LibVorbis, LibFLAC, libOpus)
                << "*.ogg" << "*.flac" << "*.opus"
                // Uncompressed audio data
                << "*.wav" << "*.voc" << "*.aiff"
                // MIDI
                << "*.mid" << "*.midi" << "*.rmi" << "*.mus" << "*.kar" << "*.xmi" << "*.cmf"
                // Id Music File (OPL2 raw) / Imago Orpheus (Tracker music)
                << "*.imf"
                // Modules (Tracker music)
                << "*.mod" << "*.it" << "*.s3m" << "*.669" << "*.med" << "*.xm" << "*.amf"
                << "*.apun" << "*.dsm" << "*.far" << "*.gdm" << "*.mtm"
                << "*.okt" << "*.stm" << "*.stx" << "*.ult" << "*.uni" << "*.mptm"
                // GAME EMU (Chiptunes)
                << "*.ay" << "*.gbs" << "*.gym" << "*.hes" << "*.kss" << "*.nsf"
                << "*.nsfe" << "*.sap" << "*.spc" << "*.vgm" << "*.vgz";
        setFilters(tr("Supported music formats"), filters);
    }

}

MusicFileList::~MusicFileList()
{}
