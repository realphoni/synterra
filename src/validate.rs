use crate::Result;
use std::{
    collections::HashSet,
    fs,
    path::{Path, PathBuf},
};

fn files(root: &Path, output: &mut Vec<PathBuf>) -> Result {
    for entry in fs::read_dir(root)? {
        let path = entry?.path();
        if path.is_dir() {
            files(&path, output)?;
        } else if path.is_file() {
            output.push(path);
        }
    }
    Ok(())
}

fn require(condition: bool, message: impl Into<String>) -> Result {
    if condition {
        Ok(())
    } else {
        Err(message.into().into())
    }
}

pub fn validate(repo: &Path) -> Result {
    let root = repo.join("profile/airootfs");
    let mut all = Vec::new();
    files(&root, &mut all)?;
    let mut svgs = 0;
    for path in &all {
        let ext = path.extension().and_then(|s| s.to_str()).unwrap_or("");
        if ext == "json" || ext == "jsonc" {
            serde_json::from_slice::<serde_json::Value>(&fs::read(path)?)?;
        }
        if ext != "svg" {
            continue;
        }
        let text = fs::read_to_string(path)?;
        let xml = roxmltree::Document::parse(&text)?;
        let mut ids = HashSet::new();
        for id in xml.descendants().filter_map(|n| n.attribute("id")) {
            require(
                ids.insert(id),
                format!("Duplicate SVG ID {id}: {}", path.display()),
            )?;
        }
        let name = path.file_name().unwrap().to_string_lossy();
        if matches!(
            name.as_ref(),
            "panel-background.svg" | "background.svg" | "tooltip.svg"
        ) {
            for part in [
                "center",
                "top",
                "bottom",
                "left",
                "right",
                "topleft",
                "topright",
                "bottomleft",
                "bottomright",
            ] {
                require(
                    ids.contains(part) && ids.contains(format!("mask-{part}").as_str()),
                    format!("Missing frame/blur mask: {}", path.display()),
                )?;
            }
        }
        if name == "decoration.svg" {
            require(
                ids.contains("decoration-top") && ids.contains("mask-top"),
                "Missing decoration frame/mask",
            )?;
        }
        if matches!(
            name.as_ref(),
            "close.svg" | "maximize.svg" | "minimize.svg" | "restore.svg"
        ) {
            require(
                ids.contains("active-center") && ids.contains("hover-center"),
                "Missing button states",
            )?;
        }
        svgs += 1;
    }
    for name in ["aurora", "graphite"] {
        let data = fs::read(repo.join(format!("assets/wallpapers/Prism-{name}-4K.png")))?;
        require(
            data.len() >= 24 && &data[..8] == b"\x89PNG\r\n\x1a\n",
            "Invalid PNG",
        )?;
        let w = u32::from_be_bytes(data[16..20].try_into()?);
        let h = u32::from_be_bytes(data[20..24].try_into()?);
        require((w, h) == (3840, 2160), "Wrong wallpaper dimensions")?;
    }
    for path in fs::read_dir(repo.join("scripts"))?.chain(fs::read_dir(root.join("usr/local/bin"))?)
    {
        let path = path?.path();
        if path.extension().is_some_and(|e| e == "sh")
            || path.parent() == Some(root.join("usr/local/bin").as_path())
        {
            let data = fs::read(&path)?;
            require(
                !data.contains(&b'\r') && data.starts_with(b"#!/usr/bin/env bash\n"),
                format!(
                    "Invalid Linux script line endings/shebang: {}",
                    path.display()
                ),
            )?;
        }
    }
    let scheme = fs::read_to_string(root.join("usr/share/color-schemes/SynterraGlass.colors"))?;
    for group in ["Window", "View", "Button", "Selection", "Tooltip"] {
        require(
            scheme.contains(&format!("[Colors:{group}]")),
            "Missing palette group",
        )?;
    }
    let manifest = fs::read_to_string(repo.join("profile/packages.txt"))?;
    let packages: Vec<_> = manifest
        .lines()
        .filter(|s| !s.trim().is_empty() && !s.starts_with('#'))
        .collect();
    let unique: HashSet<_> = packages.iter().copied().collect();
    require(unique.len() == packages.len(), "Duplicate packages")?;
    for package in [
        "plasma-desktop",
        "sddm",
        "open-vm-tools",
        "aurorae",
        "rust",
        "cargo",
        "qt6-webengine",
        "spectacle",
        "gwenview",
        "okular",
        "vlc",
        "vlc-plugins-all",
        "filelight",
    ] {
        require(
            unique.contains(package),
            format!("Missing package: {package}"),
        )?;
    }
    require(
        !unique.contains("firefox"),
        "Firefox must be replaced by Synterra Surf",
    )?;
    println!("PASS: JSON metadata, {svgs} SVGs and blur masks, 4K wallpapers, LF scripts, palette and package manifest.");
    Ok(())
}
