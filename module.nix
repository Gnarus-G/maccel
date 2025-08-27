# NixOS module for maccel mouse acceleration driver
{
  config,
  lib,
  pkgs,
  ...
}:
with lib; let
  cfg = config.hardware.maccel;

  # Extract version from PKGBUILD
  pkgbuildContent = builtins.readFile ./PKGBUILD;
  kernelModuleVersion = builtins.head (builtins.match ".*pkgver=([^[:space:]]+).*" pkgbuildContent);

  # Extract version from cli/Cargo.toml
  cliCargoToml = builtins.fromTOML (builtins.readFile ./cli/Cargo.toml);
  cliVersion = cliCargoToml.package.version;

  # Convert float to fixed-point integer (64-bit, 32 fractional bits)
  fixedPointScale = 4294967296; # 2^32
  toFixedPoint = value: builtins.floor (value * fixedPointScale + 0.5);

  # Mode enum mapping (from driver/accel/mode.h)
  modeMap = {
    linear = 0;
    natural = 1;
    synchronous = 2;
    no_accel = 3;
  };

  # Parameter mapping (from driver/params.h)
  parameterMap = {
    # Common parameters
    SENS_MULT = cfg.parameters.sensMultiplier;
    YX_RATIO = cfg.parameters.yxRatio;
    INPUT_DPI = cfg.parameters.inputDpi;
    ANGLE_ROTATION = cfg.parameters.angleRotation;
    MODE = cfg.parameters.mode;

    # Linear mode parameters
    ACCEL = cfg.parameters.acceleration;
    OFFSET = cfg.parameters.offset;
    OUTPUT_CAP = cfg.parameters.outputCap;

    # Natural mode parameters
    DECAY_RATE = cfg.parameters.decayRate;
    LIMIT = cfg.parameters.limit;

    # Synchronous mode parameters
    GAMMA = cfg.parameters.gamma;
    SMOOTH = cfg.parameters.smooth;
    MOTIVITY = cfg.parameters.motivity;
    SYNC_SPEED = cfg.parameters.syncSpeed;
  };

  # Generate modprobe parameter string
  kernelModuleParams = let
    validParams = filterAttrs (_: v: v != null) parameterMap;
    formatParam = name: value:
      if name == "MODE"
      then "${name}=${toString (modeMap.${value})}"
      else "${name}=${toString (toFixedPoint value)}";
  in
    concatStringsSep " " (mapAttrsToList formatParam validParams);

  # Build kernel module
  maccel-kernel-module = config.boot.kernelPackages.callPackage ({
    lib,
    stdenv,
    kernel,
  }:
    stdenv.mkDerivation rec {
      pname = "maccel-dkms";
      version = kernelModuleVersion;

      src = ./.;

      nativeBuildInputs = kernel.moduleBuildDependencies;

      makeFlags =
        [
          "KVER=${kernel.modDirVersion}"
          "KDIR=${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
          "DRIVER_CFLAGS=-DFIXEDPT_BITS=64"
        ]
        ++ optionals cfg.debug ["DRIVER_CFLAGS+=-g -DDEBUG"];

      preBuild = "cd driver";

      installPhase = ''
        mkdir -p $out/lib/modules/${kernel.modDirVersion}/kernel/drivers/usb
        cp maccel.ko $out/lib/modules/${kernel.modDirVersion}/kernel/drivers/usb/
      '';

      meta = with lib; {
        description = "Mouse acceleration driver and kernel module for Linux.";
        homepage = "https://www.maccel.org/";
        license = licenses.gpl2Plus;
        platforms = platforms.linux;
      };
    }) {};

  # Optional CLI tools
  maccel-cli = pkgs.rustPlatform.buildRustPackage rec {
    pname = "maccel-cli";
    version = cliVersion;

    src = ./.;

    cargoLock.lockFile = "${src}/Cargo.lock";

    cargoBuildFlags = ["--bin" "maccel"];

    meta = with lib; {
      description = "CLI and TUI tools for configuring maccel.";
      homepage = "https://www.maccel.org/";
      license = licenses.gpl2Plus;
      platforms = platforms.linux;
    };
  };
