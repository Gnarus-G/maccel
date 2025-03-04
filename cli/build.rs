use std::{
    env::{self, consts::ARCH},
    path::PathBuf,
};

fn main() {
    let out = PathBuf::from(
        env::var("OUT_DIR").expect("Expected OUT_DIR to be defined in the environment"),
    );

    let fixedpt_bits = match ARCH {
        "x86" => "32",
        "x86_64" => "64",
        a => panic!("unsupported/untested architecture: {a}"),
    };
    let mut compiler = cc::Build::new();
    compiler
        .file("src/libmaccel.c")
        .define("FIXEDPT_BITS", fixedpt_bits);

    if cfg!(feature = "debug") {
        compiler.define("DEBUG", "1");
        compiler.debug(true);
    }

    compiler.compile("maccel");

    println!("cargo:rust-link-search=static={}", out.display());

    println!("cargo:rerun-if-changed=src/libmaccel.c");
    println!("cargo:rerun-if-changed=../driver/accel/natural.h");
    println!("cargo:rerun-if-changed=../driver/accel.h");
    println!("cargo:rerun-if-changed=../driver/accel_rs.h");
    println!("cargo:rerun-if-changed=../driver/fixedptc.h");
}
