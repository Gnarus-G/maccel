use anyhow::Context;
use clap::{CommandFactory, Parser};
use maccel_core::{
    fixedptc::Fpt,
    persist::{ParamStore, SysFsStore},
    subcommads::*,
    AccelMode, Param, ALL_COMMON_PARAMS, ALL_LINEAR_PARAMS, ALL_NATURAL_PARAMS,
    ALL_SYNCHRONOUS_PARAMS,
};
use maccel_tui::run_tui;

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

    #[cfg(debug_assertions)]
    Debug {
        #[clap(subcommand)]
        command: DebugCommands,
    },
}

#[cfg(debug_assertions)]
#[derive(Debug, clap::Subcommand)]
enum DebugCommands {
    Print { nums: Vec<f64> },
}

fn main() -> anyhow::Result<()> {
    let args = Cli::parse();

    // tracing_subscriber::fmt()
    //     .with_max_level(Level::DEBUG)
    //     .with_writer(File::create("./maccel.log")?)
    //     .init();

    let mut param_store = SysFsStore;

    match args.command.unwrap_or_default() {
        CLiCommands::Set { command } => match command {
            CliSubcommandSetParams::Param { name, value } => param_store.set(name, value)?,
            CliSubcommandSetParams::All { command } => match command {
                SetParamByModesSubcommands::Linear(param_args) => {
                    param_store.set_all_linear(param_args)?
                }
                SetParamByModesSubcommands::Natural(param_args) => {
                    param_store.set_all_natural(param_args)?
                }
                SetParamByModesSubcommands::Common(param_args) => {
                    param_store.set_all_common(param_args)?
                }
                SetParamByModesSubcommands::Synchronous(param_args) => {
                    param_store.set_all_synchronous(param_args)?
                }
                SetParamByModesSubcommands::NoAccel(param_args) => {
                    eprintln!(
                        "NOTE: There are no parameters specific here except for the common ones."
                    );
                    eprintln!();
                    param_store.set_all_no_accel(param_args)?
                }
            },
            CliSubcommandSetParams::Mode { mode } => SysFsStore.set_current_accel_mode(mode)?,
        },
        CLiCommands::Get { command } => match command {
            CliSubcommandGetParams::Param { name } => {
                let value = param_store.get(name)?;
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
                GetParamsByModesSubcommands::Common => {
                    print_all_params(ALL_COMMON_PARAMS.iter(), oneline, quiet)?;
                }
                GetParamsByModesSubcommands::Synchronous => {
                    print_all_params(ALL_SYNCHRONOUS_PARAMS.iter(), oneline, quiet)?;
                }
                GetParamsByModesSubcommands::NoAccel => {
                    eprintln!(
                        "NOTE: There are no parameters specific here except for the common ones."
                    );
                    eprintln!();
                    print_all_params(ALL_COMMON_PARAMS.iter(), oneline, quiet)?;
                }
            },
            CliSubcommandGetParams::Mode => {
                let mode = SysFsStore.get_current_accel_mode()?;
                println!("{}\n", mode.as_title());
                match mode {
                    AccelMode::Linear => {
                        print_all_params(ALL_LINEAR_PARAMS.iter(), false, false)?;
                    }
                    AccelMode::Natural => {
                        print_all_params(ALL_NATURAL_PARAMS.iter(), false, false)?;
                    }
                    AccelMode::Synchronous => {
                        print_all_params(ALL_SYNCHRONOUS_PARAMS.iter(), false, false)?;
                    }
                    AccelMode::NoAccel => {
                        eprintln!(
                            "NOTE: There are no parameters specific here except for the common ones."
                        );
                        eprintln!();
                        print_all_params(ALL_COMMON_PARAMS.iter(), false, false)?;
                    }
                }
            }
        },
        CLiCommands::Tui => run_tui()?,
        CLiCommands::Completion { shell } => {
            clap_complete::generate(shell, &mut Cli::command(), "maccel", &mut std::io::stdout())
        }
        #[cfg(debug_assertions)]
        CLiCommands::Debug { command } => match command {
            DebugCommands::Print { nums } => {
                if nums.is_empty() {
                    for n in std::io::stdin()
                        .lines()
                        .map_while(Result::ok)
                        .flat_map(|s| s.parse::<f64>())
                    {
                        println!("{}", Fpt::from(n).0);
                    }
                }

                for n in nums {
                    println!("{}", Fpt::from(n).0);
                }
            }
        },
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
        .map(|&p| {
            SysFsStore.get(p).and_then(|_p: Fpt| {
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
