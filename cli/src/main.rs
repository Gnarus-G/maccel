mod inputspeed;
mod libmaccel;
mod params;
mod tui;

use std::fs::File;

use anyhow::Context;
use clap::{CommandFactory, Parser};
use libmaccel::fixedptc::Fixedpt;
use params::{
    linear::ALL_LINEAR_PARAMS, natural::ALL_NATURAL_PARAMS, set_all_linear, set_all_natural,
    set_parameter, CliSubcommandGetParams, CliSubcommandSetParams, GetParamsByModesSubcommands,
    Param, SetParamByModesSubcommands,
};
use tracing::Level;
use tui::{context::AccelMode, run_tui};

#[derive(Parser)]
#[clap(author, about, version)]
/// CLI to control the parameters for the maccel driver.
struct Cli {
    #[clap(subcommand)]
    command: Option<CLiCommands>,
}

#[derive(clap::Subcommand, Default)]
enum CLiCommands {
    /// Open the Terminal UI to manage the parameters
    /// and see a graph of the sensitivity
    #[default]
    Tui,
    /// Set the value for a parameter of the maccel driver
    Set {
        #[clap(subcommand)]
        command: CliSubcommandSetParams,
    },
    /// Get the values for parameters of the maccel driver
    Get {
        #[clap(subcommand)]
        command: CliSubcommandGetParams,
    },
    /// Generate a completions file for a specified shell
    Completion {
        // The shell for which to generate completions
        shell: clap_complete::Shell,
    },
}

fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    tracing_subscriber::fmt()
        .with_max_level(Level::DEBUG)
        .with_writer(File::create("./maccel.log")?)
        .init();

    match args.command.unwrap_or_default() {
        CLiCommands::Set { command } => match command {
            CliSubcommandSetParams::Param { name, value } => name.set(value)?,
            CliSubcommandSetParams::All { command } => match command {
                SetParamByModesSubcommands::Linear(param_args) => {
                    set_parameter(AccelMode::PARAM_NAME, AccelMode::Linear.ordinal())
                        .expect("Failed to set kernel param to change modes");
                    set_all_linear(param_args)?
                }
                SetParamByModesSubcommands::Natural(param_args) => {
                    set_parameter(AccelMode::PARAM_NAME, AccelMode::Natural.ordinal())
                        .expect("Failed to set kernel param to change modes");
                    set_all_natural(param_args)?
                }
            },
        },
        CLiCommands::Get { command } => match command {
            CliSubcommandGetParams::Param { name } => {
                let value = name.get()?;
                let string_value: &str = (&value).try_into()?;
                println!("{}", string_value);
            }
            CliSubcommandGetParams::All {
                oneline,
                quiet,
                command,
            } => match command {
                GetParamsByModesSubcommands::Linear => {
                    print_all_params(ALL_LINEAR_PARAMS.iter(), oneline, quiet)?;
                }
                GetParamsByModesSubcommands::Natural => {
                    print_all_params(ALL_NATURAL_PARAMS.iter(), oneline, quiet)?;
                }
            },
        },
        CLiCommands::Tui => run_tui()?,
        CLiCommands::Completion { shell } => {
            clap_complete::generate(shell, &mut Cli::command(), "maccel", &mut std::io::stdout())
        }
    }

    Ok(())
}

fn print_all_params<'p>(
    params: impl Iterator<Item = &'p Param>,
    oneline: bool,
    quiet: bool,
) -> anyhow::Result<()> {
    let delimiter = if oneline { " " } else { "\n" };

    let params = params
        .map(|p| {
            p.get().and_then(|_p: Fixedpt| {
                let value: &str = (&_p).try_into()?;
                Ok((p.display_name(), value.to_string()))
            })
        })
        .collect::<anyhow::Result<Vec<_>>>()
        .context("failed to get all parameters")?;

    for (name, value) in params {
        if !quiet {
            print!("{}:  \t", name);
        }
        print!("{}", value);

        print!("{}", delimiter);
    }

    Ok(())
}
