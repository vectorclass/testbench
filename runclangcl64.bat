rem Batch file for running Vector class testbench with the version of Clang
rem that comes as a plugin to MS Visual Studio (64 bits).
rem Run this from a Windows command prompt.
rem (c) 2019 Agner Fog, GPL 3.0 or later

rem The list of test cases is in file t.lst. Set $compiler=2 in t.lst
set oldpath=%path%

rem Set path to bash and other Linux-emulating commands and to the Clang compiler.
rem Avoid unnecessary paths.
rem Use Cygwin bash, not Mingw or WSL.
set path=C:\Windows\system32;C:\Windows;C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\Llvm\8.0.0\bin

rem Set paths and include directories for MS compiler
rem You may need to modify this path to fit your installation
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

echo running bash

rem Run bash script with listfile t.lst
bash runtest.sh t.lst

echo returned from bash

pause

set path=%oldpath%
