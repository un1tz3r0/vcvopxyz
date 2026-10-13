# Installing on Windows

vcvopxyz needs VCV Rack 2.4 or later, 64-bit.

## From a package

1. Get `vcvopxyz-<version>-win-x64.vcvplugin`:
   - from the [Releases](https://github.com/un1tz3r0/vcvopxyz/releases) page, once a version is tagged; or
   - from the latest [Build run](https://github.com/un1tz3r0/vcvopxyz/actions/workflows/build.yml) on `main`: the
     `win-x64` artifact under **Artifacts** (you must be signed in to GitHub). It downloads as a `.zip` holding the
     `.vcvplugin`; extract it.
2. In Rack, choose **Help > Open user folder**, and copy the `.vcvplugin` into its `plugins-win-x64` folder (usually
   `%LOCALAPPDATA%\Rack2\plugins-win-x64\`). In PowerShell:

   ```powershell
   Copy-Item "$env:USERPROFILE\Downloads\vcvopxyz-*-win-x64.vcvplugin" "$env:LOCALAPPDATA\Rack2\plugins-win-x64\"
   ```

3. Restart Rack. It unpacks the package into a `vcvopxyz` folder beside it, and the modules appear in the browser
   under the brand **un1tz3r0**.

To update, copy the new package in the same way and restart Rack. To uninstall, delete the `vcvopxyz` folder and any
`vcvopxyz-*.vcvplugin` left in `plugins-win-x64`.

## Building from source

1. Install [MSYS2](https://www.msys2.org), open its **MINGW64** shell (not MSYS or UCRT64), and install the toolchain:

   ```sh
   pacman -S git make tar unzip zstd jq curl mingw-w64-x86_64-gcc
   ```

2. Get the source and the Rack SDK, and build and install:

   ```sh
   git clone https://github.com/un1tz3r0/vcvopxyz.git
   cd vcvopxyz
   curl -fLO https://vcvrack.com/downloads/Rack-SDK-2.6.6-win-x64.zip
   unzip -q Rack-SDK-2.6.6-win-x64.zip
   make install RACK_DIR=Rack-SDK -j$(nproc)
   ```

3. Restart Rack.

See [Development](docs/development.md) for the rest of the build.
