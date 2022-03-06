# Compat Title Installer

Install a channel to the vWii Menu from Wii U Mode. In its current state, it
simply installs the Homebrew Channel.

## Building

You need devkitPPC and devkitARM installed, and the environment variables set
correctly. You will need the following libraries:

* [dynamic_libs](https://github.com/Maschell/dynamic_libs)
* [libutils](https://github.com/Maschell/libutils)
* [libiosuhax](https://github.com/dimok789/libiosuhax)

If everything is installed, run 'make' and the output will be available as
'compat-installer.elf'.

## Credits

* Dimok, smealum, others for iosuhax and Mocha CFW.
* FIX94, this repo is largely based off of wuphax.

Portions of Mocha CFW are included in binary form. The source code of the
specific version used here is from
[FIX94/wuphax](https://github.com/fix94/wuphax).

## License

This software is licensed under the GNU General Public License version 2 (or any
later version). The full license can be found in the LICENSE file.

The Homebrew Channel is licensed under GPLv2 and is included in binary form. A
copy of the source code is available at
[fail0verflow/hbc](https://github.com/fail0verflow/hbc).