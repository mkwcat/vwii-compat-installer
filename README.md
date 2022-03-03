# Compat Title Installer

Install a channel to the vWii Menu from Wii U Mode. In its current state, it
simply installs the Homebrew Channel.

## Building

You need devkitPPC and WUT installed, and the environment variables set
correctly. You will need the following libraries:

* [libiosuhax](https://github.com/wiiu-env/libiosuhax)

If everything is installed, run 'make' and the output will be available as
'compat-installer.rpx' in the source folder.

## Credits

* Dimok, smealum, others for iosuhax and Mocha CFW.
* FIX94, this repo is largely based off of wuphax.

## License

This software is licensed under the GNU General Public License version 2 (or any
later version). The full license can be found in the LICENSE file.

The Homebrew Channel is licensed under GPLv2 and is included in binary form. A
copy of the source code is available at
[fail0verflow/hbc](https://github.com/fail0verflow/hbc).
