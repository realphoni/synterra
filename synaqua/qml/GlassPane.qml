import QtQuick
Rectangle {
    id: pane
    property bool dark: false
    radius: 22
    border.width: 1
    border.color: dark ? "#6bafc9e1" : "#cce8f7ff"
    gradient: Gradient {
        GradientStop { position: 0; color: pane.dark ? "#f247617e" : "#f2eaf6ff" }
        GradientStop { position: .12; color: pane.dark ? "#e52a405a" : "#d2bdd7ec" }
        GradientStop { position: 1; color: pane.dark ? "#eb14283e" : "#e5d5e9f9" }
    }
    Rectangle { anchors.fill: parent; anchors.margins: 2; radius: parent.radius - 2; color: "transparent"; border.color: pane.dark ? "#233e6388" : "#50ffffff" }
}
