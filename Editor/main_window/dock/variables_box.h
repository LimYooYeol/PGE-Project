#pragma once
#ifndef VARIABLES_BOX_H
#define VARIABLES_BOX_H

#include <QDockWidget>
#include "mwdock_base.h"

class MainWindow;

namespace Ui {
class VariablesBox;
}

class VariablesBox : public QDockWidget, public MWDock_Base
{
    Q_OBJECT

    friend class MainWindow;
public:
    explicit VariablesBox(QWidget *parent = nullptr);
    ~VariablesBox();

public slots:
    void re_translate();
    void refreshInformation();

private:
    Ui::VariablesBox *ui;
};

#endif // VARIABLES_BOX_H
