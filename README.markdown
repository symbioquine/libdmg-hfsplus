# About

`libdmg-hfsplus` includes libraries and commands-line utilities for manipulating hfs+ volume images, disk images, and dmg images.

# Warning

**THE CODE HEREIN SHOULD BE CONSIDERED HIGHLY EXPERIMENTAL**

Extensive testing have not been done, but comparatively simple tasks like
adding and removing files from a mostly contiguous filesystem are well
proven.

Please note that these tools and routines are currently only suitable to be
accessed by other programs that know what they're doing. I.e., doing
something you "shouldn't" be able to do, like removing non-existent files is
probably not a very good idea.

# License

This work is released under the terms of the GNU General Public License,
version 3. The full text of the license can be found in the LICENSE file.

# Dependencies

The HFS portion will work on any platform that supports GNU C and POSIX
conventions. The dmg portion has dependencies on the fairly ubiquitous zlib.

# Building

## Prerequisites

* **Ubuntu:** `sudo apt-get install build-essential cmake`
* **Archlinux:** `sudo pacman -S git make clang cmake`

## Build

Assuming the sources have been downloaded and unpacked to a directory named `libdmg-hfsplus`;

```sh
cd libdmg-hfsplus/
cmake .
make
sudo make install
```

# Usage Example

Here is an example showing how one could use the `hdutil`, `hfsplus`, and `dmg` utilities
together to produce a standard dmg installer;

```sh
hdutil partition.img add $APP_BUILD_DIR/$BACKGROUND_FILENAME .background.png
hdutil partition.img add $APP_BUILD_DIR/.VolumeIcon.icns .VolumeIcon.icns
hdutil partition.img add $APP_BUILD_DIR/.DS_Store .DS_Store
hdutil partition.img mkdir "$APP_FILENAME"
hdutil partition.img addall $APP_BUILD_DIR/"$APP_FILENAME" "$APP_FILENAME"
hdutil partition.img symlink Applications /Applications/

hdutil partition.img chmod 0755 "$APP_FILENAME"/Contents/MacOS/universalJavaApplicationStub

hfsplus partition.img setfinderflags / +HasCustomIcon

# Hacky approach to place the partition within the disk image - can be done more
# elegantly with losetup/partx, but that requires root which gets messy for some
# continuous integration build environments.
dd if=partition.img conv=notrunc of=disk.img bs=$SECTOR_SIZE seek=$START_SECTOR count=$(( $NUM_PARTITION_SECTORS + 1 ))

dmg dmg disk.img disk.dmg
```

*Note: There are some implicit assumptions in the above example. In particular, the math to
place a partition at the correct offsets within a GPT disk image. Also the volume UUID and
catalog-node-id of the .background.png file would be needed to generate a valid .DS_Store
that references said .background.png image.*

# Background

The targets of the current repository are three command-line utilities that
demonstrate the usage of the library functions (except cmd_grow, which really
ought to be moved to catalog.c). The dmg portion of the code has dependencies
on the HFS+ portion of the code. The "hdutil" section contains a version of the
HFS+ utility that supports directly reading from dmgs. It is separate from the
HFS+ utility in order that the hfs directory does not have dependencies on the
dmg directory.

This project was first conceived to manipulate Apple's software restore
packages (IPSWs) and hence much of it is geared specifically toward that
format. Useful tools to read and manipulate the internal data structures of
those files have been created to that end, and with minor changes, more
generality can be achieved in the general utility. An inexhaustive list of
such changes would be selectively enabling folder counts in HFS+, switching
between case sensitivity and non-sensitivity, and more fine-grained control
over the layout of created dmgs.
