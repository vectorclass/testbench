rem Batch file for running Vector class testbench with MS compiler (64 bits).
rem Run this from a Windows command prompt.
rem (c) 2019 Agner Fog, GPL 3.0 or later

rem Note: Make sure environment variables and command lines are not too long.
rem The list of test cases is in file t.lst. Set $compiler=10 in t.lst
set oldpath=%path%

rem Set path to bash and other Linux-emulating commands. Avoid unnecessary paths
rem Use Cygwin bash, not Mingw or WSL.
set path=C:\Windows\system32;C:\Windows;C:\cygwin64\bin

rem Set paths and include directories for MS compiler
rem You may need to modify this path to fit your installation
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

echo running bash

rem Run bash script with listfile t.lst
bash runtest.sh t.lst

echo returned from bash

pause

set path=%oldpath%
