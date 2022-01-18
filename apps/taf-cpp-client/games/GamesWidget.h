#pragma once

#include <QtWidgets/qwidget.h>

#include "ui_GamesWidget.h"
#include "tafclient/TafLobbyClient.h"
#include "GameCreateDialog.h"

class GamesWidget : public QWidget
{
    Q_OBJECT

public:
    GamesWidget(QWidget* parent);
    ~GamesWidget();

private slots:
    void on_createGame_clicked();

private:
    QSharedPointer<Ui::GamesWidget> m_ui;
    GameCreateDialog* m_gameDialog;
};
