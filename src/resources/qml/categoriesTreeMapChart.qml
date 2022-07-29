import QtQuick 2.15

Item {
    width: 500
    height: 500
    visible: true
    id: window

    onWidthChanged: {
        root.updateView()
    }

    onHeightChanged: {
        root.updateView()
    }

    function onModelSet() {
        sModel.onUpdated.connect(onModelUpdated)
        root.updateView()
    }

    function onModelUpdated() {
        root.updateView()
    }

    function onRectInside(rect) {
        sModel.gotoNode(rect.name)
    }

    function onUp() {
        sModel.goUp()
    }

    Rectangle {
        id: root
        anchors.fill: parent

        property var rects: []

        function updateView() {

            if(!sModel) {
                return
            }

            var i = 0
            for(; i < rects.length; ++i) {
                rects[i].destroy()
            }
            rects = []

            var pallette = [
                "#C45A5C",
                "#ffad98",
                "#ffd89c",
                "#415d85",
                "#7EB056",
            ]

            //Fisher-Yates shuffle algorithm.
            function shuffleArray(array) {
                for (var i = array.length - 1; i > 0; i--) {
                    var j = Math.floor(Math.random() * (i + 1));
                    var temp = array[i];
                    array[i] = array[j];
                    array[j] = temp;
                }
                return array;
            }
            shuffleArray(pallette)
            var colorIdx = 0

            var spacing = 0
            var modelRects = sModel.getCurrenRects(width - spacing, height - spacing) // `spacing` is for top-left margin. other margins are handled below

            i = 0
            for (; i < modelRects.length; ++i) {
                var modelRect = modelRects[i]

                var component = Qt.createComponent("treeMapRect.qml")
                var rect = component.createObject(root)

                rect.color = pallette[colorIdx]
                ++colorIdx
                if(colorIdx >= pallette.length) {
                    colorIdx = 0
                }
                rect.setForegroudColor()

                rect.x = modelRect.x + spacing
                rect.y = modelRect.y + spacing
                rect.width = modelRect.w - spacing
                rect.height = modelRect.h - spacing
                rect.name = modelRect.name
                rect.sum = modelRect.sum
                rect.percentage = /*Math.round(modelRect.percentage, 2)*/ Number((modelRect.percentage*100).toFixed(2)) + '%'

                rect.onLeftClicked.connect(onRectInside)
                rects.push(rect)
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent

            acceptedButtons: Qt.RightButton
            onClicked: (mouse) => {
                 if (mouse.button === Qt.RightButton) {
                    onUp()
                }
            }
        }
    }
}
