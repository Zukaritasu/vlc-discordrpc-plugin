COMPILATION AND INSTALLATION

This file describes step-by-step how to compile and install the `vlc-discordrpc-plugin` on Windows and Linux.

---

## WINDOWS (Using MSYS2 / MinGW64)

To compile on Windows, we recommend using **MSYS2** with the **MinGW64** environment.

### 1. Prerequisites
1. Download and install **MSYS2** from [msys2.org](https://www.msys2.org/).
2. Open the **MSYS2 MinGW64** terminal (not the normal MSYS terminal).
3. Update the packages:
   ```bash
   pacman -Syu
   ```
4. Install the necessary build tools (CMake, GCC, Make, Git) and the **VLC development package**:
   ```bash
   pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-vlc make git
   ```

### 2. Compilation
From the MSYS2 terminal, navigate to the project folder and run:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
This will generate the project in the `build/` directory and compile the plugin.

### 3. Installation
To install the plugin automatically to your VLC plugins folder (requires running MSYS2 as **Administrator** if VLC is installed in `C:\Program Files`):

```bash
cmake --install build
```
This will copy `libdiscordrpc_plugin.dll` to the correct VLC plugins directory.

### 4. Update Plugin Cache (Important)
After installation, you **must** update the VLC plugin cache for the new plugin to be detected. Open **Command Prompt (cmd) as Administrator** and run:

```cmd
cd "C:\Program Files\VideoLAN\VLC" && vlc --reset-plugins-cache
```

Restart VLC to load the plugin.

---

## LINUX

### 1. Prerequisites
You need to install the VLC development libraries (`libvlccore-dev` or `vlc-devel`), CMake, and a compiler.

* **Debian / Ubuntu / Mint:**
  ```bash
  sudo apt update
  sudo apt install build-essential cmake pkg-config libvlccore-dev
  ```

* **Fedora:**
  ```bash
  sudo dnf install cmake gcc-c++ vlc-devel
  ```

* **Arch Linux:**
  ```bash
  sudo pacman -S cmake base-devel vlc
  ```

### 2. Compilation
Navigate to the project folder and run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
This will compile the plugin inside the `build/` directory.

### 3. Installation
To install the plugin tailored to your system (requires root permissions):

```bash
sudo cmake --install build
```

**Note regarding installation path:**
By default, on many systems like Ubuntu/Debian, this will install the plugin to:
`/usr/lib/x86_64-linux-gnu/vlc/plugins/misc/`

However, this path **can vary depending on your distribution**. If `cmake --install` does not work as expected or puts it in a different location, you may need to copy the file manually (see below).

### 4. Manual Installation (Optional)
If you prefer to install it manually:

* **System-wide (Example path):**
  ```bash
  sudo cp build/libdiscordrpc_plugin.so /usr/lib/x86_64-linux-gnu/vlc/plugins/misc/
  ```

* **Per-user (Recommended for testing):**
  ```bash
  mkdir -p ~/.local/lib/vlc/plugins/misc/
  cp build/libdiscordrpc_plugin.so ~/.local/lib/vlc/plugins/misc/
  ```

### 5. Update Plugin Cache (Important)
After installation, you **must** update the VLC plugin cache. Run the following command in your terminal:

```bash
vlc --reset-plugins-cache
```

Restart VLC.
