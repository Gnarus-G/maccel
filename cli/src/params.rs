use crate::libmaccel::fixedptc::Fixedpt;
use std::{
    fs::File,
    io::Read,
    path::{Path, PathBuf},
};

use anyhow::{anyhow, Context};
use clap::ValueEnum;

const SYS_MODULE_PATH: &str = "/sys/module/maccel";

#[derive(Debug, Clone, Copy, ValueEnum, PartialEq)]
pub enum Param {
    SensMult,
    Accel,
    Offset,
    OutputCap,
}

impl Param {
    pub fn set(&self, value: f32) -> anyhow::Result<()> {
        let value: Fixedpt = value.into();

        let mut file = File::create(self.path()?)
            .context("failed to open the parameter's file for writing")?;

        use std::io::Write;

        let value = value.0;
        write!(file, "{}", value).context("failed to write to parameter file")?;
        self.save_reset_script(value)?;

        Ok(())
    }

    fn save_reset_script(&self, value: i32) -> anyhow::Result<()> {
        let script_dir = "/var/opt/maccel/resets";
        if !Path::new(script_dir).exists() {
            std::fs::create_dir_all(script_dir).context(format!("failed create directory: {}", script_dir))
                .context("failed to create the directory where we'd save the parameter value to apply on reboot")?;
        }
        std::fs::write(
            format!("{script_dir}/set_last_{}_value.sh", self.name()),
            format!(
                "echo {} > /sys/module/maccel/parameters/{};",
                value,
                self.name()
            ),
        )
        .context("failed to write reset script")?;
        Ok(())
    }

    pub fn get(&self) -> anyhow::Result<Fixedpt> {
        let mut file =
            File::open(self.path()?).context("failed to open the parameter's file for reading: this shouldn't happen unless the maccel kernel module is not installed.")?;

        let mut buf = String::new();

        file.read_to_string(&mut buf)
            .context("failed to read the parameter's value")?;

        let value: i32 = buf
            .trim()
            .parse()
            .context(format!("couldn't interpret the parameter's value {}", buf))?;

        return Ok(Fixedpt(value));
    }

    pub fn display_name(&self) -> String {
        return format!("{:?}", self);
    }

    /// The canonical name of parameter, exactly what can be read from /sys/module/maccel/parameters
    fn name(&self) -> &'static str {
        let param_name = match self {
            Param::SensMult => "SENS_MULT",
            Param::Accel => "ACCEL",
            Param::Offset => "OFFSET",
            Param::OutputCap => "OUTPUT_CAP",
        };
        return param_name;
    }

    fn path(&self) -> anyhow::Result<PathBuf> {
        let params_path = Path::new(SYS_MODULE_PATH)
            .join("parameters")
            .join(self.name());

        if !params_path.exists() {
            return Err(anyhow!("no such parameter {:?}", self));
        }

        return Ok(params_path);
    }
}

#[cfg(test)]
mod tests {
    use std::path::PathBuf;

    use crate::libmaccel::fixedptc::Fixedpt;

    use super::Param;

    fn test(param: Param) {
        param.set(3.0).unwrap();
        assert_eq!(param.get().unwrap(), Fixedpt(768));

        param.set(4.0).unwrap();
        assert_eq!(param.get().unwrap(), Fixedpt(1024));

        param.set(4.5).unwrap();
        assert_eq!(param.get().unwrap(), (4.5).into());

        let save_script = PathBuf::from(format!(
            "/var/opt/maccel/resets/set_last_{}_value.sh",
            param.name()
        ));

        assert!(save_script.exists());
    }

    #[test]
    fn getters_setters_work() {
        use Param::*;

        test(SensMult);
        test(Accel);
        test(Offset);
        test(OutputCap);
    }
}
