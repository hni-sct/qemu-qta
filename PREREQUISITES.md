# Prerequisite installation

Depending on your operating system you (almost certainly) need to install some prerequisites first. The following sections will give you a brief overview of how to install the necessary software to build QEMU from source code under recent versions of Arch Linux, Debian and Ubuntu Linux as well as under macOS.

## Alternative A - Arch Linux

```bash
# This also install all optional QEMU dependencies
sudo pacman -S seabios gnutls libpng libaio numactl libnfs lzo snappy curl \
    vde2 libcap-ng spice libcacard usbredir libslirp libssh zstd liburing \
    virglrenderer sdl2 vte3 libpulse libjack.so brltty spice-protocol python \
    ceph libiscsi glusterfs python-sphinx xfsprogs
```

## Alternative B - Ubuntu 20.04 LTS

```bash
sudo apt install -y build-essential git libglib2.0-dev ninja-build meson libpixman-1-dev python3-venv python3-pip
```

## Alternative C - macOS Mojave 10.14 and above

```bash
# Install Homebrew Package Manager (https://brew.sh/)
# -> Run the following command in macOS Terminal
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"

# Install prerequisites
brew install glib gnutls jpeg libpng libtool libusb lzo make nettle ninja pixman pkg-config snappy

# Make newly installed gmake (temporarily) take higher precedence
export PATH="/usr/local/opt/make/libexec/gnubin:$PATH"
```

# Further reading and additional instructions
- [The QEMU build system architecture](https://qemu.readthedocs.io/en/latest/devel/build-system.html)
- [README on github.com/qemu](https://github.com/qemu/qemu/blob/master/README.rst)
- [QEMU Wiki: Hosts/Mac](https://wiki.qemu.org/Hosts/Mac) 
- [QEMU Wiki: Hosts/Linux](https://wiki.qemu.org/Hosts/Linux) 
- [QEMU Wiki: Hosts/W32](https://wiki.qemu.org/Hosts/W32) 
