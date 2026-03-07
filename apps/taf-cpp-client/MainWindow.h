#pragma once

#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qpushbutton.h>
#include <QtCore/qlist.h>

#include "ui_MainWindow.h"
#include "games/GamesWidget.h"
#include "LogWindow.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);
    ~MainWindow();

private slots:
    void onShowLogsToggled(bool checked);
    void onHostingActiveChanged(bool active);
    void onConnectionStateChanged(bool connected);

private:
    void openGameLogWindows();
    void closeGameLogWindows();

    QSharedPointer<Ui::MainWindow> m_ui;
    GamesWidget* m_gamesWidget;

    QPushButton*      m_connectionIndicator = nullptr;
    LogWindow*        m_clientLogWindow = nullptr;
    QList<LogWindow*> m_gameLogWindows;
};
