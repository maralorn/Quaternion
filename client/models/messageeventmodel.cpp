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

#include "messageeventmodel.h"

#include <QtCore/QStringBuilder>
#include <QtCore/QSettings>
#include <QtCore/QDebug>

#include "../message.h"
#include "../quaternionroom.h"
#include "lib/connection.h"
#include "lib/room.h"
#include "lib/user.h"
#include "lib/events/event.h"
#include "lib/events/roommessageevent.h"
#include "lib/events/roommemberevent.h"
#include "lib/events/roomnameevent.h"
#include "lib/events/roomaliasesevent.h"
#include "lib/events/roomcanonicalaliasevent.h"
#include "lib/events/roomtopicevent.h"
#include "lib/events/unknownevent.h"

enum EventRoles {
    EventTypeRole = Qt::UserRole + 1,
    EventIdRole,
    TimeRole,
    DateRole,
    AuthorRole,
    ContentRole,
    ContentTypeRole,
    HighlightRole,
    NewTimeRole,
    NewUserRole
};

QHash<int, QByteArray> MessageEventModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();
    roles[EventTypeRole] = "eventType";
    roles[EventIdRole] = "eventId";
    roles[TimeRole] = "time";
    roles[DateRole] = "date";
    roles[AuthorRole] = "author";
    roles[ContentRole] = "content";
    roles[ContentTypeRole] = "contentType";
    roles[HighlightRole] = "highlight";
    roles[NewTimeRole] = "newTime";
    roles[NewUserRole] = "newUser";
    return roles;
}

MessageEventModel::MessageEventModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_currentRoom(nullptr)
    , lastShownIndex(-1)
    , m_readMarkerIndex(-1)
{ }

MessageEventModel::~MessageEventModel()
{
}

void MessageEventModel::changeRoom(QuaternionRoom* room)
{
    if (room == m_currentRoom)
        return;

    beginResetModel();
    if( m_currentRoom )
        m_currentRoom->disconnect( this );

    m_currentRoom = room;
    lastShownIndex = -1;
    m_readMarkerIndex = -1;
    if( room )
    {
        using namespace QMatrixClient;
        connect(m_currentRoom, &Room::aboutToAddNewMessages, this,
                [=](const Events& events)
                {
                    beginInsertRows(QModelIndex(),
                                    rowCount(), rowCount() + events.size() - 1);
                });
        connect(m_currentRoom, &Room::aboutToAddHistoricalMessages, this,
                [=](const Events& events)
                {
                    beginInsertRows(QModelIndex(), 0, events.size() - 1);
                });
        connect(m_currentRoom, &Room::addedMessages, this,
                [=]()
                {
                    endInsertRows();
                    updateReadMarkerIndex();
                });
        connect(m_currentRoom, &Room::readMarkerPromoted,
                this, &MessageEventModel::updateReadMarkerIndex);
        qDebug() << "connected" << room;
    }
    endResetModel();
    emit lastShownIndexChanged(lastShownIndex);
    updateReadMarkerIndex();
}

int MessageEventModel::rowCount(const QModelIndex& parent) const
{
    if( !m_currentRoom || parent.isValid() )
        return 0;
    return m_currentRoom->messages().count();
}

