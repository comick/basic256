# Compiling basic256 - Ubuntu 24.04 LTS

## 2024-10-31 j.m.reneau

sudo apt install subversion

svn checkout --username=YOURUSER svn+ssh://renejm@svn.code.sf.net/p/kidbasic/code/trunk basic256

sudo apt install bison flex qt5-qmake qtbase5-dev libqt5serialport5-dev libqt5texttospeech5-dev qtmultimedia5-dev qttools5-dev-tools

qmake BASIC256.pro -config debug

make

./basic256