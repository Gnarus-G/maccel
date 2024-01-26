use crate::libmaccel::fixedptc::{fixedpt, fixedpt_as_str, Fixedpt};
use std::{
    fs::File,
    io::Read,
    path::{Path, PathBuf},
};

use anyhow::{anyhow, Context};
use clap::ValueEnum;

const SYS_MODULE_PATH: &str = "/sys/module/maccel";

#[derive(Debug, Clone, Copy, ValueEnum)]
pub enum Param {
    Accel,
    Offset,
    OutputCap,
}

impl Param {
    pub fn set(&self, value: f32) -> anyhow::Result<()> {
        let param_path = get_param_path(self)?;
        let value = fixedpt(value);

        let mut file =
            File::create(param_path).context("failed to open the parameter's file for writing")?;

        use std::io::Write;

        write!(file, "{}", value.0).context("failed to write to parameter file")?;

        Ok(())
    }

    pub fn get(&self) -> anyhow::Result<Fixedpt> {
        let param_path = get_param_path(self)?;

        let mut file =
            File::open(param_path).context("failed to open the parameter's file for reading: this shouldn't happen unless the maccel kernel module is not installed.")?;

        let mut buf = String::new();

        file.read_to_string(&mut buf)
            .context("failed to read the parameter's value")?;

        let value: i32 = buf
            .trim()
            .parse()
            .context(format!("couldn't interpert the parameter's value {}", buf))?;

        return Ok(Fixedpt(value));
    }

    pub fn get_as_str(&self) -> anyhow::Result<String> {
        let value = self.get()?;
        let value = fixedpt_as_str(&value.0)?.to_string();

        return Ok(value);
    }

    pub fn display_name(&self) -> String {
        return format!("{:?}", self);
    }
}

fn get_param_path(name: &Param) -> anyhow::Result<PathBuf> {
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
