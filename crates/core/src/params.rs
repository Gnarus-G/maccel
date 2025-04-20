use crate::libmaccel::fixedptc::Fpt;
use paste::paste;

/// Declare an enum for every parameter.
macro_rules! declare_common_params {
    ($($param:tt,)+) => {
        #[cfg_attr(feature = "clap", derive(clap::ValueEnum))]
        #[derive(Debug, Clone, Copy, PartialEq)]
        pub enum Param {
            $($param),+
        }

        paste!(
            #[derive(Debug)]
            pub struct AllParamArgs {
                $( pub [< $param:snake:lower >]: Fpt ),+
            }
        );

        pub const ALL_PARAMS: &[Param] = &[ $(Param::$param),+ ];
    };
}

macro_rules! declare_params {
    ( Common { $($common_param:tt),+$(,)? } , $( $mode:tt { $($param:tt),+$(,)? }, )+) => {
        declare_common_params! {
            $( $common_param, )+
            $( $( $param, )+ )+
        }

        /// Array of all the common parameters for convenience.
        pub const ALL_COMMON_PARAMS: &[Param] = &[ $( Param::$common_param),+ ];


        #[cfg_attr(feature = "clap", derive(clap::ValueEnum))]
        #[derive(Debug, Default, PartialEq, Clone, Copy)]
        #[repr(u8)]
        pub enum AccelMode {
            #[default]
            $( $mode, )+
        }

        pub const ALL_MODES: &[AccelMode] = &[ $( AccelMode::$mode, )+ ];

        paste! {
            /// Define the complete shape (and memory layout) of the argument
            /// of the sensitivity function as it is expected to be in `C`
            #[repr(C)]
            pub struct AccelParams {
                $( pub [< $common_param:snake:lower >] : Fpt, )+
                pub by_mode: AccelParamsByMode,
            }

            /// Represents the tagged union of curve-specific parameters.
            #[repr(C, u8)]
            pub enum AccelParamsByMode {
                $(
                    $mode([< $mode CurveParams >] ),
                )+
            }

            /// Represents the common parameters and their float values.
            /// Use it to bulk set the common parameters.
            #[cfg_attr(feature = "clap", derive(clap::Args))]
            #[derive(Debug, Clone, Copy, PartialEq)]
            pub struct CommonParamArgs {
                $( pub [< $common_param:snake:lower >]: f64 ),+
            }
        }

        paste! {
            $(
                /// Represents curve-specific parameters.
                #[repr(C)]
                pub struct [< $mode CurveParams >] {
                    $( pub [< $param:snake:lower >]: Fpt ),+
                }

                #[doc = "Array of all parameters for the `"  $mode "` mode for convenience." ]
                pub const [< ALL_ $mode:upper _PARAMS >]: &[Param] = &[ $( Param::$param),+ ];

                #[doc = "Represents the parameters for `" $mode "` curve and their float values"]
                /// Use it to bulk set the curve's parameters.
                #[cfg_attr(feature = "clap", derive(clap::Args))]
                #[derive(Debug, Clone, Copy, PartialEq)]
                pub struct [< $mode ParamArgs >] {
                    $( pub [< $param:snake:lower >]: f64 ),+
                }
            )+

            /// Subcommands for the CLI
            #[cfg(feature = "clap")]
            pub mod subcommads {
                #[derive(clap::Subcommand)]
                pub enum SetParamByModesSubcommands {
                    /// Set all the common parameters
                    Common(super::CommonParamArgs),
                    $(
                        #[doc = "Set all the parameters for the " $mode " curve" ]
                        $mode(super::[< $mode ParamArgs >]),
                    )+
                }

                #[derive(clap::Subcommand)]
                pub enum CliSubcommandSetParams {
                    /// Set the value for a single parameter
                    Param { name: crate::params::Param, value: f64 },
                    /// Set the acceleration mode (curve)
                    Mode { mode: crate::params::AccelMode },
                    /// Set the values for all parameters for a curve in order
                    All {
                        #[clap(subcommand)]
                        command: SetParamByModesSubcommands
                    },
                }

                #[derive(clap::Subcommand)]
                pub enum GetParamsByModesSubcommands {
                    /// Get all the common parameters
                    Common,
                    $(
                        #[doc = "Get all the parameters for the " $mode " curve" ]
                        $mode
                     ),+
                }

                #[derive(clap::Subcommand)]
                pub enum CliSubcommandGetParams {
                    /// Get the value for a single parameter
                    Param { name: crate::params::Param },
                    /// Get the current acceleration mode (curve)
                    Mode,
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
        }
    };
}

declare_params!(
    Common {
        SensMult,
        YxRatio,
        InputDpi
    },
    Linear {
        Accel,
        OffsetLinear,
        OutputCap,
    },
    Natural {
        DecayRate,
        OffsetNatural,
        Limit,
    },
    Synchronous {
        Gamma,
        Smooth,
        Motivity,
        SyncSpeed,
    },
);

impl AccelMode {
    pub fn as_title(&self) -> &'static str {
        match self {
            AccelMode::Linear => "Linear Acceleration",
            AccelMode::Natural => "Natural (w/ Gain)",
            AccelMode::Synchronous => "Synchronous",
        }
    }
}

impl AccelMode {
    pub const PARAM_NAME: &'static str = "MODE";

    pub fn ordinal(&self) -> i64 {
        (*self as i8).into()
    }
}

impl Param {
    /// The canonical name of the parameter, as defined by the kernel module,
    /// and exactly what can be read from /sys/module/maccel/parameters
    pub fn name(&self) -> &'static str {
        match self {
            Param::SensMult => "SENS_MULT",
            Param::YxRatio => "YX_RATIO",
            Param::InputDpi => "INPUT_DPI",
            Param::Accel => "ACCEL",
            Param::OffsetLinear => "OFFSET",
            Param::OffsetNatural => "OFFSET",
            Param::OutputCap => "OUTPUT_CAP",
            Param::DecayRate => "DECAY_RATE",
            Param::Limit => "LIMIT",
            Param::Gamma => "GAMMA",
            Param::Smooth => "SMOOTH",
            Param::Motivity => "MOTIVITY",
            Param::SyncSpeed => "SYNC_SPEED",
        }
    }

    pub fn display_name(&self) -> &'static str {
        match self {
            Param::SensMult => "Sens-Multiplier",
            Param::Accel => "Accel",
            Param::InputDpi => "Input DPI",
            Param::OffsetLinear => "Offset",
            Param::OffsetNatural => "Offset",
            Param::OutputCap => "Output-Cap",
            Param::YxRatio => "Y/x Ratio",
            Param::DecayRate => "Decay-Rate",
            Param::Limit => "Limit",
            Param::Gamma => "Gamma",
            Param::Smooth => "Smooth",
            Param::Motivity => "Motivity",
            Param::SyncSpeed => "Sync Speed",
        }
    }
}