QVariant MessageEventModel::data(const QModelIndex& index, int role) const
{
    using namespace QMatrixClient;
    if( !m_currentRoom ||
            index.row() < 0 || index.row() >= m_currentRoom->messages().count())
        return QVariant();

    const Message* message = m_currentRoom->messages().at(index.row());
    Event* prev_event;
    if (index.row() > 1)
    {
        const Message* prev_message = m_currentRoom->messages().at(index.row() - 1);
        prev_event = prev_message->messageEvent();
    }
    Event* event = message->messageEvent();
    // FIXME: Rewind to the name that was at the time of this event
    QString senderName = m_currentRoom->roomMembername(event->senderId());

    if( role == Qt::DisplayRole )
    {
        if( event->type() == EventType::RoomMessage )
        {
            RoomMessageEvent* e = static_cast<RoomMessageEvent*>(event);
            return QString("%1: %2").arg(senderName, e->body());
        }
        if( event->type() == EventType::RoomMember )
        {
            RoomMemberEvent* e = static_cast<RoomMemberEvent*>(event);
            switch( e->membership() )
            {
                case MembershipType::Join:
                    return QString("%1 (%2) joined the room").arg(e->displayName(), e->userId());
                case MembershipType::Leave:
                    return QString("%1 (%2) left the room").arg(e->displayName(), e->userId());
                case MembershipType::Ban:
                    return QString("%1 (%2) was banned from the room").arg(e->displayName(), e->userId());
                case MembershipType::Invite:
                    return QString("%1 (%2) was invited to the room").arg(e->displayName(), e->userId());
                case MembershipType::Knock:
                    return QString("%1 (%2) knocked").arg(e->displayName(), e->userId());
            }
        }
        if( event->type() == EventType::RoomAliases )
        {
            RoomAliasesEvent* e = static_cast<RoomAliasesEvent*>(event);
            return QString("Current aliases: %1").arg(e->aliases().join(", "));
        }
        return "Unknown Event";
    }

    if( role == Qt::ToolTipRole )
    {
        return event->originalJson();
    }

    if( role == EventTypeRole )
    {
        switch (event->type())
        {
            case EventType::RoomMessage:
            {
                auto msgType = static_cast<RoomMessageEvent*>(event)->msgtype();
                if( msgType == MessageEventType::Image )
                    return "image";
                else if( msgType == MessageEventType::Emote )
                    return "emote";
                return "message";
            }
            case EventType::RoomMember:
            case EventType::RoomAliases:
            case EventType::RoomCanonicalAlias:
            case EventType::RoomName:
            case EventType::RoomTopic:
                return "state";
            default:
                return "other";
        }
    }

    if( role == TimeRole )
    {
        return event->timestamp();
    }

    if( role == DateRole )
    {
        return event->timestamp().toLocalTime().date();
    }

    if( role == AuthorRole )
    {
        return senderName;
    }

    if (role == ContentTypeRole  || role == ContentRole)
    {
        if( event->type() == EventType::RoomMessage )
        {
            using namespace MessageEventContent;

            RoomMessageEvent* e = static_cast<RoomMessageEvent*>(event);
            switch (e->msgtype())
            {
            case MessageEventType::Emote:
            case MessageEventType::Text:
            case MessageEventType::Notice:
                {
                    QString body;
                    QString contentType;
                    auto textContent = static_cast<TextContent*>(e->content());
                    if (textContent && textContent->mimeType.inherits("text/html"))
                    {
                        body = textContent->body;
                        contentType = "text/html";
                    }
                    else
                    {
                        body = e->plainBody();
                        contentType = "text/plain";
                    }

                    if (role == ContentRole)
                        return body;
                    else
                        return contentType;
                }
            case MessageEventType::Image:
                {
                    auto content = static_cast<ImageContent*>(e->content());
                    if (role == ContentRole)
                        return QUrl("image://mtx/" +
                                    content->url.host() + content->url.path());
                    else
                        return content->mimetype.name();
                }
            case MessageEventType::File:
            case MessageEventType::Location:
            case MessageEventType::Video:
            case MessageEventType::Audio:
                {
                    auto fileInfo = static_cast<FileInfo*>(e->content());
                    if (role == ContentRole)
                        return e->body(); // TODO
                    else
                        return fileInfo ? fileInfo->mimetype.name() : "unknown";
                }
            default:
                if (role == ContentRole)
                    return e->body();
                else
                    return "unknown";
            }
        }
        if( event->type() == EventType::RoomMember )
        {
            RoomMemberEvent* e = static_cast<RoomMemberEvent*>(event);
            // FIXME: Rewind to the name that was at the time of this event
            QString subjectName = m_currentRoom->roomMembername(e->userId());
            // The below code assumes senderName output in AuthorRole
            switch( e->membership() )
            {
                case MembershipType::Join:
                    return tr("joined the room");
                case MembershipType::Leave:
                    if (e->senderId() != e->userId())
                        return tr("doesn't want %1 in the room anymore").arg(subjectName);
                    else
                        return tr("left the room");
                case MembershipType::Ban:
                    if (e->senderId() != e->userId())
                        return tr("banned %1 from the room").arg(subjectName);
                    else
                        return tr("self-banned from the room");
                case MembershipType::Invite:
                    return tr("invited %1 to the room").arg(subjectName);
                case MembershipType::Knock:
                    return tr("knocked");
            }
        }
        if( event->type() == EventType::RoomAliases )
        {
            auto e = static_cast<RoomAliasesEvent*>(event);
            return tr("set aliases to: %1").arg(e->aliases().join(", "));
        }
        if( event->type() == EventType::RoomCanonicalAlias )
        {
            auto e = static_cast<RoomCanonicalAliasEvent*>(event);
            return tr("set the room main alias to: %1").arg(e->alias());
        }
        if( event->type() == EventType::RoomName )
        {
            auto e = static_cast<RoomNameEvent*>(event);
            return tr("set the room name to: %1").arg(e->name());
        }
        if( event->type() == EventType::RoomTopic )
        {
            auto e = static_cast<RoomTopicEvent*>(event);
            return tr("set the topic to: %1").arg(e->topic());
        }
        return "Unknown Event";
    }

    if( role == HighlightRole )
    {
        return message->highlight();
    }

    if( role == Qt::DecorationRole )
    {
        if (message->highlight())
        {
            return QSettings().value("UI/highlight_color", "orange");
        }
    }
    if( role == EventIdRole )
    {
        return event->id();
    }
    if( role == NewTimeRole )
        return !prev_event ||
            event->timestamp().time().minute() != prev_event->timestamp().time().minute() ||
            event->timestamp().time().hour() != prev_event->timestamp().time().hour();
    if( role == NewUserRole )
        return !prev_event || event->senderId() != prev_event->senderId();
    return QVariant();
}

void MessageEventModel::markShownAsRead()
{
    if (m_currentRoom && lastShownIndex > -1 && lastShownIndex < m_currentRoom->messages().count())
    {
        auto lastShownMessage = m_currentRoom->messages().at(lastShownIndex);
        m_currentRoom->markMessagesAsRead(lastShownMessage->messageEvent()->id());
    }
}

void MessageEventModel::updateReadMarkerIndex()
{
    if( !m_currentRoom )
        return;
    for (int newReadMarkerIndex = m_readMarkerIndex; newReadMarkerIndex < m_currentRoom->messages().count() ; ++newReadMarkerIndex)
        {
        if (newReadMarkerIndex >= 0 && m_currentRoom->readMarkerEventId() == m_currentRoom->messages().at(newReadMarkerIndex)->messageEvent()->id())
        {
            if (newReadMarkerIndex != m_readMarkerIndex)
            {
                m_readMarkerIndex = newReadMarkerIndex;
                emit readMarkerIndexChanged(newReadMarkerIndex);
            }
            return;
        }
    }
}

bool MessageEventModel::awaitingMarkRead()
{
    qDebug() << "readMarker: " << m_readMarkerIndex << "lastShown: " << lastShownIndex;
    return m_readMarkerIndex < lastShownIndex;
}
