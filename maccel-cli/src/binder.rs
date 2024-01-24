use std::io::Write;
use std::{fs::File, path::PathBuf};

use anyhow::Context;

pub fn bind_device(device_id: &str) -> anyhow::Result<()> {
    eprintln!("[INFO] unbinding from hid-generic");
    unbind_device_from_driver("usbhid", device_id)?;
    eprintln!("[INFO] binding to maccel");
    bind_device_to_driver("maccel", device_id)?;
    Ok(())
}

pub fn unbind_device(device_id: &str) -> anyhow::Result<()> {
    eprintln!("[INFO] unbinding from maccel");
    unbind_device_from_driver("maccel", device_id)?;
    eprintln!("[INFO] binding to hid-generic");
    bind_device_to_driver("usbhid", device_id)?;
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
