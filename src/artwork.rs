use crate::Result;
use std::{collections::BTreeMap, fs, path::Path};

fn frame(prefix: &str, offset: i32, opacity: &str, mask: bool, title: i32) -> String {
    let mut result = String::new();
    for (part, x, mut y, w, mut h) in [
        ("topleft", 0, 0, 7, 7),
        ("top", 7, 0, 50, 7),
        ("topright", 57, 0, 7, 7),
        ("left", 0, 7, 7, 50),
        ("center", 7, 7, 50, 50),
        ("right", 57, 7, 7, 50),
        ("bottomleft", 0, 57, 7, 7),
        ("bottom", 7, 57, 50, 7),
        ("bottomright", 57, 57, 7, 7),
    ] {
        if y == 0 {
            h = title;
        } else {
            y += title - 7;
        }
        let id = if prefix.is_empty() {
            part.into()
        } else {
            format!("{prefix}-{part}")
        };
        // A vertical titlebar gradient repeats in each tiled side segment.
        // Keep the gloss on horizontal edges and a uniform tint on the sides.
        let smooth_edge = title > 7 && !matches!(part, "top" | "topleft" | "topright");
        let fill = if mask {
            "#000000"
        } else if smooth_edge {
            "#7897b4"
        } else {
            "url(#glass)"
        };
        let alpha = if !mask && smooth_edge {
            if opacity == "1" {
                "0.58"
            } else {
                "0.38"
            }
        } else {
            opacity
        };
        result.push_str(&format!(r#"<rect id="{id}" x="{}" y="{y}" width="{w}" height="{h}" fill="{fill}" opacity="{alpha}"/>"#, x+offset));
        result.push('\n');
    }
    result
}

fn svg(body: &str, width: i32, height: i32) -> String {
    format!(
        r##"<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<defs><linearGradient id="glass" x1="0" y1="0" x2="0" y2="1" gradientUnits="objectBoundingBox">
<stop offset="0" stop-color="#eff8ff" stop-opacity=".70"/>
<stop offset=".08" stop-color="#a7cbe9" stop-opacity=".65"/>
<stop offset=".48" stop-color="#537eaf" stop-opacity=".58"/>
<stop offset=".51" stop-color="#32537e" stop-opacity=".68"/>
<stop offset="1" stop-color="#87afd3" stop-opacity=".72"/>
</linearGradient></defs>{body}</svg>"##
    )
}

fn write(path: &Path, text: &str) -> Result {
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::write(path, format!("{}\n", text.trim_end()))?;
    Ok(())
}

type Ini = BTreeMap<String, BTreeMap<String, String>>;
fn parse_ini(text: &str) -> Ini {
    let mut result: Ini = BTreeMap::new();
    let mut section = String::new();
    for line in text.lines().map(str::trim) {
        if line.starts_with('[') && line.ends_with(']') {
            section = line[1..line.len() - 1].into();
        } else if let Some((key, value)) = line.split_once('=') {
            result
                .entry(section.clone())
                .or_default()
                .insert(key.into(), value.into());
        }
    }
    result
}
fn ini_text(ini: &Ini) -> String {
    let mut text = String::new();
    for (section, values) in ini {
        text.push_str(&format!("[{section}]\n"));
        for (key, value) in values {
            text.push_str(&format!("{key}={value}\n"));
        }
        text.push('\n');
    }
    text
}

pub fn generate(repo: &Path) -> Result {
    let root = repo.join("profile/airootfs/usr/share");
    let style = root.join("plasma/desktoptheme/SynterraGlass");
    let aurorae = root.join("aurorae/themes/SynterraGlass");
    let mut body = frame("", 0, "1", false, 7) + &frame("mask", 80, "1", true, 7);
    body.push_str(r#"<rect id="hint-stretch-borders" x="0" y="75" width="1" height="1"/>"#);
    for side in ["top", "bottom", "left", "right"] {
        body.push_str(&format!(
            r#"<rect id="hint-{side}-margin" x="10" y="75" width="7" height="7"/>"#
        ));
    }
    for file in [
        "widgets/panel-background.svg",
        "dialogs/background.svg",
        "widgets/background.svg",
        "widgets/tooltip.svg",
    ] {
        write(&style.join(file), &svg(&body, 300, 90))?;
    }
    let decoration = frame("decoration", 0, "1", false, 34)
        + &frame("decoration-inactive", 80, "0.65", false, 34)
        + &frame("mask", 160, "1", true, 34);
    write(&aurorae.join("decoration.svg"), &svg(&decoration, 300, 100))?;
    for (name, glyph) in [
        ("close", r#"<path d="M17 8l9 9m0-9l-9 9"/>"#),
        ("minimize", r#"<path d="M10 17h10"/>"#),
        (
            "maximize",
            r#"<rect x="10" y="7" width="10" height="9" fill="none"/>"#,
        ),
        (
            "restore",
            r#"<path d="M10 10h9v8h-9zM13 10V7h9v8h-3" fill="none"/>"#,
        ),
    ] {
        let width = if name == "close" { 43 } else { 29 };
        let mut groups = String::new();
        for (index, state) in [
            "active",
            "inactive",
            "hover",
            "pressed",
            "hover-inactive",
            "pressed-inactive",
        ]
        .iter()
        .enumerate()
        {
            let color = match (name == "close", state.contains("hover")) {
                (true, true) => "#fb8f79",
                (true, false) => "#b83b35",
                (false, true) => "#6dccff",
                (false, false) => "#7194b8",
            };
            let opacity = if *state == "inactive" { ".55" } else { ".95" };
            groups.push_str(&format!(r##"<g id="{state}-center" transform="translate({},0)" opacity="{opacity}">
<rect x="0" y="0" width="{width}" height="23" rx="3" fill="{color}" stroke="#e3f3ff" stroke-opacity=".65"/>
<rect x="1" y="1" width="{}" height="10" rx="2" fill="#ffffff" opacity=".25"/>
<g stroke="#f8fcff" stroke-width="1.8" stroke-linecap="square">{glyph}</g></g>"##,index*50,width-2));
        }
        write(&aurorae.join(format!("{name}.svg")), &svg(&groups, 300, 25))?;
    }
    let orb = r##"<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64">
<defs><radialGradient id="o" cx=".4" cy=".25" r=".8"><stop stop-color="#b4eaff"/><stop offset=".5" stop-color="#3c9bea"/><stop offset="1" stop-color="#183f83"/></radialGradient></defs>
<circle cx="32" cy="32" r="29" fill="url(#o)" stroke="#e4f6ff" stroke-width="2"/>
<ellipse cx="32" cy="19" rx="22" ry="11" fill="#fff" opacity=".22"/>
<path d="M43 20C25 9 13 28 30 32c22 5 8 23-10 11" fill="none" stroke="#fff" stroke-width="5" stroke-linecap="round"/>
</svg>"##;
    write(&root.join("icons/hicolor/scalable/apps/synterra.svg"), orb)?;
    let mut scheme = parse_ini("[General]\nName=Synterra Glass\nColorScheme=SynterraGlass\nshadeSortColumn=true\n[WM]\nactiveBackground=80,132,182\nactiveForeground=248,252,255\ninactiveBackground=156,177,199\ninactiveForeground=222,233,247\n");
    for (name, bg, fg) in [
        ("Window", "239,245,251", "23,37,53"),
        ("View", "255,255,255", "23,37,53"),
        ("Button", "230,240,249", "23,37,53"),
        ("Selection", "58,126,194", "255,255,255"),
        ("Tooltip", "242,248,255", "23,37,53"),
        ("Complementary", "37,58,84", "247,251,255"),
        ("Header", "229,240,250", "23,37,53"),
    ] {
        let mut values = BTreeMap::new();
        for (key, value) in [
            ("BackgroundNormal", bg),
            ("BackgroundAlternate", "231,239,247"),
            ("ForegroundNormal", fg),
            ("ForegroundInactive", "108,123,140"),
            ("ForegroundActive", "42,118,191"),
            ("ForegroundLink", "31,102,175"),
            ("ForegroundVisited", "112,78,168"),
            ("ForegroundNegative", "189,57,57"),
            ("ForegroundNeutral", "165,108,28"),
            ("ForegroundPositive", "34,128,91"),
            ("DecorationFocus", "76,157,231"),
            ("DecorationHover", "116,187,243"),
        ] {
            values.insert(key.into(), value.into());
        }
        scheme.insert(format!("Colors:{name}"), values);
    }
    let colors = ini_text(&scheme);
    write(&root.join("color-schemes/SynterraGlass.colors"), &colors)?;
    write(&style.join("colors"), &colors)?;
    let defaults = repo.join("profile/airootfs/etc/skel/.config/kdeglobals");
    let mut settings = parse_ini(&fs::read_to_string(&defaults)?);
    for (section, values) in scheme {
        if section.starts_with("Colors:") || section == "WM" {
            settings.entry(section).or_default().extend(values);
        }
    }
    write(&defaults, &ini_text(&settings))?;
    println!("Generated Synterra glass style, Aurorae frames, color scheme and launcher orb.");
    Ok(())
}
