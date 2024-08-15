mod inputspeed;
mod libmaccel;
mod params;
mod tui;

use std::{os::unix::process::CommandExt, process};

use anyhow::{anyhow, Context};
use clap::{CommandFactory, Parser};
use params::Param;
use tui::run_tui;

#[derive(Parser)]
#[clap(author, about, version)]
/// CLI to control the parameters for the maccel driver.
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
    /// Set the value for a parameter of the maccel driver
    Set {
        #[clap(subcommand)]
        command: SetSubcommands,
    },
    /// Get the values for parameters of the maccel driver
    Get {
        #[clap(subcommand)]
        command: GetSubcommands,
    },
    /// Generate a completions file for a specified shell
    Completion {
        // The shell for which to generate completions
        shell: clap_complete::Shell,
    },
    /// Manage binding for the usbmouse driver.
    Driver { args: Vec<String> },
}

#[derive(clap::Subcommand)]
enum SetSubcommands {
    /// Set the value for a single parameter
    Param { name: Param, value: f32 },
    /// Set the values for all parameters in order
    All {
        sens_mult: f32,
        accel: f32,
        offset: f32,
        output_cap: f32,
    },
}

#[derive(clap::Subcommand)]
enum GetSubcommands {
    /// Get the value for a single parameter
    Param { name: Param },
    /// Get the values for all parameters in order
    All {
        /// Print the values in one line, separated by a space
        #[arg(long)]
        oneline: bool,
        #[arg(short, long)]
        /// Print only the values
        quiet: bool,
    },
}

fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    match args.command.unwrap_or_default() {
        ParamsCommand::Set { command } => match command {
            SetSubcommands::All {
                sens_mult,
                accel,
                offset,
                output_cap,
            } => {
                Param::SensMult.set(sens_mult)?;
                Param::Accel.set(accel)?;
                Param::Offset.set(offset)?;
                Param::OutputCap.set(output_cap)?;
            }
            SetSubcommands::Param { name, value } => name.set(value)?,
        },
        ParamsCommand::Get { command } => match command {
            GetSubcommands::All { oneline, quiet } => {
                let delimiter = if oneline { " " } else { "\n" };
                let params = [
                    Param::SensMult,
                    Param::Accel,
                    Param::Offset,
                    Param::OutputCap,
                ]
                .map(|p| {
                    p.get().and_then(|_p| {
                        let value: &str = (&_p).try_into()?;
                        Ok((p.display_name(), value.to_string()))
                    })
                })
                .into_iter()
                .collect::<anyhow::Result<Vec<_>>>()
                .context("failed to get all parameters")?;

                for (name, value) in params {
                    if !quiet {
                        print!("{}:  \t", name);
                    }
                    print!("{}", value);

                    print!("{}", delimiter);
                }
            }
            GetSubcommands::Param { name } => {
                let value = name.get()?;
                let string_value: &str = (&value).try_into()?;
                println!("{}", string_value);
            }
        },
        ParamsCommand::Tui => run_tui()?,
        ParamsCommand::Completion { shell } => {
            clap_complete::generate(shell, &mut Cli::command(), "maccel", &mut std::io::stdout())
        }
        ParamsCommand::Driver { args } => {
            let err = process::Command::new("maccel-driver-binder")
                .args(args)
                .exec();
            return Err(anyhow!("{:#}", err)).context("failed to run the driver binder cli");
        }
    }

    Ok(())
}
