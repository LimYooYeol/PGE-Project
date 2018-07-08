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

#pragma once
#ifndef APP_PATH_H
#define APP_PATH_H

#include <QString>

extern QString ApplicationPath;
extern QString ApplicationPath_x;

class AppPathManager
{
    public:
        static void initAppPath();
        static QString settingsFile();
        static QString userAppDir();
        static void install();
        static bool isPortable();
        static bool userDirIsAvailable();

private:
        /**
         * @brief Makes settings path if not exists
         */
        static void initSettingsPath();
        //! Full path to settings INI file
        static QString m_settingsPath;
        static QString m_userPath;
};

#endif // APP_PATH_H
