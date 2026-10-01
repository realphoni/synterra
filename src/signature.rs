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
        .args(["--no-options", "--homedir", &native(gpg, home)?, "--batch"])
        .args(args)
        .output()?;
    if check && !output.status.success() {
        return Err(String::from_utf8_lossy(&output.stderr).into_owned().into());
    }
    Ok(output)
}

pub fn test() -> Result {
    let gpg = executable("gpg")?;
    let test_parent = Path::new(env!("CARGO_MANIFEST_DIR")).join("out");
    fs::create_dir_all(&test_parent)?;
    let stamp = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)?
        .as_nanos();
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
        fs::write(&key, exported)?;
        let archive = temp.join("bootstrap.fixture");
        let signature = temp.join("bootstrap.fixture.sig");
        fs::write(&archive, b"Synterra bootstrap regression fixture\n")?;
        let key_arg = native(&gpg, &key)?;
        let archive_arg = native(&gpg, &archive)?;
        let sig_arg = native(&gpg, &signature)?;
        run(
            &gpg,
            &signer,
            &[
                "--pinentry-mode",
                "loopback",
                "--passphrase",
                "",
                "--output",
                &sig_arg,
                "--detach-sign",
                &archive_arg,
            ],
            true,
        )?;
        run(&gpg, &verifier, &["--import", &key_arg], true)?;
        run(&gpg, &verifier, &["--verify", &sig_arg, &archive_arg], true)?;
        fs::write(&archive, b"Tampered bootstrap fixture\n")?;
        if run(
            &gpg,
            &verifier,
            &["--verify", &sig_arg, &archive_arg],
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
                .args(["--homedir", &native(&gpg, home)?, "--kill", "all"])
                .output();
        }
    }
    // Remove only the freshly-created test directory within the project's out/.
    let resolved = temp.canonicalize()?;
    if resolved.parent() != Some(test_parent.canonicalize()?.as_path()) {
        return Err("Refusing to clean a test directory outside out/".into());
    }
    fs::remove_dir_all(resolved)?;
    result
}
