# oldabi-c

A reimplementation of [OldABI](https://github.com/tealbathingsuit/OldABI/) by Evelyn in C. This project aims to act as a compatibility layer for tweaks compiled with the legacy arm64e ABI.

Built for iOS 15+.

More information is available here: https://github.com/NightwindDev/Tweak-Tutorial/blob/main/oldabi.md

### How to install
Head over to the [Releases](https://github.com/NightwindDev/oldabi-c/releases) section and download the correct `.deb` for your device.

### How to compile manually
Make sure you have [Theos](https://github.com/theos/theos) installed and configured.

Clone the repo and run the command you need:
```bash
# To build for rootful
make clean package FINALPACKAGE=1
# To build for rootless
make clean package THEOS_PACKAGE_SCHEME=rootless FINALPACKAGE=1
```

#### License
This project is licensed under [MIT](LICENSE).

###### Copyright (c) 2026 Nightwind