pub(crate) fn format_param_value(value: f64) -> String {
    let mut number = format!("{:.5}", value);

    for idx in (1..number.len()).rev() {
        let this_char = &number[idx..idx + 1];
        if this_char != "0" {
            if this_char == "." {
                number.remove(idx);
            }
            break;
        }
        number.remove(idx);
    }

    number
}

#[cfg(test)]
#[test]
fn format_param_value_works() {
    assert_eq!(format_param_value(1.5), "1.5");
    assert_eq!(format_param_value(1.50), "1.5");
    assert_eq!(format_param_value(100.0), "100");
    assert_eq!(format_param_value(0.0600), "0.06");
    assert_eq!(format_param_value(0.055000), "0.055");
}

pub(crate) fn validate_param_value(param_tag: Param, value: f64) -> anyhow::Result<()> {
    match param_tag {
        Param::SensMult => {}
        Param::YxRatio => {}
        Param::InputDpi => {
            if value <= 0.0 {
                anyhow::bail!("Input DPI must be positive");
            }
        }
        Param::Accel => {}
        Param::OutputCap => {}
        Param::OffsetLinear | Param::OffsetNatural => {
            if value < 0.0 {
                anyhow::bail!("offset cannot be less than 0");
            }
        }
        Param::DecayRate => {
            if value <= 0.0 {
                anyhow::bail!("decay rate must be positive");
            }
        }
        Param::Limit => {
            if value < 1.0 {
                anyhow::bail!("limit cannot be less than 1");
            }
        }
        Param::Gamma => {
            if value <= 0.0 {
                anyhow::bail!("Gamma must be positive");
            }
        }
        Param::Smooth => {
            if !(0.0..=1.0).contains(&value) {
                anyhow::bail!("Smooth must be between 0 and 1");
            }
        }
        Param::Motivity => {
            if value <= 1.0 {
                anyhow::bail!("Motivity must be greater than 1");
            }
        }
        Param::SyncSpeed => {
            if value <= 0.0 {
                anyhow::bail!("'Synchronous speed' must be positive");
            }
        }
    }

    Ok(())
}
