Name:           vlc-discordrpc-plugin
Version:        1.2.0
Release:        2%{?dist}
Summary:        Discord Rich Presence plugin for VLC

License:        GPL-2.0-or-later
URL:            https://github.com/Zukaritasu/vlc-discordrpc-plugin
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  vlc-devel
BuildRequires:  cmake
BuildRequires:  pkg-config

Requires:       vlc

%description
This plugin integrates VLC Media Player with Discord Rich Presence, 
showing playback status, title, and progress directly in Discord.

%prep
%autosetup

%build
%cmake
%cmake_build

g++ %{optflags} %{build_ldflags} nsis/vlcrcedit.cpp -o nsis/vlcrcedit

%install
%cmake_install

install -D -m 0755 nsis/vlcrcedit %{buildroot}%{_bindir}/vlcrcedit

%files
%license LICENSE
%{_bindir}/vlcrcedit

%doc README.md
%doc INSTALL.md
%{_libdir}/vlc/plugins/misc/libdiscordrpc_plugin.so

%changelog
 * Sun Apr 13 2026 Zukaritasu <zukaritasu@gmail.com> - 1.2.0-2
 - Fixed packaging issues.

 * Sat Apr 04 2026 Zukaritasu <zukaritasu@gmail.com> - 1.2.0
 - The static Rich Presence has been replaced with a user-customizable Rich Presence using tokens
 - In addition, several internal bugs were fixed.

 * Wed Mar 04 2026 Zukaritasu <zukaritasu@gmail.com> - 1.1.0
 - Initial package