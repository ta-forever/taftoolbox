#include "GameCreateDialog.h"

#include "GameCardDelegate.h"
#include "NewGameInfo.h"

#include "GameService.h"
#include "TafService.h"
#include "maps/MapService.h"
#include "player/PlayerService.h"

#include "taflib/Logger.h"

#include "QtCore/qjsonarray.h"
#include "QtCore/qjsondocument.h"
#include "QtCore/qjsonobject.h"
#include "QtCore/qsortfilterproxymodel.h"
#include "QtGui/qpixmap.h"

GameCreateDialog::GameCreateDialog(QWidget* parent) :
    QDialog(parent),
    m_ui(new Ui::GameCreateDialog())
{
    qInfo() << "[GameCreateDialog::GameCreateDialog]";
    m_ui->setupUi(this);
    m_ui->gameCardListView->setModel(&m_prototypeGamesListModel);
    m_ui->gameCardListView->setItemDelegate(new GameCardDelegate());

    m_ui->liveReplayOptionComboBox->addItem("Disabled", QVariant::fromValue(-1));
    m_ui->liveReplayOptionComboBox->addItem("Zero Delay", QVariant::fromValue(0));
    m_ui->liveReplayOptionComboBox->addItem("Five Minute Delay", QVariant::fromValue(300));

    m_ui->mapPreviewComboBox->addItem("Minimap", QVariant::fromValue(MapPreviewType::Mini));
    m_ui->mapPreviewComboBox->addItem("Positions", QVariant::fromValue(MapPreviewType::Positions));
    m_ui->mapPreviewComboBox->addItem("Metal Patches", QVariant::fromValue(MapPreviewType::Mexes));
    m_ui->mapPreviewComboBox->addItem("Geo Vents", QVariant::fromValue(MapPreviewType::Geos));
    m_ui->mapPreviewComboBox->addItem("Reclaim (metal)", QVariant::fromValue(MapPreviewType::Rocks));
    m_ui->mapPreviewComboBox->addItem("Reclaim (energy)", QVariant::fromValue(MapPreviewType::Trees));
    m_ui->mapPreviewComboBox->setCurrentIndex(1);

    initMapListView();

    startTimer(1000);
}

void GameCreateDialog::initMapListView()
{
    m_mapListProxyModel.setSourceModel(&m_mapListTableModel);
    m_mapListProxyModel.sort(int(MapToolDto::Fields::NameStr));
    m_ui->mapListView->setModel(&m_mapListProxyModel);
    m_ui->mapListView->setModelColumn(int(MapToolDto::Fields::NameStr));
    QObject::connect(m_ui->mapListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &GameCreateDialog::onSelectedMapChanged);
    QObject::connect(m_ui->mapPreviewComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int) {
        _updateMapPreview();
    });
    QObject::connect(m_ui->positionsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), [=](int) {
        _updateMapPreview();
    });
}

GameCreateDialog::~GameCreateDialog()
{
}

void GameCreateDialog::timerEvent(QTimerEvent* event)
{
    _updateGamePreview();
}

void GameCreateDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    TafService::getInstance()->getFeaturedMods([this](QSharedPointer<DtoTableModel<FeaturedModDto> > modsTableModel) {
        m_featuredModsTableModel = modsTableModel;
        m_featuredModsProxyModel.setSourceModel(m_featuredModsTableModel.data());
        m_featuredModsProxyModel.sort(int(FeaturedModDto::Fields::OrderInt));
        m_featuredModsProxyModel.setFilterKeyColumn(int(FeaturedModDto::Fields::VisibleBool));
        m_featuredModsProxyModel.setFilterFixedString("true");
        m_ui->modListView->setModel(&m_featuredModsProxyModel);
        m_ui->modListView->setModelColumn(int(FeaturedModDto::Fields::DisplayNameStr));
        QObject::disconnect(m_ui->modListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &GameCreateDialog::onSelectedModChanged);
        QObject::connect(m_ui->modListView->selectionModel(), &QItemSelectionModel::currentChanged, this, &GameCreateDialog::onSelectedModChanged);
        m_ui->modListView->setCurrentIndex(m_featuredModsProxyModel.index(0, 0));
    });
}

void GameCreateDialog::onSelectedModChanged(const QModelIndex& current, const QModelIndex& previous)
{
    populateMapList();
}

void GameCreateDialog::onSelectedMapChanged(const QModelIndex& current, const QModelIndex& previous)
{
    _updateMapPreview();
}

