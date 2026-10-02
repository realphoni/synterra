// First-session layout only. Plasma runs this global theme layout for new users.
var panel = new Panel;
panel.location = 'bottom';
panel.height = 46;
panel.floating = false;
var launcher = panel.addWidget('org.kde.plasma.kickoff');
launcher.currentConfigGroup = ['General'];
launcher.writeConfig('icon', 'synterra');
var tasks = panel.addWidget('org.kde.plasma.icontasks');
tasks.currentConfigGroup = ['General'];
tasks.writeConfig('launchers', 'applications:org.kde.dolphin.desktop,applications:synterra-surf.desktop,applications:org.kde.konsole.desktop,applications:systemsettings.desktop');
panel.addWidget('org.kde.plasma.systemtray');
var clock = panel.addWidget('org.kde.plasma.digitalclock');
clock.currentConfigGroup = ['Appearance'];
clock.writeConfig('showDate', true);
clock.writeConfig('use24hFormat', 2);
panel.addWidget('org.kde.plasma.showdesktop');
var desktops = desktopsForActivity(currentActivity());
for (var i = 0; i < desktops.length; i++) {
    desktops[i].wallpaperPlugin = 'org.kde.image';
    desktops[i].currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General'];
    desktops[i].writeConfig('Image', 'file:///usr/share/wallpapers/Synterra-aurora/contents/images/3840x2160.png');
    desktops[i].writeConfig('FillMode', 2);
}
