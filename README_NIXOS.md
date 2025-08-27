# maccel NixOS Flake

If you're on NixOS, maccel provides a declarative flake module to seamlessly integrate and configure the mouse acceleration driver through your system configuration.

**Benefits of the NixOS module:**

- **Declarative configuration**: All parameters defined in your NixOS config
- **Direct kernel module parameters**: More efficient than reset scripts approach
- **No manual setup**: Kernel module, udev rules, and CLI tools installed automatically
- **Parameter discovery**: Optional CLI/TUI for real-time parameter tuning

## Quick Start

Add to your `flake.nix` inputs:

```nix
maccel.url = "github:Gnarus-G/maccel";
```

Create your `maccel.nix` module:

```nix
{inputs, ...}: {
  imports = [
    inputs.maccel.nixosModules.default
  ];

  hardware.maccel = {
    enable = true;
    enableCli = true; # Optional

    parameters = {
      # Common (all modes)
      sensMultiplier = 1.0;
      yxRatio = 1.0;
      inputDpi = 1000.0;
      angleRotation = 0.0;
      mode = "synchronous";

      # Linear mode
      acceleration = 0.3;
      offset = 2.0;
      outputCap = 2.0;

      # Natural mode
      decayRate = 0.1;
      offset = 2.0;
      limit = 1.5;

      # Synchronous mode
      gamma = 1.0;
      smooth = 0.5;
      motivity = 2.5;
      syncSpeed = 10.0;
    };
  };

  # To use maccel CLI/TUI without sudo
  users.groups.maccel.members = ["your_username"];
}
```

## How it Works

### Parameter Management

- **NixOS config**: Parameters are set directly as kernel module parameters at boot for maximum efficiency
- **CLI/TUI**: When `enableCli = true`, you can use `maccel tui` or `maccel set` for real-time adjustments
- **Temporary changes**: CLI/TUI modifications are session-only and revert after reboot

### Recommended Workflow

1. **Enable CLI tools**: Set `enableCli = true` for parameter discovery
2. **Find optimal values**: Use `maccel tui` to experiment with parameters in real-time
3. **Apply permanently**: Copy your optimal values to `hardware.maccel.parameters` in your NixOS config
4. **Rebuild system**: Run `sudo nixos-rebuild switch --flake .` to make settings persistent
