Name:           vlc-discordrpc-plugin
Version:        1.1.0
Release:        1%{?dist}
Summary:        Discord Rich Presence plugin for VLC

License:        GPLv2+
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

%pre
if pgrep -x "vlc" > /dev/null; then
    echo "   \033[0;31merror: VLC is running. Please close it before installing the Discord RPC plugin.\033[0m"
    exit 1
fi

%post
echo "Configuring vlc-discordrpc-plugin..."

if command -v vlc >/dev/null 2>&1; then
	vlc -I dummy --no-interact --reset-plugins-cache vlc://quit >/dev/null 2>&1 || true
	echo "   \033[0;32m[+]\033[0m VLC plugins cache updated successfully."
else
	echo "   \033[0;33m[!]\033[0m warning: VLC binary not found. Please ensure VLC is installed to use this plugin."
fi

if [ -x /usr/bin/vlcrcedit ]; then
	/usr/bin/vlcrcedit --install >/dev/null 2>&1 || true
	echo "   \033[0;32m[+]\033[0m VLC vlcrc file edit updated successfully."
else
	echo "   \033[0;33m[!]\033[0m warning: vlcrcedit not found. Please ensure vlcrcedit is installed to update VLC vlcrc file."
fi

%preun
if pgrep -x "vlc" > /dev/null; then
    echo "   \033[0;31merror: VLC is running. Please close it before removing the Discord RPC plugin.\033[0m"
    exit 1
fi

echo "Removing vlc-discordrpc-plugin..."

if [ -x /usr/bin/vlcrcedit ]; then
	/usr/bin/vlcrcedit --uninstall >/dev/null 2>&1 || true
	echo "   \033[0;32m[-]\033[0m VLC vlcrc file edit updated successfully."
else
	echo "   \033[0;33m[!]\033[0m warning: vlcrcedit not found. Please ensure vlcrcedit is installed to update VLC vlcrc file."
fi
	
if command -v vlc >/dev/null 2>&1; then
	vlc -I dummy --no-interact --reset-plugins-cache vlc://quit >/dev/null 2>&1 || true
	echo "   \033[0;32m[-]\033[0m VLC plugins cache cleaned."
else
	echo "   \033[0;33m[!]\033[0m warning: VLC binary not found. The VLC plugins cache may not be updated."
fi

%prep
%autosetup

%build
%cmake
%cmake_build

%install
%cmake_install

%files
%license LICENSE

%doc README.md
%{_libdir}/vlc/plugins/misc/libdiscordrpc_plugin.so

%changelog
 * Wed Mar 04 2026 Zukaritasu <zukaritasu@gmail.com> - 1.1.0
 - Initial package