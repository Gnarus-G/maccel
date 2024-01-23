use std::ffi::{c_char, CStr};

extern "C" {
    fn fixedpt_to_str(num: i32) -> *const c_char;
    fn fixedpt_from_float(value: f32) -> i32;
}

pub fn fixedpt_as_str(num: &i32) -> anyhow::Result<&str> {
    unsafe {
        let s = CStr::from_ptr(fixedpt_to_str(*num));
        let s = core::str::from_utf8(s.to_bytes())?;
        return Ok(s);
    }
}

pub struct Fixedpt(pub i32);

pub fn fixedpt(num: f32) -> Fixedpt {
    unsafe {
        let i = fixedpt_from_float(num);
        return Fixedpt(i);
    }
}
