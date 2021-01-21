/*
 * Copyright (C) 2016 ~ 2018 Wuhan Deepin Technology Co., Ltd.
 *
 * Author:     Iceyer <me@iceyer.net>
 *
 * Maintainer: Iceyer <me@iceyer.net>
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

#include "musicsonglistview.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMimeData>

#include <DMenu>
#include <DDialog>
#include <DScrollBar>
#include <DPalette>
#include <DApplicationHelper>
#include <QDomDocument>
#include <QDomElement>
#include <QSvgRenderer>
#include <QPainter>
#include <QShortcut>
#include <QUuid>
#include "mediameta.h"

#include "musicsettings.h"
#include "commonservice.h"
#include "musicbaseandsonglistmodel.h"
#include "databaseservice.h"
#include "player.h"
#include "playlistview.h"
#include "ac-desktop-define.h"

DGUI_USE_NAMESPACE

MusicSongListView::MusicSongListView(QWidget *parent) : DListView(parent)
{
    this->setEditTriggers(NoEditTriggers);

    model = new MusicBaseAndSonglistModel(this);
    setModel(model);
    delegate = new DStyledItemDelegate(this);
    //delegate->setBackgroundType(DStyledItemDelegate::NoBackground);
    auto delegateMargins = delegate->margins();
    delegateMargins.setLeft(18);
    delegate->setMargins(delegateMargins);
    setItemDelegate(delegate);

    setViewportMargins(8, 0, 8, 0);

    playingPixmap = QPixmap(":/mpimage/light/music1.svg");
    albumPixmap = QPixmap(":/mpimage/light/music_withe_sidebar/music1.svg");
    defaultPixmap = QPixmap(":/mpimage/light/music_withe_sidebar/music1.svg");
    auto font = this->font();
    font.setFamily("SourceHanSansSC");
    font.setWeight(QFont::Medium);
    setFont(font);

    setIconSize(QSize(ItemIconSide, ItemIconSide));
    setItemSize(QSize(ItemHeight, ItemHeight));

    setFrameShape(QFrame::NoFrame);

    DPalette pa = DApplicationHelper::instance()->palette(this);
    pa.setColor(DPalette::ItemBackground, Qt::transparent);
    DApplicationHelper::instance()->setPalette(this, pa);

    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setSelectionMode(QListView::SingleSelection);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &MusicSongListView::customContextMenuRequested,
            this, &MusicSongListView::showContextMenu);

    init();
    initShortcut();
    initRenameLineEdit();

    connect(this, &MusicSongListView::clicked, this, [](QModelIndex midx) {
        qDebug() << "customize midx.row()" << midx.row();
        emit CommonService::getInstance()->signalSwitchToView(CustomType, midx.data(Qt::UserRole).toString());
    });

    connect(Player::getInstance(), &Player::signalUpdatePlayingIcon,
            this, &MusicSongListView::slotUpdatePlayingIcon);

    connect(this, &MusicSongListView::doubleClicked, this, &MusicSongListView::slotDoubleClicked);
    connect(this, &MusicSongListView::currentChanged, this, &MusicSongListView::slotCurrentChanged);

    connect(CommonService::getInstance(), &CommonService::signalAddNewSongList, this, &MusicSongListView::addNewSongList);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &MusicSongListView::setThemeType);

    setThemeType(DGuiApplicationHelper::instance()->themeType());
}

MusicSongListView::~MusicSongListView()
{

}

void MusicSongListView::init()
{
    QList<DataBaseService::PlaylistData> list = DataBaseService::getInstance()->allPlaylistMeta();

    for (int i = 0; i < list.size(); i++) {
        DataBaseService::PlaylistData data = list.at(i);
        if (data.uuid == "album" || data.uuid == "artist" || data.uuid == "all" || data.uuid == "fav" ||
                data.uuid == "play" || data.uuid == "musicCand" || data.uuid == "albumCand" || data.uuid == "artistCand" ||
                data.uuid == "musicResult" || data.uuid == "albumResult" || data.uuid == "artistResult" ||
                data.uuid == "search") {
            continue;
        }
        QString displayName = data.displayName;
        DStandardItem *item = new DStandardItem(QIcon::fromTheme("music_famousballad"), displayName);

        item->setData(data.uuid, Qt::UserRole);

        if (DGuiApplicationHelper::instance()->themeType() == 1) {
            item->setForeground(QColor("#414D68"));
        } else {
            item->setForeground(QColor("#C0C6D4"));
        }
        model->appendRow(item);
    }

    setMinimumHeight(model->rowCount() * ItemHeight);
}

void MusicSongListView::showContextMenu(const QPoint &pos)
{
    auto index = indexAt(pos);
    if (!index.isValid())
        return;

    auto item = model->itemFromIndex(index);
    if (!item) {
        return;
    }

    QPoint globalPos = this->mapToGlobal(pos);


    DMenu menu;
    connect(&menu, &DMenu::triggered, this, &MusicSongListView::slotMenuTriggered);

    QAction *playact = nullptr;
    QAction *pauseact = nullptr;

    QString hash = index.data(Qt::UserRole).value<QString>();
    emit CommonService::getInstance()->signalSwitchToView(ListPageSwitchType::CustomType, hash);

    if (hash == Player::getInstance()->getCurrentPlayListHash() && Player::getInstance()->status() == Player::Playing) {
        pauseact = menu.addAction(tr("Pause"));
        pauseact->setDisabled(0 == DataBaseService::getInstance()->customizeMusicInfos(hash).size());
    } else {
        playact = menu.addAction(tr("Play"));
        playact->setDisabled(0 == DataBaseService::getInstance()->customizeMusicInfos(hash).size());
    }

    if (hash != "album" || hash != "artist" || hash != "all" || hash != "fav") {
        menu.addAction(tr("Rename"));
        menu.addAction(tr("Delete"));
    }

    menu.exec(globalPos);
}

void MusicSongListView::adjustHeight()
{
    setMinimumHeight(model->rowCount() * ItemHeight);
}

bool MusicSongListView::getHeightChangeToMax()
{
    return m_heightChangeToMax;
}

void MusicSongListView::addNewSongList()
{
// 编辑功能逻辑变更，代码废弃
//    //close editor
//    for (int i = 0; i < model->rowCount(); i++) {
//        auto item = model->index(i, 0);
//        if (this->isPersistentEditorOpen(item))
//            closePersistentEditor(item);
//    }
    // 如果二级页面显示则隐藏
    emit CommonService::getInstance()->signalHideSubSonglist();

    qDebug() << "new item";
    QIcon icon = QIcon::fromTheme("music_famousballad");

    QString displayName = newDisplayName(); //translation? from playlistmanager
    DStandardItem *item = new DStandardItem(icon, displayName);
    if (DGuiApplicationHelper::instance()->themeType() == 1) {
        item->setForeground(QColor("#414D68"));
    } else {
        item->setForeground(QColor("#C0C6D4"));
    }
    model->appendRow(item);
    setMinimumHeight(model->rowCount() * ItemHeight);
    setCurrentIndex(model->indexFromItem(item));
    slotDoubleClicked(model->indexFromItem(item));
    scrollToBottom();

    //record to db
    DataBaseService::PlaylistData info;
    info.editmode = true;
    info.readonly = false;
    info.uuid = QUuid::createUuid().toString().remove("{").remove("}").remove("-");
    info.displayName = displayName;
    info.sortID = DataBaseService::getInstance()->getPlaylistMaxSortid();
    info.sortType = DataBaseService::SortByAddTimeASC;
    DataBaseService::getInstance()->addPlaylist(info);
    item->setData(info.uuid, Qt::UserRole); //covert to hash
    // 切换listpage
    emit CommonService::getInstance()->signalSwitchToView(CustomType, info.uuid);

    //主页面清空选择项
    emit sigAddNewSongList();
    m_heightChangeToMax = true;
    adjustHeight();
}

void MusicSongListView::rmvSongList()
{
    QModelIndex index = this->currentIndex();
    if (index.row() >= 0) {
        QString message = QString(tr("Are you sure you want to delete this playlist?"));

        DDialog warnDlg(this);
        warnDlg.setIcon(QIcon::fromTheme("deepin-music"));
        warnDlg.setTextFormat(Qt::AutoText);
        warnDlg.setTitle(message);
        warnDlg.addSpacing(20);
        warnDlg.addButton(tr("Cancel"), false, Dtk::Widget::DDialog::ButtonNormal);
        warnDlg.addButton(tr("Delete"), true, Dtk::Widget::DDialog::ButtonWarning);
        warnDlg.setObjectName(AC_MessageBox);
        if (1 == warnDlg.exec()) {
            QString hash = index.data(Qt::UserRole).value<QString>();

            QStandardItem *item = model->itemFromIndex(index);
            model->removeRow(item->row());
            // 清除选中，防止连续点击delete删除
            this->setCurrentIndex(QModelIndex());
            DataBaseService::getInstance()->deletePlaylist(hash);
            // 切换到所有音乐界面
            emit CommonService::getInstance()->signalSwitchToView(AllSongListType, "all");
            // 清除选中状态
            this->clearSelection();
            // 设置所有音乐播放状态
            Player::getInstance()->setCurrentPlayListHash("all", false);
            // 记忆播放歌单
            MusicSettings::setOption("base.play.last_playlist", "all");
        }
        //删除消息，让scroll自动刷新
//        emit sigRmvSongList();
        m_heightChangeToMax = false;
        adjustHeight();
    }
}

void MusicSongListView::slotUpdatePlayingIcon()
{
    for (int i = 0; i < model->rowCount(); i++) {
        QModelIndex index = model->index(i, 0);
        DStandardItem *item = dynamic_cast<DStandardItem *>(model->item(i, 0));
        if (item == nullptr) {
            continue;
        }
        QString hash = index.data(Qt::UserRole).value<QString>();
        if (hash == Player::getInstance()->getCurrentPlayListHash()) {
            QPixmap playingPixmap = QPixmap(ItemIconSide, ItemIconSide);
            playingPixmap.fill(Qt::transparent);
            QPainter painter(&playingPixmap);
            DTK_NAMESPACE::Gui::DPalette pa;// = this->palette();
            if (selectedIndexes().size() > 0 && (selectedIndexes().at(0) == index)) {
                painter.setPen(QColor(Qt::white));
            } else {
                painter.setPen(pa.color(QPalette::Active, DTK_NAMESPACE::Gui::DPalette::Highlight));
            }
            Player::getInstance()->playingIcon().paint(&painter, QRect(0, 0, ItemIconSide, ItemIconSide), Qt::AlignCenter, QIcon::Active, QIcon::On);

            QIcon playingIcon(playingPixmap);
            DViewItemActionList actionList = item->actionList(Qt::RightEdge);
            if (!actionList.isEmpty()) {
                actionList.first()->setIcon(playingIcon);
            } else {
                actionList.clear();
                auto viewItemAction = new DViewItemAction(Qt::AlignCenter, QSize(ItemIconSide, ItemIconSide));
                viewItemAction->setIcon(playingIcon);
                actionList.append(viewItemAction);
                dynamic_cast<DStandardItem *>(item)->setActionList(Qt::RightEdge, actionList);
            }
        } else {
            QIcon playingIcon;
            DViewItemActionList actionList = item->actionList(Qt::RightEdge);
            if (!actionList.isEmpty()) {
                actionList.first()->setIcon(playingIcon);
            } else {
                actionList.clear();
                auto viewItemAction = new DViewItemAction(Qt::AlignCenter, QSize(ItemIconSide, ItemIconSide));
                viewItemAction->setIcon(playingIcon);
                actionList.append(viewItemAction);
                dynamic_cast<DStandardItem *>(item)->setActionList(Qt::RightEdge, actionList);
            }
        }
    }
    update();
}

void MusicSongListView::slotMenuTriggered(QAction *action)
{
    QModelIndex index = this->currentIndex();
    if (!index.isValid())
        return;

    if (action->text() == tr("Play")) {
        emit CommonService::getInstance()->signalPlayAllMusic();
    } else if (action->text() == tr("Pause")) {
        Player::getInstance()->pause();
    } else if (action->text() == tr("Rename")) {
        slotDoubleClicked(index);
    } else if (action->text() == tr("Delete")) {
        rmvSongList();
    }
}

void MusicSongListView::slotCurrentChanged(const QModelIndex &cur, const QModelIndex &pre)
{
// 逻辑变更，目前不用这段代码
//    m_renameItem = dynamic_cast<DStandardItem *>(model->itemFromIndex(cur));
//    if (m_renameItem) {
//        m_renameItem->setIcon(QIcon::fromTheme("music_famousballad"));
//    }

    DStandardItem *preStandardItem = dynamic_cast<DStandardItem *>(model->itemFromIndex(pre));
    if (preStandardItem) {
        preStandardItem->setIcon(QIcon::fromTheme("music_famousballad"));
    }
}

void MusicSongListView::slotDoubleClicked(const QModelIndex &index)
{
    m_renameItem = dynamic_cast<DStandardItem *>(model->itemFromIndex(index));

    if (!m_renameItem)
        return;

    m_renameLineEdit->setVisible(true);
    m_renameLineEdit->move(50, m_renameItem->row() * this->sizeHintForIndex(index).height() + ItemEditMargin);
    m_renameLineEdit->setVisible(true);
    m_renameLineEdit->lineEdit()->setText(m_renameItem->text());
    m_renameLineEdit->lineEdit()->selectAll();
    m_renameLineEdit->lineEdit()->setFocus();
}

void MusicSongListView::slotLineEditingFinished()
{
    if (m_renameLineEdit->isVisible()) {
        m_renameLineEdit->setVisible(false);

        if (!m_renameItem)
            return;

        m_renameItem->setText(m_renameLineEdit->text());
        QString uuid = m_renameItem->data(Qt::UserRole).value<QString>();
        // 歌单名去重
        for (int i = 0; i < model->rowCount() ; ++i) {
            DStandardItem *tmpItem = dynamic_cast<DStandardItem *>(model->itemFromIndex(model->index(i, 0)));
            if (m_renameItem->text().isEmpty() || (m_renameItem->row() != tmpItem->row() && m_renameItem->text() == tmpItem->text())) {
                QList<DataBaseService::PlaylistData> plist = DataBaseService::getInstance()->getCustomSongList();
                for (DataBaseService::PlaylistData data : plist) {
                    if (uuid == data.uuid) {
                        m_renameItem->setText(data.displayName);// 还原歌单名
                    }
                }
                m_renameItem->setIcon(QIcon::fromTheme("music_famousballad"));
                return;
            }
        }

        DataBaseService::getInstance()->updatePlaylistDisplayName(m_renameItem->text(), uuid);
        m_renameItem->setIcon(QIcon::fromTheme("music_famousballad"));
    }
}

void MusicSongListView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    m_renameLineEdit->resize(this->width() - 64, ItemHeight - ItemEditMargin * 2);
}

void MusicSongListView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete) {
        rmvSongList();
    }
    DListView::keyReleaseEvent(event);
}

void MusicSongListView::dragEnterEvent(QDragEnterEvent *event)
{
    auto t_formats = event->mimeData()->formats();
    if (event->mimeData()->hasFormat("text/uri-list") || event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
    }
}

void MusicSongListView::dragMoveEvent(QDragMoveEvent *event)
{
    auto index = indexAt(event->pos());
    if (index.isValid() && (event->mimeData()->hasFormat("text/uri-list")  || event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))) {
        event->setDropAction(Qt::CopyAction);
        event->acceptProposedAction();
    } else {
        DListView::dragMoveEvent(event);
    }
}

void MusicSongListView::dropEvent(QDropEvent *event)
{
    QModelIndex indexDrop = indexAt(event->pos());
    if (!indexDrop.isValid())
        return;
    QString hash = indexDrop.data(Qt::UserRole).value<QString>();

    //    auto t_playlistPtr = playlistPtr(index);
    if (/*t_playlistPtr == nullptr || */(!event->mimeData()->hasFormat("text/uri-list") && !event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))) {
        return;
    }

    if (event->mimeData()->hasFormat("text/uri-list")) {
        auto urls = event->mimeData()->urls();
        QStringList localpaths;
        for (auto &url : urls) {
            localpaths << url.toLocalFile();
        }

        if (!localpaths.isEmpty()) {
            DataBaseService::getInstance()->importMedias(hash, localpaths);
        }
    } else {
        PlayListView *source = qobject_cast<PlayListView *>(event->source());
        if (source != nullptr) {
            QList<MediaMeta> metas;
            for (auto index : source->selectionModel()->selectedIndexes()) {
                MediaMeta imt = index.data(Qt::UserRole).value<MediaMeta>();
                metas.append(imt);
            }

            if (!metas.isEmpty()) {
                int insertCount = DataBaseService::getInstance()->addMetaToPlaylist(hash, metas);
                CommonService::getInstance()->signalShowPopupMessage(model->itemFromIndex(indexDrop)->text(), metas.size(), insertCount);
            }
        }
    }

    DListView::dropEvent(event);
}

