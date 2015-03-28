import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

Item {
    id: item1
    width: 640
    height: 480

    property alias button2: button2
    property alias button1: button1

    Rectangle {
        id: rectangle1
        x: 220
        y: 80
        width: 200
        height: 320
        color: "#ccffffff"
        radius: 8
        border.color: "#40000000"
        border.width: 4
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        Column {
            id: column1
            anchors.rightMargin: 8
            anchors.leftMargin: 8
            anchors.bottomMargin: 8
            anchors.topMargin: 8
            anchors.fill: parent
            Label {
                id: label1
                text: qsTr("SomeCity")
                anchors.leftMargin: 0
                font.pointSize: 32
                anchors.rightMargin: 0
                anchors.right: parent.right
                verticalAlignment: Text.AlignTop
                anchors.left: parent.left
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                id: button1
                text: qsTr("Button")
                anchors.leftMargin: 0
                anchors.rightMargin: 0
                anchors.right: parent.right
                anchors.left: parent.left
            }

            Button {
                id: button2
                text: qsTr("Button")
                anchors.leftMargin: 0
                anchors.rightMargin: 0
                anchors.right: parent.right
                enabled: true
                anchors.left: parent.left
            }
            spacing: 12
        }
    }
}
