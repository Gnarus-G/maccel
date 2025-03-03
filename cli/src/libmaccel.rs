pub mod fixedptc {
    use std::ffi::CStr;

    use super::c_libmaccel;

    fn fixedpt_as_str(num: &i64) -> anyhow::Result<&str> {
        unsafe {
            let s = CStr::from_ptr(c_libmaccel::fixedpt_to_str(*num));
            let s = core::str::from_utf8(s.to_bytes())?;
            Ok(s)
        }
    }

    #[derive(Debug, Default, Clone, Copy, PartialEq)]
    pub struct Fixedpt(pub i64);

    impl From<Fixedpt> for f64 {
        fn from(value: Fixedpt) -> Self {
            unsafe { c_libmaccel::fixedpt_to_float(value.0) }
        }
    }

    impl From<f64> for Fixedpt {
        fn from(value: f64) -> Self {
            unsafe {
                let i = c_libmaccel::fixedpt_from_float(value);
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

pub mod c_libmaccel {
    use std::ffi::c_char;

    #[repr(C)]
    pub struct Vector {
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
