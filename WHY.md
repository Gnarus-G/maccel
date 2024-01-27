## How I achieve Linear mouse acceleration on linux.

### Why?

Many gamers, specifically those that enjoy first person shooters, know you should disable the mouse pointer acceleration
that windows has enabled by default. You disable so that you get a 1:1 correlation between how you mouse physically and how
the pointer moves.

Some of us, however, know that mouse acceleration, if done right, can be very useful and that windows just does it very poorly.
This [video](https://www.youtube.com/watch?v=SBXv0xi-wyQ) by [pinguefy](https://www.youtube.com/@Pinguefy) helped to convince me
of this. We also know that [Raw Accel](https://github.com/a1xd/rawaccel/blob/master/doc/Guide.md), a mouse pointer acceleration driver for windows, does it right.

### on Linux

We want rawaccel on linux too, but we won't get it.
So we're relegated to projects like [leetmouse](https://github.com/Skyl3r/leetmouse) that I found difficult to install,
and not user friendly at all; or [libinput](https://wiki.archlinux.org/title/Libinput) with a (custom acceleration profile)[https://wayland.freedesktop.org/libinput/doc/latest/pointer-acceleration.html#the-custom-acceleration-profile].

Now, my gripe with `leetmouse` comes down to, mostly, skill issue. I eventually got it working, but it was still hard to change
parameters since I had to recompile the source code to do so.

My gripe with `libinput` is that while having a terrible user experience on par `leetmouse`, it also impossible to expression
certain curve. I even built a [cli tool](https://github.com/Gnarus-G/libinput-custom-points-gen) to generate the configuration that will approximate the curve I want.

Accel=0.3; Offset=2; CAP=2;

```sh
libinput-points 0.3 2 2 -x
```

```
Section "InputClass"
    Identifier "My Mouse"
    Driver "libinput"
    MatchIsPointer "yes"

    Option "AccelProfile" "custom"
    Option "AccelStepMotion" "1"
    Option "AccelPointsMotion" "0 1 2 3.3 5.2 7.699999999999999 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 42 44 46 48 50 52 54 56 58 60 62 64 66 68 70 72 74 76 78 80 82 84 86 88 90 92 94 96 98 100 102 104 106 108 110 112 114 116 118 120 122 124"
EndSection

```

If you need more context on this, refer to https://wiki.archlinux.org/title/Libinput#Via_Xorg_configuration_file, and https://man.archlinux.org/man/libinput.4#SUPPORTED_PROPERTIES
I don't recommend using `libinput-points` to mimic Rawaccel's linear acceleration as I understood much less than I do now
how the math works.

Note then, that my main goal, and only goal for the moment, is to replicate the model for the linear type acceleration curves
from `Rawaccel`

### My solution

So we need an easy install story, and a very easy configuration story.

I made my own driver, [maccel](https://github.com/Gnarus-G/maccel). It's easy enough for me to install, but it's still only
comfortable to those who are familiar with building from source. So the work there is very much in progress.

But past that hurdle is a decent enough cli tool with a user friendly Terminal UI.

```sh
maccel tui
```

![image](https://github.com/Gnarus-G/maccel/assets/37311893/dd62fc9a-3558-46a4-847e-05f691c31054)

## Install Instructions

Clone the repo, and run some make commands.

```sh
git clone https://github.com/Gnarus-G/maccel.git
cd maccel
```

### Install the cli.

If you don't have `cargo`, then get it with rust
from https://www.rust-lang.org/tools/install

```sh
cargo install --path maccel-cli
```

### Instal the driver and the udev rules.

```sh
make install
make udev_install # This depends on the cli and will install in /usr/local/bin
```

Optionally, it's recommended that you add yourself to the `maccel` group;
and remember to restart your session after you do so it takes effect.

Adding yourself to that `maccel` group allows you to run the maccel-cli
to set parameter values without `sudo`

### Uninstall

```sh
make uninstall
make udev_uninstall
```
