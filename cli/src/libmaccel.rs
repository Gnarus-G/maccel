use crate::params::Param;

use self::fixedptc::Fixedpt;

pub struct Params {
    sens_mult: i32,
    accel: i32,
    motivity: i32,
    gamma: i32,
    sync_speed: i32,
    offset: i32,
    output_cap: i32,
}

impl Params {
    pub fn new() -> Self {
        Self {
            sens_mult: Param::SensMult
                .get()
                .expect("failed to read Sens_Mult parameter")
                .0,
            accel: Param::Accel
                .get()
                .expect("failed to read Accel parameter")
                .0,
            motivity: Param::Motivity
                .get()
                .expect("failed to read Motivity parameter")
                .0,
            gamma: Param::Gamma
                .get()
                .expect("failed to read Gamma parameter")
                .0,
            sync_speed: Param::SyncSpeed
                .get()
                .expect("failed to read SyncSpeed parameter")
                .0,
            offset: Param::Offset
                .get()
                .expect("failed to read Offset parameter")
                .0,
            output_cap: Param::OutputCap
                .get()
                .expect("failed to read Output_Cap parameter")
                .0,
        }
    }
}

/// Ratio of Output speed to Input speed
pub fn sensitivity(s_in: f32, params: Params) -> f64 {
    let s_in: Fixedpt = s_in.into();
    let a_factor = unsafe {
        c_lib::sensitivity(
            s_in.0,
            params.sens_mult,
            params.accel,
            params.motivity,
            params.gamma,
            params.sync_speed,
            params.offset,
            params.output_cap,
        )
    };
    let a_factor: f32 = Fixedpt(a_factor).into();

    return a_factor as f64;
}

pub mod fixedptc {
    use std::ffi::CStr;

    use super::c_lib;

    fn fixedpt_as_str(num: &i32) -> anyhow::Result<&str> {
        unsafe {
            let s = CStr::from_ptr(c_lib::fixedpt_to_str(*num));
            let s = core::str::from_utf8(s.to_bytes())?;
            return Ok(s);
        }
    }

    #[derive(Debug, PartialEq)]
    pub struct Fixedpt(pub i32);

    impl From<Fixedpt> for f32 {
        fn from(value: Fixedpt) -> Self {
            unsafe { c_lib::fixedpt_to_float(value.0) }
        }
    }

    impl From<f32> for Fixedpt {
        fn from(value: f32) -> Self {
            unsafe {
                let i = c_lib::fixedpt_from_float(value);
                return Fixedpt(i);
            }
        }
    }

    impl<'a> TryFrom<&'a Fixedpt> for &'a str {
        type Error = anyhow::Error;

        fn try_from(value: &'a Fixedpt) -> Result<Self, Self::Error> {
            return fixedpt_as_str(&value.0);
        }
    }
}

mod c_lib {
    use std::ffi::c_char;

    extern "C" {
        pub fn sensitivity(
            speed_in: i32,
            param_sens_mult: i32,
            param_accel: i32,
            param_motivity: i32,
            param_gamma: i32,
            param_sync_speed: i32,
            param_offset: i32,
            param_output_cap: i32,
        ) -> i32;
    }

    extern "C" {
        pub fn fixedpt_to_str(num: i32) -> *const c_char;
        pub fn fixedpt_from_float(value: f32) -> i32;
        pub fn fixedpt_to_float(value: i32) -> f32;
    }
}
