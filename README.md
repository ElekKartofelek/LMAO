# LMAO - Lightweight Multimedia & Audio Opener 🤣

A minimal media player with video trimming capability built with **Qt6** and **libmpv**.
Yeah, another player nobody asked for. Created mainly for my own peace of mind but I hope it might be of use to some of you as well <3

**Linux** and **Windows** are both supported. **MacOS** will be looked into in the near future.


## Built With

- [Qt6](https://www.qt.io/) - UI framework
- [libmpv](https://mpv.io/) - media playback engine
- [FFmpeg](https://ffmpeg.org/) - optional, for full-res screenshots and bitrate detection


## Features

- Lightweight and fast
- Basic keybinds
- Audio track selection for playback
- Edit mode - trim and export clips including all audio tracks **(Requires FFmpeg)**
- Full resolution frame capture to clipboard **(Requires FFmpeg)**
- Drag and drop file support
- Cross-platform - Linux and Windows
- Glossy but readable UI without heavy effects


## Installation

**Releases:**
- **Linux AppImage** - single file, runs on any distro. `chmod +x` and run.
- **Linux tar.gz** - for manual install. Requires `qt6-qtbase` and `mpv-libs`.
- **Windows (Lite/Full)** - extract and run `lmao.exe`.

**Fedora (COPR):**
```bash
sudo dnf copr enable elekkartofelek/LMAO
sudo dnf install lmao
```


## Which build should I get?

**Linux** has one version - FFmpeg is picked up from your system automatically.

To make sure all features work:
```bash
# Fedora
sudo dnf install ffmpeg-free

# Ubuntu/Debian
sudo apt install ffmpeg

# Arch
sudo pacman -S ffmpeg
```

Most distros include FFmpeg by default. Without it, LMAO works fine for playback but won't have edit mode, full resolution frame capture, or bitrate detection.


**Windows** comes in two versions:

| Feature | Lite | Full |
|---|---|---|
| Media playback | ✓ | ✓ |
| Edit mode (trim & export) | x | ✓ |
| Frame capture | visible size | full resolution |
| High bitrate detection | x | ✓ |
| Download size | ~150 MB | ~350 MB |

**Lite** - lightweight and fully functional for playback. Frame capture grabs what's on screen (go fullscreen for best quality). Edit mode is unavailable due to lack of export capability without FFmpeg.

**Full** - bundles FFmpeg and FFprobe. This enables Edit Mode, full resolution frame capture regardless of window size, and early detection of high bitrate files that might cause playback issues.


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