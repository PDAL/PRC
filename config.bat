@echo off

:: First create a subdirectory called build. Change to the new folder, and run
:: this script there.

:: This assumes you have built and installed ZLIB, PDAL, Boost, and Haru. If you
:: haven't, checkout the unofficial OSGeo Superbuild on Github at
:: http://github.com/chambbj/osgeo-superbuild.

set GENERATOR="NMake Makefiles"

set BUILD_TYPE=Release

set PRC_DIR=..

:: This will vary depending on your system and third-party installations.

set VENDOR_DIR=C:\Users\chambersbj\osgeo\build\nmake64\install
set INCLUDE_DIR=%VENDOR_DIR%\include
set LIBRARY_DIR=%VENDOR_DIR%\lib
set INSTALL_DIR=C:\Users\chambersbj\Documents\usr

cmake -G %GENERATOR% ^
  -DZLIB_INCLUDE_DIR=%INCLUDE_DIR% ^
  -DZLIB_LIBRARY=%LIBRARY_DIR%\zlibd.lib ^
  -DPDAL_INCLUDE_DIR=%INCLUDE_DIR%\pdal-0.8 ^
  -DPDAL_LIBRARY=%LIBRARY_DIR%\pdal.lib ^
  -DBOOST_ROOT=%INCLUDE_DIR%\boost-1_53 ^
  -DHPDF_INCLUDE_DIR=C:\Users\chambersbj\Documents\usr\include ^
  -DHPDF_LIBRARY=C:\Users\chambersbj\Documents\usr\lib\libhpdfsd.lib ^
  -DCMAKE_INSTALL_PREFIX=C:\Users\chambersbj\Documents\usr ^
  %PRC_DIR%
