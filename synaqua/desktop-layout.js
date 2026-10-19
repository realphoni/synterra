// Synaqua uses its own layout file; the ordinary Plasma layout stays separate.
// The native synaqua-shell service provides the dock and Start menu.
var desktops = desktopsForActivity(currentActivity());
for (var i = 0; i < desktops.length; i++) {
    desktops[i].wallpaperPlugin = 'org.kde.image';
    desktops[i].currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General'];
    desktops[i].writeConfig('Image', 'file:///usr/share/wallpapers/Synterra-aurora/contents/images/3840x2160.png');
    desktops[i].writeConfig('FillMode', 2);
}
