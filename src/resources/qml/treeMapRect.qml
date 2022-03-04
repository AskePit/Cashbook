import QtQuick 2.0

Rectangle {
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
}