in {
  options.hardware.maccel = {
    enable = mkEnableOption "Enable maccel mouse acceleration driver (kernel module). Parameters must be configured via `hardware.maccel.parameters`.";

    debug = mkOption {
      type = types.bool;
      default = false;
      description = "Enable debug build of the kernel module.";
    };

    enableCli = mkOption {
      type = types.bool;
      default = false;
      description = "Install CLI and TUI tools for real-time parameter tuning. You may add your user to the `maccel` group using `users.groups.maccel.members = [\"your_username\"];` to run maccel CLI/TUI without sudo. Note: Changes made via CLI/TUI are temporary and do not persist across reboots. Use this to discover optimal parameter values, then apply them permanently via `hardware.maccel.parameters`.";
    };

    parameters = {
      # Common parameters
      sensMultiplier = mkOption {
        type = types.nullOr types.float;
        default = null;
        description = "Sensitivity multiplier applied after acceleration calculation.";
      };

      yxRatio = mkOption {
        type = types.nullOr types.float;
        default = null;
        description = "Y/X ratio - factor by which Y-axis sensitivity is multiplied.";
      };

      inputDpi = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x > 0.0)
            // {description = "positive float";});
        default = null;
        description = "DPI of the mouse, used to normalize effective DPI. Must be positive.";
      };

      angleRotation = mkOption {
        type = types.nullOr types.float;
        default = null;
        description = "Apply rotation in degrees to mouse movement input.";
      };

      mode = mkOption {
        type = types.nullOr (types.enum ["linear" "natural" "synchronous" "no_accel"]);
        default = null;
        description = "Acceleration mode.";
      };

      # Linear mode parameters
      acceleration = mkOption {
        type = types.nullOr types.float;
        default = null;
        description = "Linear acceleration factor.";
      };

      offset = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x >= 0.0)
            // {description = "non-negative float";});
        default = null;
        description = "Input speed past which to allow acceleration. Cannot be negative.";
      };

      outputCap = mkOption {
        type = types.nullOr types.float;
        default = null;
        description = "Maximum sensitivity multiplier cap.";
      };

      # Natural mode parameters
      decayRate = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x > 0.0)
            // {description = "positive float";});
        default = null;
        description = "Decay rate of the Natural acceleration curve. Must be positive.";
      };

      limit = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x >= 1.0)
            // {description = "float >= 1.0";});
        default = null;
        description = "Limit of the Natural acceleration curve. Cannot be less than 1.";
      };

      # Synchronous mode parameters
      gamma = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x > 0.0)
            // {description = "positive float";});
        default = null;
        description = "Controls how fast you get from low to fast around the midpoint. Must be positive.";
      };

      smooth = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x >= 0.0 && x <= 1.0)
            // {description = "float between 0.0 and 1.0";});
        default = null;
        description = "Controls the suddenness of the sensitivity increase. Must be between 0 and 1.";
      };

      motivity = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x > 1.0)
            // {description = "float > 1.0";});
        default = null;
        description = "Sets max sensitivity while setting min to 1/motivity. Must be greater than 1.";
      };

      syncSpeed = mkOption {
        type =
          types.nullOr (types.addCheck types.float (x: x > 0.0)
            // {description = "positive float";});
        default = null;
        description = "Sets the middle sensitivity between min and max sensitivity. Must be positive.";
      };
    };
  };

  config = mkIf cfg.enable {
    # Add kernel module
    boot.extraModulePackages = [maccel-kernel-module];

    # Load module with parameters
    boot.kernelModules = ["maccel"];
    boot.extraModprobeConfig = mkIf (kernelModuleParams != "") ''
      options maccel ${kernelModuleParams}
    '';

    # Create maccel group
    users.groups.maccel = {};

    # Install CLI tools if requested
    environment.systemPackages = mkIf cfg.enableCli [maccel-cli];

    # Create reset scripts directory
    systemd.tmpfiles.rules = mkIf cfg.enableCli [
      "d /var/opt/maccel/resets 0775 root maccel"
    ];

    # Add udev rules
    services.udev.extraRules = mkIf cfg.enableCli ''
      # Set sysfs parameter permissions
      ACTION=="add", SUBSYSTEM=="module", DEVPATH=="/module/maccel", \
        RUN+="${pkgs.coreutils}/bin/chgrp -R maccel /sys/module/maccel/parameters"
      # Set /dev/maccel character device permissions
      ACTION=="add", KERNEL=="maccel", \
        GROUP="maccel", MODE="0640"
    '';
  };
}