void GameCreateDialog::on_createPushButton_clicked(bool)
{
    qInfo() << "[GameCreateDialog::on_create_PushButton_clicked]";
    const FeaturedModDto* selectedMod = _getSelectedMod();
    const MapToolDto* selectedMap = _getSelectedMap();
    QString title = m_ui->titleLineEdit->text();

    if (!selectedMod || !selectedMap || title.isEmpty())
    {
        return;
    }

    GameService::getInstance()->hostGame(NewGameInfo(
        title, m_ui->passwordLineEdit->text(), selectedMod->technicalName, selectedMap->name,
        m_ui->friendsOnlyCheckBox->isChecked() ? "friends" : "public",
        300, "global"));
}

void GameCreateDialog::populateMapList()
{
    const FeaturedModDto* selectedMod = _getSelectedMod();
    if (!selectedMod)
    {
        return;
    }

    this->m_mapListTableModel.clear();
    MapService::getInstance()->getInstalledMaps(selectedMod->technicalName, false, [=](const QList<MapToolDto>& mapList) {
        this->m_mapListTableModel.append(mapList.begin(), mapList.end());
    });
}


void GameCreateDialog::_updateGamePreview()
{
    const FeaturedModDto* selectedMod = _getSelectedMod();
    const MapToolDto* selectedMap = _getSelectedMap();

    QSharedPointer<TafLobbyGameInfo> prototypeGame(new TafLobbyGameInfo());
    prototypeGame->id = 0;
    prototypeGame->host = PlayerService::getInstance()->getCurrentUser()->login;
    prototypeGame->title = m_ui->titleLineEdit->text();
    if (selectedMod)
    {
        prototypeGame->featuredMod = selectedMod->technicalName;
    }
    if (selectedMap)
    {
        prototypeGame->mapName = selectedMap->name;
    }
    prototypeGame->ratingType = "global";
    prototypeGame->visibility = m_ui->friendsOnlyCheckBox->isChecked() ? "friends" : "public";
    prototypeGame->passwordProtected = !m_ui->passwordLineEdit->text().isEmpty();
    prototypeGame->state = "staging";
    prototypeGame->replayDelaySeconds = m_ui->liveReplayOptionComboBox->currentData().toInt();
    prototypeGame->numPlayers = 1;
    prototypeGame->maxPlayers = 10;
    m_prototypeGamesListModel.updateGame(prototypeGame);
}

void GameCreateDialog::_updateMapPreview()
{
    const FeaturedModDto* selectedMod = _getSelectedMod();
    const MapToolDto* selectedMap = _getSelectedMap();
    if (!selectedMod || !selectedMap)
    {
        return;
    }

    MapPreviewType previewType = m_ui->mapPreviewComboBox->currentData().value<MapPreviewType>();
    int positionCount = m_ui->positionsSpinBox->value();
    MapService::getInstance()->getPreview(selectedMap->name, previewType, positionCount, selectedMod->technicalName, [=](QString previewPath) {
        if (previewPath.isEmpty())
        {
            previewPath = ":/res/games/unknown_map.png";
        }
        m_ui->mapPreviewImageLabel->setPixmap(QPixmap(previewPath).scaled(360, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    });

    if (previewType != MapPreviewType::Mini)
    {
        MapService::getInstance()->getPreview(selectedMap->name, MapPreviewType::Mini, positionCount, selectedMod->technicalName, [=](QString) {
        });
    }
}

const FeaturedModDto* GameCreateDialog::_getSelectedMod() const
{
    if (!m_ui->modListView->selectionModel())
    {
        return NULL;
    }

    QModelIndex modIndex = m_ui->modListView->selectionModel()->currentIndex();
    if (!modIndex.isValid())
    {
        return NULL;
    }

    int row = m_featuredModsProxyModel.mapToSource(modIndex).row();
    return m_featuredModsTableModel->getDtoByRow(row);
}

const MapToolDto* GameCreateDialog::_getSelectedMap() const
{
    if (!m_ui->mapListView->selectionModel())
    {
        return NULL;
    }

    QModelIndex modIndex = m_ui->mapListView->selectionModel()->currentIndex();
    if (!modIndex.isValid())
    {
        return NULL;
    }

    int row = m_mapListProxyModel.mapToSource(modIndex).row();
    return m_mapListTableModel.getDtoByRow(row);
}
