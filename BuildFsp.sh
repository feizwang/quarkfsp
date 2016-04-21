## @file
# @copyright
#  Copyright (c) 2008 - 2016, Intel Corporation All rights reserved
#  This software and associated documentation (if any) is furnished
#  under a license and may only be used or copied in accordance
#  with the terms of the license. Except as permitted by such
#  license, no part of this software or documentation may be
#  reproduced, stored in a retrieval system, or transmitted in any
#  form or by any means without the express written consent of
#  Intel Corporation.
#  This file contains an 'Intel Peripheral Driver' and is
#  licensed for Intel CPUs and chipsets under the terms of your
#  license agreement with Intel or your vendor.  This file may
#  be modified by the user, subject to additional terms of the
#  license agreement
##
#!/bin/sh

FSP_PKG_NAME=QuarkFspPkg
FSP_BIN_PKG_NAME=QuarkFspBinPkg
TOOL_CHAIN=GCC48
Edksetup=edksetup.sh
UPD_GUID=3E18A0B3-C3B5-492b-86B4-53E3D401C249
ErrorCode=0
OUT_DIR=Build

export TOOL_CHAIN

function USAGE()
{
  echo
  echo  "$0 \[-h \| -? \| -r32 \| -d32 \| -clean\]"
  echo
  return 1
}

function Clean(){
  echo Removing Build and Conf directories ...
  if [ -d Build ]
   then 
    rm -r Build
  fi
  if [ -d Conf/.cache ]
    then 
     rm  -r Conf/.cache
  fi
  if [ -f *.log ]
   then 
    rm *.log
  fi
  WORKSPACE=
  EDK_TOOLS_PATH=
  return 0
}


function  PreBuildFail(){
  if [ -f $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/FspUpdVpd.h ]
   then
    rm  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/FspUpdVpd.h
  fi
  #/q /f
  return 0
}

function PreBuildRet(){
  set $1=0
  echo End of PreBuild ...
  echo
  return 0
}


