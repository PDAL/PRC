# PDAL PRC Plugin

[![Build Status](https://travis-ci.org/PDAL/PRC.png?branch=master)](https://travis-ci.org/PDAL/PRC)

The PDAL PRC Plugin is released as LGPL v3.

Begin by installing the plugin's only required dependencies, PDAL (pdal.io)
and Haru (libharu.org). The latter should work up to version 2.3, but recent
changes in master affect the PRC file loading. There is a note in the source as
to how to modify the code to work with master.

In a nutshell, the PDAL PRC Plugin can configured, built, and installed thusly:

```
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make
make install
```
