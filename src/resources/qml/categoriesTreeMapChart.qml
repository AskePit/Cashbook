import QtQuick 2.15
import QtQuick.Layouts 1.2

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

    function onStatement(rect) {
        sModel.showCategoryStatement(rect.name)
    }

    ColumnLayout {
        id: treemapLayout
        anchors.fill: parent

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: false
            Layout.alignment: Qt.AlignTop
            Layout.bottomMargin: 10

            // spacer
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: false
            }

            Text {
                id: pathText
                color: "white"

                font.pixelSize: 18

                Layout.fillWidth: false
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignTop

                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                text: ": "
                color: "white"

                font.pixelSize: 18

                Layout.fillWidth: false
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignTop

                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                id: totalSumText
                color: "white"

                font.pixelSize: 18

                Layout.fillWidth: false
                Layout.fillHeight: false
                Layout.alignment: Qt.AlignTop

                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
            }

            // spacer
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: false
            }
        }

        Rectangle {
            id: root
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignTop

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
                    '#264653',
                    '#d9b45A',
                    "#b1876b",
                    "#733136",
                    "#efe8ba",
                    "#92574b",
                    '#442F5F',
                    "#d0b990",
                    '#E76F51',
                    '#202020'
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
                //shuffleArray(pallette)
                var colorIdx = 0

                var spacing = 3
                var modelRects = sModel.getCurrenRects(width - spacing, height - spacing) // `spacing` is for top-left margin. other margins are handled below

                i = 0
                for (; i < modelRects.length; ++i) {
                    var modelRect = modelRects[i]

                    var component = Qt.createComponent("treeMapRect.qml")
                    var rect = component.createObject(root)

                    /*if (modelRect.isLeaf) {
                        rect.color = "#606060"
                    } else {*/
                        rect.color = pallette[colorIdx]
                        rect.color = Qt.hsva(rect.color.hsvHue, rect.color.hsvSaturation-0.2, rect.color.hsvValue, 1)

                        ++colorIdx
                        if(colorIdx >= pallette.length) {
                            colorIdx = 0
                        }
                    //}
                    rect.setForegroudColor()

                    rect.x = modelRect.x + spacing
                    rect.y = modelRect.y + spacing
                    rect.width = modelRect.w - spacing
                    rect.height = modelRect.h - spacing
                    rect.name = modelRect.name
                    rect.sum = modelRect.sum
                    rect.percentage = /*Math.round(modelRect.percentage, 2)*/ Number((modelRect.percentage*100).toFixed(2)) + '%'
                    rect.isLeaf = modelRect.isLeaf

                    rect.onGoInside.connect(onRectInside)
                    rect.onStatement.connect(onStatement)
                    rects.push(rect)
                }

                totalSumText.text = sModel.getTotalSum()
                pathText.text = sModel.getCategoryPath()
                if(pathText.text === "") {
                    pathText.text = "/"
                }
            }

            MouseArea {
                id: mouseArea
                anchors.fill: parent

                acceptedButtons: Qt.RightButton

                onWheel: (event) => {
                    if(event.angleDelta.y < 0) {
                        onUp()
                    }
                }
            }
        }
    }
}
