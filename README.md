## QTA: The QEMU Timing Analyzer Plugin

Welcome to the QEMU QTA plugin. This extension adds time simulation and analysis support to the free System Emulator QEMU.

### Installation
We have provided a simple bash script that should work on most Unix systems. It was testes on Ubuntu 20.04 and on MacOS Big Sur. To invoke the QEMU / QTA build, simply run

```bash
./build_qemu_qta.sh
```

from here. This should fetch, setup and compile QEMU for the use with plugins and also build the QTA plugin itself automatically. Please carefully watch the output for any errors that might occur during build.  

After a successful build, a file named `libqta.so` should have been created.

> **IMPORTANT**:
>
> The QEMU compilation usually needs some libraries and tools that are not available on all systems. Please refer to [/PREREQUISITES.md](/PREREQUISITES.md) for details how to set up your system accordingly. 

### Testing

If you want to test if QTA is running properly, you can trigger a set of tests that are shipped with QTA. For this, simply run

```bash
make tests
```

from here. For each of the three test binaries a time simulation will be run and their simulation output is validated against reference results.

> **IMPORTANT**: 
> 
> The last test depends on the python packages *lxml* and *pygraphviz*. Please ensure those are installed either globally or inside a python virtual environment. In the latter case, do not forget to always enter the virtual env before using QTA!

### Usage
Please refer to [/TUTORIAL.md](/TUTORIAL.md) for a step-by-step guide that explains how to write a QTA timing annotation database (.qtdb) for binary program and how to run the time simulation afterwards.