void MusicSongListView::initRenameLineEdit()
{
    m_renameLineEdit = new DLineEdit(this);
    m_renameLineEdit->setVisible(false);
    m_renameLineEdit->setClearButtonEnabled(false);

    connect(m_renameLineEdit, &DLineEdit::editingFinished, this, &MusicSongListView::slotLineEditingFinished);
}

void MusicSongListView::slotRenameShortcut()
{
    if (selectedIndexes().size() > 0 && !m_renameLineEdit->isVisible()) {
        slotDoubleClicked(currentIndex());
    }
}

void MusicSongListView::slotEscShortcut()
{
    if (m_renameLineEdit->isVisible()) {
        m_renameLineEdit->setVisible(false);
    }
}

void MusicSongListView::initShortcut()
{
    // 移动到mainframe中统一管理
//    m_newItemShortcut = new QShortcut(this);
//    m_newItemShortcut->setContext(Qt::WidgetWithChildrenShortcut);
//    m_newItemShortcut->setKey(QKeySequence(QLatin1String("Ctrl+Shift+N")));
//    connect(m_newItemShortcut, &QShortcut::activated, this, &MusicSongListView::addNewSongList);

    m_renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    m_renameShortcut->setContext(Qt::WindowShortcut);
    connect(m_renameShortcut, &QShortcut::activated, this, &MusicSongListView::slotRenameShortcut);

    m_escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    m_escShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(m_escShortcut, &QShortcut::activated, this, &MusicSongListView::slotEscShortcut);
}

