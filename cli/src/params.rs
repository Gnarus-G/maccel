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
    Mode,
    Accel,
    Motivity,
    Gamma,
    SyncSpeed,
    Offset,
    OutputCap,
}

impl Param {
    pub fn set(&self, value: f32) -> anyhow::Result<()> {
        let value: Fixedpt = value.into();
        let path = self.path()?;
        let mut file = File::create(&path).context(anyhow!(
            "failed to open the parameter's file for writing: {}",
            path.display()
        ))?;

        use std::io::Write;

        let value = value.0;
        write!(file, "{}", value).context(anyhow!(
            "failed to write to parameter file: {}",
            path.display()
        ))?;
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
        let path = self.path()?;
        let mut file = File::open(&path)
            .context(anyhow!(
                "failed to open the parameter's file for reading: {}",
                path.display()
            ))
            .context("this shouldn't happen unless the maccel kernel module is not installed.")?;

        let mut buf = String::new();

        file.read_to_string(&mut buf)
            .context("failed to read the parameter's value")?;

        let value: i32 = buf
            .trim()
            .parse()
            .context(format!("couldn't interpret the parameter's value {}", buf))?;

        return Ok(Fixedpt(value));
    }

    pub fn display_name(&self) -> &'static str {
        return match self {
            Param::SensMult => "Sens-Multiplier",
            Param::Mode => "Mode",
            Param::Accel => "Accel",
            Param::Motivity => "Motivity",
            Param::Gamma => "Gamma",
            Param::SyncSpeed => "SyncSpeed",
            Param::Offset => "Offset",
            Param::OutputCap => "Output-Cap",
        };
    }

    /// The canonical name of parameter, exactly what can be read from /sys/module/maccel/parameters
    fn name(&self) -> &'static str {
        let param_name = match self {
            Param::SensMult => "SENS_MULT",
            Param::Mode => "MODE",
            Param::Accel => "ACCEL",
            Param::Motivity => "MOTIVITY",
            Param::Gamma => "GAMMA",
            Param::SyncSpeed => "SYNC_SPEED",
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
            return Err(anyhow!("no such path: {}", params_path.display()))
                .context(anyhow!("no such parameter {:?}", self));
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
        test(Mode);
        test(Accel);
        test(Motivity);
        test(Gamma);
        test(SyncSpeed);
        test(Offset);
        test(OutputCap);
    }
}
