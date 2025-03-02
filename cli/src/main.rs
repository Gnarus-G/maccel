mod inputspeed;
mod libmaccel;
mod params;
mod tui;

use anyhow::Context;
use clap::{CommandFactory, Parser};
use params::{Param, ALL_PARAMS};
use tui::run_tui;

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
        command: SetParamsSubcommands,
    },
    /// Get the values for parameters of the maccel driver
    Get {
        #[clap(subcommand)]
        command: GetParamsSubcommands,
    },
    /// Generate a completions file for a specified shell
    Completion {
        // The shell for which to generate completions
        shell: clap_complete::Shell,
    },
}

#[derive(clap::Subcommand)]
enum SetParamsSubcommands {
    /// Set the value for a single parameter
    Param { name: Param, value: f64 },
    /// Set the values for all parameters in order
    All {
        sens_mult: f64,
        yx_ratio: f64,
        accel: f64,
        offset: f64,
        output_cap: f64,
    },
}

#[derive(clap::Subcommand)]
enum GetParamsSubcommands {
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

    // tracing_subscriber::fmt()
    //     .with_max_level(Level::DEBUG)
    //     .with_writer(File::create("./maccel.log")?)
    //     .init();

    match args.command.unwrap_or_default() {
        CLiCommands::Set { command } => match command {
            SetParamsSubcommands::All {
                sens_mult,
                yx_ratio,
                accel,
                offset,
                output_cap,
            } => {
                Param::SensMult.set(sens_mult)?;
                Param::YxRatio.set(yx_ratio)?;
                Param::Accel.set(accel)?;
                Param::Offset.set(offset)?;
                Param::OutputCap.set(output_cap)?;
            }
            SetParamsSubcommands::Param { name, value } => {
                name.set(value)?;
            }
        },
        CLiCommands::Get { command } => match command {
            GetParamsSubcommands::All { oneline, quiet } => {
                let delimiter = if oneline { " " } else { "\n" };
                let params = ALL_PARAMS
                    .iter()
                    .map(|p| {
                        p.get().and_then(|_p| {
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
            }
            GetParamsSubcommands::Param { name } => {
                let value = name.get()?;
                let string_value: &str = (&value).try_into()?;
                println!("{}", string_value);
            }
        },
        CLiCommands::Tui => run_tui()?,
        CLiCommands::Completion { shell } => {
            clap_complete::generate(shell, &mut Cli::command(), "maccel", &mut std::io::stdout())
        }
    }

    Ok(())
}