void MusicSongListView::setAttrRecur(QDomElement elem, QString strtagname, QString strattr, QString strattrval)
{
    // if it has the tagname then overwritte desired attribute
    if (elem.tagName().compare(strtagname) == 0) {
        elem.setAttribute(strattr, strattrval);
    }
    // loop all children
    for (int i = 0; i < elem.childNodes().count(); i++) {
        if (!elem.childNodes().at(i).isElement()) {
            continue;
        }
        this->setAttrRecur(elem.childNodes().at(i).toElement(), strtagname, strattr, strattrval);
    }
}

QString MusicSongListView::newDisplayName()
{
    QStringList existNames;
    for (DataBaseService::PlaylistData &playlistData : DataBaseService::getInstance()->allPlaylistMeta()) {
        existNames.append(playlistData.displayName);
    }

    QString temp = tr("New playlist");
    if (!existNames.contains(temp)) {
        return temp;
    }

    int i = 1;
    for (i = 1; i < existNames.size() + 1; ++i) {
        QString newName = QString("%1 %2").arg(temp).arg(i);
        if (!existNames.contains(newName)) {
            return newName;
        }
    }
    return QString("%1 %2").arg(temp).arg(i);
}

void MusicSongListView::setThemeType(int type)
{
    for (int i = 0; i < model->rowCount(); i++) {
        auto curIndex = model->index(i, 0);
        auto curStandardItem = dynamic_cast<DStandardItem *>(model->itemFromIndex(curIndex));
//        auto curItemRow = curStandardItem->row();
//        if (curItemRow < 0 || curItemRow >= allPlaylists.size())
//            continue;

        if (type == 1) {
            curStandardItem->setForeground(QColor("#414D68"));
        } else {
            curStandardItem->setForeground(QColor("#C0C6D4"));
        }
    }
}