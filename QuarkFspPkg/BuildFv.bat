@REM
@REM This file contains an 'Intel Peripheral Driver' and is      
@REM licensed for Intel CPUs and chipsets under the terms of your
@REM license agreement with Intel or your vendor.  This file may 
@REM be modified by the user, subject to additional terms of the 
@REM license agreement                                           
@REM
@REM
@REM Copyright (c) 2008 - 2016, Intel Corporation All rights reserved
@REM This software and associated documentation (if any) is furnished
@REM under a license and may only be used or copied in accordance
@REM with the terms of the license. Except as permitted by such
@REM license, no part of this software or documentation may be
@REM reproduced, stored in a retrieval system, or transmitted in any
@REM form or by any means without the express written consent of
@REM Intel Corporation.
@REM

@echo off
@if /I "%1"=="/h" goto Usage
@if /I "%1"=="/?" goto Usage

@if not defined WORKSPACE (
  call %~dp0\..\edksetup.bat
)

@echo off
set VS_VERSION=
set VSCOMNTOOLS=

@if defined VS120COMNTOOLS (
  echo.
  echo Set the VS2013 environment.
  echo.
  if /I "%VS120COMNTOOLS%" == "C:\Program Files\Microsoft Visual Studio 12.0\Common7\Tools\" (
    set VS_VERSION=VS2013
  ) else (
    set VS_VERSION=VS2013x86
  )
)else if defined VS110COMNTOOLS (
  echo.
  echo Set the VS2012 environment.
  echo.
  if /I "%VS110COMNTOOLS%" == "C:\Program Files\Microsoft Visual Studio 11.0\Common7\Tools\" (
    set VS_VERSION=VS2012
  ) else (
    set VS_VERSION=VS2012x86
  )
) else if defined VS100COMNTOOLS (
  echo.
  echo Set the VS2010 environment.
  echo.
  if /I "%VS100COMNTOOLS%" == "C:\Program Files\Microsoft Visual Studio 10.0\Common7\Tools\" (
    set VS_VERSION=VS2010
  ) else (
    set VS_VERSION=VS2010x86
  )
) else if defined VS90COMNTOOLS (
  echo.
  echo Set the VS2008 environment.
  echo.
  if /I "%VS90COMNTOOLS%" == "C:\Program Files\Microsoft Visual Studio 9.0\Common7\Tools\" (
    set VS_VERSION=VS2008
  ) else (
    set VS_VERSION=VS2008x86
  )
) else if defined VS80COMNTOOLS (
  echo.
  echo Set the VS2005 environment.
  echo.
  if /I "%VS80COMNTOOLS%" == "C:\Program Files\Microsoft Visual Studio 8\Common7\Tools\" (
    set VS_VERSION=VS2005
  ) else (
    set VS_VERSION=VS2005x86
  )
) else (
  echo.
  echo !!! ERROR !!! VS2008/2005 not installed correctly. !!!
  echo !!! ERROR !!! VS120COMNTOOLS/VS90COMNTOOLS/VS80COMNTOOLS not defined. !!!
  echo.
  set SCRIPT_ERROR=1
  goto :End
)

@echo off
set OUT_DIR=Build
set FSP_BASENAME=Quark
set FSP_PKG_NAME=%FSP_BASENAME%FspPkg
set FSP_PKG_VPD_NAME=%FSP_PKG_NAME%Vpd
@REM Set build TARGET.
if exist %FSP_PKG_NAME%\Bsf\%FSP_BASENAME%Fsp.bsf attrib -r %FSP_PKG_NAME%\Bsf\%FSP_BASENAME%Fsp.bsf
if exist %FSP_PKG_NAME%\Include\FspUpdVpd.h attrib -r %FSP_PKG_NAME%\Include\FspUpdVpd.h
@if /I "%1" == "" (
  goto DebugBuild32
) else if /I "%1" == "/d32" (
  goto DebugBuild32
) else if /I "%1" == "/r32" (
  goto ReleaseBuild32
) else if /I "%1" == "/clean" (
  goto Clean
) else (
  echo.
  echo  ERROR: "%1" is not valid parameter.
  echo.
  goto Usage
)

:Clean
echo Removing Build and Conf directories ...
if exist Build rmdir Build /s /q
if exist Conf\.cache  rmdir Conf\.cache  /s /q
if exist Conf\target.txt  del Conf\target.txt /q /f
if exist Conf\build_rule.txt  del Conf\build_rule.txt /q /f
if exist Conf\tools_def.txt  del Conf\tools_def.txt /q /f
if exist %~dp0\*.log  del %~dp0\*.log /q /f
if exist %~dp0\..\*.log  del %~dp0\..\*.log /q /f
if exist %~dp0\%FSP_PKG_NAME%\*.log  del %~dp0\%FSP_PKG_NAME%\*.log /q /f
if exist %~dp0\..\*.bin  del %~dp0\..\*.bin /q /f
if exist %~dp0\..\*.fd  del %~dp0\..\*.fd /q /f
if exist %~dp0\..\*.bsf  del %~dp0\..\*.bsf /q /f
set WORKSPACE=
set EDK_TOOLS_PATH=
goto End

