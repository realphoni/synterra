import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: root
    width: 1280; height: 800
    color: "#162a43"
    property bool dark: true
    property color ink: dark ? "#edf6ff" : "#20384f"
    property color surface: dark ? "#dd182c43" : "#e8edf6ff"
    function submit() { if (userName.text.trim().length > 0 && session.count > 0) sddm.login(userName.text.trim(), password.text, session.currentIndex) }
    Image { anchors.fill: parent; source: "file://" + config.background; fillMode: Image.PreserveAspectCrop }
    Rectangle { anchors.fill: parent; color: "#23132339" }
    Rectangle {
        width: Math.min(450, root.width - 40); height: column.implicitHeight + 64
        anchors.centerIn: parent; radius: 16; color: root.surface; border.color: "#85cbe8ff"
        gradient: Gradient {
            GradientStop { position: 0; color: root.dark ? "#ee3f5c79" : "#eff6fbff" }
            GradientStop { position: .12; color: root.surface }
            GradientStop { position: 1; color: root.dark ? "#e8152539" : "#e2c9dff0" }
        }
        ColumnLayout {
            id: column; anchors.centerIn: parent; width: parent.width - 64; spacing: 15
            Image { Layout.alignment: Qt.AlignHCenter; source: "file://" + config.logo; sourceSize.width: 72; sourceSize.height: 72 }
            Label { text: "Synterra 1.1"; color: root.ink; font.pixelSize: 28; Layout.alignment: Qt.AlignHCenter }
            Label { text: "Build 1130 · Prism"; color: root.ink; opacity: .75; Layout.alignment: Qt.AlignHCenter }
            Label { text: qsTr("Username"); color: root.ink }
            TextField {
                id: userName; objectName: "username"; Layout.fillWidth: true; text: userModel.lastUser; color: root.ink
                Accessible.name: qsTr("Username"); selectByMouse: true; onAccepted: password.forceActiveFocus()
                background: Rectangle { radius: 5; color: root.dark ? "#162638" : "#f6fbff"; border.color: parent.activeFocus ? "#8acfff" : "#7399b7" }
            }
            Label { text: qsTr("Password"); color: root.ink }
            TextField {
                id: password; objectName: "password"; Layout.fillWidth: true; color: root.ink; echoMode: TextInput.Password
                Accessible.name: qsTr("Password"); onAccepted: root.submit()
                background: Rectangle { radius: 5; color: root.dark ? "#162638" : "#f6fbff"; border.color: parent.activeFocus ? "#8acfff" : "#7399b7" }
            }
            Label { text: qsTr("Desktop session"); color: root.ink }
            ComboBox {
                id: session; objectName: "session"; Layout.fillWidth: true; model: sessionModel; textRole: "name"; currentIndex: sessionModel.lastIndex
                Accessible.name: qsTr("Desktop session"); palette.text: root.ink; palette.buttonText: root.ink; palette.window: root.dark ? "#253e56" : "#e2eef8"; palette.base: root.dark ? "#253e56" : "#e2eef8"
                background: Rectangle { radius: 5; color: root.dark ? "#253e56" : "#e2eef8"; border.color: "#7399b7" }
            }
            Label { id: error; objectName: "errorMessage"; text: ""; textFormat: Text.PlainText; color: root.dark ? "#ffb8b8" : "#9c2929"; wrapMode: Text.WordWrap; Layout.fillWidth: true; visible: text.length > 0 }
            Button {
                objectName: "loginButton"; text: qsTr("Sign in"); Layout.fillWidth: true; enabled: userName.text.trim().length > 0 && session.count > 0; onClicked: root.submit()
                palette.buttonText: "#f8fcff"
                background: Rectangle { radius: 6; border.color: "#b2def9"; opacity: parent.enabled ? 1 : .5; gradient: Gradient { GradientStop { position: 0; color: "#72add5" } GradientStop { position: .5; color: "#3f719f" } GradientStop { position: 1; color: "#284e79" } } }
            }
            Button { text: root.dark ? qsTr("Light glass") : qsTr("Dark glass"); Layout.alignment: Qt.AlignHCenter; onClicked: root.dark = !root.dark; palette.buttonText: root.ink; background: Rectangle { color: "transparent" } }
        }
    }
    Connections {
        target: sddm
        function onLoginFailed() { password.text = ""; error.text = qsTr("Sign-in failed. Check your username and password."); password.forceActiveFocus() }
        function onLoginSucceeded() { error.text = "" }
    }
    Component.onCompleted: { if (userName.text.length > 0) password.forceActiveFocus(); else userName.forceActiveFocus() }
}
