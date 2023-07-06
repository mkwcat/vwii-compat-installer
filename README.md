# Compat Title Installer

Install a channel to the vWii Menu from Wii U Mode. In its current state, it
simply installs the Homebrew Channel.

## Building

You need devkitPPC, devkitARM and WUT installed, and the environment variables set
correctly. You will need the following libraries:

* [libmocha](https://github.com/wiiu-env/libmocha)

If everything is installed, run 'make release' and the output will be available as
'compat_installer-HBL.zip' and 'compat_installer-Aroma.zip'.

## Credits

* Dimok, smealum, others for iosuhax and Mocha CFW.
* FIX94, this repo is largely based off of wuphax.
* Adinaton on Discord, for the awesome icons
* TheLordScruffy/mkwcat, for the original Compat Title Installer

## License

This software is licensed under the GNU General Public License version 2 (or any
later version). The full license can be found in the LICENSE file.

The Homebrew Channel is licensed under GPLv2 and is included in binary form. A
copy of the source code is available at
[fail0verflow/hbc](https://github.com/fail0verflow/hbc).