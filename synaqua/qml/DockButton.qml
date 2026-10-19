import QtQuick
import QtQuick.Controls.Basic
import org.kde.kirigami as Kirigami
ToolButton {
    id: button
    property var iconSource: "application-x-executable"
    property string label: ""
    property bool dark: false
    property bool running: false
    width: 50; height: 58
    hoverEnabled: true
    Accessible.name: label
    ToolTip.visible: hovered
    ToolTip.text: label
    ToolTip.delay: 450
    contentItem: Item {
        Kirigami.Icon { anchors.centerIn: parent; anchors.verticalCenterOffset: button.hovered ? -3 : 0; width: 34; height: 34; source: button.iconSource; implicitWidth: 34; implicitHeight: 34; Behavior on anchors.verticalCenterOffset { NumberAnimation { duration: 100 } } }
        Rectangle { visible: button.running; anchors.bottom: parent.bottom; anchors.horizontalCenter: parent.horizontalCenter; width: 13; height: 3; radius: 2; color: "#70cfff" }
    }
    background: Rectangle { radius: 13; color: button.down ? "#553f86b4" : (button.hovered || button.activeFocus) ? (button.dark ? "#42577b9c" : "#70ffffff") : "transparent"; border.width: button.activeFocus ? 1 : 0; border.color: "#8ed5ff" }
}
