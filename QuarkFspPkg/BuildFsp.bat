@rem Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.
@rem This software and associated documentation (if any) is furnished
@rem under a license and may only be used or copied in accordance
@rem with the terms of the license. Except as permitted by such
@rem license, no part of this software or documentation may be
@rem reproduced, stored in a retrieval system, or transmitted in any
@rem form or by any means without the express written consent of
@rem Intel Corporation.

@echo off
setlocal enableextensions enabledelayedexpansion

set DEFAULT_TARGET=Quark
set PYTHON_MIN_VERSION=(2,6)

rem Move to script directory and start the build process
rem ----------------------------------------------------
if "%~1" == "/?" goto USAGE
if "%~1" == "/h" goto USAGE
REM pushd %~dp0
call :BUILD %*
REM popd
if not %ERRORLEVEL% == 0 goto DIE
goto END

:PREBUILD
rem ---------------------------------------------------------------------------
rem Perform pre-build actions
rem ---------------------------------------------------------------------------
	rem echo Performing pre-build actions.

	rem Remove shifted arguments by reconstructing BUILD_ARGS
	rem -----------------------------------------------------
	set BUILD_ARGS=
	:CONSTRUCT_BUILD_ARGS_LOOP
		if "%~1"=="" goto CONSTRUCT_BUILD_ARGS_CONTINUE
		set BUILD_ARGS=%BUILD_ARGS% %1
		shift
		goto CONSTRUCT_BUILD_ARGS_LOOP
	:CONSTRUCT_BUILD_ARGS_CONTINUE

	rem Check for Python support
	rem ------------------------
	call :FIND_PYTHON
	if not %ERRORLEVEL% == 0 goto DIE

	goto END

:BUILD
rem ---------------------------------------------------------------------------
rem Controller of the build process
rem ---------------------------------------------------------------------------
	set BUILD_ARGS=%*

	rem Perform pre-build
	rem -----------------
	call :PREBUILD %BUILD_ARGS%
	if not %ERRORLEVEL% == 0 goto DIE

	rem Build package
	rem -------------
	rem Find build script.
	set BUILD_TARGET=%DEFAULT_TARGET%FspPkg\BuildFv.bat
	if not exist %BUILD_TARGET% (
		echo ERROR: Unable to find '%BUILD_TARGET%'.
		goto DIE
		)
	call %BUILD_TARGET% %BUILD_ARGS%
	if not %ERRORLEVEL% == 0 goto DIE

	rem Perform post-build
	rem ------------------
	call :POSTBUILD %BUILD_ARGS%
	if not %ERRORLEVEL% == 0 goto DIE

	rem Done
	rem ----
	goto END

:POSTBUILD
rem ---------------------------------------------------------------------------
rem Perform post-build actions
rem ---------------------------------------------------------------------------
	rem echo Performing post-build actions.
	goto END

:FIND_PYTHON
rem ---------------------------------------------------------------------------
rem Looks for Python installed on the system.
rem ---------------------------------------------------------------------------
	set PYTHON_DEF_EXE=python.exe
	set PYTHON_EXE=
	set PYTHON_DIR=
	set PYTHON_FOUND_IN_PATH=0

	rem Check the standard installation location for Python 2.7
	if exist C:\Python27\%PYTHON_DEF_EXE% (
		set PYTHON_EXE=C:\Python27\%PYTHON_DEF_EXE%
		)
	if defined PYTHON_EXE goto FIND_PYTHON_FOUND

	rem Check the standard installation location for Python 2.6
	if exist C:\Python26\%PYTHON_DEF_EXE% (
		set PYTHON_EXE=C:\Python26\%PYTHON_DEF_EXE%
		)
	if defined PYTHON_EXE goto FIND_PYTHON_FOUND

	rem Check the PATH environment variable
	for %%X in (%PYTHON_DEF_EXE%) do (set PYTHON_EXE=%%~$PATH:X)
	if defined PYTHON_EXE (
		set PYTHON_FOUND_IN_PATH=1
		goto FIND_PYTHON_FOUND
		)

	rem Python not found
	echo Error: %PYTHON_DEF_EXE% could not be located on this system. If^
 Python has been installed to a nonstandard location (the typical installation^
 location is C:\Python26\%PYTHON_DEF_EXE%), ensure that the directory^
 containing the Python executable has been added to the PATH environment^
 variable.
 	call :FIND_PYTHON_CLEANUP
	goto DIE

	:FIND_PYTHON_FOUND
	echo Python was found at '%PYTHON_EXE%'.
	call :PYTHON_VERSION_TEST "%PYTHON_EXE%" "%PYTHON_MIN_VERSION%"
	if not %ERRORLEVEL% == 0 (
	echo Error: The version of Python installed on this system at^
 '%PYTHON_EXE%' is too old to run necessary scripts. Please upgrade^
 your Python installation to version !PYTHON_MIN_VERSION! or newer.
		call :FIND_PYTHON_CLEANUP
		goto DIE
		)

	rem Add Python to the PATH if necessary
	if not %PYTHON_FOUND_IN_PATH% == 0 goto FIND_PYHTON_CLEANUP
	for %%X in (%PYTHON_EXE%) do set PYTHON_DIR=%%~pdX
	set PATH=%PATH%;%PYTHON_DIR%

	:FIND_PYTHON_CLEANUP
	set PYTHON_DEF_EXE=
	set PYTHON_DEF_DIR=
	set PYTHON_EXE=
	set PYTHON_DIR=
	set PYTHON_FOUND_IN_PATH=

	goto END

:PYTHON_VERSION_TEST
rem ---------------------------------------------------------------------------
rem Test the Python executable %1 for version >= %2.
rem ---------------------------------------------------------------------------
	%1 -c "import sys; sys.exit(sys.version_info<%~2)"
	if not %ERRORLEVEL% == 0 goto DIE
	goto END

:USAGE
rem ---------------------------------------------------------------------------
rem Print usage and exit
rem ---------------------------------------------------------------------------
	echo Usage: QuarkFspPkg/BuildFsp [/h ^| /? ^| /r32 ^| /d32 ^| /clean]
	echo     Supported command list:
	echo       /h or /?  - Help
	echo       /r32 - 32 bit release build
	echo       /d32 - 32 bit debug build
	echo       /clean - Clean up
	goto END

:DIE
rem ---------------------------------------------------------------------------
rem Exit returning a failure error code
rem ---------------------------------------------------------------------------
	exit /B 1

:END
rem ---------------------------------------------------------------------------
rem Exit returning a success error code
rem ---------------------------------------------------------------------------
	exit /B 0
