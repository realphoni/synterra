use crate::Result;
use serde_json::json;
use std::{
    fs,
    io::{self, Write},
    path::Path,
    process::Command,
};

const TARGET: &str = "/mnt/synterra-install";
const PAYLOAD: &str = "/usr/share/synterra/install-overlay";

fn command(program: &str, args: &[&str]) -> Result {
    if !Command::new(program).args(args).status()?.success() {
        return Err(
            format!("{program} failed. Installation is incomplete; do not reboot yet.").into(),
        );
    }
    Ok(())
}

fn user_names(passwd: &str) -> Vec<String> {
    passwd
        .lines()
        .filter_map(|line| {
            let parts: Vec<_> = line.split(':').collect();
            if parts.len() != 7 {
                return None;
            }
            let uid = parts[2].parse::<u32>().ok()?;
            let name = parts[0];
            let valid = !name.is_empty()
                && name
                    .bytes()
                    .all(|c| c.is_ascii_alphanumeric() || c == b'_' || c == b'-');
            (valid
                && (1000..60000).contains(&uid)
                && parts[5] == format!("/home/{name}")
                && name != "live")
                .then(|| name.to_string())
        })
        .collect()
}

fn validate_target(target: &Path) -> Result<Vec<String>> {
    if !target.join("etc/synterra-install-ready").is_file()
        || !target.join("etc/fstab").is_file()
        || !target.join("usr/bin/pacman").is_file()
    {
        return Err("Arch installation was cancelled, failed, or used a different target. No Synterra files were applied.".into());
    }
    let fstab = fs::read_to_string(target.join("etc/fstab"))?;
    if !fstab.lines().any(|line| {
        let line = line.trim();
        !line.starts_with('#') && line.split_whitespace().nth(1) == Some("/")
    }) {
        return Err("The installed system has no root mount in fstab. Finish Archinstall before applying Synterra.".into());
    }
    let users = user_names(&fs::read_to_string(target.join("etc/passwd"))?);
    if users.is_empty() {
        return Err(
            "Create a regular administrator account in Archinstall before finalizing Synterra."
                .into(),
        );
    }
    for user in &users {
        let home = target.join("home").join(user);
        if !fs::symlink_metadata(&home)?.is_dir() {
            return Err(
                "An installed user home is missing or symlinked. No Synterra files were applied."
                    .into(),
            );
        }
    }
    Ok(users)
}

pub fn install() -> Result {
    if std::env::consts::OS != "linux" || !Path::new("/run/archiso").is_dir() {
        return Err("Run the installer from the Synterra live ISO, not from Windows, WSL or an installed system.".into());
    }
    let uid = Command::new("id").arg("-u").output()?;
    if String::from_utf8(uid.stdout)?.trim() != "0" {
        return Err("Run the installer with sudo.".into());
    }
    if Command::new("mountpoint")
        .args(["-q", TARGET])
        .status()?
        .success()
    {
        return Err("The installer target is already mounted. Finish or unmount the earlier installation before retrying.".into());
    }
    if !Path::new(PAYLOAD).join("etc/skel").is_dir() {
        return Err("Installer desktop payload is missing.".into());
    }
    println!("\nSynterra 1.1 Build 1105 installer\n\nConnect to the internet first. Arch's guided installer will ask for disks,\npartitions, timezone, bootloader and a password-protected administrator.\nReview its disk summary carefully: formatting destroys existing data.\nKeep the KDE Plasma profile and Synterra package list selected.\n\nIMPORTANT: At Archinstall's completion screen choose EXIT, not Reboot.\nSynterra must finish applying its desktop before you restart.\n\nPress Enter to start, or type cancel to leave.");
    let mut answer = String::new();
    io::stdin().read_line(&mut answer)?;
    if !answer.trim().is_empty() {
        println!("Cancelled.");
        return Ok(());
    }
    let packages: Vec<_> = fs::read_to_string("/usr/share/synterra/install-packages.txt")?
        .lines()
        .map(str::trim)
        .filter(|s| !s.is_empty() && !s.starts_with('#'))
        .map(String::from)
        .collect();
    let config = json!({
        "hostname": "synterra", "kernels": ["linux"],
        "bootloader_config": {"bootloader": "Grub", "uki": false, "removable": true},
        "packages": packages,
        "profile_config": {"profile": {"main": "Desktop", "details": ["KDE Plasma"]}, "gfx_driver": "All open-source", "greeter": "sddm"},
        "network_config": {"type": "nm"}, "audio_config": {"audio": "pipewire"},
        // The selected Archinstall profile owns its greeter until finalization.
        "services": ["NetworkManager", "vmtoolsd"],
        "custom_commands": ["touch /etc/synterra-install-ready"]
    });
    let config_path = format!("/run/synterra-install-{}.json", std::process::id());
    fs::write(&config_path, serde_json::to_vec_pretty(&config)?)?;
    let result = command(
        "archinstall",
        &["--config", &config_path, "--mountpoint", TARGET],
    );
    let _ = fs::remove_file(&config_path);
    result?;
    command("mountpoint", &["-q", TARGET])?;
    let target = Path::new(TARGET);
    let users = validate_target(target)?;
    println!("\nApplying Synterra desktop and branding…");
    command("cp", &["-a", &format!("{PAYLOAD}/."), TARGET])?;
    command(
        "cp",
        &[
            &format!("{TARGET}/usr/share/synterra/os-release"),
            &format!("{TARGET}/usr/lib/os-release"),
        ],
    )?;
    for user in users {
        let home = target.join("home").join(&user);
        if fs::symlink_metadata(&home)?.file_type().is_symlink() {
            return Err("Refusing to copy desktop defaults into a symlinked user home.".into());
        }
        command(
            "cp",
            &[
                "-a",
                &format!("{PAYLOAD}/etc/skel/."),
                home.to_str().ok_or("Invalid home path")?,
            ],
        )?;
        let group = Command::new("arch-chroot")
            .args([TARGET, "id", "-gn", &user])
            .output()?;
        if !group.status.success() {
            return Err("Could not determine installed user group.".into());
        }
        let group = String::from_utf8(group.stdout)?;
        command(
            "arch-chroot",
            &[
                TARGET,
                "chown",
                "-R",
                &format!("{user}:{}", group.trim()),
                &format!("/home/{user}"),
            ],
        )?;
    }
    fs::create_dir_all(target.join("etc/sddm.conf.d"))?;
    fs::write(
        target.join("etc/sddm.conf.d/10-synterra.conf"),
        "[Theme]\nCurrent=SynterraGlass\n",
    )?;
    command(
        "systemctl",
        &[
            "--root",
            TARGET,
            "--force",
            "enable",
            "NetworkManager",
            "sddm",
            "vmtoolsd",
            "bluetooth",
        ],
    )?;
    command(
        "systemctl",
        &["--root", TARGET, "set-default", "graphical.target"],
    )?;
    if target.join("boot/grub/grub.cfg").is_file() {
        command(
            "arch-chroot",
            &[TARGET, "/usr/local/libexec/synterra-grub-setup"],
        )?;
    } else {
        println!(
            "The selected bootloader is not GRUB; the Synterra GRUB splash will not be displayed."
        );
    }
    fs::remove_file(target.join("etc/synterra-install-ready"))?;
    command("sync", &[])?;
    println!("\nSynterra installation finished. Shut down, remove the live ISO, and boot\nfrom your installed disk. Sign in with the account created in Archinstall.\nNo live-user autologin or passwordless sudo was copied.\n");
    io::stdout().flush()?;
    Ok(())
}

