use crate::params::Param;

use self::fixedptc::Fixedpt;

extern "C" {
    fn acceleration_factor(
        speed_in: i32,
        param_sensitivity: i32,
        param_accel: i32,
        param_offset: i32,
        param_output_cap: i32,
    ) -> i32;
}

pub struct Params {
    sensitivity: i32,
    accel: i32,
    offset: i32,
    output_cap: i32,
}

impl Params {
    pub fn new() -> Self {
        Self {
            sensitivity: Param::Sensitivity
                .get()
                .expect("failed to read Sensitivity parameter")
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
        }
    }
}

/// Ratio of Output speed to Input speed
pub fn sensitivity(s_in: f32, params: Params) -> f64 {
    let s_in = fixedptc::fixedpt(s_in);
    let a_factor = unsafe {
        acceleration_factor(
            s_in.0,
            params.sensitivity,
            params.accel,
            params.offset,
            params.output_cap,
        )
    };
    let a_factor: f32 = Fixedpt(a_factor).into();

    return a_factor as f64;
}

pub mod fixedptc {
    use std::ffi::c_char;
    use std::ffi::CStr;

    extern "C" {
        fn fixedpt_to_str(num: i32) -> *const c_char;
        fn fixedpt_from_float(value: f32) -> i32;
        fn fixedpt_to_float(value: i32) -> f32;
    }

    pub fn fixedpt_as_str(num: &i32) -> anyhow::Result<&str> {
        unsafe {
            let s = CStr::from_ptr(fixedpt_to_str(*num));
            let s = core::str::from_utf8(s.to_bytes())?;
            return Ok(s);
        }
    }

    pub struct Fixedpt(pub i32);

    impl From<Fixedpt> for f32 {
        fn from(value: Fixedpt) -> Self {
            unsafe { fixedpt_to_float(value.0) }
        }
    }

    impl From<f32> for Fixedpt {
        fn from(value: f32) -> Self {
            unsafe {
                let i = fixedpt_from_float(value);
                return Fixedpt(i);
            }
        }
    }

    pub fn fixedpt(num: f32) -> Fixedpt {
        unsafe {
            let i = fixedpt_from_float(num);
            return Fixedpt(i);
        }
    }
}
