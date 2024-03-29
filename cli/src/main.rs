use std::{fs::File, io::Read};

mod binder;
mod libmaccel;
mod params;
mod tui;

use anyhow::{anyhow, Context};
use binder::{bind_device, disabling_udev_rules, unbind_device};
use clap::{builder::OsStr, CommandFactory, Parser};
use glob::glob;
use params::Param;
use tui::run_tui;

#[derive(Parser)]
#[clap(author, about, version)]
/// CLI to control the parameters for the maccel driver, and manage mice bindings
struct Cli {
    #[clap(subcommand)]
    command: Option<ParamsCommand>,
}

#[derive(clap::Subcommand, Default)]
enum ParamsCommand {
    /// Open the Terminal UI to manage the parameters
    /// and see a graph of the sensitivity
    #[default]
    Tui,
    /// Attach a device to the maccel driver
    Bind { device_id: String },
    /// Attach all detected mice to the maccel driver
    Bindall,
    /// Detach a device from the maccel driver,
    /// reattach to the generic usbhid driver
    Unbind { device_id: String },
    /// Detach all detected mice from the maccel driver
    /// reattach them to the generic usbhid driver
    Unbindall,
    /// Set the value for a parameter of the maccel driver
    Set { name: Param, value: f32 },
    /// Get the value for a parameter of the maccel driver
    Get { name: Param },
    /// Generate a completions file for a specified shell
    Completion {
        // The shell for which to generate completions
        shell: clap_complete::Shell,
    },
}

fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    match args.command.unwrap_or_default() {
        ParamsCommand::Set { name, value } => {
            name.set(value)?;
        }
        ParamsCommand::Get { name } => {
            let value = name.get()?;
            let string_value: &str = (&value).try_into()?;
            println!("{}", string_value);
        }
        ParamsCommand::Bind { device_id } => {
            if let Ok(name) = get_device_name_by_id(&device_id) {
                eprintln!("[INFO] device to bind: {name}");
            };
            bind_device(&device_id)?;
        }
        ParamsCommand::Bindall => {
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
        }
        ParamsCommand::Unbind { device_id } => {
            if let Ok(name) = get_device_name_by_id(&device_id) {
                eprintln!("[INFO] device to unbind: {name}");
            };

            disabling_udev_rules(|| unbind_device(&device_id))?;
        }
        ParamsCommand::Unbindall => {
            eprintln!("[INFO] looking for all devices bound to maccel");

            disabling_udev_rules(|| {
                let dirs = std::fs::read_dir("/sys/bus/usb/drivers/maccel")?;

                for d in dirs.flatten() {
                    let path = d.path();
                    let basename = path.file_name();
                    if path.is_dir() && basename != Some(&OsStr::from("module")) {
                        let device_id = basename.unwrap().to_str().expect(
                        "basename of the /sys/*/drivers/maccel device_id paths should be strings",
                    );
                        eprint_device_name(device_id);
                        unbind_device(device_id)?;
                    }
                }

                Ok(())
            })?;
        }
        ParamsCommand::Tui => run_tui()?,
        ParamsCommand::Completion { shell } => {
            clap_complete::generate(shell, &mut Cli::command(), "maccel", &mut std::io::stdout())
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
