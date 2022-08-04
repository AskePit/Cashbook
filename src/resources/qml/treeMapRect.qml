import QtQuick 2.5
import QtQuick.Layouts 1.2
import QtQuick.Controls

Rectangle {
    id: root
    width: 10
    height: 10
    color: "gray"

    radius: 0

    property bool isLeaf: false
    property alias name: name.text
    property alias sum: sum.text
    property alias percentage: percentage.text

    function setForegroudColor() {
        // Counting the perceptive luminance - human eye favors green color...
        var luminance = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
        var foreground

        if (luminance > 0.6)
           foreground = "black"; // bright colors - black font
        else
           foreground = "white"; // dark colors - white font

        name.color = foreground
        sum.color = foreground
        percentage.color = foreground
    }

    ToolTip {
        id: toolTip
        text: name.text + '<br>' + sum.text + '<br>' + percentage.text
        delay: 200
        visible: mouseArea.containsMouse && (!name.visible || !sum.visible || !percentage.visible)
        x: mouseArea.mouseX - width
        y: mouseArea.mouseY - height
    }

    ColumnLayout {
        id: rootLayout
        anchors.fill: parent

        property bool debugDraw: false

        // spacer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        Text {
            id: name

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Layout.leftMargin: 15
            Layout.rightMargin: 15
            Layout.topMargin: 10

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            fontSizeMode: Text.Fit
            minimumPixelSize: 8
            font.pixelSize: 22

            visible: contentWidth < rootLayout.width && contentHeight < rootLayout.height/2

            // debug borders drawer
            Rectangle {
                visible: rootLayout.debugDraw

                property var toFill: parent          // instantiation site "can" (optionally) override
                property color customColor: 'yellow' // instantiation site "can" (optionally) override
                property int customThickness: 1      // instantiation site "can" (optionally) override

                anchors.fill: toFill
                z: 200
                color: 'transparent'
                border.color: customColor
                border.width: customThickness
            }
        }

        Text {
            id: sum

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Layout.leftMargin: 15
            Layout.rightMargin: 15

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            fontSizeMode: Text.Fit
            minimumPixelSize: 8
            font.pixelSize: 44

            visible: name.visible && contentWidth < rootLayout.width && contentHeight < rootLayout.height/3

            // debug borders drawer
            Rectangle {
                visible: rootLayout.debugDraw

                property var toFill: parent          // instantiation site "can" (optionally) override
                property color customColor: 'yellow' // instantiation site "can" (optionally) override
                property int customThickness: 1      // instantiation site "can" (optionally) override

                anchors.fill: toFill
                z: 200
                color: 'transparent'
                border.color: customColor
                border.width: customThickness
            }
        }

        Text {
            id: percentage

            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            Layout.leftMargin: 15
            Layout.rightMargin: 15
            Layout.bottomMargin: 10

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            fontSizeMode: Text.Fit
            minimumPixelSize: 8
            font.pixelSize: 18

            visible: sum.visible && contentWidth < rootLayout.width && contentHeight < rootLayout.height/3

            // debug borders drawer
            Rectangle {
                visible: rootLayout.debugDraw

                property var toFill: parent          // instantiation site "can" (optionally) override
                property color customColor: 'yellow' // instantiation site "can" (optionally) override
                property int customThickness: 1      // instantiation site "can" (optionally) override

                anchors.fill: toFill
                z: 200
                color: 'transparent'
                border.color: customColor
                border.width: customThickness
            }
        }

        // spacer
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    signal onGoInside(rect: Rectangle)
    signal onStatement(rect: Rectangle)

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true

        acceptedButtons: Qt.RightButton

        onWheel: (event) => {
            if(event.angleDelta.y > 0) {
                root.onGoInside(root)
            }

            event.accepted = false
        }

        onClicked: {
            if (mouse.button === Qt.RightButton) {
                contextMenu.popup()
            }
        }

        Menu {
            id: contextMenu
            MenuItem {
                text: qsTr("Выписка")
                onTriggered: onStatement(root)
            }
        }
    }
}
