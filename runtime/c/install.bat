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

set major_version=0.5

if "%1" == "--prefix" goto opt_prefix
echo Usage: install.bat --prefix path
exit /b 1

:opt_prefix
if not "%~2" == "" goto has_prefix
echo --prefix requires an argument.
exit /b 1

:has_prefix
set prefix=%~2

mkdir "%prefix%\bin" 2>nul
mkdir "%prefix%\lib" 2>nul

rem Install libraries.
copy clownfish-%major_version%.dll "%prefix%\bin" >nul
copy libclownfish-%major_version%.dll "%prefix%\bin" >nul
copy clownfish-%major_version%.lib "%prefix%\lib" >nul
copy libclownfish-%major_version%.dll.a "%prefix%\lib" >nul

rem Install executables.
copy ..\..\compiler\c\cfc.exe "%prefix%\bin" >nul

rem Install Clownfish header files and HTML documentation.
xcopy /siy autogen\share "%prefix%\share" >nul

rem Install man pages.
xcopy /siy autogen\man "%prefix%\man" >nul

