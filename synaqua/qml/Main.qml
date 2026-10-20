import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.taskmanager as TaskManager

FocusScope {
    id: root
    width: 1440; height: 900; focus: true
    readonly property bool menuOpen: synaqua.menuOpen
    readonly property color ink: synaqua.dark ? "#edf6ff" : "#16334e"
    readonly property color subtle: synaqua.dark ? "#b7ccdf" : "#476780"
    readonly property real availableDockWidth: Math.max(1, width - 48)
    readonly property real baseHalfPins: Math.max(synaqua.leftPins.length, synaqua.rightPins.length) * 50 + Math.max(0, Math.max(synaqua.leftPins.length, synaqua.rightPins.length)-1) * synaqua.dockSpacing
    readonly property real baseUtilityWidth: (synaqua.showClock ? 72 : 0) + 30 + 28 + 10 + 14
    readonly property real minimumDockWidth: 2 * (baseHalfPins + 32 + 12 + baseUtilityWidth + 12)
    readonly property real dockScale: Math.min(synaqua.dockScale / 100, availableDockWidth / minimumDockWidth)
    readonly property real pinSize: 50 * dockScale
    function syncRegions() { synaqua.updateRegions(Qt.rect(dock.x,dock.y,dock.width,dock.height), Qt.rect(menu.x,menu.y,menu.width,menu.height),Qt.rect(errorToast.x,errorToast.y,errorToast.width,errorToast.height),menuOpen) }
    function closeMenu() { synaqua.hideMenu() }
    function openApp(id) { closeMenu(); synaqua.launch(id) }
    onWidthChanged: Qt.callLater(syncRegions)
    onHeightChanged: Qt.callLater(syncRegions)
    onMenuOpenChanged: {
        synaqua.query = ""; search.text = ""; syncRegions()
        if (menuOpen) Qt.callLater(function() { search.forceActiveFocus() })
    }
    Keys.onEscapePressed: closeMenu()
    Connections { target: synaqua; function onErrorChanged() { Qt.callLater(root.syncRegions) } function onDockSettingsChanged() { Qt.callLater(root.syncRegions) } }
    Component.onCompleted: Qt.callLater(syncRegions)

    Image { anchors.fill: parent; visible: synaqua.preview; source: "file:///usr/share/wallpapers/Synterra-aurora/contents/images/3840x2160.png"; fillMode: Image.PreserveAspectCrop }
    // Preview-only desktop shortcuts show the same applications provided by
    // the real session's Plasma folder view, without creating duplicate icons.
    Column {
        visible: synaqua.preview; x: 28; y: 28; spacing: 14
        Repeater { model: [{name:"Home",icon:"user-home",id:"org.kde.dolphin.desktop"},{name:"Synterra Surf",icon:"synterra-surf",id:"synterra-surf.desktop"}]
            delegate: Column {
                required property var modelData
                DockButton { anchors.horizontalCenter: parent.horizontalCenter; iconSource: modelData.icon; label: modelData.name; onClicked: root.openApp(modelData.id) }
                Text { width: 104; text: modelData.name; color: "white"; style: Text.Outline; styleColor: "#304560"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12 }
            }
        }
    }
    MouseArea { anchors.fill: parent; enabled: root.menuOpen; onClicked: root.closeMenu() }
    GlassPane {
        id: menu; visible: root.menuOpen; dark: synaqua.dark
        width: Math.min(700, root.width - 40); height: Math.min(570,root.height - dock.height - synaqua.dockMargin - 56)
        anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: dock.top; anchors.bottomMargin: 14
        // Consume clicks on glass instead of dismissing through its parent.
        MouseArea { anchors.fill: parent }
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 22; spacing: 13
            RowLayout {
                Kirigami.Icon { source: "synterra"; Layout.preferredWidth: 36; Layout.preferredHeight: 36 }
                ColumnLayout { spacing: 1; Label { text: "Synaqua"; color: root.ink; font.pixelSize: 24; font.weight: Font.DemiBold } Label { text: "Make yourself at home, " + synaqua.userName; color: root.subtle; font.pixelSize: 12 } }
                Item { Layout.fillWidth: true }
                ToolButton { text: "✕"; Accessible.name: "Close Start menu"; palette.buttonText: root.ink; onClicked: root.closeMenu() }
            }
            TextField {
                id: search; objectName: "appSearch"; Layout.fillWidth: true; Layout.preferredHeight: 44
                placeholderText: "Search your applications…"; color: root.ink; placeholderTextColor: root.subtle; font.pixelSize: 15; selectByMouse: true
                Accessible.name: "Search applications"; leftPadding: 15; rightPadding: 15
                onTextChanged: synaqua.query = text
                onAccepted: { if (synaqua.applications.length > 0) root.openApp(synaqua.applications[0].desktopId) }
                background: Rectangle { radius: 12; color: synaqua.dark ? "#c0112236" : "#eaffffff"; border.color: search.activeFocus ? "#76bdf0" : "#7092b5cd" }
                Keys.onDownPressed: apps.forceActiveFocus()
            }
            RowLayout {
                Label { text: search.text.length ? "SEARCH RESULTS" : "YOUR APPLICATIONS"; color: root.subtle; font.pixelSize: 11; font.letterSpacing: 1.5 }
                Item { Layout.fillWidth: true }
                Label { text: "☆ to pin · up to 8 apps"; color: root.subtle; font.pixelSize: 11 }
            }
            GridView {
                id: apps; objectName: "applicationGrid"; Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                cellWidth: width / 2; cellHeight: 62; model: synaqua.applications
                boundsBehavior: Flickable.StopAtBounds; keyNavigationEnabled: true; currentIndex: 0
                ScrollBar.vertical: ScrollBar { }
                Keys.onReturnPressed: { if (currentIndex >= 0 && currentIndex < synaqua.applications.length) root.openApp(synaqua.applications[currentIndex].desktopId) }
                Keys.onEscapePressed: root.closeMenu()
                delegate: Item {
                    required property var modelData; required property int index
                    width: apps.cellWidth - 6; height: 58
                    Rectangle { anchors.fill: parent; radius: 12; color: appLaunch.hovered || (apps.activeFocus && apps.currentIndex === index) ? (synaqua.dark ? "#405b80a5" : "#80ffffff") : "transparent" }
                    ToolButton {
                        id: appLaunch; anchors.fill: parent; anchors.rightMargin: 36
                        Accessible.name: modelData.name; hoverEnabled: true
                        onClicked: root.openApp(modelData.desktopId)
                        background: Item { }
                        contentItem: RowLayout {
                            spacing: 10
                            Kirigami.Icon { source: modelData.icon; Layout.preferredWidth: 32; Layout.preferredHeight: 32 }
                            ColumnLayout { Layout.fillWidth: true; spacing: 2
                                Label { Layout.fillWidth: true; text: modelData.name; color: root.ink; font.pixelSize: 13; elide: Text.ElideRight }
                                Label { Layout.fillWidth: true; text: modelData.description || "Application"; color: root.subtle; font.pixelSize: 10; elide: Text.ElideRight }
                            }
                        }
                    }
                    ToolButton { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; width: 34; text: modelData.pinned ? "★" : "☆"; palette.buttonText: root.ink; Accessible.name: (modelData.pinned ? "Unpin " : "Pin ") + modelData.name; onClicked: synaqua.togglePin(modelData.desktopId) }
                }
                Label { anchors.centerIn: parent; visible: apps.count === 0; text: "No applications found.\nTry another name."; horizontalAlignment: Text.AlignHCenter; color: root.subtle }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: synaqua.dark ? "#406b8aa7" : "#608bacbe" }
            RowLayout {
                ToolButton { text: "Home"; palette.buttonText: root.ink; onClicked: { root.closeMenu(); synaqua.openHome() } }
                ToolButton { text: synaqua.dark ? "Light glass" : "Dark glass"; palette.buttonText: root.ink; onClicked: synaqua.setGlass(!synaqua.dark) }
                ToolButton { objectName: "dockSettingsButton"; text: "Dock settings"; palette.buttonText: root.ink; onClicked: synaqua.openDockSettings() }
                ToolButton { visible: synaqua.live; text: "Install"; palette.buttonText: root.ink; onClicked: root.openApp("synterra-install.desktop") }
                Item { Layout.fillWidth: true }
                Label { text: "Meta+Space"; color: root.subtle; font.pixelSize: 11 }
                ToolButton { text: "Lock"; palette.buttonText: root.ink; onClicked: { root.closeMenu(); synaqua.lockScreen() } }
                ToolButton { text: "Power"; palette.buttonText: root.ink; onClicked: { root.closeMenu(); synaqua.openSessionDialog() } }
            }
            Label { text: "Synterra 1.1 · Build 1170 · Synaqua 0.2"; color: root.subtle; font.pixelSize: 10; Layout.alignment: Qt.AlignHCenter }
        }
    }
    GlassPane {
        id: dock; objectName: "dock"; dark: synaqua.dark
        width: Math.min(root.availableDockWidth, Math.max(root.minimumDockWidth * root.dockScale, root.availableDockWidth * synaqua.dockWidth / 100))
        height: Math.round(84 * root.dockScale)
        anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.bottomMargin: synaqua.dockMargin
        onWidthChanged: Qt.callLater(root.syncRegions)
        onHeightChanged: Qt.callLater(root.syncRegions)
        onXChanged: Qt.callLater(root.syncRegions)
        onYChanged: Qt.callLater(root.syncRegions)
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.LeftButton | Qt.RightButton; onClicked: function(mouse) { if (mouse.button === Qt.RightButton) synaqua.openDockSettings() } }
        Column {
            id: brand; objectName: "dockBranding"
            x: 18 * root.dockScale; anchors.verticalCenter: parent.verticalCenter; spacing: 3; visible: synaqua.showBranding && leftGroup.x > 236 * root.dockScale
            Text { text: "SYNAQUA"; color: root.ink; font.pixelSize: 10 * root.dockScale; font.weight: Font.DemiBold; font.letterSpacing: 2 }
            Text { text: "PRISM / 1170"; color: root.subtle; font.pixelSize: 8 * root.dockScale; font.letterSpacing: 1 }
        }
        TaskManager.TasksModel { id: tasks; groupMode: TaskManager.TasksModel.GroupDisabled; filterByVirtualDesktop: false }
        ListView {
            objectName: "windowStrip"; visible: synaqua.showWindows && width >= 20 * root.dockScale
            x: (brand.visible ? 112 : 12) * root.dockScale; width: Math.max(0, leftGroup.x - x - 12 * root.dockScale); height: 48 * root.dockScale
            anchors.verticalCenter: parent.verticalCenter; orientation: ListView.Horizontal; spacing: 2; model: tasks; clip: true
            delegate: DockButton {
                required property var model; required property int index
                width: 34 * root.dockScale; height: 48 * root.dockScale; iconSize: 28 * root.dockScale; dark: synaqua.dark; iconSource: model.decoration; label: model.display; running: true
                onClicked: { if (model.IsActive) tasks.requestToggleMinimized(tasks.makeModelIndex(index)); else tasks.requestActivate(tasks.makeModelIndex(index)); root.closeMenu() }
            }
            Text { visible: tasks.count === 0 && parent.width > 40; anchors.centerIn: parent; text: "" }
        }
        Row {
            id: leftGroup; objectName: "leftPins"; anchors.right: start.left; anchors.rightMargin: 12 * root.dockScale; anchors.verticalCenter: parent.verticalCenter; spacing: synaqua.dockSpacing * root.dockScale
            Repeater { model: synaqua.leftPins; delegate: DockButton { required property var modelData; width: root.pinSize; height: 58 * root.dockScale; iconSize: 34 * root.dockScale; dark: synaqua.dark; iconSource: modelData.icon; label: modelData.name; onClicked: root.openApp(modelData.desktopId) } }
        }
        ToolButton {
            id: start; objectName: "startButton"; width: 64 * root.dockScale; height: 64 * root.dockScale
            anchors.centerIn: parent; hoverEnabled: true; Accessible.name: "Open Synaqua Start menu"
            ToolTip.visible: hovered; ToolTip.text: "Start · Meta+Space"; ToolTip.delay: 400
            onClicked: synaqua.toggleMenu()
            background: Rectangle { radius: 17; border.color: "#cff1ff"; border.width: 1; gradient: Gradient { GradientStop { position: 0; color: start.hovered || root.menuOpen ? "#a5d7f4" : "#89b9da" } GradientStop { position: .48; color: "#437fae" } GradientStop { position: .49; color: "#285c8a" } GradientStop { position: 1; color: "#3d78a5" } } }
            contentItem: Kirigami.Icon { source: "synterra"; implicitWidth: 45; implicitHeight: 45 }
        }
        Row {
            objectName: "rightPins"; id: rightGroup
            anchors.left: start.right; anchors.leftMargin: 12 * root.dockScale; anchors.verticalCenter: parent.verticalCenter; spacing: synaqua.dockSpacing * root.dockScale
            Repeater { model: synaqua.rightPins; delegate: DockButton { required property var modelData; width: root.pinSize; height: 58 * root.dockScale; iconSize: 34 * root.dockScale; dark: synaqua.dark; iconSource: modelData.icon; label: modelData.name; onClicked: root.openApp(modelData.desktopId) } }
        }
        Row {
            objectName: "dockUtilities"; id: utilities
            anchors.right: parent.right; anchors.rightMargin: 14 * root.dockScale; anchors.verticalCenter: parent.verticalCenter; spacing: 5 * root.dockScale
            DockButton { width: 30 * root.dockScale; height: 40 * root.dockScale; iconSize: 28 * root.dockScale; iconSource: "network-wireless"; label: "Network settings"; dark: synaqua.dark; onClicked: { root.closeMenu(); synaqua.openSettings("kcm_networkmanagement") } }
            ToolButton { objectName: "clockButton"; visible: synaqua.showClock; width: 72 * root.dockScale; height: 52 * root.dockScale; Accessible.name: "Clock and calendar settings"; onClicked: { root.closeMenu(); synaqua.openSettings("kcm_clock") }
                background: Rectangle { color: parent.hovered ? "#30ffffff" : "transparent"; radius: 10 }
                contentItem: Column { spacing: 2; Text { id: timeLabel; anchors.horizontalCenter: parent.horizontalCenter; color: root.ink; font.pixelSize: 18 * root.dockScale } Text { id: dateLabel; anchors.horizontalCenter: parent.horizontalCenter; color: root.subtle; font.pixelSize: 10 * root.dockScale } }
            }
            DockButton { width: 28 * root.dockScale; height: 40 * root.dockScale; iconSize: 28 * root.dockScale; iconSource: "view-restore"; label: "Show desktop"; dark: synaqua.dark; onClicked: { root.closeMenu(); synaqua.showDesktop() } }
        }
        Timer { interval: 1000; running: true; repeat: true; triggeredOnStart: true; onTriggered: { timeLabel.text = Qt.formatTime(new Date(), "HH:mm"); dateLabel.text = Qt.formatDate(new Date(), "ddd, d MMM") } }
    }
    Rectangle {
        id: errorToast
        visible: synaqua.error.length > 0; anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: dock.top; anchors.bottomMargin: 16; width: Math.min(640,root.width-60); height: errorText.implicitHeight + 28; radius: 12; color: "#f32b3c50"; border.color: "#c7dfed"
        Text { id: errorText; anchors.fill: parent; anchors.margins: 14; anchors.rightMargin: 44; text: synaqua.error; color: "#f6fbff"; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
        ToolButton { anchors.right: parent.right; anchors.top: parent.top; text: "×"; palette.buttonText: "white"; Accessible.name: "Dismiss error"; onClicked: synaqua.clearError() }
    }
}
