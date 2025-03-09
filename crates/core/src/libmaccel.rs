pub mod fixedptc {
    use std::{
        ffi::{CStr, CString},
        str::FromStr,
    };

    use anyhow::Context;

    use super::c_libmaccel::{self, str_to_fpt};

    #[derive(Debug, Default, Clone, Copy, PartialEq)]
    #[repr(transparent)]
    pub struct Fpt(pub i64);

    impl From<Fpt> for f64 {
        fn from(value: Fpt) -> Self {
            unsafe { c_libmaccel::fpt_to_float(value) }
        }
    }

    impl From<f64> for Fpt {
        fn from(value: f64) -> Self {
            unsafe { c_libmaccel::fpt_from_float(value) }
        }
    }

    #[cfg(test)]
    #[test]
    fn fpt_and_float_conversion_to_and_fro() {
        macro_rules! assert_for {
            ($value:literal) => {{
                let fp = Fpt::from($value);
                assert_eq!(f64::from(fp), $value);
            }};
        }

        assert_for!(1.5);
        assert_for!(4.5);
        assert_for!(1.0);
        assert_for!(0.0);
        assert_for!(0.125);
        assert_for!(0.5);
    }

    impl<'a> TryFrom<&'a Fpt> for &'a str {
        type Error = anyhow::Error;

        fn try_from(value: &'a Fpt) -> Result<Self, Self::Error> {
            unsafe {
                let s = CStr::from_ptr(c_libmaccel::fpt_to_str(*value));
                let s = core::str::from_utf8(s.to_bytes())?;
                Ok(s)
            }
        }
    }

    impl FromStr for Fpt {
        type Err = anyhow::Error;

        fn from_str(s: &str) -> Result<Self, Self::Err> {
            let cstr = CString::new(s).context("Failed to convert to a C string")?;
            let f = unsafe { str_to_fpt(cstr.as_ptr()) };
            Ok(f)
        }
    }
}

#[repr(C)]
pub struct Vector {
    pub x: i64,
    pub y: i64,
}

mod c_libmaccel {
    use super::{fixedptc, Vector};
    use crate::params::AccelParams;
    use std::ffi::c_char;

    unsafe extern "C" {
        pub fn sensitivity_rs(speed_in: fixedptc::Fpt, args: AccelParams) -> Vector;
    }

    unsafe extern "C" {
        pub fn fpt_to_str(num: fixedptc::Fpt) -> *const c_char;
        pub fn str_to_fpt(string: *const c_char) -> fixedptc::Fpt;
        pub fn fpt_from_float(value: f64) -> fixedptc::Fpt;
        pub fn fpt_to_float(value: fixedptc::Fpt) -> f64;
    }
}

pub use c_libmaccel::sensitivity_rs;
