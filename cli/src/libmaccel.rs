use crate::params::Param;

use self::fixedptc::Fixedpt;

pub struct SensitivityParams {
    sens_mult: i64,
    yx_ratio: i64,
    accel: i64,
    offset: i64,
    output_cap: i64,
}

impl SensitivityParams {
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
            offset: Param::Offset
                .get()
                .expect("failed to read Offset parameter")
                .0,
            output_cap: Param::OutputCap
                .get()
                .expect("failed to read Output_Cap parameter")
                .0,
            yx_ratio: Param::YxRatio
                .get()
                .expect("failed to read Output_Cap parameter")
                .0,
        }
    }
}

pub type SensXY = (f64, f64);

/// Ratio of Output speed to Input speed
pub fn sensitivity(s_in: f64, params: SensitivityParams) -> SensXY {
    let s_in: Fixedpt = s_in.into();
    let sens = unsafe {
        c_lib::sensitivity_rs(
            s_in.0,
            params.sens_mult,
            params.yx_ratio,
            params.accel,
            params.offset,
            params.output_cap,
        )
    };
    let ratio_x: f64 = Fixedpt(sens.x).into();
    let ratio_y: f64 = Fixedpt(sens.y).into();

    (ratio_x, ratio_y)
}

pub mod fixedptc {
    use std::ffi::CStr;

    use super::c_lib;

    fn fixedpt_as_str(num: &i64) -> anyhow::Result<&str> {
        unsafe {
            let s = CStr::from_ptr(c_lib::fixedpt_to_str(*num));
            let s = core::str::from_utf8(s.to_bytes())?;
            Ok(s)
        }
    }

    #[derive(Debug, Default, PartialEq)]
    pub struct Fixedpt(pub i64);

    impl From<Fixedpt> for f64 {
        fn from(value: Fixedpt) -> Self {
            unsafe { c_lib::fixedpt_to_float(value.0) }
        }
    }

    impl From<f64> for Fixedpt {
        fn from(value: f64) -> Self {
            unsafe {
                let i = c_lib::fixedpt_from_float(value);
                Fixedpt(i)
            }
        }
    }

    impl<'a> TryFrom<&'a Fixedpt> for &'a str {
        type Error = anyhow::Error;

        fn try_from(value: &'a Fixedpt) -> Result<Self, Self::Error> {
            fixedpt_as_str(&value.0)
        }
    }
}

mod c_lib {
    use std::ffi::c_char;

    #[repr(C)]
    pub(super) struct Vector {
        pub x: i64,
        pub y: i64,
    }

    extern "C" {
        pub fn sensitivity_rs(
            speed_in: i64,
            param_sens_mult: i64,
            param_yx_ratio: i64,
            param_accel: i64,
            param_offset: i64,
            param_output_cap: i64,
        ) -> Vector;
    }

    extern "C" {
        pub fn fixedpt_to_str(num: i64) -> *const c_char;
        pub fn fixedpt_from_float(value: f64) -> i64;
        pub fn fixedpt_to_float(value: i64) -> f64;
    }
}
