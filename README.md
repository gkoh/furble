# furble - FUjifilm Remote Bluetooth Low Energy

![PlatformIO CI](https://github.com/gkoh/furble/workflows/PlatformIO%20CI/badge.svg)

A wireless remote shutter release originally targeted at Fujifilm mirrorless
cameras.

The remote uses the camera's native Bluetooth Low Energy interface so additional
adapters are not required.

furble is developed as a PlatformIO project for the M5StickC (an ESP32 based
device).

## Supported Cameras

The following devices have actually been tested:
- Fujifilm X-T30
- Canon EOS M6

## Motivation

I found current smartphone apps for basic wireless remote shutter control to be
generally terrible.
Research revealed the main alternative was attaching a dongle to the camera, of
which there were many options varying in price and quality.
I really just wanted the [Canon
BR-E1](https://www.eos-magazine.com/articles/remotes/br-e1-canon-bluetooth-remote.html),
but for my camera.

## What Works

- scanning for supported cameras
- initial pairing
- saving pairing data
- connecting to previously paired camera
- shutter release

### Possibly Supported Cameras

Given the similarities of the Fujifilm X-T3 and X-T30, I'd suspect it may 'just
work', but I do not have access to an X-T3 to confirm.
Other Fujifilm cameras may also work, but without access to such devices I
really do not know.

With access to a Canon EOS M6, I was able to implement support for it. Other
Canon cameras might work, but I suspect the shutter control protocol will be
different.

#### Protocol Reverse Engineering

Android supports snooping bluetooth traffic so it was trivial to grab a HCI log
to see what the manufacturer supplied camera app was doing.

For all supported cameras, a snoop log of:
- scanning
- pairing
- re-pairing
- shutter release

was analysed with Wireshark.

It was then an experiment in reducing the interaction to the bare minimum just
to trigger the shutter release.

### Supporting More Cameras

The best way is to repeat the previous steps, analyse the bluetooth HCI snoop
log with Wireshark, implement, then test against the actual device.

## Requirements

### Hardware

I wanted a complete solution out of the box to have:
- bluetooth low energy
- physical button
- visual indicator (LED or display)
- battery
- case
- low cost

My search concluded with the [M5StickC](https://m5stack.com/products/stick-c)
from [M5Stack](https://m5stack.com).

The M5StickC is an ESP32 based mini-IoT development kit which covered all of the
requirements (and more). At time of writing, M5Stack sell the M5StickC for
US$9.95.

### Software

The project is built with [PlatformIO](https://platformio.org) and depends on
the following libraries:
- [Battery Sense](https://github.com/rlogiacco/BatterySense)
  - only the voltage to capacity function is used
- [M5ez](https://github.com/M5ez/M5ez)
  - severely butchered version to work on the M5StickC
- [M5StickC](https://github.com/m5stack/M5StickC)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)

## Installation

PlatformIO does everything assuming things are installed and connected properly.
In most cases it should be:
- clone the repository
- plug in the M5StickC
- `platformio run -t upload`

## Usage

The top level menu has the following entries:
- `Connect`
- `Scan`
- `Delete Saved`
- `Power Off`

On first use, put the target camera into pairing mode, then hit `Scan`. If the
camera advertises a known, matching signature, it should appear in the list.
You can then connect to the target camera, which, if successful, will save the
entry and show the remote menu.

Upon subsequent use it should be enough to hit `Connect`, selecting the
previously paired device and leads to the remote menu.

From the remote menu you may choose to disconnect or control the shutter.

# Things To Do
- error handling is atrocious (it'll probably crash, then restart, which is OK,
  the M5StickC boots quickly)
- improve the device matching and connection abstractions
  - especially if more cameras get supported
- get some continuous integration going

# Links

Inspiration for this project came from the following project/posts:
- https://github.com/hkr/fuji-cam-wifi-tool
- https://iandouglasscott.com/2017/09/04/reverse-engineering-the-canon-t7i-s-bluetooth-work-in-progress/
