Name:           vlc-discordrpc-plugin
Version:        1.2.1
Release:        1%{?dist}
Summary:        Discord Rich Presence plugin for VLC

License:        GPL-2.0-or-later
URL:            https://github.com/Zukaritasu/%{name}
Source0:        https://github.com/Zukaritasu/%{name}/archive/refs/tags/%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  vlc-devel >= 3.0.0
BuildRequires:  cmake
BuildRequires:  pkg-config

Requires:       vlc >= 3.0.0

%description
This plugin integrates VLC Media Player with Discord Rich Presence, 
showing playback status, title, and progress directly in Discord.

%post
if [ $1 -gt 1 ] ; then
    echo "======================================================================="
	echo " Plugin installed! To enable it in your VLC, run the following command"
	echo " as your regular user in the terminal:"
	echo "    vlcrcedit --install"
	echo "======================================================================="
fi

%prep
%setup -q -n %{name}

%build
%cmake
%cmake_build

g++ %{optflags} %{build_ldflags} nsis/vlcrcedit.cpp -o nsis/vlcrcedit

%install
%cmake_install

mkdir -p %{buildroot}%{_mandir}/man1
install -m 644 docs/vlcrcedit.1 %{buildroot}%{_mandir}/man1/vlcrcedit.1
install -D -m 755 nsis/vlcrcedit %{buildroot}%{_bindir}/vlcrcedit

%files
%license LICENSE
%{_bindir}/vlcrcedit
%{_mandir}/man1/vlcrcedit.1*

%doc README.md
%{_libdir}/vlc/plugins/misc/libdiscordrpc_plugin.so

%changelog
 * Tue Apr 21 2026 Zukaritasu <zukaritasu@gmail.com> - 1.2.1-1
 - An issue with configuring the plugin on systems such as Debian/Ubuntu
   and Fedora during installation or uninstallation has been resolved.

 * Sat Apr 04 2026 Zukaritasu <zukaritasu@gmail.com> - 1.2.0-1
 - The static Rich Presence has been replaced with a user-customizable Rich Presence using tokens
 - In addition, several internal bugs were fixed.

 * Wed Mar 04 2026 Zukaritasu <zukaritasu@gmail.com> - 1.1.0-1
 - Initial package