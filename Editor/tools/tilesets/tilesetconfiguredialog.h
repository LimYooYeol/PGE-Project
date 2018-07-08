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
#ifndef TILESETCONFIGUREDIALOG_H
#define TILESETCONFIGUREDIALOG_H

#include <QDialog>
#include <QMenu>

#include <data_configs/data_configs.h>

#include "tileset.h"
#include "piecesmodel.h"


namespace Ui {
class TilesetConfigureDialog;
}

class TilesetConfigureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TilesetConfigureDialog(dataconfigs* conf, QGraphicsScene *scene=0, QWidget *parent = 0);
    ~TilesetConfigureDialog();
    enum GFXMode
    {
        GFX_Staff=0,
        GFX_Level,
        GFX_World
    };

    void openTileset(QString filePath, bool isCustom);
    void loadSimpleTileset(const SimpleTileset &tileset, bool isCustom);

private slots:
    void on_clearTileset_clicked();
    void setUpItems(int type);
    void setUpTileset(int type);
    void on_SaveTileset_clicked();

    void on_OpenTileset_clicked();

    void on_customOnly_clicked();
    void on_defaultOnly_clicked();
    void showEvent( QShowEvent * event );
    void showNotify();
    void on_delete_me_clicked();
    void on_search_textChanged(const QString &arg1);

signals:
    void windowShowed();

protected:
    void keyPressEvent(QKeyEvent *event);

private:
    QString lastFileName;
    QString lastFullPath;
    Ui::TilesetConfigureDialog *ui;

    QMenu   m_searchSetup;
    int     m_searchBy = 0;

    GFXMode mode;
    QGraphicsScene *scn;

    tileset* m_tileset;
    int oldWidth;
    int oldHeight;
    ElementsListModel* m_model;
    dataconfigs* m_conf;
    //void setUpItems(ItemTypes type);
    ElementsListModel::ElementType toElementType(int type);
};

#endif // TILESETCONFIGUREDIALOG_H
