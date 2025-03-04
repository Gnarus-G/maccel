use crate::libmaccel::fixedptc::Fixedpt;
use anyhow::{anyhow, Context};
use clap::ValueEnum;
use paste::paste;
use std::{
    fmt::Display,
    fs::File,
    io::Read,
    path::{Path, PathBuf},
    str::FromStr,
};

const SYS_MODULE_PATH: &str = "/sys/module/maccel";

macro_rules! declare_all_params {
    ($($name:tt,)+) => {
        #[derive(Debug, Clone, Copy, ValueEnum, PartialEq)]
        pub enum Param {
            $($name),+
        }

        paste!(
            #[derive(Debug)]
            pub struct AllParamArgs {
                $( pub [< $name:snake:lower >]: Fixedpt ),+
            }
        );

        pub const ALL_PARAMS: &[Param] = &[ $(Param::$name),+ ];
    };
}

macro_rules! declare_params {
    ( $( $mode:tt { $($name:tt,)+ }, )+) => {
        paste! {
            $(
                pub mod [< $mode:lower >]{
                    use clap::{Args};

                    pub const [< ALL_ $mode:upper _PARAMS >]: &[super::Param] = &[ $( super::Param::$name),+ ];

                    #[derive(Debug, Clone, Copy, Args, PartialEq)]
                    pub struct ParamArgs {
                        $( pub [< $name:snake:lower >]: f64 ),+
                    }
                }
            )+

            #[derive(clap::Subcommand)]
            pub enum SetParamByModesSubcommands {
                $(
                    #[doc = "Set all the parameters for the " $mode " curve" ]
                    $mode([< $mode:lower >]::ParamArgs),
                )+
            }

            #[derive(clap::Subcommand)]
            pub enum CliSubcommandSetParams {
                /// Set the value for a single parameter
                Param { name: super::Param, value: f64 },
                /// Set the values for all parameters for a curve in order
                All {
                    #[clap(subcommand)]
                    command: SetParamByModesSubcommands
                },
            }

            #[derive(clap::Subcommand)]
            pub enum GetParamsByModesSubcommands {

                $(
                    #[doc = "Get all the parameters for the " $mode " curve" ]
                    $mode
                 ),+
            }

            #[derive(clap::Subcommand)]
            pub enum CliSubcommandGetParams {
                /// Get the value for a single parameter
                Param { name: super::Param },
                /// Get the values for all parameters for a curve in order
                All {
                    /// Print the values in one line, separated by a space
                    #[arg(long)]
                    oneline: bool,
                    #[arg(short, long)]
                    /// Print only the values
                    quiet: bool,

                    #[clap(subcommand)]
                    command: GetParamsByModesSubcommands
                },
            }
        }
    };
}

declare_all_params! {
    SensMult,
    YxRatio,
    Accel,
    Offset,
    OutputCap,
    DecayRate,
    Limit,
}

declare_params!(
    Linear {
        SensMult,
        YxRatio,
        Accel,
        Offset,
        OutputCap,
    },
    Natural {
        SensMult,
        YxRatio,
        DecayRate,
        Offset,
        Limit,
    },
);

impl Param {
    pub fn set(&self, value: f64) -> anyhow::Result<()> {
        let value: Fixedpt = value.into();
        let value = value.0;
        set_parameter(self.name(), value)
    }

    pub fn get(&self) -> anyhow::Result<Fixedpt> {
        let value = get_paramater(self.name())?;
        let value = Fixedpt::from_str(&value).context(format!(
            "couldn't interpret the parameter's value {}",
            value
        ))?;
        Ok(value)
    }

    /// The canonical name of the parameter, as defined by the kernel module,
    /// and exactly what can be read from /sys/module/maccel/parameters
    pub fn name(&self) -> &'static str {
        match self {
            Param::SensMult => "SENS_MULT",
            Param::YxRatio => "YX_RATIO",
            Param::Accel => "ACCEL",
            Param::Offset => "OFFSET",
            Param::OutputCap => "OUTPUT_CAP",
            Param::DecayRate => "DECAY_RATE",
            Param::Limit => "LIMIT",
        }
    }

    pub fn display_name(&self) -> &'static str {
        match self {
            Param::SensMult => "Sens-Multiplier",
            Param::Accel => "Accel",
            Param::Offset => "Offset",
            Param::OutputCap => "Output-Cap",
            Param::YxRatio => "Y/x Ratio",
            Param::DecayRate => "Decay-Rate",
            Param::Limit => "Limit",
        }
    }
}

