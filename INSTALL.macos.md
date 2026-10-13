# Installing on macOS

vcvopxyz needs VCV Rack 2.4 or later. There are two packages, one per kind of Mac build of Rack:

| Your Rack | Package | Plugins folder |
| --- | --- | --- |
| Apple silicon (M1 and later), running natively | `mac-arm64` | `plugins-mac-arm64` |
| Intel Mac, or the Intel build of Rack under Rosetta on Apple silicon | `mac-x64` | `plugins-mac-x64` |

The package has to match Rack, not the Mac: an Intel build of Rack on an Apple silicon Mac loads only `mac-x64`
plugins. The name of the plugins folder in Rack's user folder tells you which Rack you have.

## From a package

1. Get `vcvopxyz-<version>-mac-arm64.vcvplugin` or `vcvopxyz-<version>-mac-x64.vcvplugin`:
   - from the [Releases](https://github.com/un1tz3r0/vcvopxyz/releases) page, once a version is tagged; or
   - from the latest [Build run](https://github.com/un1tz3r0/vcvopxyz/actions/workflows/build.yml) on `main`: the
     `mac-arm64` or `mac-x64` artifact under **Artifacts** (you must be signed in to GitHub). It downloads as a
     `.zip` holding the `.vcvplugin`; Safari usually unzips it for you.
2. In Rack, choose **Help > Open user folder**, and copy the `.vcvplugin` into its `plugins-mac-arm64` or
   `plugins-mac-x64` folder (usually in `~/Library/Application Support/Rack2/`):

   ```sh
   cp ~/Downloads/vcvopxyz-*-mac-arm64.vcvplugin ~/Library/Application\ Support/Rack2/plugins-mac-arm64/
   ```

3. Restart Rack. It unpacks the package into a `vcvopxyz` folder beside it, and the modules appear in the browser
   under the brand **un1tz3r0**.

The plugin isn't signed or notarized. If macOS refuses to load it, clear the quarantine flag from the unpacked folder
and restart Rack:

```sh
xattr -dr com.apple.quarantine ~/Library/Application\ Support/Rack2/plugins-mac-arm64/vcvopxyz
```

To update, copy the new package in the same way and restart Rack. To uninstall, delete the `vcvopxyz` folder and any
`vcvopxyz-*.vcvplugin` left in the plugins folder.

## Building from source

1. Install the Xcode command line tools, and `jq` and `zstd` from [Homebrew](https://brew.sh):

   ```sh
   xcode-select --install
   brew install jq zstd
   ```

2. Get the source and the Rack SDK for your Rack (`mac-arm64` or `mac-x64`), and build and install:

   ```sh
   git clone https://github.com/un1tz3r0/vcvopxyz.git
   cd vcvopxyz
   curl -fLO https://vcvrack.com/downloads/Rack-SDK-2.6.6-mac-arm64.zip
   unzip -q Rack-SDK-2.6.6-mac-arm64.zip
   make install RACK_DIR=Rack-SDK -j$(sysctl -n hw.ncpu)
   ```

3. Restart Rack.

To build the other kind on the same Mac, use that kind's SDK and name its target, as CI does. For an Intel build on
Apple silicon:

```sh
curl -fLO https://vcvrack.com/downloads/Rack-SDK-2.6.6-mac-x64.zip
unzip -q Rack-SDK-2.6.6-mac-x64.zip -d x64
make dist RACK_DIR=x64/Rack-SDK CROSS_COMPILE=x86_64-apple-darwin
```

For an Apple silicon build on Intel, use the `mac-arm64` SDK and `CROSS_COMPILE=arm64-apple-darwin`. Run `make clean`
between the two, since they share the build folder. See [Development](docs/development.md) for the rest of the build.
