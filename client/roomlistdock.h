/**************************************************************************
 *                                                                        *
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>                        *
 *                                                                        *
 * This program is free software; you can redistribute it and/or          *
 * modify it under the terms of the GNU General Public License            *
 * as published by the Free Software Foundation; either version 3         *
 * of the License, or (at your option) any later version.                 *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                        *
 **************************************************************************/

#ifndef ROOMLISTDOCK_H
#define ROOMLISTDOCK_H

#include <QtWidgets/QDockWidget>
#include <QtWidgets/QListView>
#include <QtCore/QStringListModel>

#include "lib/connection.h"

class RoomListModel;
class QuaternionRoom;

class RoomListDock : public QDockWidget
{
        Q_OBJECT
    public:
        RoomListDock(QWidget* parent = nullptr);
        virtual ~RoomListDock();

        void setConnection( QMatrixClient::Connection* connection );

    signals:
        void roomSelected(QuaternionRoom* room);

    private slots:
        void rowSelected(const QModelIndex& index);
        void showContextMenu(const QPoint& pos);
        void menuJoinSelected();
        void menuLeaveSelected();

    private:
        QMatrixClient::Connection* connection;
        QListView* view;
        RoomListModel* model;
        QMenu* contextMenu;
        QAction* joinAction;
        QAction* leaveAction;
};

#endif // ROOMLISTDOCK_H
