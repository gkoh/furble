# furble - FUjifilm Remote Bluetooth Low Energy

![PlatformIO CI](https://github.com/gkoh/furble/workflows/PlatformIO%20CI/badge.svg)

A wireless remote shutter release originally targeted at Fujifilm mirrorless
cameras.

The remote uses the camera's native Bluetooth Low Energy interface so additional
adapters are not required.

furble is developed as a PlatformIO project for the M5StickC and M5StickC Plus
(ESP32 based devices). Additionally, it can be used on the M5Stack Core2.

## Supported Cameras

The following devices have actually been tested and confirmed to work:
- Fujifilm GFX100S ([@adrianuseless](https://github.com/adrianuseless))
- Fujifilm X-H2S ([@val123456](https://github.com/val123456))
- Fujifilm X-S10 ([@dimitrij2k](https://github.com/dimitrij2k))
- Fujifilm X-T200 ([@Cronkan](https://github.com/Cronkan))
- Fujifilm X-T30
- Fujifilm X-T5 ([@stulevine](https://github.com/stulevine))
- Fujifilm X100V
- Canon EOS M6 ([@tardisx](https://github.com/tardisx))
- Canon EOS RP ([@wolcano](https://github.com/wolcano))

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

#### Fujifilm
Given reports from the community and access to additional cameras, it
seems many (all?) Fujifilm cameras use the same Bluetooth protocol.
Reports of further confirmed working Fujifilm cameras are welcome.

#### Canon
With access to a Canon EOS M6, I was able to implement support for it. Other
Canon cameras might work, but I suspect the shutter control protocol will be
different.
@wolcano kindly implemented support for the Canon EOS RP.

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
The M5StickC has since been EOL and replaced with the [M5StickC Plus](https://shop.m5stack.com/collections/stick-series/products/m5stickc-plus-esp32-pico-mini-iot-development-kit).

The M5StickC is an ESP32 based mini-IoT development kit which covered all of the
requirements (and more). At time of writing, M5Stack sell the M5StickC for
US$9.95.
The M5StickC Plus sells for US$19.95.

### Software

The project is built with [PlatformIO](https://platformio.org) and depends on
the following libraries:
- [Battery Sense](https://github.com/rlogiacco/BatterySense)
  - only the voltage to capacity function is used
- [M5ez](https://github.com/M5ez/M5ez)
  - severely butchered version to work on the M5StickC
- [M5StickC](https://github.com/m5stack/M5StickC)
- [M5StickC Plus](https://github.com/m5stack/M5StickC-Plus)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)

## Installation

### Easy Install

The simplest way to get started is using `esphome-flasher` and the release binaries.
Follow the instructions on the wiki: [Easy Install](https://github.com/gkoh/furble/wiki/Easy-Install)

### PlatformIO

PlatformIO does everything assuming things are installed and connected properly.
In most cases it should be:
- clone the repository
- plug in the M5StickC
    - `platformio run -e m5stick-c -t upload`
- OR plug in the M5StickC Plus
    - `platformio run -e m5stick-c-plus -t upload`
- OR plug in the M5Stack Core2
    - `platformio run -e m5stack-core2 -t upload`

More details are on the wiki: [PlatformIO](https://github.com/gkoh/furble/wiki/Linux-Command-Line-(For-Developers))

## Usage

The top level menu has the following entries:
- `Connect`
- `Scan`
- `Delete Saved`
- `Settings`
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

# Links

Inspiration for this project came from the following project/posts:
- https://github.com/hkr/fuji-cam-wifi-tool
- https://iandouglasscott.com/2017/09/04/reverse-engineering-the-canon-t7i-s-bluetooth-work-in-progress/

Related projects:
- https://github.com/ArthurFDLR/BR-M5
