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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <QtCore/QDateTime>

namespace QMatrixClient
{
    class Connection;
    class Event;
    class Room;
}

class Message
{
    public:
        Message(QMatrixClient::Connection* connection,
                QMatrixClient::Event* event,
                QMatrixClient::Room* room);
        virtual ~Message();

        QMatrixClient::Event* messageEvent() const;
        QDateTime timestamp() const;

        bool highlight() const;
        bool isStatusMessage() const;

    private:
        QMatrixClient::Connection* m_connection;
        QMatrixClient::Event* m_event;
        bool m_isHighlight;
        bool m_isStatusMessage;
};

#endif // MESSAGE_H
