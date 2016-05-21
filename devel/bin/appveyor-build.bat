@echo off

rem Licensed to the Apache Software Foundation (ASF) under one or more
rem contributor license agreements.  See the NOTICE file distributed with
rem this work for additional information regarding copyright ownership.
rem The ASF licenses this file to You under the Apache License, Version 2.0
rem (the "License"); you may not use this file except in compliance with
rem the License.  You may obtain a copy of the License at
rem
rem     http://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by applicable law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and
rem limitations under the License.

if "%CLOWNFISH_HOST%" == "c" goto test_c
if "%CLOWNFISH_HOST%" == "perl" goto test_perl

echo unknown CLOWNFISH_HOST: %CLOWNFISH_HOST%
exit /b 1

:test_c

if "%BUILD_ENV%" == "msvc" goto test_msvc
if "%BUILD_ENV%" == "mingw32" goto test_mingw32

echo unknown BUILD_ENV: %BUILD_ENV%
exit /b 1

:test_msvc

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

:test_mingw32

path C:\MinGW\bin;%path%

cd compiler\c
call configure && mingw32-make && mingw32-make test || exit /b

cd ..\..\runtime\c
call configure && mingw32-make && mingw32-make test

exit /b

:test_perl

path C:\MinGW\bin;%path%
call ppm install dmake

perl -V

cd compiler\perl
perl Build.PL && call Build && call Build test || exit /b

cd ..\..\runtime\perl
perl Build.PL && call Build && call Build test

exit /b

