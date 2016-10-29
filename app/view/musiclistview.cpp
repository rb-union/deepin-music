/**
 * Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 **/

#include "musiclistview.h"

#include <dthememanager.h>
DWIDGET_USE_NAMESPACE

MusicListView::MusicListView(QWidget *parent) : QListWidget(parent)
{
    setObjectName("MusicListView");

    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(false);

    setSelectionMode(QListView::ExtendedSelection);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    setFocusPolicy(Qt::ClickFocus);
    D_THEME_INIT_WIDGET(MusicListView);
}