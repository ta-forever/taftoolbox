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
    void on_joinGame_clicked();
    void on_launchGame_clicked();
    void on_closeGame_clicked();
    void onGameSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
    void onHostingActiveChanged(bool active);
    void onGameListContextMenu(const QPoint& pos);

private:
    void enforceSelectionLock();

    QSharedPointer<Ui::GamesWidget> m_ui;
    GameCreateDialog* m_gameDialog;
    bool m_lockingSelection = false;
};
