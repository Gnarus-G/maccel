use std::{fs::File, io::Read, os::unix::fs::PermissionsExt};
mod binder;

use anyhow::{anyhow, Context};
use binder::{bind_device, disabling_udev_rules, unbind_device};
use clap::{builder::OsStr, Parser};
use glob::glob;

#[derive(Parser)]
#[clap(author, about, version)]
/// CLI for the maccel driver to manage mice bindings
struct Cli {
    #[clap(subcommand)]
    command: ParamsCommand,
}

#[derive(clap::Subcommand)]
enum ParamsCommand {
    /// Attach a device to the maccel driver
    Bind { device_id: String },
    /// Attach all detected mice to the maccel driver
    Bindall {
        /// Install the udev rules to rebind automatically after reboot.
        #[arg(long, short)]
        install: bool,
    },
    /// Detach a device from the maccel driver,
    /// reattach to the generic usbhid driver
    Unbind { device_id: String },
    /// Detach all detected mice from the maccel driver
    /// reattach them to the generic usbhid driver
    Unbindall {
        /// Uninstall the udev rules to not rebind automatically after reboot.
        #[arg(long, short)]
        uninstall: bool,
    },
}

fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    match args.command {
        ParamsCommand::Bind { device_id } => {
            if let Ok(name) = get_device_name_by_id(&device_id) {
                eprintln!("[INFO] device to bind: {name}");
            };
            bind_device(&device_id)?;
        }
        ParamsCommand::Bindall { install } => {
            eprintln!("[INFO] looking for all mice bound to usbhid: the generic hid driver");

            let paths = glob("/sys/bus/usb/drivers/usbhid/[0-9]*")?;

            for path in paths.flatten() {
                let basename = path.file_name();
                if path.is_dir() && basename != Some(&OsStr::from("module")) {
                    let protocol =
                        File::open(path.join("bInterfaceProtocol")).and_then(|mut f| {
                            let mut buf = String::new();
                            return f.read_to_string(&mut buf).map(|_| buf);
                        })?;

                    let subclass =
                        File::open(path.join("bInterfaceSubClass")).and_then(|mut f| {
                            let mut buf = String::new();
                            return f.read_to_string(&mut buf).map(|_| buf);
                        })?;

                    // checking for the observed invariant for a usb mouse device
                    if protocol.trim() == "02" && subclass.trim() == "01" {
                        let device_id = basename.unwrap().to_str().expect(
                        "basename of the /sys/*/drivers/usbhid device_id paths should be strings",
                    );
                        eprint_device_name(device_id);
                        bind_device(device_id)?;
                    }
                }
            }

            if install {
                setup_bind_udev_rules().context("failed to install udev rules")?;
            }
        }
        ParamsCommand::Unbind { device_id } => {
            if let Ok(name) = get_device_name_by_id(&device_id) {
                eprintln!("[INFO] device to unbind: {name}");
            };

            disabling_udev_rules(|| unbind_device(&device_id))?;
        }
        ParamsCommand::Unbindall { uninstall } => {
            eprintln!("[INFO] looking for all devices bound to maccel");

            disabling_udev_rules(|| {
                let dirs = std::fs::read_dir("/sys/bus/usb/drivers/maccel_usbmouse")?;

                for d in dirs.flatten() {
                    let path = d.path();
                    let basename = path.file_name();
                    if path.is_dir() && basename != Some(&OsStr::from("module")) {
                        let device_id = basename.unwrap().to_str().expect(
                        "basename of the /sys/**/drivers/maccel_usbmouse device_id paths should be strings",
                    );
                        eprint_device_name(device_id);
                        unbind_device(device_id)?;
                    }
                }

                Ok(())
            })?;

            if uninstall {
                cleanup_bind_udev_rules().context("failed to uninstall udev rules")?;
            }
        }
    }

    Ok(())
}

fn eprint_device_name(device_id: &str) {
    match get_device_name_by_id(device_id) {
        Ok(name) => {
            eprintln!(
                "[INFO] found device to unbind, id: {}, name: {}",
                device_id, name
            );
        }
        Err(err) => {
            eprintln!("[INFO] found device to unbind, id: {}", device_id);
            eprintln!(
                "[ERROR] failed to get name of device, id: {} -> {:#}",
                device_id, err
            );
        }
    }
}

fn get_device_name_by_id(id: &str) -> anyhow::Result<String> {
    let pattern = &format!("/sys/bus/usb/devices/{id}/input/*/name");
    let pattern2 = &format!("/sys/bus/usb/devices/{id}/*/input/*/name");
    // eprintln!("[DEBUG] looking for device names with pattern: {}", pattern);
    let paths = glob(pattern).expect("invalid glob pattern, shouldn't happen");
    // eprintln!(
    //     "[DEBUG] looking for device names with pattern: {}",
    //     pattern2
    // );
    let paths2 = glob(pattern2).expect("invalid glob pattern, shouldn't happen");

    let name = paths
        .chain(paths2)
        .next()
        .ok_or(anyhow!(
            "no paths found by either of the patterns: '{}' or '{}'",
            pattern,
            pattern2
        ))
        .and_then(|p| p.context("bad path"))
        .and_then(|p| {
            // eprintln!("[DEBUG] found path to read device name: {}", p.display());
            std::fs::read_to_string(p).context("failed to read name of device from path")
        })?;

    Ok(name.trim().to_string())
}

const UDEV_LIB_PATH: &str = "/usr/lib/udev";

const BIND_RULE: &str = include_str!("../../../udev_rules/99-maccel-bind.rules");
const BIND_RULE_SCRIPT: &str = include_str!("../../../udev_rules/maccel_bind");

macro_rules! relevant_paths {
    () => {
        (
            format!("{UDEV_LIB_PATH}/maccel_bind"),
            format!("{UDEV_LIB_PATH}/rules.d/99-maccel-bind.rules"),
        )
    };
}

fn setup_bind_udev_rules() -> anyhow::Result<()> {
    use std::fs;

    let (bind_script_path, bind_rule_path) = relevant_paths!();

    fs::write(&bind_script_path, BIND_RULE_SCRIPT)?;
    fs::write(bind_rule_path, BIND_RULE)?;

    {
        // Allow execution of bind script
        let mut bind_script_perms = fs::metadata(&bind_script_path)?.permissions();
        bind_script_perms.set_mode(0o755);
        fs::set_permissions(&bind_script_path, bind_script_perms)?;
    }

    Ok(())
}

fn cleanup_bind_udev_rules() -> anyhow::Result<()> {
    use std::fs;

    let (bind_script_path, bind_rule_path) = relevant_paths!();

    fs::remove_file(bind_script_path)?;
    fs::remove_file(bind_rule_path)?;

    Ok(())
}
