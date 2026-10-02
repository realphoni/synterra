use crate::Result;
use std::{
    fs,
    path::{Path, PathBuf},
    process::{Command, Output},
};

fn executable(name: &str) -> Result<PathBuf> {
    let suffix = if cfg!(windows) { ".exe" } else { "" };
    for dir in std::env::split_paths(&std::env::var_os("PATH").unwrap_or_default()) {
        let file = dir.join(format!("{name}{suffix}"));
        if file.is_file() {
            return Ok(file);
        }
    }
    Err(format!("{name} is required for the signature regression").into())
}

fn native(gpg: &Path, path: &Path) -> Result<String> {
    let cygpath = gpg.with_file_name("cygpath.exe");
    if cfg!(windows) && cygpath.is_file() {
        let output = Command::new(cygpath).arg("-u").arg(path).output()?;
        if !output.status.success() {
            return Err("cygpath failed".into());
        }
        return Ok(String::from_utf8(output.stdout)?.trim().into());
    }
    Ok(path.to_string_lossy().into_owned())
}

fn run(gpg: &Path, home: &Path, args: &[&str], check: bool) -> Result<Output> {
    let output = Command::new(gpg)
        .args(["--no-options", "--homedirn        .as_nanos();
    let temp = test_parent.join(format!("gpg-test-{}-{stamp}", std::process::id()));
    fs::create_dir(&temp)?;
    let signer = temp.join("signer");
    let verifier = temp.join("verifier");
    fs::create_dir(&signer)?;
    fs::create_dir(&verifier)?;
    #[cfg(unix)]
    {
        use std::os::unix::fs::PermissionsExt;
        for home in [&signer, &verifier] {
            fs::set_permissions(home, fs::Permissions::from_mode(0o700))?;
        }
    }
    let result = (|| -> Result {
        run(
            &gpg,
            &signer,
            &[
                "--pinentry-mode",
                "loopback",
                "--passphrase",
                "",
                "--quick-generate-key",
                "Synterra regression <test@example.invalid>",
                "ed25519",
                "sign",
                "0",
            ],
            true,
        )?;
        let key = temp.join("archlinux.gpg");
        let exported = run(&gpg, &signer, &["--armor", "--export"], true)?.stdout;
        if !exported.starts_with(b"-----BEGIN PGP PUBLIC KEY BLOCK-----") {
            return Err("Expected armored public key".into());
        }
        fs::write(       "--passphrase",
                "",
                "--output",
                &sig_arg,
                "--detach-sign",
                &archive_arg,
            ],
            true,
        )?;
        run(&gpg, &verifier, &["-erify", &sig_arg, &archive_arg],
            false,
        )?
        .status
        .success()
        {
            return Err("Tampered signature unexpectedly accepted".into());
        }
        println!("PASS: armored key import, valid signature, and tamper rejection.");
        Ok(())
    })();
    if let Ok(gpgconf) = executable("gpgconf") {
        for home in [&signer, &verifier] {
            let _ = Command::new(&gpgconf)
                .args(["--homedir", &native(&gpg, s(&path, output)?;
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
        Err(me",
                "topright",
                "bottomleft",
                "bottomright",
            ] {
                require(
                    ids.contains(part) && ids.contains(format!("mask-{part}").as_str()),
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
        let data = fs::read(repo.join(f      !data.contains(&b'\r') && data.starts_with(b"#!/usr/bin/env bash\n"),
                format!(
                    "Invalid Linux script line endings/shebang: {}",
                    path.display()
                ),
            )?;
    & !s.starts_with('#'))
        .collect();
    let unique: HashSet<_> = packages.iter().copied().collect();
    require(unique.len() == packages.len(), "Duplicate packages")?;
    for package in [
        "plasma-desktop",
        "sddm",
    package: {package}"),
        )?;
    }
    require(
        !unique.contains("firefox"),
        "Firefox must be replaced by Synterra Surf",
    )?;
    println!("PASS: JSON metadata, {svgs} SVGs and blur masks, 4K wallpapers, LF scripts, palette and package manifest.");
    Ok(())
}
