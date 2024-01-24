use std::{
    fs::File,
    io::Read,
    path::{Path, PathBuf},
};

mod binder;
mod fixedptc_proxy;

use anyhow::{anyhow, Context};
use binder::{bind_device, unbind_device};
use clap::{builder::OsStr, Parser, ValueEnum};
use fixedptc_proxy::{fixedpt, fixedpt_as_str};
use glob::glob;

#[derive(Parser)]
#[clap(author, about, version)]
/// CLI to control the paramters for the maccel driver, and manage mice bindings
struct Cli {
    #[clap(subcommand)]
    command: ParamsCommand,
}

#[derive(clap::Subcommand)]
enum ParamsCommand {
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
}

#[derive(Debug, Clone, ValueEnum)]
enum Param {
    Accel,
    Offset,
    OutputCap,
}

fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    match args.command {
        ParamsCommand::Set { name, value } => {
            let param_path = get_param_path(name)?;
            let value = fixedpt(value);

            let mut file = File::create(param_path)
                .context("failed to open the parameter's file for writing")?;

            use std::io::Write;

            write!(file, "{}", value.0).context("failed to write to parameter file")?;
        }
        ParamsCommand::Get { name } => {
            let param_path = get_param_path(name)?;

            let mut file = File::open(param_path)
                .context("failed to open the parameter's file for reading")?;

            let mut buf = String::new();

            file.read_to_string(&mut buf)
                .context("failed to read the parameter's value")?;

            let value = buf
                .trim()
                .parse()
                .context(format!("couldn't interpert the parameter's value {}", buf))?;

            println!("{}", fixedpt_as_str(&value)?);
        }
        ParamsCommand::Bind { device_id } => {
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
                        eprintln!("[INFO] found device to bind, id: {}", device_id);
                        bind_device(device_id)?;
                        eprintln!()
                    }
                }
            }
        }
        ParamsCommand::Unbind { device_id } => {
            unbind_device(&device_id)?;
        }
        ParamsCommand::Unbindall => {
            eprintln!("[INFO] looking for all devices bound to maccel");

            let dirs = std::fs::read_dir("/sys/bus/usb/drivers/maccel")?;

            for d in dirs.flatten() {
                let path = d.path();
                let basename = path.file_name();
                if path.is_dir() && basename != Some(&OsStr::from("module")) {
                    let device_id = basename.unwrap().to_str().expect(
                        "basename of the /sys/*/drivers/maccel device_id paths should be strings",
                    );
                    eprintln!("[INFO] found device to unbind, id: {}", device_id);
                    unbind_device(device_id)?;
                    eprintln!()
                }
            }
        }
    }

    Ok(())
}

const SYS_MODULE_PATH: &str = "/sys/module/maccel";

fn get_param_path(name: Param) -> anyhow::Result<PathBuf> {
    let param_name = match name {
        Param::Accel => "ACCEL",
        Param::Offset => "OFFSET",
        Param::OutputCap => "OUTPUT_CAP",
    };

    let params_path = Path::new(SYS_MODULE_PATH)
        .join("parameters")
        .join(param_name);

    if !params_path.exists() {
        return Err(anyhow!("no such parameter {:?}", name));
    }

    return Ok(params_path);
}
