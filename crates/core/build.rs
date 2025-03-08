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
        #[cfg(feature = "long_bit_32")]
        "x86_64" => "32",
        #[cfg(not(feature = "long_bit_32"))]
        "x86_64" => "64",
        a => panic!("unsupported/untested architecture: {a}"),
    };
    let mut compiler = cc::Build::new();
    compiler
        .file("src/libmaccel.c")
        .define("FIXEDPT_BITS", fixedpt_bits);

    if cfg!(feature = "dbg") {
        compiler.define("DEBUG", "1");
        compiler.debug(true);
    }

    compiler.compile("maccel");

    println!("cargo:rust-link-search=static={}", out.display());

    const DRIVER_DIR: &str = "../../driver";
    println!("cargo:rerun-if-changed=src/libmaccel.c");
    println!("cargo:rerun-if-changed={DRIVER_DIR}/accel/natural.h");
    println!("cargo:rerun-if-changed={DRIVER_DIR}/accel.h");
    println!("cargo:rerun-if-changed={DRIVER_DIR}/accel_rs.h");
    println!("cargo:rerun-if-changed={DRIVER_DIR}/fixedptc.h");
}
