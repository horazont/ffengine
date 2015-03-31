import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.3
import SCC 1.0

ApplicationWindow {
    title: qsTr("Hello World")
    width: 640
    height: 480
    visible: true

    GLScene {
        id: gLScene1
        anchors.fill: parent;

        MainForm {
            width: 206
            height: 340
            anchors.left: parent.left
            anchors.leftMargin: -8
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    style: ApplicationWindowStyle {
        background: Item{
        }
    }

}
