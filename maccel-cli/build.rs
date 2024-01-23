use std::{env, path::PathBuf};

fn main() {
    let out = PathBuf::from(env::var("OUT_DIR").unwrap());

    cc::Build::new()
        .file("src/fixedptc_proxy.c")
        .compile("fixedptc_proxy");

    println!("cargo:rust-link-search=static={}", out.display());

    println!("cargo:rerun-if-changed=src/fixedptc_proxy.c");
    println!("cargo:rerun-if-changed=../driver/fixedptc.h");
}
