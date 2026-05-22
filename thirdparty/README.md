\# Third-party dependencies (Windows only)



On Linux, install dependencies via package manager. On Windows, place the following here:



\---------------------------------------------------------------------



\## mpv SDK (required)

Download from: https://sourceforge.net/projects/mpv-player-windows/files/libmpv/

Get: `mpv-dev-x86\_64-20XXXXXX-git-XXXXXXX.7z` (non-v3 build)



Extract into `thirdparty/mpv/` so the structure is:



thirdparty/mpv/

├── include/

│   └── mpv/

│       ├── client.h

│       ├── render.h

│       └── render\_gl.h

├── libmpv.dll.a

└── libmpv-2.dll



\---------------------------------------------------------------------



\## ffmpeg/ffprobe (optional)

Download from: https://github.com/BtbN/FFmpeg-Builds/releases

Get: `ffmpeg-master-latest-win64-gpl.zip` (static build, NOT shared)



Place in `thirdparty/`:



thirdparty/

├── ffmpeg.exe

└── ffprobe.exe



These enable:

\- `ffprobe`: High bitrate file detection before loading

\- `ffmpeg`: Full resolution screenshots to clipboard



Without them the app still works — bitrate check is skipped and screenshots use the current window resolution instead.

