use std::{
    fmt::{Debug, Display},
    io::Read,
    path::{Path, PathBuf},
    str::FromStr,
};

use anyhow::{Context, anyhow};

use crate::{
    fixedptc::Fpt,
    params::{
        ALL_MODES, AccelMode, CommonParamArgs, LinearParamArgs, NaturalParamArgs, Param,
        SynchronousParamArgs, format_param_value, validate_param_value,
    },
};

pub trait ParamStore: Debug {
    fn set(&mut self, param: Param, value: f64) -> anyhow::Result<()>;
    fn get(&self, param: Param) -> anyhow::Result<Fpt>;

    fn set_current_accel_mode(&mut self, mode: AccelMode) -> anyhow::Result<()>;
    fn get_current_accel_mode(&self) -> anyhow::Result<AccelMode>;
}

const SYS_MODULE_PATH: &str = "/sys/module/maccel";

#[derive(Debug)]
pub struct SysFsStore;

impl ParamStore for SysFsStore {
    fn set(&mut self, param: Param, value: f64) -> anyhow::Result<()> {
        validate_param_value(param, value)?;

        let value: Fpt = value.into();
        let value = value.0;
        set_parameter(param.name(), value)
    }
    fn get(&self, param: Param) -> anyhow::Result<Fpt> {
        let value = get_paramater(param.name())?;
        let value = Fpt::from_str(&value).context(format!(
            "couldn't interpret the parameter's value {}",
            value
        ))?;
        Ok(value)
    }

    fn set_current_accel_mode(&mut self, mode: AccelMode) -> anyhow::Result<()> {
        set_parameter(AccelMode::PARAM_NAME, mode.ordinal())
            .context("Failed to set kernel param to change modes")
    }
    fn get_current_accel_mode(&self) -> anyhow::Result<AccelMode> {
        get_paramater(AccelMode::PARAM_NAME)
            .map(|mode_tag| -> anyhow::Result<AccelMode> {
                let id: u8 = mode_tag
                    .parse()
                    .context("Failed to parse an id for mode parameter")?;
                let idx = id as usize % ALL_MODES.len();
                Ok(ALL_MODES[idx])
            })?
            .context("Failed to read a kernel parameter to get the acceleration mode desired")
    }
}

impl SysFsStore {
    pub fn set_all_common(&mut self, args: CommonParamArgs) -> anyhow::Result<()> {
        let CommonParamArgs {
            sens_mult,
            yx_ratio,
            input_dpi,
        } = args;

        self.set(Param::SensMult, sens_mult)?;
        self.set(Param::YxRatio, yx_ratio)?;
        self.set(Param::InputDpi, input_dpi)?;

        Ok(())
    }

    pub fn set_all_linear(&mut self, args: LinearParamArgs) -> anyhow::Result<()> {
        let LinearParamArgs {
            accel,
            offset_linear,
            output_cap,
        } = args;

        self.set(Param::Accel, accel)?;
        self.set(Param::OffsetLinear, offset_linear)?;
        self.set(Param::OutputCap, output_cap)?;

        Ok(())
    }

    pub fn set_all_natural(&mut self, args: NaturalParamArgs) -> anyhow::Result<()> {
        let NaturalParamArgs {
            decay_rate,
            limit,
            offset_natural,
        } = args;

        self.set(Param::DecayRate, decay_rate)?;
        self.set(Param::OffsetNatural, offset_natural)?;
        self.set(Param::Limit, limit)?;

        Ok(())
    }

    pub fn set_all_synchronous(&mut self, args: SynchronousParamArgs) -> anyhow::Result<()> {
        let SynchronousParamArgs {
            gamma,
            smooth,
            motivity,
            sync_speed,
        } = args;

        self.set(Param::Gamma, gamma)?;
        self.set(Param::Smooth, smooth)?;
        self.set(Param::Motivity, motivity)?;
        self.set(Param::SyncSpeed, sync_speed)?;

        Ok(())
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

fn get_paramater(name: &'static str) -> anyhow::Result<String> {
    let path = parameter_path(name)?;
    let mut file = std::fs::File::open(&path)
        .context(anyhow!(
            "failed to open the parameter's file for reading: {}",
            path.display()
        ))
        .context("this shouldn't happen unless the maccel kernel module is not installed.")?;

    let mut buf = String::new();

    file.read_to_string(&mut buf)
        .context("failed to read the parameter's value")?;

    Ok(buf.trim().to_string())
}

fn set_parameter(name: &'static str, value: i64) -> anyhow::Result<()> {
    let path = parameter_path(name)?;

    std::fs::write(&path, format!("{}", value)).context(anyhow!(
        "failed to write to parameter file: {}",
        path.display()
    ))?;

    save_parameter_reset_script(name, value)?;

    Ok(())
}

impl Display for Fpt {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&format_param_value(f64::from(*self)))
    }
}
