import QtQuick 2.0

Rectangle {
    id: root
    width: 10
    height: 10
    color: "gray"

    radius: 0

    property alias name: name.text
    property alias percentage: percentage.text

    Component.onCompleted: {
    }

    Text {
        y: 15
        anchors.horizontalCenter: parent.horizontalCenter
        id: name
        text: ""
        color: "white"
    }

    Text {
        y: 30
        anchors.horizontalCenter: parent.horizontalCenter
        id: percentage
        text: ""
        color: "white"
    }

    signal onLeftClicked(rect: Rectangle)
    signal onRightClicked()

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button === Qt.LeftButton) { // 'mouse' is a MouseEvent argument passed into the onClicked signal handler
                root.onLeftClicked(root)
            } else if (mouse.button === Qt.RightButton) {
                root.onRightClicked()
            }
        }
    }
}
