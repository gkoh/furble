# furble - ***FU***jifilm ***R***emote ***B***luetooth ***L***ow ***E***nergy

![PlatformIO CI](https://github.com/gkoh/furble/workflows/PlatformIO%20CI/badge.svg)

A Bluetooth wireless remote shutter release originally targeted at Fujifilm mirrorless
cameras. furble now supports:
- Fujifilm
- Canon
- Nikon
- Sony

The remote uses the camera's native Bluetooth Low Energy interface thus additional
adapters are not required.

furble is developed as a PlatformIO project.

## M5StickC Plus2 (Dark theme)

![furble - Dark - M5StickC Plus2](https://github.com/user-attachments/assets/62a507cc-1c75-4d7a-a8de-b9e0bf78e0e1)

## M5Core2 (Default theme)

![furble - Default - M5Core2](https://github.com/user-attachments/assets/6c2d3293-201f-4330-a7c0-716c91807dca)

## Supported Cameras

The following devices have been tested and confirmed to work:
- Fujifilm
   - Fujifilm GFX100 II ([@matthudsonau](https://github.com/matthudsonau))
   - Fujifilm GFX100S ([@adrianuseless](https://github.com/adrianuseless))
   - Fujifilm GFX50S II ([@TomaszLojewski](https://github.com/TomaszLojewski))
   - Fujifilm X-E4 ([@Rediwed](https://github.com/Rediwed))
   - Fujifiml X-H1
   - Fujifilm X-H2S ([@val123456](https://github.com/val123456))
   - Fujifilm X-S10 ([@dimitrij2k](https://github.com/dimitrij2k))
   - Fujifilm X-S20 ([@kelvincabaldo07](https://github.com/kelvincabaldo07))
   - Fujifilm X-T200 ([@Cronkan](https://github.com/Cronkan))
   - Fujifilm X-T3 ([@ubuntuproductions](https://github.com/ubuntuproductions))
   - Fujifilm X-T30
   - Fujifilm X-T4 ([@TomaszLojewski](https://github.com/TomaszLojewski))
   - Fujifilm X-T5 ([@stulevine](https://github.com/stulevine))
   - Fujifilm X100V
- Canon
   - Canon EOS M6 ([@tardisx](https://github.com/tardisx))
   - Canon EOS RP ([@wolcano](https://github.com/wolcano))
   - Canon EOS R6 Mark II ([@hijae](https://github.com/hijae))
- Nikon
   - Nikon COOLPIX B600
   - Nikon Z6 III ([@herrfrei](https://github.com/herrfrei))
- Sony
   - Sony ZV-1F
- Mobile Devices (beta)
   - Android
   - iOS

## Supported Controllers

Initially targeted at the M5StickC, the following controllers are supported:
* M5StickC (EOL)
* M5StickC Plus
* M5StickC Plus2
* M5Core Basic
* M5Core2

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

| Camera | Scanning & Pairing | Shutter Release | Focus  | GPS     |
| :---:  | :---:              | :---:           | :---:  | :---:   |
| Fujifilm X & GFX  | ✔️      | ✔️              | ✔️[^1] | ✔️      |
| Canon EOS M6      | ✔️      | ✔️              | :x:    | :x:[^2] |
| Canon EOS R       | ✔️      | ✔️              | ✔️     | ✔️      |
| Nikon COOLPIX & Z | ✔️      | ✔️              | :x:    | :x:     |
| Sony ZV           | ✔️      | ✔️              | ✔️     | ✔️      |

[^1]: see [#99](https://github.com/gkoh/furble/discussions/99)
[^2]: WiFi only

## Installation

### Easy Install

The simplest way to get started is with the web installer.
Follow the instructions on the wiki: [Easy Web Install](https://github.com/gkoh/furble/wiki/Easy-Web-Install)

### PlatformIO

PlatformIO does everything assuming things are installed and connected properly.
In most cases it should be:
- clone the repository
- plug in the M5StickC
    - `platformio run -e m5stick-c -t upload`
- OR plug in M5StickC Plus/Plus2
    - `platformio run -e m5stick-c-plus -t upload`
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

`furble` will identify as `furble-xxxx` where `xxxx` is a consistent identifier enabling one to differentiate mutiple controllers.

Upon subsequent use it should be enough to hit `Connect`, selecting the
previously paired device and leading to the remote menu.

From the remote menu you may choose to disconnect, control the shutter or activate the intervalometer.

### Navigation

#### Hardware Buttons

All supported controllers have three buttons designated:
- previous
- select
- next

For M5StickC format controllers:
- previous == power button
- select == big M5 button below screen
- next == right side button

For M5 Core controllers:
- previous == left button
- select == middle button
- next == right button

Basic navigation:
* previous == highlight up/previous/right entry
* next == highlight down/next/left entry
* select == action

For slider and roller elements (eg. brightness control, intervalometer numbers):
* highlight the desired element with previous/next
* press select to 'edit' the element
* press next/previous to up/down increase/decrease
* press select to 'confirm' the change

#### Touch Screen

At time of writing, only the M5Core2 has been tested. All user interface widgets are touch responsive:
* in menus
   * touch the desired entry
   * drag up/down to scroll entries
* in rollers (ie intervalometer configuration)
   * drag up/down to adjust entry

For M5Core2, the 'hardware' touch buttons are active and operate identically to the M5 Core.

During shutter control:
* press 'Shutter' button to release shutter
* press and hold 'Shutter' button for continuous shooting
* press and/or hold 'Focus' button to auto-focus
* press and hold 'Shutter Lock' to lock shutter open
   * press and hold 'Shutter Lock' again to release

To activate screen lock:
* double click the PWR button
   * a message box will open
* double click the PWR button to unlock

### Menu Map

* Connect (if connections are saved)
   * list of connections
      * Shutter
         * Shutter
         * Focus
         * Shutter Lock
      * Interval
         * Start
            * Stop
         * Count
         * Delay
         * Shutter
      * Disconnect
* Scan
   * list of matching targets
      * <goes to active single connection>
* Delete (if connections are saved)
   * list of connections
* Settings
   * Backlight
      * Backlight brightness
      * Inactivity timeout
   * Features
      * Faux-NY
      * Infinite-Reconnect
      * Multi-Connect
   * GPS
      * GPS
      * GPS Data (if GPS enabled)
   * Intervalometer
      * Count
      * Delay
      * Shutter
   * Theme
   * Transmit Power
   * About
* Power Off

### Mobile Devices

Android and iOS devices are supported. `furble` connects as a Bluetooth HID keyboard and sends the _Volume Up_ key stroke to trigger the shutter.
Connection to mobile devices is a little iffy:
- hit `Scan`
- on the mobile device:
   - pair with `furble`
- on `furble` the mobile device should appear as a connectable target if the pairing was successful
- forget `furble` on the mobile device to remove such a pair

### GPS Location Tagging

For Fujifilm & Sony cameras, location tagging is supported with the M5Stack GPS unit:
- [GPS/BDS Unit v1.1 (AT6668)](https://shop.m5stack.com/products/gps-bds-unit-v1-1-at6668)

The previous unit is now EOL:
- [Mini GPS/BDS Unit](https://shop.m5stack.com/products/mini-gps-bds-unit)

GPS support can be enabled in `furble` in `Settings->GPS`, the camera must also be configured to request location data.

The default baud rate for the GPS unit is 9600.
The new v1.1 unit runs at a higher baud rate and must be configured under
`Settings->GPS->GPS baud 115200` for correct operation.

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
* Enable `Settings->Features->Multi-Connect`
* In `Connect` select one or more cameras
* Select `Multi-Connect`
   * Selected cameras will be connected in sequence
* If all cameras are connected, the standard remote control is shown

> [!CAUTION]
> * mobile device connections are extremely finnicky
> * multi-connect involving mobile devices is not well tested and can easily crash
>   * multi-connect involving mobile devices is not recommended

### Infinite-ReConnect

This is useful for using furble as a passive, always on GPS data source.
With this, the camera will attempt to reconnect indefinitely.
You don't need to turn on this setting if you are actively using the remote.

To use:
* Enable `Settings->Features->Infinite-ReConnect`

> [!WARNING]
> This will not be kind to battery life

### Themes

A few basic themes are included, to change:
* `Settings->Themes-><desired theme>`
   * hit 'Restart' to save and restart for the theme to take effect
   * better dynamic theme change support is improving in upstream LVGL

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
@wolcano kindly implemented initial support for the Canon EOS RP.
@hijae kindly helped with better Canon EOS R support.

#### Nikon

Nikon cameras that support the remote wireless controller (ML-L7) should work,
use the "Connection to remote" menu option.
This has been tested on a Nikon COOLPIX B600. Unfortunately, the remote wireless
mode has no support for GPS or focus functions, thus only shutter release works.
Note that other Nikon cameras will appear in the scan, but will not pair
(further support is under investigation).
@herrfrei kindly assisted with Z6 III support.

#### Sony

Sony cameras appear to use a reasonably uniform and robust bluetooth control
protocol. Most modern Sony cameras should be supported. Testing was performed
on Sony ZV-1F.

To pair with a Sony camera (some models may have different menu options, the
following matches the ZV-1F):
- set 'Bluetooth Rmt Ctrl' to 'On'
- set 'Bluetooth Function' 'On'
- under Bluetooth, start 'Pairing'
- start 'Scan' with `furble'
   - due to an oddity with the Bluetooth library, if `furble` 'Scan' is started
     first, the Sony camera may not appear

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
The M5StickC Plus(2) sells for US$19.95.

#### Software

The project is built with [PlatformIO](https://platformio.org) and depends on
the following libraries:
- [LVGL](https://github.com/lvgl/lvgl)
- [M5Unified](https://github.com/m5stack/M5Unified)
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
- [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)

# Known Issues

- depending on your perspective, battery life is anywhere from reasonable to bad
   - with an active BLE connection and power management, the ESP32 consumes around 30mA
      - an M5StickC Plus2 would last around 6 hours
      - an M5StickC Plus would last around 4 hours
      - an old M5StickC would last around 3 hours
   - if battery life is crucial, and form factor is not, consider an M5Stack Core with the 1500mAh module
      - this might last 50 hours

# Things To Do

- error handling is ~atrocious~improving (it might crash, then restart, which is OK,
  the M5StickC boots quickly)
- improve the device matching and connection abstractions
  - especially if more cameras get supported
- Support more camera makes and models
   - Get access to and support the following:
     - Nikon Z
     - Others?

# Links

Inspiration, references and related information for this project came from the following projects/posts:
- Canon
   - https://iandouglasscott.com/2017/09/04/reverse-engineering-the-canon-t7i-s-bluetooth-work-in-progress/
   - https://github.com/ArthurFDLR/BR-M5
   - https://github.com/RReverser/eos-remote-web
- Fujifilm
  - https://github.com/hkr/fuji-cam-wifi-tool
- Sony
   - https://gethypoxic.com/blogs/technical/sony-camera-ble-control-protocol-di-remote-control
   - https://gregleeds.com/reverse-engineering-sony-camera-bluetooth
   - https://github.com/Staacks/alpharemote
