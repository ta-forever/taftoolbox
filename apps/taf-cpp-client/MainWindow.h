#pragma once

#include <QtWidgets/qmainwindow.h>

#include "ui_MainWindow.h"
#include "games/GamesWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    QSharedPointer<Ui::MainWindow> m_ui;
    GamesWidget* m_gamesWidget;
};
