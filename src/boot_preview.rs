use crate::Result;

#[cfg(not(target_os = "linux"))]
pub fn preview() -> Result {
    Err("Run boot-preview on the Linux build host.".into())
}

#[cfg(target_os = "linux")]
pub fn preview() -> Result {
    use serde_json::{json, Value};
    use std::{
        env, fs,
        io::{BufRead, BufReader, Write},
        os::unix::net::UnixStream,
        path::PathBuf,
        process::{Command, Stdio},
        thread,
        time::Duration,
    };
    fn request(reader: &mut BufReader<UnixStream>, value: Value) -> Result<Value> {
        writeln!(reader.get_mut(), "{value}")?;
        loop {
            let mut line = String::new();
            if reader.read_line(&mut line)? == 0 {
                return Err("QEMU monitor closed.".into());
            }
            let value: Value = serde_json::from_str(&line)?;
            if value.get("error").is_some() {
                return Err(format!("QEMU: {value}").into());
            }
            if value.get("return").is_some() {
                return Ok(value);
            }
        }
    }
    let args: Vec<_> = env::args().collect();
    let iso = fs::canonicalize(
        args.get(2)
            .ok_or("Usage: boot-preview ISO OUTPUT.png [WAIT_SECONDS] [bios|uefi]")?,
    )?;
    let output = PathBuf::from(args.get(3).ok_or("Missing screenshot path")?);
    let wait: u64 = args.get(4).map(|s| s.parse()).transpose()?.unwrap_or(7);
    if !iso.is_file()
        || iso.to_string_lossy().contains(',')
        || wait > 30
        || output.extension().is_none_or(|s| s != "png")
    {
        return Err("Invalid boot preview arguments.".into());
    }
    let parent = output.parent().ok_or("Invalid screenshot parent")?;
    fs::create_dir_all(parent)?;
    let output = fs::canonicalize(parent)?.join(output.file_name().ok_or("Missing filename")?);
    let temp = env::temp_dir().join(format!("synterra-boot-preview-{}", std::process::id()));
    fs::create_dir(&temp)?;
    let socket = temp.join("qmp.sock");
    let log = fs::File::create(output.with_extension("qemu.log"))?;
    let mut command = Command::new("qemu-system-x86_64");
    command.arg("-serial").arg(format!(
        "file:{}",
        output.with_extension("serial.log").display()
    ));
    command
        .args([
            "-machine",
            "q35,accel=tcg",
            "-m",
            "512",
            "-smp",
            "2",
            "-nic",
            "none",
            "-vga",
            "std",
            "-display",
            "none",
            "-boot",
            "order=d",
            "-cdrom",
        ])
        .arg(&iso)
        .arg("-qmp")
        .arg(format!("unix:{},server=on,wait=off", socket.display()))
        .stdout(Stdio::null())
        .stderr(log);
    if args.get(5).is_none_or(|s| s == "uefi") {
        let vars = temp.join("OVMF_VARS.fd");
        fs::copy("/usr/share/edk2/x64/OVMF_VARS.4m.fd", &vars)?;
        command
            .args([
                "-drive",
                "if=pflash,format=raw,readonly=on,file=/usr/share/edk2/x64/OVMF_CODE.4m.fd",
            ])
            .arg("-drive")
            .arg(format!("if=pflash,format=raw,file={}", vars.display()));
    } else if args[5] != "bios" {
        return Err("Firmware must be bios or uefi.".into());
    }
    let mut child = command.spawn()?;
    let result = (|| -> Result {
        for _ in 0..50 {
            if socket.exists() {
                break;
            }
            if child.try_wait()?.is_some() {
                return Err("QEMU exited; inspect the screenshot's .qemu.log file.".into());
            }
            thread::sleep(Duration::from_millis(100));
        }
        let stream = UnixStream::connect(&socket)?;
        stream.set_read_timeout(Some(Duration::from_secs(5)))?;
        let mut reader = BufReader::new(stream);
        let mut greeting = String::new();
        reader.read_line(&mut greeting)?;
        request(&mut reader, json!({"execute":"qmp_capabilities"}))?;
        thread::sleep(Duration::from_secs(wait));
        request(&mut reader, json!({"execute":"stop"}))?;
        request(
            &mut reader,
            json!({"execute":"screendump","arguments":{"filename":output,"format":"png"}}),
        )?;
        Ok(())
    })();
    let _ = child.kill();
    let _ = child.wait();
    let _ = fs::remove_dir_all(temp);
    result?;
    println!("Saved real GRUB/firmware screenshot: {}", output.display());
    Ok(())
}