:ReleaseBuild32
set  BD_TARGET=RELEASE
set  DEBUG_PRINT_ERROR_LEVEL=-DDEBUG_PRINT_ERROR_LEVEL=0x80000000
set  DEBUG_PROPERTY_MASK=-DDEBUG_PROPERTY_MASK=0x23
set  BD_MACRO=-D CFG_OUTDIR=%OUT_DIR% %DEBUG_PRINT_ERROR_LEVEL% %DEBUG_PROPERTY_MASK%
set  BD_ARGS=-p %FSP_PKG_NAME%\%FSP_PKG_NAME%.dsc  -Y PCD -Y LIBRARY -y %~dp0\Report%BD_TARGET%.log -b %BD_TARGET% %BD_MACRO% -a IA32 -n 4 -t %VS_VERSION%
goto Build32

:DebugBuild32
set  BD_TARGET=DEBUG
set  DEBUG_PRINT_ERROR_LEVEL=-DDEBUG_PRINT_ERROR_LEVEL=0x80000042
set  DEBUG_PROPERTY_MASK=-DDEBUG_PROPERTY_MASK=0x27
set  BD_MACRO=-D CFG_DEBUG=1 -D CFG_OUTDIR=%OUT_DIR% %DEBUG_PRINT_ERROR_LEVEL% %DEBUG_PROPERTY_MASK%
set  BD_ARGS=-p %FSP_PKG_NAME%\%FSP_PKG_NAME%.dsc -Y PCD -Y LIBRARY -y %~dp0\Report%BD_TARGET%.log -b %BD_TARGET% %BD_MACRO% -a IA32 -n 4 -t %VS_VERSION%
goto Build32

:Build32
build  -m %FSP_PKG_NAME%\FspHeader\FspHeader.inf -D CFG_PREBUILD=1 %BD_ARGS%
if ERRORLEVEL 1 goto DIE
call :PreBuild  CALL_RET
if "%CALL_RET%"=="1" goto DIE 
build  %BD_ARGS%
if ERRORLEVEL 1 goto DIE
call :PostBuild
goto End

:Usage
echo.
echo  Usage: "%0 [/h | /? | /r32 | /d32 | /clean]"
echo.
goto End

:CopyBin
@if exist %1\*.efi   xcopy %1\*.efi   %2 /D /U /Y > NUL
@if exist %1\*.inf   xcopy %1\*.inf   %2 /D /U /Y > NUL
@if exist %1\*.depex xcopy %1\*.depex %2 /D /U /Y > NUL
goto:EOF

:PreBuild
echo Start of PreBuild ...
set %~1=1
set UPD_GUID=3E18A0B3-C3B5-492b-86B4-53E3D401C249
if not exist  %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV (
     mkdir    %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV
)

echo Generate UPD Header File ...
python IntelFspPkg\Tools\GenCfgOpt.py UPDTXT ^
     %FSP_PKG_NAME%\%FSP_PKG_VPD_NAME%.dsc ^
     %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV ^
     %BD_MACRO%
if "%ERRORLEVEL%"=="256" (
  REM  DSC is not changed, no need to recreate MAP and BIN file    
) else (
  if ERRORLEVEL 1 goto:PreBuildFail  
  echo UPD TXT file was generated successfully !
    
  echo Generate VPD Header File ...    
  del /q /f %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV\%UPD_GUID%.bin ^
            %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV\%UPD_GUID%.map 2>nul

  BaseTools\Bin\Win32\BPDG.exe ^
       %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV\%UPD_GUID%.txt ^
       -o %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV\%UPD_GUID%.bin ^
       -m %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV\%UPD_GUID%.map
  if ERRORLEVEL 1 goto:PreBuildFail
)

python IntelFspPkg\Tools\GenCfgOpt.py HEADER ^
         %FSP_PKG_NAME%\%FSP_PKG_VPD_NAME%.dsc ^
         %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV ^
         %FSP_PKG_NAME%\Include\BootLoaderPlatformData.h ^
         %BD_MACRO%
