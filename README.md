# maccel

Mouse pointer acceleration driver. A CLI tool and TUI allows you to easily edit a curve's parameters.

## Linear Acceleration Function

![image](https://github.com/user-attachments/assets/a9e42195-41f9-419a-9de9-2f7ad83604aa)

$$V = \frac{\sqrt{dx_0^2 + dy_0^2}}{i}$$

$$dx_f = dx_0 * (1 + aV)$$

$$dy_f = dy_0 * (1 + aV)$$

Where $dx$ and $dy$ are the directional displacements read from the mouse,
$i$ is the polling interval of the mouse; this would be 1ms if the polling rate is 1000Hz,
and $a$ is the user provided
acceleration factor.

The more general function, which is relevant with a set input offset, is:

$$(dx_f, dy_f) = (dx_0, dy_0) * (1 + a * (V - offset_in)^2 / V)$$

## Other Curves

- [x] **Natural**
      ![image](https://github.com/user-attachments/assets/d14d0fa3-f762-4ad6-911c-cf564227d1ac)

- [x] **Synchronous**
      ![image](https://github.com/user-attachments/assets/cd0aefaa-43d1-4f31-8326-334fac2a2210)

- [ ] **Look up table**

## Install

### Shell Script (Recommended)

Make sure to have these dependencies installed on your machine:
`curl`, `git`, `make`, `dkms`, and the linux headers in `/lib/modules/`

You might also, and you probably don't, have to install `gcc` or `clang`
depending on with which your distro's kernel was built.

```sh
curl -fsSL https://www.maccel.org/install.sh | sudo sh
```

If you choose to build the cli from source:

```sh
curl -fsSL https://www.maccel.org/install.sh | sudo BUILD_CLI_FROM_SOURCE=1 sh
```

You'll need [`cargo`](https://www.rust-lang.org/tools/install)

### NixOS (Flake)

For NixOS users, maccel provides a flake module that integrates seamlessly with your system configuration. Parameters are configured declaratively and applied directly as kernel module parameters for maximum efficiency.

```nix
# In your flake.nix inputs
maccel.url = "github:Gnarus-G/maccel";

# In your configuration
{inputs, ...}: {
  imports = [
    inputs.maccel.nixosModules.default
  ];

  hardware.maccel = {
    enable = true;
    enableCli = true; # Optional: for parameter discovery
    parameters = {
      mode = "linear";
      sensMultiplier = 1.0;
      acceleration = 0.3;
      offset = 2.0;
      outputCap = 2.0;
    };
  };
}
```

See [README_NIXOS.md](README_NIXOS.md) for detailed setup instructions and all available parameters.

### Arch (PKGBUILD)

```sh
git clone https://github.com/Gnarus-G/maccel
cd maccel
makepkg -si
```

Run `modprobe maccel` after installing.

Optionally, add `modprobe_on_install=true` to your dkms
config file (usually located at /etc/dkms/framework.conf) to automatically modprobe after installing
a dkms module.

#### Post-install recommendation

Optionally, add yourself to the maccel group using `usermod -aG maccel $USER` after installing,
if you want to run `maccel` without running as root.

## Uninstall

```sh
sh /opt/maccel/uninstall.sh
```

## CLI Usage

```
CLI to control the parameters for the maccel driver

Usage: maccel [COMMAND]

Commands:
  tui         Open the Terminal UI to manage the parameters and see a graph of the sensitivity
  set         Set the value for a parameter of the maccel driver
  get         Get the values for parameters of the maccel driver
  completion  Generate a completions file for a specified shell
  help        Print this message or the help of the given subcommand(s)

Options:
  -h, --help     Print help
  -V, --version  Print version
```

## Notes

One should disable the acceleration done by default in some distros, e.g. by `xset` or `libinput`.
[Full Guide](https://wiki.archlinux.org/title/Mouse_acceleration#Disabling_mouse_acceleration)

Here is [Breakdown of why and how I ended up making this](https://www.bytin.tech/blog/maccel/)

## Troubleshooting Install

### linux headers

You want to make sure that `/lib/modules/` is not empty. For example mine looks like this:

```
total 0
drwxr-xr-x 1 root root    114 Jan 29 17:59 .
drwxr-xr-x 1 root root 159552 Jan 29 22:39 ..
drwxr-xr-x 1 root root     10 Jan 29 17:59 6.6.14-1-lts
drwxr-xr-x 1 root root     12 Jan 29 17:59 6.7.0-zen3-1-zen
drwxr-xr-x 1 root root    494 Jan 29 17:59 6.7.2-arch1-1
drwxr-xr-x 1 root root    494 Jan 31 21:54 6.7.2-zen1-1-zen
```

You want to find headers that match your kernel as represented by

```
uname -r
```

On an arch based distro you search for the available headers with

```
pacman -Ss linux headers
```

### gcc

The version matters, it must match the version with which the kernel was built.

For example you might encounter such an error:

![image](https://github.com/Gnarus-G/maccel/assets/37311893/6147e20a-a132-4132-a45e-2af3dc035552)

And you'll have to find a version of `gcc` that matches. This will be more or less annoying
depending on your distro and/or how familiar you are with it.

### debian stable

This distro is effectively not supported for the foreseeable future. It's hard to install and build the kernel module for it due to missing/outdated and hard-to-install dependencies.
See issues: [#81](https://github.com/Gnarus-G/maccel/issues/81) [#43](https://github.com/Gnarus-G/maccel/issues/43)

### Miscellaneous

If you notice any weird behavior and are looking to investigate it,  
then try a debug build of the driver. Run this modified install command.

```sh
curl -fsSL https://www.maccel.org/install.sh | sudo DEBUG=1 sh
```

Watch the extra log messages flowing though:

```sh
dmesg -w
```

This debugging experience might be lacking still. Feel free to report any issue

## References

- https://lwn.net/Kernel/LDD3/
- https://www.kernel.org/doc/html/latest/input/index.html
- https://github.com/a1xd/rawaccel/blob/master/doc/Guide.md
- https://github.com/Skyl3r/leetmouse/blob/master/driver/accel.c
- https://sourceforge.net/p/fixedptc/code/ci/default/tree/
- https://github.com/torvalds/linux/blob/master/drivers/input/evdev.c
- https://github.com/freedesktop-unofficial-mirror/evtest/blob/master/evtest.c
- https://docs.kernel.org/input/input-programming.html
- https://www.kernel.org/doc/Documentation/input/input.txt
- https://docs.kernel.org/driver-api/input.html
- https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html
- https://www.youtube.com/watch?v=oX9ZwMQL2f4
- https://gist.github.com/fstiehle/17fca11d7d1b4c2b8dfd982e1cf39caf
- https://man.archlinux.org/man/PKGBUILD.5#OPTIONS_AND_DIRECTIVES
- https://stackoverflow.com/questions/669452/are-double-square-brackets-preferable-over-single-square-brackets-in-b
- https://github.com/Kuuuube/rawaccel_convert

## Contributing

First off, I appreciate you. Now here are some guidelines:

### Commit messages

Subject line:

- First letter is capitalized
- Imperative tense (e.g. "Add ...", not "Added ..." or "Adds ...")
- Include as much detail as you can.

[See more](https://github.com/Gnarus-G/maccel/blob/main/CONTRIBUTING.md)

### Add new acceleration curves/modes

See PR https://github.com/Gnarus-G/maccel/pull/60 which implements the `Synchronous` mode for a walkthrough of how
to do it.
