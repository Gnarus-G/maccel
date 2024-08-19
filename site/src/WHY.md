## Linear Mouse Acceleration on Linux.

### Why?

Many gamers, specifically those whom enjoy first person shooters, know that you should disable the mouse pointer acceleration
that windows has enabled by default. You disable it so that you get a 1:1 correlation between how you mouse physically and how
the pointer moves.

Some of us, however, know that mouse acceleration, if done right, can be very useful and that windows just does it very poorly.
I became convinced of this after some youtube videos on the subject, mainly this [video](https://www.youtube.com/watch?v=SBXv0xi-wyQ) by [pinguefy](https://www.youtube.com/@Pinguefy).
We also know that [Raw Accel](https://github.com/a1xd/rawaccel/blob/master/doc/Guide.md), a mouse pointer acceleration driver for **windows**, does it right.

### Linux

We want rawaccel on **linux** too, but we won't get it.
So we're relegated to projects like [leetmouse](https://github.com/Skyl3r/leetmouse) that I found difficult to install,
and user unfriendly; or [libinput](https://wiki.archlinux.org/title/Libinput) with a [custom acceleration profile](https://wayland.freedesktop.org/libinput/doc/latest/pointer-acceleration.html#the-custom-acceleration-profile).

Now, my gripe with `leetmouse` comes down to, mostly, skill issue. I eventually got it working, but it was still hard to change
parameters since I had to recompile the source code to do so.

My gripe with `libinput` is that while having a terrible user experience on par `leetmouse`, it is also impossible to express
certain curves. I even built a [cli tool](https://github.com/Gnarus-G/libinput-custom-points-gen) to generate the configuration that will approximate the curve I want.

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

If you need more context on this, refer to [libinput xorg configuration](https://wiki.archlinux.org/title/Libinput#Via_Xorg_configuration_file), and [libinput properties](https://man.archlinux.org/man/libinput.4#SUPPORTED_PROPERTIES).
I don't recommend using `libinput-points` to mimic Rawaccel's linear acceleration because, when I built it, I understood much less than I do now
about how the math works.

Note then, that my main goal, and only goal for the moment, is to replicate the linear type acceleration option from `Rawaccel`

### Solution: Home-made driver

So we need an easy install story, and a very easy configuration story.

I made my own driver, [maccel](https://github.com/Gnarus-G/maccel). It's easy enough for me to install. Sometimes a system might be missing some
dependencies and it can be a chore getting them; But past that hurdle is a decent enough cli tool with a user friendly Terminal UI.

```sh
maccel tui
```

![image](https://github.com/Gnarus-G/maccel/assets/37311893/dd62fc9a-3558-46a4-847e-05f691c31054)

## Install Dependencies

Make sure to have these dependencies installed on your machine:
`curl`, `git`, `make`, `gcc`, and the linux headers in `/lib/modules/`

### gcc

The version matters, it must match the version with which the kernel was built.

For example you might encounter such an error:

![image](https://github.com/Gnarus-G/maccel/assets/37311893/6147e20a-a132-4132-a45e-2af3dc035552)

And you'll have to find a version of `gcc` that matches. This will be more or less annoying
depending on your distro and/or how familiar you are with it.

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

On an arch based distro you list the available headers with

```
sudo pacman -Ss linux headers
```

## Uninstall

```sh
sudo sh /opt/maccel/uninstall.sh
```
