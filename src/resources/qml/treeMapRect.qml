import QtQuick 2.0

Rectangle {
    id: root
    width: 10
    height: 10
    color: "gray"

    radius: 0

    property alias name: name.text
    property alias sum: sum.text
    property alias percentage: percentage.text

    function setForegroudColor() {
        // Counting the perceptive luminance - human eye favors green color...
        var luminance = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
        var foreground

        if (luminance > 0.5)
           foreground = "black"; // bright colors - black font
        else
           foreground = "white"; // dark colors - white font

        name.color = foreground
        sum.color = foreground
        percentage.color = foreground
    }

    Component.onCompleted: {
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: sum.top
        id: name
        text: ""
        color: "white"
        fontSizeMode: Text.Fit
        minimumPixelSize: 8
        font.pixelSize: 30
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        id: sum
        text: ""
        color: "white"
        fontSizeMode: Text.Fit
        minimumPixelSize: 8
        font.pixelSize: 62
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: sum.bottom
        id: percentage
        text: ""
        color: "white"
        fontSizeMode: Text.Fit
        minimumPixelSize: 8
        font.pixelSize: 30
    }

    signal onLeftClicked(rect: Rectangle)
    signal onRightClicked()

    MouseArea {
        id: mouseArea
        anchors.fill: parent

        acceptedButtons: Qt.LeftButton
        onClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) { // 'mouse' is a MouseEvent argument passed into the onClicked signal handler
                root.onLeftClicked(root)
            }
        }
    }
}
