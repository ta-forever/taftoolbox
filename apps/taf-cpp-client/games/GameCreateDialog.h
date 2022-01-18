#pragma once

#include "api/FeaturedModDto.h"
#include "DtoTableModel.h"
#include "GamesListModel.h"
#include "ui_GameCreateDialog.h"
#include "ta/MapToolDto.h"

#include <QtWidgets/qdatawidgetmapper.h>
#include <QtCore/qsortfilterproxymodel.h>

class GameCreateDialog : public QDialog
{
    Q_OBJECT

public:
    GameCreateDialog(QWidget* parent = 0);
    ~GameCreateDialog();

    void showEvent(QShowEvent* event);
    void populateMapList();

public slots:
    void onSelectedModChanged(const QModelIndex& current, const QModelIndex& previous);
    void onSelectedMapChanged(const QModelIndex& current, const QModelIndex& previous);
    void on_createPushButton_clicked(bool);

private:
    QSharedPointer<Ui::GameCreateDialog> m_ui;
    GamesListModel m_prototypeGamesListModel;
    QSharedPointer <DtoTableModel<FeaturedModDto> > m_featuredModsTableModel;
    QSortFilterProxyModel m_featuredModsProxyModel;

    DtoTableModel<MapToolDto> m_mapListTableModel;
    QSortFilterProxyModel m_mapListProxyModel;
    void initMapListView();

    void timerEvent(QTimerEvent* event) override;

    void _updateGamePreview();
    void _updateMapPreview();

    const FeaturedModDto* _getSelectedMod() const;
    const MapToolDto* _getSelectedMap() const;
};
