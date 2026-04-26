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

%check
# No upstream tests available

%changelog
* Thu Apr 23 2026 Zukaritasu <zukaritasu@gmail.com> - 1.2.1-1
- Initial Release