fn parameter_path(name: &'static str) -> anyhow::Result<PathBuf> {
    let params_path = Path::new(SYS_MODULE_PATH).join("parameters").join(name);

    if !params_path.exists() {
        return Err(anyhow!("no such path: {}", params_path.display()))
            .context(anyhow!("no such parameter {:?}", name));
    }

    Ok(params_path)
}

fn save_parameter_reset_script(name: &'static str, value: i64) -> anyhow::Result<()> {
    let script_dir = "/var/opt/maccel/resets";
    if !Path::new(script_dir).exists() {
        std::fs::create_dir_all(script_dir).context(format!("failed create directory: {}", script_dir))
                .context("failed to create the directory where we'd save the parameter value to apply on reboot")?;
    }
    std::fs::write(
        format!("{script_dir}/set_last_{}_value.sh", name),
        format!("echo {} > /sys/module/maccel/parameters/{};", value, name),
    )
    .context("failed to write reset script")?;
    Ok(())
}

pub fn get_paramater(name: &'static str) -> anyhow::Result<String> {
    let path = parameter_path(name)?;
    let mut file = File::open(&path)
        .context(anyhow!(
            "failed to open the parameter's file for reading: {}",
            path.display()
        ))
        .context("this shouldn't happen unless the maccel kernel module is not installed.")?;

    let mut buf = String::new();

    file.read_to_string(&mut buf)
        .context("failed to read the parameter's value")?;

    Ok(buf)
}

pub fn set_parameter(name: &'static str, value: i64) -> anyhow::Result<()> {
    let path = parameter_path(name)?;

    std::fs::write(&path, format!("{}", value)).context(anyhow!(
        "failed to write to parameter file: {}",
        path.display()
    ))?;

    save_parameter_reset_script(name, value)?;

    Ok(())
}

pub fn set_all_linear(args: linear::ParamArgs) -> anyhow::Result<()> {
    let linear::ParamArgs {
        sens_mult,
        yx_ratio,
        accel,
        offset,
        output_cap,
    } = args;

    Param::SensMult.set(sens_mult)?;
    Param::YxRatio.set(yx_ratio)?;
    Param::Accel.set(accel)?;
    Param::Offset.set(offset)?;
    Param::OutputCap.set(output_cap)?;

    Ok(())
}

pub fn set_all_natural(args: natural::ParamArgs) -> anyhow::Result<()> {
    let natural::ParamArgs {
        sens_mult,
        yx_ratio,
        decay_rate,
        offset,
        limit,
    } = args;

    Param::SensMult.set(sens_mult)?;
    Param::YxRatio.set(yx_ratio)?;
    Param::DecayRate.set(decay_rate)?;
    Param::Offset.set(offset)?;
    Param::Limit.set(limit)?;

    Ok(())
}

impl Display for Fixedpt {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&format_param_value(f64::from(*self)))
    }
}

fn format_param_value(value: f64) -> String {
    let mut number = format!("{:.5}", value);

    for idx in (1..number.len()).rev() {
        let this_char = &number[idx..idx + 1];
        if this_char != "0" && this_char != "." {
            break;
        }
        number.remove(idx);
    }

    number
}

// #[cfg(test)]
// mod tests {
//     use std::path::PathBuf;
//
//     use crate::libmaccel::fixedptc::Fixedpt;
//
//     use super::Param;
//
//     fn test(param: Param) {
//         param.set(3.0).unwrap();
//         assert_eq!(param.get().unwrap(), Fixedpt(12884901888));
//
//         param.set(4.0).unwrap();
//         assert_eq!(param.get().unwrap(), Fixedpt(17179869184));
//
//         param.set(4.5).unwrap();
//         assert_eq!(param.get().unwrap(), (4.5).into());
//
//         let save_script = PathBuf::from(format!(
//             "/var/opt/maccel/resets/set_last_{}_value.sh",
//             param.name()
//         ));
//
//         assert!(save_script.exists());
//     }
//
//     #[test]
//     fn getters_setters_work() {
//         use Param::*;
//
//         test(SensMult);
//         test(Accel);
//         test(Offset);
//         test(OutputCap);
//     }
// }
