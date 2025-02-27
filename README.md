# Cubiomes Viewer

Cubiomes Viewer provides a graphical interface for the efficient and flexible seed-finding utilities provided by [cubiomes](https://github.com/Cubitect/cubiomes) and a map viewer for the Minecraft biomes and structure generation.

The tool is designed for high performance and is currently limited to overworld features for Minecraft 1.6 - 1.16.


## Download

Precompiled binaries can be found for Linux and Windows under [Releases on github](https://github.com/Cubitect/cubiomes-viewer/releases). The builds are statically linked against [Qt](https://www.qt.io) and should run as-is on most newer distributions.

For the linux build you will probably have to add the executable flags to the binary (github seems to remove them upon upload).

## Build from source

Install Qt5 development files
```
$ sudo apt install build-essential qt5-default
```
get sources
```
$ git clone --recursive https://github.com/Cubitect/cubiomes-viewer.git
```
compile cubiomes library
```
$ cd cubiomes-viewer/cubiomes/
$ make
```
build cubiomes-viewer
```
$ mkdir ../build
$ cd ../build
$ qmake ..
$ make
```

