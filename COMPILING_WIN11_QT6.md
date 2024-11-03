# Windows 11 - QT6
## 2024-11-02

## Installing

Download the QT online installer from qt.io

Perform a custom install and install:
* QT > QT 6.8.0 including all additional libraries
* QT > Developer and Designer Tools > LLVM-MinGW 17.xxx
* QT > Developer and Designer Tools > MinGW 13.xx
* QT > Developer and Designer Tools > CMake 3.xx

Install flex and bison from 
https://sourceforge.net/projects/winflexbison/files/latest/download
* in the folder c:\qt\Tools\win_flex_bison 
* rename win_flex.exe to flex.exe
* rename win_bison.exe to bison.exe

In the folder C:\Qt\Tools\mingw1310_64\bin
* rename mingw32_make.exe to make.exe

Make sure that the following have been added to your path system environment variable:
* C:\Qt\6.8.0\mingw_64\bin
* C:\Qt\Tools\mingw1310_64\bin
* C:\Qt\Tools\win_flex_bison

## Compiling - Debug

From cmd in the basic256 folder:
* qmake basic256.pro -config debug
* make

