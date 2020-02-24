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

#include "file_keeper.h"

#include <QFileInfo>
#include <QDir>

static QString getRandomString()
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    const int randomStringLength = 12; // assuming you want random strings of 12 characters

    QString randomString;
    for(int i = 0; i < randomStringLength; ++i)
    {
        int index = qrand() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}

FileKeeper::FileKeeper(const QString &path) :
    m_origPath(path)
{
    if(!QFile::exists(path))
    {
        m_tempPath = m_origPath;
        return; //Do nothing when file is not exists (aka, new-made)
    }

    QFileInfo info = QFileInfo(path);
    QString dirPath = info.absoluteDir().absolutePath() + "/";
    QString baseName = info.baseName();
    QString suffix = info.suffix();

    QString targetPath;
    do
    {
        targetPath = dirPath + baseName + "." + getRandomString() + "." + suffix;
    } while(QFile::exists(targetPath));

    if(QFile::rename(m_origPath, targetPath))
    {
        m_tempPath = targetPath;
    }
}

FileKeeper::~FileKeeper()
{
    remove();
}

QString FileKeeper::tempPath()
{
    return m_tempPath;
}

void FileKeeper::remove()
{
    if(m_tempPath == m_origPath)
        return;
    if(!m_tempPath.isEmpty() && QFile::exists(m_tempPath))
    {
        QFile::remove(m_tempPath);
        m_tempPath.clear();
    }
}

void FileKeeper::restore()
{
    if(m_tempPath == m_origPath)
        return;
    if(!m_origPath.isEmpty() && !m_tempPath.isEmpty() && QFile::exists(m_tempPath))
    {
        if(QFile::exists(m_origPath))
            QFile::remove(m_origPath);// Clean up for just a case
        QFile::rename(m_tempPath, m_origPath);
        m_tempPath.clear();
    }
}
