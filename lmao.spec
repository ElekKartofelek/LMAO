Name:           lmao
Version:        1.1.0
Release:        1%{?dist}
Summary:        Lightweight Multimedia & Audio Opener
License:        GPL-2.0-or-later
URL:            https://github.com/ElekKartofelek/LMAO
Source0:        %{url}/archive/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  mpv-devel
BuildRequires:  dbus-devel

Requires:       qt6-qtbase
Requires:       mpv-libs

# Optional: full-res screenshots and bitrate detection
Recommends:     ffmpeg-free

%description
A minimal, media player with video trimming capability built with Qt6 and libmpv.
Yeah, another player nobody asked for.
Created mainly for my own peace of mind but I hope it might be of use to some of you as well <3

%prep
%autosetup -n LMAO-%{version}

%build
%cmake
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/lmao
%{_datadir}/applications/dev.elek.LMAO.desktop
%{_datadir}/metainfo/dev.elek.LMAO.metainfo.xml
%{_datadir}/icons/hicolor/16x16/apps/dev.elek.LMAO.png
%{_datadir}/icons/hicolor/32x32/apps/dev.elek.LMAO.png
%{_datadir}/icons/hicolor/48x48/apps/dev.elek.LMAO.png
%{_datadir}/icons/hicolor/64x64/apps/dev.elek.LMAO.png
%{_datadir}/icons/hicolor/128x128/apps/dev.elek.LMAO.png
%{_datadir}/icons/hicolor/256x256/apps/dev.elek.LMAO.png
%{_datadir}/icons/hicolor/512x512/apps/dev.elek.LMAO.png

%changelog
* Sun May 25 2026 ElekKartofelek <elek@users.noreply.github.com> - 1.1.0-1
- Edit Mode - trim and export (requires FFmpeg)
- Glossy UI overhaul with directional lighting
- Layout shows only the most important elements at small window width
- Keybinds panel - view only, not reprogrammable (yet?)

* Fri May 22 2026 ElekKartofelek <elek@users.noreply.github.com> - 1.0.0-1
- Initial release
