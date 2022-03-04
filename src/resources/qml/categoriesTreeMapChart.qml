import QtQuick 2.15

Item {
    width: 500
    height: 500
    visible: true
    id: window

    onWidthChanged: {
        root.kek()
    }

    onHeightChanged: {
        root.kek()
    }

    Rectangle {
        id: root
        anchors.fill: parent

        property var rects: []

        function kek() {
            var i = 0
            for(; i < rects.length; ++i) {
                rects[i].destroy()
            }
            rects = []

            var spacing = 10

            var modelRects = sModel.getCurrenRects(width - spacing, height - spacing) // `spacing` is for top-left margin. other margins are handled below

            i = 0
            for (; i < modelRects.length; ++i) {
                var modelRect = modelRects[i]

                var component = Qt.createComponent("treeMapRect.qml")
                var rect = component.createObject(root)

                rect.x = modelRect.x + spacing
                rect.y = modelRect.y + spacing
                rect.width = modelRect.w - spacing
                rect.height = modelRect.h - spacing
                rect.name = modelRect.name
                rect.percentage = /*Math.round(modelRect.percentage, 2)*/ Number((modelRect.percentage*100).toFixed(2)) + '%'
                rects.push(rect)
            }
        }

        Component.onCompleted: {
            sModel.gotoNode("products")
            root.kek()
        }
    }
}
