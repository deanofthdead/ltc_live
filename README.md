# ltc_live

A **lightweight LTC (Linear Time Code) decoder** for macOS, Linux, and Windows (via PortAudio + libltc).  
It captures audio from a selected input device and decodes SMPTE LTC timecode in **real-time**.

## Features
- **List all input audio devices** with `--list`
- **Select a device by index** with `--device N`
- **Decode LTC in real-time** and print frames as `LTC: HH:MM:SS:FF`
- Lightweight: **PortAudio + libltc**, no external dependencies beyond these.

## Installation

### **Dependencies**
You need:
- [libltc](https://github.com/x42/libltc)
- [PortAudio](http://www.portaudio.com/)

**macOS (Homebrew):**
```bash
brew install libltc portaudio
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt install libltc-dev portaudio19-dev
```

## Build
```bash
make
```
This compiles `ltc_live` into a standalone binary.

## Usage
**List available input devices:**
```bash
./ltc_live --list
```

**Run LTC decoding from a device:**
```bash
./ltc_live --device 2
```
Where `2` is the index of the desired input device.

## Example Output
```
Listening for LTC on device 2: BlackHole 2ch
Press Ctrl+C to stop.
LTC: 11:34:12:13
LTC: 11:34:12:14
LTC: 11:34:12:15
```

## License
MIT — free to use, modify, and share.
