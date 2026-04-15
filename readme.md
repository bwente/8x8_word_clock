# Word Clock Matrix (WLED Usermod)

An 8x8 LED matrix word clock usermod for [WLED](https://github.com/Aircoookie/WLED).

By [@bwente](https://github.com/bwente)

## Features

- Displays time as illuminated words (IT IS, HALF, QUARTER, etc.)
- Automatic day/night brightness adjustment
- Works with an 8x8 WS2812B matrix and ESP8266/ESP32
- Minimal hardware: just two components!

## Hardware Guide

See the full build tutorial on Hackster.io:  
https://www.hackster.io/bwente/word-clock-with-just-two-components-073834

## Installation

1. Copy `usermod_word_clock_matrix.h` to your WLED `usermods/` folder
2. Register the usermod in `usermods_list.cpp`
3. Build and flash WLED

## Files

| File | Description |
|------|-------------|
| `usermod_word_clock_matrix.h` | Main usermod code |
| `legacy/word-clock-matrix-overlay.cpp` | Preserved legacy overlay-based implementation |
| `word clock stencil.svg` | Laser cut stencil template |
| `Word Clock Baffle.stl` | 3D printable light baffle |

## Notes

- The header-based `usermod_word_clock_matrix.h` implementation is the main WLED usermod for this repo.
- The older overlay-based renderer is preserved under `legacy/` for reference and comparison.
- If you use the preset-based options, point them at existing WLED presets in your installation.

## Images

![image](https://user-images.githubusercontent.com/371964/197094071-f8ccaf59-1d85-4dd2-8e09-1389675291e1.png)

![image](https://user-images.githubusercontent.com/371964/197094211-6c736257-95ff-491f-9f0d-35d5135ecfea.png)

### Templates

![stencil](https://user-images.githubusercontent.com/371964/197094410-7c275f3f-743b-477a-bc15-5e7bdbcbd833.svg)

![box](https://user-images.githubusercontent.com/371964/197094496-fa49b355-164b-4bf5-84fd-f22f5206c645.svg)

## Legacy Version

Looking for the standalone (non-WLED) Arduino version? Check the [`legacy` branch](https://github.com/bwente/8x8_word_clock/tree/legacy).
