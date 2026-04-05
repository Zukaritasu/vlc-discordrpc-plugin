Name:           vlc-discordrpc-plugin
Version:        1.2.0
Release:        1%{?dist}
Summary:        Discord Rich Presence plugin for VLC

License:        GPLv2+
URL:            https://github.com/Zukaritasu/vlc-discordrpc-plugin
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  glibc-static
BuildRequires:  libstdc++-static
BuildRequires:  vlc-devel
BuildRequires:  cmake
BuildRequires:  pkg-config

Requires:       vlc

%description
This plugin integrates VLC Media Player with Discord Rich Presence, 
showing playback status, title, and progress directly in Discord.

%pre
if pgrep -x "vlc" > /dev/null; then
    echo "   error: VLC is running. Please close it before installing the plugin."
    exit 1
fi

%post
echo "Configuring vlc-discordrpc-plugin..."
		
if command -v vlc >/dev/null 2>&1; then
	vlc -I dummy --no-interact --reset-plugins-cache vlc://quit >/dev/null 2>&1 || true
	echo "   [+] VLC plugins cache updated successfully."
else
	echo "   [!] warning: VLC binary not found. The VLC plugins cache could not be updated."
	exit 1
fi

if [ -x /usr/bin/vlcrcedit ]; then
	/usr/bin/vlcrcedit --install
	if [ $? -eq 0 ]; then
		echo "   [+] Plugin configuration installed successfully."
	else
		exit 1
	fi
else
	echo "   [!] error: vlcrcedit could not be found during installation"
	exit 1
fi

%preun
if pgrep -x "vlc" > /dev/null; then
    echo "   error: VLC is running. Please close it before removing the plugin."
    exit 1
fi

echo "Removing vlc-discordrpc-plugin..."

if [ -x /usr/bin/vlcrcedit ]; then
	/usr/bin/vlcrcedit --uninstall
	if [ $? -eq 0 ]; then
		echo "   [-] Plugin configuration successfully removed."
	else
		echo "   [!] warning: Failed to remove plugin configuration. Please remove the plugin configuration manually from vlcrc file."
	fi
else
	echo "   [!] warning: vlcrcedit not found. Please remove the plugin configuration manually from vlcrc file."
fi
	
if command -v vlc >/dev/null 2>&1; then
	vlc -I dummy --no-interact --reset-plugins-cache vlc://quit >/dev/null 2>&1 || true
	echo "   [-] VLC plugins cache updated successfully."
else
	echo "   [!] warning: VLC binary not found. The VLC plugins cache could not be updated."
fi

%prep
%autosetup

%build
%cmake
%cmake_build

make -C nsis

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
 * Sat Apr 04 2026 Zukaritasu <zukaritasu@gmail.com> - 1.2.0
 - The static Rich Presence has been replaced with a user-customizable Rich Presence using tokens
 - In addition, several internal bugs were fixed.

 * Wed Mar 04 2026 Zukaritasu <zukaritasu@gmail.com> - 1.1.0
 - Initial package