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

setlocal EnableExtensions EnableDelayedExpansion

set base_dir=%~dp0\..\..
call :normalize "%base_dir%"
set base_dir=%retval%
set runtime_dir=%base_dir%\runtime
set compiler_dir=%base_dir%\compiler

call :add_to_path "%PATH%" "%compiler_dir%\c"
set PATH=%retval%
call :add_to_path "%PATH%" "%runtime_dir%\c"
set PATH=%retval%
call :add_to_path "%INCLUDE%" "%runtime_dir%\perl\xs"
set INCLUDE=%retval%
call :add_to_path "%LIB%" "%runtime_dir%\c"
set LIB=%retval%
call :add_to_path "%CLOWNFISH_INCLUDE%" "%runtime_dir%\core"
set CLOWNFISH_INCLUDE=%retval%
call :add_to_path "%PERL5LIB%" "%compiler_dir%\perl\blib\arch"
set PERL5LIB=%retval%
call :add_to_path "%PERL5LIB%" "%compiler_dir%\perl\blib\lib"
set PERL5LIB=%retval%
call :add_to_path "%PERL5LIB%" "%runtime_dir%\perl\blib\arch"
set PERL5LIB=%retval%
call :add_to_path "%PERL5LIB%" "%runtime_dir%\perl\blib\lib"
set PERL5LIB=%retval%

endlocal & (
    set "PATH=%PATH%"
    set "INCLUDE=%INCLUDE%"
    set "LIB=%LIB%"
    set "CLOWNFISH_INCLUDE=%CLOWNFISH_INCLUDE%"
    set "PERL5LIB=%PERL5LIB%"
)

exit /b

:normalize
set retval=%~f1
goto :eof

:add_to_path
set _path=%~1
set _dir=%~2
if "%_path%" == "" (
    set "retval=%_dir%"
    goto :eof
)
if "!_path:%_dir%=!" == "%_path%" (
    set "retval=%_path%;%_dir%"
    goto :eof
)
set retval=%_path%
goto :eof
