use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::path::{Path, PathBuf};

use crate::params::{
    AccelMode, Param, ALL_COMMON_PARAMS, ALL_LINEAR_PARAMS, ALL_NATURAL_PARAMS,
    ALL_SYNCHRONOUS_PARAMS,
};
use crate::persist::{ParamStore, SysFsStore};

use anyhow::{Context, Result};

#[derive(Debug, Clone, Serialize, Deserialize)]
#[serde(rename_all = "PascalCase")]
pub struct Profile {
    pub name: String,
    pub mode: AccelMode,
    pub params: HashMap<String, f64>,
    pub created_at: chrono::DateTime<chrono::Utc>,
}

impl Profile {
    pub fn new(name: impl Into<String>, mode: AccelMode) -> Self {
        Self {
            name: name.into(),
            mode,
            params: HashMap::new(),
            created_at: chrono::Utc::now(),
        }
    }

    pub fn from_current_settings(name: impl Into<String>) -> Result<Self> {
        let mode = SysFsStore.get_current_accel_mode()?;
        let mut profile = Self::new(name, mode);

        for param in ALL_COMMON_PARAMS
            .iter()
            .chain(ALL_LINEAR_PARAMS.iter())
            .chain(ALL_NATURAL_PARAMS.iter())
            .chain(ALL_SYNCHRONOUS_PARAMS.iter())
        {
            let value: f64 = SysFsStore.get(*param)?.into();
            profile.params.insert(param.name().to_string(), value);
        }

        Ok(profile)
    }

    pub fn apply(&self) -> Result<()> {
        SysFsStore.set_current_accel_mode(self.mode)?;

        for (param_name, value) in &self.params {
            if let Some(param) = param_from_name(param_name) {
                SysFsStore.set(param, *value)?;
            }
        }

        Ok(())
    }
}

fn param_from_name(name: &str) -> Option<Param> {
    ALL_COMMON_PARAMS
        .iter()
        .chain(ALL_LINEAR_PARAMS.iter())
        .chain(ALL_NATURAL_PARAMS.iter())
        .chain(ALL_SYNCHRONOUS_PARAMS.iter())
        .find(|p| p.name() == name)
        .copied()
}

#[derive(Debug)]
pub struct ProfileStore {
    profiles_dir: PathBuf,
}

impl ProfileStore {
    pub fn new() -> Result<Self> {
        let profiles_dir = dirs::config_local_dir()
            .context("Failed to get config directory")?
            .join("maccel")
            .join("profiles");

        std::fs::create_dir_all(&profiles_dir)?;

        Ok(Self { profiles_dir })
    }

    #[cfg(test)]
    pub fn from_path(path: impl AsRef<Path>) -> Self {
        Self {
            profiles_dir: path.as_ref().to_path_buf(),
        }
    }

    pub fn list(&self) -> Result<Vec<Profile>> {
        let mut profiles = Vec::new();

        for entry in std::fs::read_dir(&self.profiles_dir)? {
            let entry = entry?;
            let path = entry.path();

            if path.extension().map(|e| e == "json").unwrap_or(false) {
                let content = std::fs::read_to_string(&path)?;
                let profile: Profile = serde_json::from_str(&content)?;
                profiles.push(profile);
            }
        }

        profiles.sort_by(|a, b| b.created_at.cmp(&a.created_at));
        Ok(profiles)
    }

    pub fn load(&self, name: &str) -> Result<Profile> {
        let path = self.profile_path(name);
        let content = std::fs::read_to_string(&path)?;
        let profile: Profile = serde_json::from_str(&content)?;
        Ok(profile)
    }

    pub fn save(&self, profile: &Profile) -> Result<()> {
        let path = self.profile_path(&profile.name);
        let content = serde_json::to_string_pretty(profile)?;
        std::fs::write(&path, content)?;
        Ok(())
    }

    pub fn delete(&self, name: &str) -> Result<()> {
        let path = self.profile_path(name);
        std::fs::remove_file(&path)?;
        Ok(())
    }

    pub fn exists(&self, name: &str) -> bool {
        self.profile_path(name).exists()
    }

    fn profile_path(&self, name: &str) -> PathBuf {
        self.profiles_dir.join(format!("{}.json", name))
    }
}

impl Default for ProfileStore {
    fn default() -> Self {
        Self::new().expect("Failed to create default ProfileStore")
    }
}
