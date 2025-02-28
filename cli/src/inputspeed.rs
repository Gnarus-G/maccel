//! Provides functions to allow us to get
//! the last immediate input speed of the user's mouse movement.
//!
//! This is mainly to allow us to visually represent the user's current
//! speed and applied sensitivity.

use std::{fs, io::Read, thread};

static mut INPUT_SPEED: f64 = 0.0;

use crate::libmaccel::fixedptc::Fixedpt;

pub fn read_input_speed() -> f64 {
    // Safety don't care about race conditions
    return unsafe { INPUT_SPEED };
}

pub fn setup_input_speed_reader() {
    thread::spawn(|| {
        let mut file = fs::File::open("/dev/maccel").expect("failed to open /dev/maccel");
        let mut buffer = [0u8; 8];

        loop {
            let nread = file
                .read(&mut buffer)
                .expect("failed to read bytes from /dev/maccel");

            let num = match nread {
                4 => {
                    let buffer = buffer
                        .first_chunk::<4>()
                        .expect("failed to grab 4 bytes from the read buffer");
                    i32::from_be_bytes(*buffer) as i64
                }
                8 => i64::from_be_bytes(buffer),
                _ => 0,
            };

            let num: f64 = Fixedpt(num).into();

            // Safety don't care about race conditions
            unsafe { INPUT_SPEED = num };

            thread::sleep(std::time::Duration::from_nanos(500));
        }
    });
}