pub fn test() -> Result {
    assert_eq!(user_names("root:x:0:0::/root:/bin/bash\nlive:x:1000:1000::/home/live:/bin/bash\nphoni:x:1001:1001::/home/phoni:/bin/bash\nbad:x:1002:1002::/../../etc:/bin/bash\nnobody:x:65534:65534::/:/bin/false"), vec!["phoni"]);
    let temp = std::env::temp_dir().join(format!("synterra-installer-test-{}", std::process::id()));
    fs::create_dir_all(temp.join("etc"))?;
    fs::create_dir_all(temp.join("usr/bin"))?;
    assert!(validate_target(&temp).is_err());
    fs::write(temp.join("etc/synterra-install-ready"), "")?;
    fs::write(temp.join("etc/fstab"), "")?;
    fs::write(temp.join("usr/bin/pacman"), "")?;
    fs::write(
        temp.join("etc/passwd"),
        "phoni:x:1000:1000::/home/phoni:/bin/bash",
    )?;
    fs::create_dir_all(temp.join("home/phoni"))?;
    assert!(
        validate_target(&temp).is_err(),
        "An empty fstab must reject an incomplete installation"
    );
    fs::write(temp.join("etc/fstab"), "UUID=fixture / ext4 defaults 0 1\n")?;
    fs::write(
        temp.join("etc/passwd"),
        "live:x:1000:1000::/home/live:/bin/bash",
    )?;
    assert!(validate_target(&temp).is_err());
    fs::write(
        temp.join("etc/passwd"),
        "phoni:x:1000:1000::/home/phoni:/bin/bash",
    )?;
    assert_eq!(validate_target(&temp)?, vec!["phoni"]);
    if std::env::consts::OS == "linux" {
        #[cfg(target_os = "linux")]
        {
            fs::remove_dir(temp.join("home/phoni"))?;
            std::os::unix::fs::symlink(temp.join("etc"), temp.join("home/phoni"))?;
            assert!(
                validate_target(&temp).is_err(),
                "A symlinked user home must be rejected before copying the payload"
            );
            fs::remove_file(temp.join("home/phoni"))?;
            fs::create_dir(temp.join("home/phoni"))?;
        }
        let units = temp.join("usr/lib/systemd/system");
        fs::create_dir_all(&units)?;
        let unit = "[Unit]\nDescription=Installer regression fixture\n[Service]\nExecStart=/usr/bin/true\n[Install]\nAlias=display-manager.service\n";
        fs::write(units.join("sddm.service"), unit)?;
        fs::write(units.join("cosmic-greeter.service"), unit)?;
        let root = temp.to_str().ok_or("Invalid test path")?;
        command("systemctl", &["--root", root, "enable", "cosmic-greeter"])?;
        let collision = Command::new("systemctl")
            .args(["--root", root, "enable", "sddm"])
            .output()?;
        assert!(
            !collision.status.success(),
            "Fixture must reproduce the greeter collision"
        );
        command("systemctl", &["--root", root, "--force", "enable", "sddm"])?;
        assert_eq!(
            fs::read_link(temp.join("etc/systemd/system/display-manager.service"))?
                .file_name()
                .unwrap(),
            "sddm.service"
        );
    }
    fs::remove_dir_all(temp)?;
    println!("PASS: installer readiness, root fstab, regular-user/home guards and COSMIC-to-SDDM alias conflict regression.");
    Ok(())
}
