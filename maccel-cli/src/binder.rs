use std::io::Read;
use std::io::Write;
use std::time::Duration;
use std::{fs::File, path::PathBuf};

use anyhow::{anyhow, Context};

pub fn bind_device(device_id: &str) -> anyhow::Result<()> {
    if NobindVar::is_set() {
        return Err(anyhow!("binding is disabled"));
    }

    eprintln!("[INFO] unbinding from hid-generic");
    unbind_device_from_driver("usbhid", device_id)?;
    eprintln!("[INFO] binding to maccel");
    bind_device_to_driver("maccel", device_id)?;
    Ok(())
}

pub fn unbind_device(device_id: &str) -> anyhow::Result<()> {
    NobindVar::set()?;
    eprintln!("[INFO] unbinding from maccel");
    unbind_device_from_driver("maccel", device_id)?;
    eprintln!("[INFO] binding to hid-generic");
    bind_device_to_driver("usbhid", device_id)?;

    std::thread::sleep(Duration::from_secs(1)); // give the usbhid driver time to bind the device
    NobindVar::unset()?;
    Ok(())
}

fn bind_device_to_driver(driver: &str, device_id: &str) -> anyhow::Result<()> {
    let bind_path = PathBuf::from(format!("/sys/bus/usb/drivers/{}/bind", driver));

    let mut bind_file = File::create(&bind_path).with_context(|| {
        format!(
            "failed to open the bind path for writing: {}",
            bind_path.display()
        )
    })?;

    bind_file.write_all(device_id.as_bytes()).with_context(|| {
        format!(
            "failed to bind device '{}' to driver '{}'",
            device_id, driver
        )
    })?;

    Ok(())
}

fn unbind_device_from_driver(driver: &str, device_id: &str) -> anyhow::Result<()> {
    let device_path = PathBuf::from(format!("/sys/bus/usb/drivers/{}/{}", driver, device_id));

    if !device_path.exists() {
        eprintln!(
            "[INFO] device {} is not bound to driver '{}'",
            device_id, driver
        );
        return Ok(());
    }

    let unbind_path = PathBuf::from(format!("/sys/bus/usb/drivers/{}/unbind", driver));

    let mut unbind_file = File::create(&unbind_path).with_context(|| {
        format!(
            "failed to open the unbind path for writing: {}",
            unbind_path.display()
        )
    })?;

    unbind_file
        .write_all(device_id.as_bytes())
        .with_context(|| {
            format!(
                "failed to unbind device '{}' from driver '{}'",
                device_id, driver
            )
        })?;

    Ok(())
}

/// When unbinding we want to temporarily disable to automatic binding
/// that will happen because of the udev rules we ship.
struct NobindVar;

impl NobindVar {
    const NO_BIND_VAR_PATH: &'static str = "/var/local/maccel-no-bind-var";

    fn is_set() -> bool {
        let Ok(mut file) = File::open(Self::NO_BIND_VAR_PATH) else {
            File::create(Self::NO_BIND_VAR_PATH).expect("failed to create variable file");
            return false;
        };

        let mut buf = String::new();

        let is_set = file
            .read_to_string(&mut buf)
            .map(|_| buf.trim() != "0")
            .unwrap_or(false);

        return is_set;
    }

    fn set() -> anyhow::Result<()> {
        let mut file = File::create(Self::NO_BIND_VAR_PATH)?;
        write!(file, "1")?;
        Ok(())
    }

    fn unset() -> anyhow::Result<()> {
        let mut file = File::create(Self::NO_BIND_VAR_PATH)?;
        write!(file, "0")?;
        Ok(())
    }
}
