@echo off

if "%CLOWNFISH_HOST%" == "c" goto test_c
if "%CLOWNFISH_HOST%" == "perl" goto test_perl

echo unknown CLOWNFISH_HOST: %CLOWNFISH_HOST%
exit /b 1

:test_c

if "%MSVC_VERSION%" == "10" goto msvc_10

call "C:\Program Files (x86)\Microsoft Visual Studio %MSVC_VERSION%.0\VC\vcvarsall.bat" amd64
goto msvc_build

:msvc_10
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

:msvc_build

cd compiler\c
call configure && nmake && nmake test || exit /b

cd ..\..\runtime\c
call configure && nmake && nmake test

exit /b

:test_perl

perl -V

cd compiler\perl
perl Build.PL && call Build && call Build test || exit /b

cd ..\..\runtime\perl
perl Build.PL && call Build && call Build test

exit /b

