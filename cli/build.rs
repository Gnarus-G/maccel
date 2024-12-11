use std::{env, path::PathBuf};

fn main() {
    let out = PathBuf::from(env::var("OUT_DIR").unwrap());

    cc::Build::new().file("src/libmaccel.c").compile("maccel");

    println!("cargo:rust-link-search=static={}", out.display());

    println!("cargo:rerun-if-changed=src/libmaccel.c");
    println!("cargo:rerun-if-changed=../driver/accel.h");
    println!("cargo:rerun-if-changed=../driver/accel_rs.h");
    println!("cargo:rerun-if-changed=../driver/fixedptc.h");
}
