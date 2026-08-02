{
  lib,
  rustPlatform,
}:
let
  cargoToml = builtins.fromTOML (builtins.readFile ./cli/Cargo.toml);
in
rustPlatform.buildRustPackage {
  pname = "maccel-cli";
  version = cargoToml.package.version;

  src = lib.cleanSource ./.;
  cargoLock.lockFile = ./Cargo.lock;
  cargoBuildFlags = ["--bin" "maccel"];

  meta = {
    description = "CLI and TUI tools for configuring maccel";
    homepage = "https://www.maccel.org/";
    license = lib.licenses.gpl2Plus;
    platforms = ["x86_64-linux"];
    mainProgram = "maccel";
  };
}
