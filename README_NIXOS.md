# maccel NixOS Flake

If you're on NixOS, maccel provides a declarative flake module to seamlessly integrate and configure the mouse acceleration driver through your system configuration.

**Benefits of the NixOS module:**

- **Declarative configuration**: All parameters defined in your NixOS config
- **Direct kernel module parameters**: More efficient than reset scripts approach
- **No manual setup**: Kernel module, udev rules, and CLI tools installed automatically
- **Seamless updates**: Update maccel alongside your system like any other flake
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

## Maintaining the NixOS Module

When new acceleration modes or parameters are added to maccel, the [NixOS module](module.nix) needs to be updated accordingly. This section provides a step-by-step guide for maintainers.

### 1. Adding New Modes

New modes are defined in [`driver/accel/mode.h`](driver/accel/mode.h). To add support:

**In [`module.nix`](module.nix):**

#### Step 1: Add to modeMap

```nix
# Update the modeMap with new modes
modeMap = {
  linear = 0;
  natural = 1;
  synchronous = 2;
  no_accel = 3;
  new_mode = 4;  # Add new mode here
};
```

#### Step 2: Add the mode option

```nix
# In options.hardware.maccel.parameters
mode = mkOption {
  type = types.nullOr (types.enum ["linear" "natural" "synchronous" "no_accel" "new_mode"]);
  default = null;
  description = "Acceleration mode.";
};
```

### 2. Adding New Parameters

New parameters are defined in [`driver/params.h`](driver/params.h). To add support:

**In [`module.nix`](module.nix):**

#### Step 1: Add to parameterMap

```nix
# In parameterMap
parameterMap = {
  # Existing parameters...
  NEW_PARAM = cfg.parameters.newParam;
};
```

#### Step 2: Add NixOS option

```nix
# In options.hardware.maccel.parameters
newParam = mkOption {
  type = types.nullOr types.float;  # or appropriate type
  default = null;
  description = "Description of the new parameter.";
};
```

#### Step 3: Add validation (if needed)

```nix
# For parameters with constraints
newParam = mkOption {
  type = types.nullOr (types.addCheck types.float (x: x > 0.0)
    // {description = "positive float";});
  default = null;
  description = "Description of the new parameter. Must be positive.";
};
```

### 3. Using LLMs

This NixOS module was developed with significant LLM assistance. Here's a comprehensive prompt for maintaining it:

```
I need to update the NixOS module for maccel. Please analyze the maccel codebase and help me maintain the NixOS module:

CONTEXT:
- maccel is a Linux mouse acceleration driver with kernel module and CLI tools
- The NixOS module is in module.nix and provides declarative configuration
- Parameters are defined in driver/params.h and modes in driver/accel/mode.h
- Read README_NIXOS.md section "Maintaining the NixOS Module" for detailed patterns and examples

TASKS:
1. **Discover changes**: Analyze driver/params.h and driver/accel/ for new parameters, modes, or modifications
2. **Compare with current**: Check what's missing from existing modeMap and parameterMap in module.nix
3. **Add new modes**: Update modeMap with correct enum values and add to mode option enum
4. **Add new parameters**: Add to parameterMap and create NixOS options with proper type validation, descriptions, and constraints
5. **Explain changes**: Summarize what was added and why

Follow the exact patterns shown in README_NIXOS.md and existing patterns in module.nix. Provide complete code snippets for module.nix.
```

### 4. Testing The Module

After modifying [`module.nix`](module.nix):

1. **Format nix code**: `alejandra .`
2. **Check nix syntax**: `nix flake check`
3. **Test parameter loading**: Confirm parameters load correctly on boot when set via NixOS config