if "%ERRORLEVEL%"=="256" (
    REM  No need to recreate header file          
) else (    
    if ERRORLEVEL 1 goto:PreBuildFail       
    echo Vpd header file was generated successfully !
           
    echo Generate BSF File ...   
    python IntelFspPkg\Tools\GenCfgOpt.py GENBSF ^
         %FSP_PKG_NAME%\%FSP_PKG_VPD_NAME%.dsc ^
         %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV ^
         %FSP_PKG_NAME%\Bsf\%FSP_BASENAME%Fsp.bsf ^
         %BD_MACRO%
         
    if ERRORLEVEL 1 goto:PreBuildFail
    echo BSF file was generated successfully !
    
    copy %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV\FspUpdVpd.h  %FSP_PKG_NAME%\Include\FspUpdVpd.h 
)

:PreBuildRet
set %~1=0
echo End of PreBuild ...
echo.
goto:EOF

:PreBuildFail
del /q /f %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV\FspUpdVpd.h
echo.
goto:EOF

:PostBuild
echo Start of PostBuild ...   
echo Patch FD Image ...
python IntelFspPkg\Tools\PatchFv.py ^
     %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV ^
     %FSP_BASENAME%FV1:%FSP_BASENAME%FV2:%FSP_BASENAME%  ^
     "0xFFFFFFFC, [0x000000B0],                                  @FVBASE" ^
     "0xFFFFFFE0, <PeiCore:__ModuleEntryPoint>,     @PeiCore Entry" ^
     "0x000000C4, <FspSecCore:_TempRamInitApi>,     @TempRamInit API" ^
     "0x000000C8, <FspSecCore:_FspInitApi>,         @FspInit API" ^
     "0x000000CC, <FspSecCore:_NotifyPhaseApi>,     @NotifyPhase API" ^
     "0x000000D0, <FspSecCore:_FspMemoryInitApi>,   @FspMemoryInit API" ^
     "0x000000D4, <FspSecCore:_TempRamExitApi>,     @TempRamExit API" ^
     "0x000000D8, <FspSecCore:_FspSiliconInitApi>,  @FspSiliconInit API" ^
     "0x000000B8, 06A70056-3D0F-4A94-A743-5491CC9391D3:0x1C,     @VPD Region offset" ^
     "0x000000BC, [06A70056-3D0F-4A94-A743-5491CC9391D3:0x14]  - 0xF800001C,    @VPD Region size" ^
     "0x00000100, PcdPeim:__gPcd_BinaryPatch_PcdVpdBaseAddress - [0x000000B0],  @VPD PCD offset" ^
     "06A70056-3D0F-4A94-A743-5491CC9391D3:0x28, ([06A70056-3D0F-4A94-A743-5491CC9391D3:0x18] + 0x00000003) & 0x00FFFFFC + 06A70056-3D0F-4A94-A743-5491CC9391D3:0x1C,  @UPD Region offset"
if ERRORLEVEL 1 exit /b 1

@REM Patch FspInfoHeader relative offset
python IntelFspPkg\Tools\PatchFv.py ^
     %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV ^
     %FSP_BASENAME%FV1:%FSP_BASENAME%FV2:%FSP_BASENAME%  ^
     "FspSecCore:_FspInfoHeaderRelativeOff, FspSecCore:_AsmGetFspBaseAddress - {912740BE-2284-4734-B971-84B027353F0C:0x1C}, @FSP Header Offset" 
if ERRORLEVEL 1 exit /b 1

@REM Patch VPD base into the PcdPeim module patchable PCD
python IntelFspPkg\Tools\PatchFv.py ^
     %OUT_DIR%\%FSP_PKG_NAME%\%BD_TARGET%_%VS_VERSION%\FV ^
     %FSP_BASENAME%FV1:%FSP_BASENAME%FV2:%FSP_BASENAME%  ^
     "PcdPeim:__gPcd_BinaryPatch_PcdVpdBaseAddress, {[0x000000B8]}, @VPD PCD base"
if ERRORLEVEL 1 exit /b 1

echo Patch is DONE

goto:EOF

:DIE
rem ---------------------------------------------------------------------------
rem Exit returning a failure error code
rem ---------------------------------------------------------------------------
    if exist %FSP_PKG_NAME%\Bsf\%FSP_BASENAME%Fsp.bsf attrib +r %FSP_PKG_NAME%\Bsf\%FSP_BASENAME%Fsp.bsf
    if exist %FSP_PKG_NAME%\Include\FspUpdVpd.h attrib +r %FSP_PKG_NAME%\Include\FspUpdVpd.h
    exit /B 1

:End
if exist %FSP_PKG_NAME%\Bsf\%FSP_BASENAME%Fsp.bsf attrib +r %FSP_PKG_NAME%\Bsf\%FSP_BASENAME%Fsp.bsf
if exist %FSP_PKG_NAME%\Include\FspUpdVpd.h attrib +r %FSP_PKG_NAME%\Include\FspUpdVpd.h
echo.
