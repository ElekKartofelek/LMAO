# LMAO - Lightweight Multimedia & Audio Opener 🤣

A minimal, media player built with **Qt6** and **libmpv**.
Yes, another one. Mainly for my own peace of mind but I see no reason not to share it with everyone. 

Linux and Windows are both supported. MacOS will be looked into in the near future.


## Built With

- [Qt6](https://www.qt.io/) - UI framework
- [libmpv](https://mpv.io/) - media playback engine
- [FFmpeg](https://ffmpeg.org/) - optional, for full-res screenshots and bitrate detection


## Installation

**Releases:**
- **Linux AppImage** - single file, runs on any distro. `chmod +x` and run.
- **Linux tar.gz** - for manual install. Requires `qt6-qtbase` and `mpv-libs`.
- **Windows** - extract and run `lmao.exe`.
- **Windows (full)** - includes ffmpeg and ffprobe for full-resolution screenshots and bitrate detection.

**Fedora (COPR):**
```bash
sudo dnf copr enable elekkartofelek/LMAO
sudo dnf install lmao
```


## Controls

| Input               | Action                         |
|---------------------|--------------------------------|
| Space               | Play / Pause                   |
| .                   | Frame step forward             |
| ,                   | Frame step backward            |
| ← / →               | Seek ±5 seconds                |
| ↑ / ↓               | Volume ±5                      |
| Scroll wheel        | Volume up / down               |
| F / Double-click    | Toggle fullscreen              |
| M                   | Mute / Unmute                  |
| S                   | Copy frame to clipboard        |
| L                   | Toggle loop                    |
| I                   | Toggle file info overlay       |
| O                   | Options panel                  |
| Ctrl+O              | Open file dialog               |
| + / −               | Playback speed ±0.25x          |
| F1                  | About                          |
| Escape              | Exit fullscreen                |
| Right-click         | Context menu                   |


## Which build should I get?

LMAO has an option to copy video frame to clipboard but it needs ffmpeg to get a full resolution frame. Otherwise it fallbacks to the exact size you see on your screen (can be worked around by resizing window or going fullscreen).

Additionally FFprobe is used for a rare edge case where insanely heavy high bitrate files are being thrown in. If FFprobe is available it reads the bitrate beforehand and warns user providing additional options before lag occurs.

I realise though these two edge cases aren't always worth 500mb download. Base app is perfectly fine.

This only concerns Windows builds. Linux builds can use system installed FFmpeg and FFprobe.


## Building

### Linux (Fedora)

```bash
sudo dnf install qt6-qtbase-devel qt6-qtbase-private-devel mpv-devel cmake gcc-c++
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Linux (Install)

```bash
sudo cmake --install .
sudo gtk-update-icon-cache -f /usr/local/share/icons/hicolor/
sudo update-desktop-database /usr/local/share/applications/
```

### Windows

Requires Qt6 (MinGW) and mpv SDK. See [thirdparty/README.md](thirdparty/README.md).

```cmd
set PATH=C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\CMake_64\bin;%PATH%
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/mingw_64"
mingw32-make -j8
```

## License

GPL-2.0-or-later — see [LICENSE](LICENSE)
