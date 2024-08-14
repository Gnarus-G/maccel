//! Provides functions to allow us to get
//! the last immediate input speed of the user's mouse movement.
//!
//! This is mainly to allow us to visually represent the user's current
//! speed and applied sensitivity.

use std::{fs, io::Read, thread};

static mut INPUT_SPEED: f32 = 0.0;

use crate::libmaccel::{fixedptc::Fixedpt, sensitivity, Params};

pub fn read_input_speed_and_resolved_sens() -> (f64, f64) {
    let input_speed = read_input_speed();
    return (input_speed as f64, sensitivity(input_speed, Params::new()));
}

pub fn read_input_speed() -> f32 {
    // Safety don't care about race conditions
    return unsafe { INPUT_SPEED };
}

pub fn setup_input_speed_reader() {
    thread::spawn(|| {
        let mut file = fs::File::open("/dev/maccel").expect("failed to open /dev/maccel");
        let mut buffer = [0u8; 4];

        loop {
            file.read_exact(&mut buffer)
                .expect("failed to read 4 bytes from /dev/maccel");

            // The buffer, we expect, is just 4 bytes for a number (fixedpt)
            let num: f32 = Fixedpt(i32::from_be_bytes(buffer)).into();

            // Safety don't care about race conditions
            unsafe { INPUT_SPEED = num };

            thread::sleep(std::time::Duration::from_nanos(500));
        }
    });
}
