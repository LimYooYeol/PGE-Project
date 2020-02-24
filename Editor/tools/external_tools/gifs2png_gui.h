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

#pragma once
#ifndef GIFS2PNG_GUI_H
#define GIFS2PNG_GUI_H

#include <QDialog>
#include <QProcess>

namespace Ui {
class gifs2png_gui;
}

class gifs2png_gui : public QDialog
{
    Q_OBJECT

public:
    explicit gifs2png_gui(QWidget *parent = 0);
    ~gifs2png_gui();

private slots:
    void on_BrowseInput_clicked();
    void on_BrowseOutput_clicked();
    void on_startTool_clicked();

    void on_close_clicked();

    void consoleMessage();

private:
    QProcess proc;
    Ui::gifs2png_gui *ui;
};

#endif // GIFS2PNG_GUI_H
