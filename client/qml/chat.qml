import QtQuick 2.2
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

Rectangle {
    id: root

    SystemPalette { id: defaultPalette; colorGroup: SystemPalette.Active }
    SystemPalette { id: disabledPalette; colorGroup: SystemPalette.Disabled }

    color: defaultPalette.highlight

    signal getPreviousContent()

    Timer{
        id: scrollTimer
        onTriggered: reallyScrollToBottom()
    }

    function reallyScrollToBottom() {
        if (chatView.stickToBottom && !chatView.nowAtYEnd)
        {
            chatView.positionViewAtEnd()
            scrollToBottom()
        }
    }

    function scrollToBottom() {
        chatView.stickToBottom = true
        scrollTimer.running = true
    }

    ListView {
        id: chatView
        anchors.fill: parent

        model: messageModel
        delegate: messageDelegate
        flickableDirection: Flickable.VerticalFlick
        flickDeceleration: 9001
        boundsBehavior: Flickable.StopAtBounds
        pixelAligned: true
        // FIXME: atYEnd is glitchy on Qt 5.2.1
        property bool nowAtYEnd: contentY - originY + height >= contentHeight
        property bool stickToBottom: true

        function rowsInserted() {
            if( stickToBottom )
                root.scrollToBottom();
        }

        Component.onCompleted: {
            console.log("onCompleted");
            model.rowsInserted.connect(rowsInserted);
        }

        section {
            property: "date"
            labelPositioning: ViewSection.InlineLabels | ViewSection.CurrentLabelAtStart

            delegate: Rectangle {
                width:parent.width
                height: childrenRect.height
                color: defaultPalette.window
                Label { text: section.toLocaleString("dd.MM.yyyy") }
            }
        }

        onHeightChanged: {
            if( stickToBottom )
                root.scrollToBottom();
        }

        onContentHeightChanged: {
            if( stickToBottom )
                root.scrollToBottom();
        }

        onContentYChanged: {
            if( (this.contentY - this.originY) < 5 )
            {
                console.log("get older content!");
                root.getPreviousContent()
            }

        }

        onMovementStarted: {
            stickToBottom = false;
        }

        onMovementEnded: {
            stickToBottom = nowAtYEnd;
        }

        footer: Rectangle {
            width:parent.width
            height: 5
            color: root.color
        }
    }

    Slider {
        id: chatViewScroller
        orientation: Qt.Vertical
        anchors.horizontalCenter: chatView.right
        anchors.verticalCenter: chatView.verticalCenter
        height: chatView.height / 2

        value: -chatView.verticalVelocity / chatView.height
        maximumValue: 10.0
        minimumValue: -10.0

        activeFocusOnPress: false
        activeFocusOnTab: false

        onPressedChanged: {
            if (!pressed)
                value = 0
        }

        onValueChanged: {
            if (pressed && value)
                chatView.flick(0, chatView.height * value)
        }
        Component.onCompleted: {
            // This will cause continuous scrolling while the scroller is out of 0
            chatView.flickEnded.connect(chatViewScroller.valueChanged)
        }
    }

    Component {
        id: messageDelegate

        Rectangle {
            width: parent.width
            height: childrenRect.height
            color: root.color

            ColumnLayout{
                id: messageContainer
                width: parent.width
                spacing: 0
                Rectangle {
                    width:parent.width
                    height: 5
                    visible: newUser
                    color: root.color
                }

                RowLayout {
                    id: message
                    width: parent.width
                    spacing: 8

                    property string textColor:
                            if (highlight) decoration
                            else if (eventType == "state" || eventType == "other") disabledPalette.text
                            else defaultPalette.text
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 60
                        Layout.fillHeight: true
                        color: root.color
                        Label {
                            id: timelabel
                            text: time.toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
                            color: defaultPalette.base
                            opacity: newTime ? 1 : 0
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        width: 30
                        height: 22
                        color: root.color
                        Image {
                            id: img
                            anchors.top:parent.top
                            anchors.topMargin: -3
                            width: 28
                            height: 28
                            opacity: newUser ? 1 : 0
                            fillMode: Image.PreserveAspectCrop
                            source: avatar
                            layer.enabled: true
                            layer.effect: OpacityMask {
                                maskSource: Rectangle {
                                    anchors.centerIn: parent
                                    width: 28
                                    height: 28
                                    radius: 14
                                }
                            }
                        }
                    }
                    Rectangle {
                        color: defaultPalette.base
                        Layout.fillWidth: true
                        Layout.minimumHeight: childrenRect.height
                        Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                        Label {
                            id: senderTag
                            width: 120
                            anchors.left: parent.left
                            anchors.leftMargin: 3
                            anchors.top: parent.top
                            anchors.topMargin: newUser ? 3 : 0
                            elide: Text.ElideRight
                            text: eventType == "state" || eventType == "emote" ? "* " + author :
                                  eventType != "other" ? author : "***"
                            horizontalAlignment: if( ["other", "emote", "state"]
                                                         .indexOf(eventType) >= 0 )
                                                 { Text.AlignRight }
                            opacity: newUser || ["other", "emote", "state"].indexOf(eventType) >= 0 ? 1 : 0
                            color: message.textColor
                        }

                        Column {
                            spacing: 0
                            anchors.top: parent.top
                            anchors.topMargin: newUser ? 3 : 0
                            anchors.right: parent.right
                            anchors.left: senderTag.right

                            TextEdit {
                                id: contentField
                                selectByMouse: true; readOnly: true; font: timelabel.font;
                                textFormat: contentType == "text/html" ? TextEdit.RichText
                                                                       : TextEdit.PlainText;
                                text: eventType != "image" ? content : ""
                                height: eventType != "image" ? implicitHeight : 0
                                wrapMode: Text.Wrap; width: parent.width
                                color: message.textColor

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.IBeamCursor
                                    acceptedButtons: Qt.NoButton
                                }
                                onLinkActivated: {
                                    Qt.openUrlExternally(link)
                                }
                            }
                            Image {
                                id: imageField
                                fillMode: Image.PreserveAspectFit
                                width: eventType == "image" ? parent.width : 0

                                sourceSize: eventType == "image" ? "500x500" : "0x0"
                                source: eventType == "image" ? content : ""
                            }
                            Loader {
                                asynchronous: true
                                visible: status == Loader.Ready
                                width: parent.width
                                property string sourceText: toolTip

                                sourceComponent: showSource.checked ? sourceArea : undefined
                            }
                        }
                        Rectangle {
                            color: message.textColor
                            width: messageModel.lastReadId == eventId ? parent.width : 0
                            height: 2
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            Behavior on width {
                                NumberAnimation { duration: 500; easing.type: Easing.OutQuad }
                            }
                        }
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        width: 30
                        Layout.fillHeight: true
                        color: root.color
                        ToolButton {
                            id: showSourceButton
                            anchors.fill: parent
                            text: "..."

                            action: Action {
                                id: showSource

                                tooltip: "Show source"
                                checkable: true
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: sourceArea

        TextArea {
            selectByMouse: true; readOnly: true; font.family: "Monospace"
            text: sourceText
        }
    }
    Rectangle {
        id: scrollindicator;
        opacity: chatView.nowAtYEnd ? 0 : 0.5
        color: defaultPalette.text
        height: 30;
        radius: height/2;
        width: height;
        anchors.left: parent.left;
        anchors.bottom: parent.bottom;
        anchors.leftMargin: width/2;
        anchors.bottomMargin: chatView.nowAtYEnd ? -height : height/2;
        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }
        Behavior on anchors.bottomMargin {
            NumberAnimation { duration: 300 }
        }
        Image {
            anchors.fill: parent
            source: "qrc:///scrolldown.svg"
        }
        MouseArea {
            anchors.fill: parent
            onClicked: root.scrollToBottom()
            cursorShape: Qt.PointingHandCursor
        }
    }
}
