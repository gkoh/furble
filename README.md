# furble - FUjifilm Remote Bluetooth Low Energy

![PlatformIO CI](https://github.com/gkoh/furble/workflows/PlatformIO%20CI/badge.svg)

A Bluetooth wireless remote shutter release originally targeted at Fujifilm mirrorless
cameras.

![furble - M5StickC-Plus2](https://github.com/user-attachments/assets/e0eebd87-3ac0-4a8b-871a-014eb8c71395)

The remote uses the camera's native Bluetooth Low Energy interface so additional
adapters are not required.

furble is developed as a PlatformIO project for the M5StickC, M5StickC Plus and M5StickC Plus2
(ESP32 based devices). Additionally, it can be used on the M5Stack Core2.

## Supported Cameras

The following devices have actually been tested and confirmed to work:
- Fujifilm
   - Fujifilm GFX100II ([@matthudsonau](https://github.com/matthudsonau))
   - Fujifilm GFX100S ([@adrianuseless](https://github.com/adrianuseless))
   - Fujifiml X-H1
   - Fujifilm X-H2S ([@val123456](https://github.com/val123456))
   - Fujifilm X-S10 ([@dimitrij2k](https://github.com/dimitrij2k))
   - Fujifilm X-S20 ([@kelvincabaldo07](https://github.com/kelvincabaldo07))
   - Fujifilm X-T200 ([@Cronkan](https://github.com/Cronkan))
   - Fujifilm X-T30
   - Fujifilm X-T5 ([@stulevine](https://github.com/stulevine))
   - Fujifilm X100V
- Canon
   - Canon EOS M6 ([@tardisx](https://github.com/tardisx))
   - Canon EOS RP ([@wolcano](https://github.com/wolcano))
- Mobile Devices (beta)
   - Android
   - iOS

## What Works

Currently supported features in `furble`:
- scanning for supported cameras
- initial pairing
- saving pairing data
- connecting to previously paired camera
- shutter release
- focus
- GPS location tagging
- intervalometer
- multi-connect

### Table of Features

| &nbsp; | Fujifilm X & GFX | Canon EOS M6 | Canon EOS RP | Android & iOS |
| --- | :---: | :---: | :---: | :---: |
| Scanning & Pairing | ✔️ | ✔️ | ✔️ | ✔️ |
| Shutter Release | ✔️ | ✔️ | ✔️ | ✔️ |
| Focus | ✔️ (see [#99](https://github.com/gkoh/furble/discussions/99)) | :x: | :x: (see [#29](https://github.com/gkoh/furble/issues/29)) | :x: |
| GPS location tagging | ✔️ | :x: (WiFi only) | :x: | :x: |

## Installation

### Easy Install

The simplest way to get started is with the web installer.
Follow the instructions on the wiki: [Easy Web Install](https://github.com/gkoh/furble/wiki/Easy-Web-Install)

### PlatformIO

PlatformIO does everything assuming things are installed and connected properly.
In most cases it should be:
- clone the repository
- plug in the M5StickC or M5StickC Plus/Plus2
    - `platformio run -e m5stick-c -t upload`
- OR plug in the M5Stack Core2
    - `platformio run -e m5stack-core2 -t upload`

More details are on the wiki: [PlatformIO](https://github.com/gkoh/furble/wiki/Linux-Command-Line-(For-Developers))

## Usage

The top level menu has the following entries:
- `Connect` (if there are saved connections)
- `Scan`
- `Delete Saved` (if there are saved connections)
- `Settings`
- `Power Off`

On first use, put the target camera into pairing mode, then hit `Scan`. If the
camera advertises a known, matching signature, it should appear in the list.
You can then connect to the target camera, which, if successful, will save the
entry and show the remote menu.

Upon subsequent use it should be enough to hit `Connect`, selecting the
previously paired device and leads to the remote menu.

From the remote menu you may choose to disconnect or control the shutter.

### Mobile Devices

Android and iOS devices are supported. `furble` connects as a Bluetooth HID keyboard and sends the _Volume Up_ key stroke to trigger the shutter.
Connection to mobile devices is a little iffy:
- hit `Scan`
- on the mobile device:
   - pair with `furble`
- on `furble` the mobile device should appear as a connectable target if the pairing was successful
- connect to the mobile device to save the pairing
  - the devices will remain paired even if you do not connect and save
  - forget `furble` on the mobile device to remove such a pair

### GPS Location Tagging

[!WARNING] This unit is EOL, support for the replacement is pending (see #141).
For Fujifilm cameras, location tagging is supported with the M5Stack GPS unit:
- [Mini GPS/BDS Unit](https://shop.m5stack.com/products/mini-gps-bds-unit)

GPS support can be enabled in `furble` in `Settings->GPS`, the camera must also be configured to request location data.

### Intervalometer

The intervalometer can be configured via three settings in `Settings->Intervalometer`:
- Count (number of images to take)
- Delay (time between images)
- Shutter (time to keep shutter open)

Count can be configured up to 999 or infinite.
Delay and shutter time can be figured with custom or preset values from 0 to 999 in milliseconds, seconds or minutes.

### Shutter Lock

When in `Shutter` remote control, holding focus (button B) then release (button A) will engage shutter lock, holding the shutter open until a button is pressed.

### Multi-Connect

Multi-Connect enables simultaneous connection to multiple cameras to synchronise
remote shutter control. Up to 9 (ESP32 hardware limit) cameras can be
simultaneously controlled.

To use:
* Pair with one or more cameras
* Enable `Settings->Multi-Connect`
* In `Connect` select cameras
   * Selected cameras will have a `*`
* Select `Connect *`
   * Selected cameras will be connected in sequence
* If all cameras are connected, the standard remote control is shown

WARNING:
* mobile device connections are extremely finnicky
* multi-connect involving mobile devices is not well tested and can easily crash

### Infinite-ReConnect

This is useful for using furble as a passive, always on GPS data source.
With this, the camera will attempt to reconnect indefinitely.
You don't need to turn on this setting if you are actively using furble.

To use:
* Enable `Settings->Infinite-ReConnect`

[!WARNING] This will not be kind to battery life

## Motivation

I found current smartphone apps for basic wireless remote shutter control to be
generally terrible.
Research revealed the main alternative was attaching a dongle to the camera, of
which there were many options varying in price and quality.
I really just wanted the [Canon
BR-E1](https://www.eos-magazine.com/articles/remotes/br-e1-canon-bluetooth-remote.html),
but for my camera.

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

## Background Story

### Requirements

#### Hardware

I wanted a complete solution out of the box to have:
- bluetooth low energy
- physical button
- visual indicator (LED or display)
- battery
- case
- low cost

My search concluded with the [M5StickC](https://m5stack.com/products/stick-c)
from [M5Stack](https://m5stack.com).
The M5StickC and M5StickC Plus have since been EOL and replaced with the [M5StickC Plus2](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit).

The M5StickC is an ESP32 based mini-IoT development kit which covered all of the
requirements (and more). At time of writing, M5Stack sell the M5StickC for
US$9.95.
The M5StickC Plus sells for US$19.95.

#### Software

The project is built with [PlatformIO](https://platformio.org) and depends on
the following libraries:
- [M5ez](https://github.com/M5ez/M5ez)
  - severely butchered version to work on the M5StickC
- [M5Unified](https://github.com/m5stack/M5Unified)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)

# Known Issues
- depending on your perspective, battery life is anywhere from reasonable to abysmal
   - with an active BLE connection, the ESP32 consumes around 50mA
      - an M5StickC Plus2 would last around 4 hours
      - an M5StickC Plus would last around 2.5 hours
      - an old M5StickC would last less than 2 hours
   - if battery life is crucial, and form factor is not, consider an M5Stack Core2 with the 1500mAh module
      - this might last 30 hours

# Things To Do
- error handling is atrocious (it'll probably crash, then restart, which is OK,
  the M5StickC boots quickly)
- improve the device matching and connection abstractions
  - especially if more cameras get supported
- Support more camera makes and models
   - Complete support for newer Canon EOS (eg. RP)
   - Get access to and support the following:
     - Sony
     - Nikon
     - Others?

# Links

Inspiration for this project came from the following project/posts:
- https://github.com/hkr/fuji-cam-wifi-tool
- https://iandouglasscott.com/2017/09/04/reverse-engineering-the-canon-t7i-s-bluetooth-work-in-progress/

Related projects:
- https://github.com/ArthurFDLR/BR-M5
