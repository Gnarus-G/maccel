use std::{
    fs::File,
    io::Read,
    path::{Path, PathBuf},
};

mod fixedptc_proxy;

use anyhow::{anyhow, Context};
use clap::{Parser, ValueEnum};
use fixedptc_proxy::{fixedpt, fixedpt_as_str};

#[derive(Parser)]
struct Cli {
    #[clap(subcommand)]
    command: ParamsCommand,
}

#[derive(clap::Subcommand)]
enum ParamsCommand {
    Set { name: Param, value: f32 },
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
