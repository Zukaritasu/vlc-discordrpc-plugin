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
    echo "ERROR: VLC is running. Please close it before installing the Discord RPC plugin."
    exit 1
fi

%post
echo "Configuring vlc-discordrpc-plugin..."
if command -v vlc >/dev/null 2>&1; then
    vlc -I dummy --no-interact --reset-plugins-cache vlc://quit >/dev/null 2>&1 || true
    echo "VLC plugin cache updated successfully."
else
    echo "Warning: VLC binary not found. Please ensure VLC is installed to use this plugin."
fi

%preun
if pgrep -x "vlc" > /dev/null; then
    echo "VLC is running. Please close it before removing the Discord RPC plugin."
    exit 1
fi

echo "Cleaning up Discord RPC plugin traces from user configurations..."

for vlcrc in /home/*/.config/vlc/vlcrc; do
    if [ -f "$vlcrc" ]; then 
        sed -i '/^\[discord_rpc\]/,/^\[/ { /^\[discord_rpc\]/d; /^\[/!d; }' "$vlcrc" 
        sed -i 's/discord_rpc//g' "$vlcrc" 
        sed -i 's/,,/,/g; s/control=,/control=/g; s/,$//g' "$vlcrc"
    fi
done

if command -v vlc >/dev/null 2>&1; then
    echo "Cleaning up VLC plugin cache before removal..."
    vlc -I dummy --no-interact --reset-plugins-cache vlc://quit >/dev/null 2>&1 || true
    echo "VLC plugin cache cleaned."
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