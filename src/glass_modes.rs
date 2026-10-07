use crate::{
    artwork::{ini_text, parse_ini, write},
    Result,
};
use std::{fs, path::Path};

fn copy_tree(source: &Path, destination: &Path) -> Result {
    fs::create_dir_all(destination)?;
    for entry in fs::read_dir(source)? {
        let entry = entry?;
        if entry.file_type()?.is_dir() {
            copy_tree(&entry.path(), &destination.join(entry.file_name()))?;
        } else {
            fs::copy(entry.path(), destination.join(entry.file_name()))?;
        }
    }
    Ok(())
}

pub fn generate(repo: &Path) -> Result {
    let root = repo.join("profile/airootfs/usr/share");
    for directory in ["plasma/desktoptheme", "aurorae/themes"] {
        let source = root.join(directory).join("SynterraGlass");
        let destination = root.join(directory).join("SynterraGlassDark");
        copy_tree(&source, &destination)?;
        let mut paths = vec![destination.clone()];
        while let Some(path) = paths.pop() {
            for entry in fs::read_dir(path)? {
                let entry = entry?;
                if entry.file_type()?.is_dir() {
                    paths.push(entry.path());
                    continue;
                }
                let mut text = fs::read_to_string(entry.path())?;
                if entry.path().extension().is_some_and(|ext| ext == "svg") {
                    for (light, dark) in [
                        ("#eff8ff", "#819fb7"),
                        ("#a7cbe9", "#476884"),
                        ("#537eaf", "#253e58"),
                        ("#32537e", "#122d48"),
                        ("#87afd3", "#38566f"),
                        ("#7897b4", "#2b425c"),
                        ("#7194b8", "#3b5875"),
                    ] {
                        text = text.replace(light, dark);
                    }
                    text = text
                        .replace("opacity=\"0.58\"", "opacity=\"0.82\"")
                        .replace("opacity=\"0.48\"", "opacity=\"0.72\"");
                } else {
                    text = text
                        .replace("SynterraGlass", "SynterraGlassDark")
                        .replace("Synterra Glass", "Synterra Glass Dark");
                }
                write(&entry.path(), &text)?;
                if entry.file_name() == "SynterraGlassrc" {
                    fs::rename(entry.path(), destination.join("SynterraGlassDarkrc"))?;
                }
            }
        }
    }
    let mut scheme = parse_ini("[General]\nName=Synterra Glass Dark\nColorScheme=SynterraGlassDark\nshadeSortColumn=true\n[WM]\nactiveBackground=35,59,82\nactiveForeground=239,247,255\ninactiveBackground=31,44,60\ninactiveForeground=164,184,207\n");
    for (group, background) in [
        ("Window", "24,36,50"),
        ("View", "17,27,39"),
        ("Button", "38,57,77"),
        ("Selection", "44,106,163"),
        ("Tooltip", "31,48,68"),
        ("Complementary", "17,27,39"),
        ("Header", "32,49,68"),
    ] {
        let values = parse_ini(&format!("[Colors:{group}]\nBackgroundNormal={background}\nBackgroundAlternate=28,43,59\nForegroundNormal=232,242,253\nForegroundInactive=150,173,197\nForegroundActive=143,206,255\nForegroundLink=122,197,255\nForegroundVisited=194,164,243\nForegroundNegative=255,145,145\nForegroundNeutral=239,198,119\nForegroundPositive=121,218,178\nDecorationFocus=95,179,239\nDecorationHover=143,206,255\n"));
        scheme.extend(values);
    }
    let colors = ini_text(&scheme);
    write(
        &root.join("color-schemes/SynterraGlassDark.colors"),
        &colors,
    )?;
    write(
        &root.join("plasma/desktoptheme/SynterraGlassDark/colors"),
        &colors,
    )?;
    let light = root.join("plasma/look-and-feel/org.synterra.glass.desktop");
    let dark = root.join("plasma/look-and-feel/org.synterra.glass.dark.desktop");
    copy_tree(&light, &dark)?;
    let metadata = fs::read_to_string(dark.join("metadata.json"))?
        .replace(
            "org.synterra.glass.desktop",
            "org.synterra.glass.dark.desktop",
        )
        .replace("Synterra Glass", "Synterra Glass Dark");
    write(&dark.join("metadata.json"), &metadata)?;
    let defaults = fs::read_to_string(dark.join("contents/defaults"))?
        .replace("ColorScheme=SynterraGlass", "ColorScheme=SynterraGlassDark")
        .replace("name=SynterraGlass", "name=SynterraGlassDark")
        .replace("Theme=SynterraGlass", "Theme=SynterraGlassDark")
        .replace(
            "__aurorae__svg__SynterraGlass",
            "__aurorae__svg__SynterraGlassDark",
        );
    write(&dark.join("contents/defaults"), &defaults)?;
    generate_icons(&root)?;
    Ok(())
}

fn generate_icons(root: &Path) -> Result {
    let icons = root.join("icons/SynterraGlass");
    write(&icons.join("index.theme"), "[Icon Theme]\nName=Synterra Glass\nComment=Original glass desktop icons with complete Breeze fallback\nInherits=breeze,hicolor\nDirectories=scalable/apps,scalable/places\n[scalable/apps]\nSize=48\nType=Scalable\nMinSize=16\nMaxSize=256\nContext=Applications\n[scalable/places]\nSize=48\nType=Scalable\nMinSize=16\nMaxSize=256\nContext=Places\n")?;
    for (name, symbol) in [
        ("system-file-manager", "<path d='M15 25h34v23H15zM15 25v-8h14l5 8'/>"),
        ("utilities-terminal", "<path d='m18 23 9 9-9 9m15 0h13'/>"),
        ("preferences-desktop-theme", "<path d='M16 18h32v24H16zM25 49h14m-7-7v7'/>"),
        ("accessories-calculator", "<rect x='20' y='13' width='24' height='38' rx='2'/><path d='M25 20h14m-14 9h3m8 0h3m-14 8h3m8 0h3m-14 8h3m8 0h3'/>"),
    ] {
        let svg = format!("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'><defs><linearGradient id='g' x2='0' y2='1'><stop stop-color='#b4dcf8'/><stop offset='.5' stop-color='#557fa9'/><stop offset='1' stop-color='#23476e'/></linearGradient></defs><rect x='5' y='5' width='54' height='54' rx='11' fill='url(#g)' stroke='#ceeaff' stroke-width='2'/><path d='M7 17q25-14 50 0v10H7z' fill='#fff' opacity='.18'/><g fill='none' stroke='#f3faff' stroke-width='3' stroke-linejoin='round' stroke-linecap='round'>{symbol}</g></svg>");
        write(&icons.join(format!("scalable/apps/{name}.svg")), &svg)?;
        if name == "system-file-manager" { write(&icons.join("scalable/places/folder.svg"), &svg)?; }
    }
    for file in ["synterra.svg", "synterra-surf.svg"] {
        fs::copy(
            root.join("icons/hicolor/scalable/apps").join(file),
            icons.join("scalable/apps").join(file),
        )?;
    }
    let dark = root.join("icons/SynterraGlassDark");
    copy_tree(&icons, &dark)?;
    let index = fs::read_to_string(dark.join("index.theme"))?
        .replace("Name=Synterra Glass", "Name=Synterra Glass Dark")
        .replace(
            "Inherits=breeze,hicolor",
            "Inherits=breeze-dark,breeze,hicolor",
        );
    write(&dark.join("index.theme"), &index)?;
    Ok(())
}
