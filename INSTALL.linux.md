# Installing on Linux

vcvopxyz needs VCV Rack 2.4 or later. The Linux package is built on Ubuntu 20.04, so it loads on any distribution
from about 2020 on.

## From a package

1. Get `vcvopxyz-<version>-lin-x64.vcvplugin`:
   - from the [Releases](https://github.com/un1tz3r0/vcvopxyz/releases) page, once a version is tagged; or
   - from the latest [Build run](https://github.com/un1tz3r0/vcvopxyz/actions/workflows/build.yml) on `main`: the
     `lin-x64` artifact under **Artifacts** (you must be signed in to GitHub). It downloads as a `.zip` holding the
     `.vcvplugin`; unzip it.
2. In Rack, choose **Help > Open user folder**, and copy the `.vcvplugin` into its `plugins-lin-x64` folder
   (usually `~/.local/share/Rack2/plugins-lin-x64/`):

   ```sh
   cp vcvopxyz-*-lin-x64.vcvplugin ~/.local/share/Rack2/plugins-lin-x64/
   ```

3. Restart Rack. It unpacks the package into a `vcvopxyz` folder beside it, and the modules appear in the browser
   under the brand **un1tz3r0**.

To update, copy the new package in the same way and restart Rack. To uninstall, delete the `vcvopxyz` folder and any
`vcvopxyz-*.vcvplugin` left in `plugins-lin-x64`.

## Building from source

1. Install the toolchain. On Debian and Ubuntu:

   ```sh
   sudo apt install build-essential git curl unzip jq zstd
   ```

   On Fedora, `sudo dnf install gcc-c++ make git curl unzip jq zstd`; on Arch, `sudo pacman -S base-devel git curl
   unzip jq zstd`.
2. Get the source and the Rack SDK, and build and install:

   ```sh
   git clone https://github.com/un1tz3r0/vcvopxyz.git
   cd vcvopxyz
   curl -fLO https://vcvrack.com/downloads/Rack-SDK-2.6.6-lin-x64.zip
   unzip -q Rack-SDK-2.6.6-lin-x64.zip
   make install RACK_DIR=Rack-SDK -j$(nproc)
   ```

3. Restart Rack.

A plugin built on your machine needs your machine's glibc or newer, so build on the oldest system you want it to run
on, or use the CI package. See [Development](docs/development.md) for the rest of the build.