function CopyBin(){
 if [ -f $1/*.efi ]
  then 
   cp $1/*.efi   $2 
   #/D /U /Y > NUL
 fi
 if [ -f $1/*.inf ]
   then
     cp $1/*.inf   $2 
   #/D /U /Y > NUL
 fi
 if [ -f $1/*.depex ]
   then
    cp $1/*.depex $2 
   #/D /U /Y > NUL
 fi
}

function PreBuild(){

  python_command="python IntelFspPkg/Tools/GenCfgOpt.py UPDTXT  $FSP_PKG_NAME/$FSP_PKG_NAME"Vpd".dsc  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV"
  echo $python_command
  $python_command

  error=256
  if [ $? -eq $error ] 
   then
    echo  DSC is not changed, no need to recreate MAP and BIN file
  else
   error=0
   if [ $? -ne $error ]
    then
     echo   error: $?
     return 1
   fi

  echo UPD TXT file was generated successfully !

  echo Generate VPD Header File ...
  rm  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/$UPD_GUID.bin \
            $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/$UPD_GUID.map
 #2>nul


  bpdg_command="BaseTools/BinWrappers/PosixLike/BPDG  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/$UPD_GUID.txt  -o $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/$UPD_GUID.bin  -m $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/$UPD_GUID.map"
  echo $bpdg_command
  $bpdg_command

      if [ $? -eq "1" ]
       then
       return 1
      fi
  fi


  touch $FSP_PKG_NAME/Include/BootLoaderPlatformData.h
  python_command="python IntelFspPkg/Tools/GenCfgOpt.py HEADER  $FSP_PKG_NAME/$FSP_PKG_NAME"Vpd".dsc  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV  $FSP_PKG_NAME/Include/BootLoaderPlatformData.h  $BD_MACRO"
  echo $python_command
  $python_command

  if [ $? -eq "256" ]
   then 
    echo
    # No need to recreate header file
   else 
      if [ $? -eq "1" ] 
       then 
        echo 
      fi

    echo Vpd header file was generated successfully !

    echo Generate BSF File ...
    if [ -f $FSP_BIN_PKG_NAME/fsp.bsf ]
     then
      rm -f $FSP_BIN_PKG_NAME/fsp.bsf
    fi
    python_command="python IntelFspPkg/Tools/GenCfgOpt.py GENBSF  $FSP_PKG_NAME/$FSP_PKG_NAME"Vpd".dsc  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV  $FSP_BIN_PKG_NAME/fsp.bsf  $BD_MACRO"
    echo $python_command
    $python_command

    if [ $? -eq "1" ] 
       then 
        return 1
    fi

    echo BSF file was generated successfully !

    if [ -f $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/FspUpdVpd.h ]
     then
      cp $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/FspUpdVpd.h  $FSP_PKG_NAME/Include/FspUpdVpd.h
    fi
  fi
}


function PostBuild(){
  echo Start of PostBuild ...
  echo Patch FD Image ...
   python IntelFspPkg/Tools/PatchFv.py \
     $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV \
     QUARKFV1:QUARKFV2:QUARK \
     "0xFFFFFFFC, [0x000000B0],                               @FVBASE" \
     "0xFFFFFFE0, PeiCore:__ModuleEntryPoint - [0x000000B0],  @PeiCore Entry" \
     "0x000000C4, FspSecCore:_TempRamInitApi - [0x000000B0],     @TempRamInit API" \
     "0x000000C8, FspSecCore:_FspInitApi - [0x000000B0],         @FspInit API" \
     "0x000000CC, FspSecCore:_NotifyPhaseApi - [0x000000B0],     @NotifyPhase API" \
     "0x000000D0, FspSecCore:_FspMemoryInitApi - [0x000000B0],   @FspMemoryInit API" \
     "0x000000D4, FspSecCore:_TempRamExitApi - [0x000000B0],     @TempRamExit API" \
     "0x000000D8, FspSecCore:_FspSiliconInitApi - [0x000000B0],  @FspSiliconInit API" \
     "0x000000B8, 06A70056-3D0F-4A94-A743-5491CC9391D3:0x1C,  @VPD Region offset" \
     "0x000000BC, [06A70056-3D0F-4A94-A743-5491CC9391D3:0x14]  - 0xF800001C,    @VPD Region size" \
     "0x00000100, PcdPeim:__gPcd_BinaryPatch_PcdVpdBaseAddress - [0x000000B0],  @VPD PCD offset" \
     "06A70056-3D0F-4A94-A743-5491CC9391D3:0x28, ([06A70056-3D0F-4A94-A743-5491CC9391D3:0x18] + 0x00000003) & 0x00FFFFFC + 06A70056-3D0F-4A94-A743-5491CC9391D3:0x1C,  @UPD Region offset"
  if [ $? -ne 0 ]
   then
    return 1
  fi
  return 0
}

function  Build32(){

   make -C ./BaseTools/Source/C
   chmod +w $FSP_PKG_NAME/$FSP_PKG_NAME.dsc
   gcc -v
   OverrideBaseTools $*
   build -m $FSP_PKG_NAME/FspHeader/FspHeader.inf $BD_ARGS -DCFG_PREBUILD
   if [ $? -ne 0 ]
   then 
     exit 1
   fi

   PreBuild $*

   if [ $? -eq 1 ]
   then
     exit 1 
   fi

   build $BD_ARGS
   if [ $? -ne 0 ]
   then 
     exit 1
   fi
   PostBuild
   if [ $? -ne 0 ]
   then 
     exit 1
   fi
}

function ReleaseBuild32(){
  BD_TARGET=RELEASE
  BD_MACRO="-D CFG_OUTDIR="$OUT_DIR
  DSCFILE="-p "$FSP_PKG_NAME/$FSP_PKG_NAME.dsc #" -y ReleaseBuild32Log.log"
  BD_ARGS=$DSCFILE" -b RELEASE "$BD_MACRO" -a IA32 -n 1 -t "$TOOL_CHAIN
  Build32 $*
  patchFspInfoHeader
}

function DebugBuild32(){
  BD_TARGET=DEBUG
  BD_MACRO="-D CFG_DEBUG=1 -D DEBUG_BIOS_ENABLE=TRUE -D CFG_OUTDIR="$OUT_DIR
  DSCFILE="-p "$FSP_PKG_NAME/$FSP_PKG_NAME.dsc   #" -y DebugBuild32Log.log"
  #echo $DSCFILE

  BD_ARGS=$DSCFILE" -b DEBUG "$BD_MACRO" -a IA32 -n 1 -t "$TOOL_CHAIN
  Build32 $*
  patchFspInfoHeader
}

function patchFspInfoHeader(){
    #Patch FspInfoHeader relative offset
    python IntelFspPkg/Tools/PatchFv.py \
     $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV \
     QUARKFV1:QUARKFV2:QUARK  \
    "FspSecCore:_FspInfoHeaderRelativeOff, FspSecCore:_AsmGetFspBaseAddress - {912740BE-2284-4734-B971-84B027353F0C:0x1C}, @FSP Header Offset" 

     # Patch VPD base into the PcdPeim module patchable PCD
     python IntelFspPkg/Tools/PatchFv.py \
        $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV QUARKFV1:QUARKFV2:QUARK \
        "PcdPeim:__gPcd_BinaryPatch_PcdVpdBaseAddress, {[0x000000B8]}, @VPD PCD base"

     if [ $? -ne 0 ]
       then
         exit 1
     fi
     echo Patch is Done
     return 0
}

function CopyFspBinaryToBinPkg(){
    echo Copy FSP binary to QuarkFspBinPkg

    if [ ! -d $WORKSPACE/$FSP_BIN_PKG_NAME/$BD_TARGET ]
      then mkdir $WORKSPACE/$FSP_BIN_PKG_NAME/$BD_TARGET
    fi
    if [ -f $WORKSPACE/$FSP_BIN_PKG_NAME/$BD_TARGET/fsp.bin ]
      then rm -f $WORKSPACE/$FSP_BIN_PKG_NAME/$BD_TARGET/fsp.bin
    fi
    cp $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/QUARK.fd $WORKSPACE/$FSP_BIN_PKG_NAME/$BD_TARGET/fsp.bin

    if [ -f $WORKSPACE/$FSP_BIN_PKG_NAME/FspUpdVpd.h ]
      then rm -f $WORKSPACE/$FSP_BIN_PKG_NAME/FspUpdVpd.h
    fi
    if [ -f $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/FspUpdVpd.h ]
      then cp $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$TOOL_CHAIN/FV/FspUpdVpd.h $WORKSPACE/$FSP_BIN_PKG_NAME
    fi
}

function OverrideBaseTools() { 
   if [ -e $FSP_PKG_NAME/Override/BaseTools/Conf/build_rule.template ] 
    then 
     echo Overriding build_rule.template... 
      cp -f $FSP_PKG_NAME/Override/BaseTools/Conf/build_rule.template Conf/build_rule.txt 
   fi 

   if [ -e $FSP_PKG_NAME/Override/BaseTools/Conf/tools_def.template ] 
     then 
     echo Overriding tools_def.template... 
      cp -f $FSP_PKG_NAME/Override/BaseTools/Conf/tools_def.template Conf/tools_def.txt 
  fi 

 } 


if [ -d Conf ] 
 then
   . ./$Edksetup
 else
   mkdir Conf
   . ./$Edksetup
fi

if [ ! -d $WORKSPACE/$FSP_BIN_PKG_NAME ] 
 then
   mkdir $WORKSPACE/$FSP_BIN_PKG_NAME
fi

if [ "$1" = "-clean" ]
 then
  Clean
elif [ "$1" = "-r32" ]
 then
  ReleaseBuild32
  CopyFspBinaryToBinPkg
elif [ "$1" = "-d32" ]
 then 
   DebugBuild32
   CopyFspBinaryToBinPkg 
elif [ -z "$1" ] 
 then
   DebugBuild32
else 
  echo
  echo  ERROR: $1 is not valid parameter.
  USAGE
fi
