#!/bin/sh
#
echo $PWD
export FSP_BASENAME=Quark
export BD_TARGET=DEBUG
export DEBUG_PRINT_ERROR_LEVEL="-DDEBUG_PRINT_ERROR_LEVEL=0x80000042"
export DEBUG_PROPERTY_MASK="-DDEBUG_PROPERTY_MASK=0x27"
toolchain=GCC48

export UPD_GUID=3E18A0B3-C3B5-492b-86B4-53E3D401C249
export OUT_DIR=Build
package_name=FspPkg
vpd_name=Vpd
export FSP_PKG_NAME=$FSP_BASENAME$package_name
export FSP_PKG_VPD_NAME=$FSP_BASENAME$package_name$vpd_name
export BD_MACRO="-D CFG_DEBUG=1 -D CFG_OUTDIR=$OUT_DIR $DEBUG_PRINT_ERROR_LEVEL $DEBUG_PROPERTY_MASK"
export BD_ARGS="-p $FSP_PKG_NAME/$FSP_PKG_NAME.dsc -a IA32 -t $toolchain -b $BD_TARGET -n 4 -DBAUD_RATE=921600 -Y PCD -Y LIBRARY -y ./Report$BD_TARGET.log $BD_MACRO"
#export

echo Remove the files ...
rm $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV/$UPD_GUID.bin
rm $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV/$UPD_GUID.map
rm $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV/$UPD_GUID.txt

echo Generate UPD Header File ...
python_command="python IntelFspPkg/Tools/GenCfgOpt.py UPDTXT  $FSP_PKG_NAME/$FSP_PKG_VPD_NAME.dsc  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"
python_command=$python_command"_$toolchain/FV  $BD_MACRO"
#echo $python_command
$python_command
echo UPD TXT file was generated successfully !

echo Generate VPD Header File ...    
bpdg_command="BaseTools/BinWrappers/PosixLike/BPDG  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV/$UPD_GUID.txt  -o $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV/$UPD_GUID.bin  -m $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV/$UPD_GUID.map"
#echo $bpdg_command
$bpdg_command

touch $FSP_PKG_NAME/Include/BootLoaderPlatformData.h
python_command="python IntelFspPkg/Tools/GenCfgOpt.py HEADER  $FSP_PKG_NAME/$FSP_PKG_VPD_NAME.dsc  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV  $FSP_PKG_NAME/Include/BootLoaderPlatformData.h  $BD_MACRO"
#echo $python_command
$python_command
echo Vpd header file was generated successfully !

echo Generate BSF File ...   
python_command="python IntelFspPkg/Tools/GenCfgOpt.py GENBSF  $FSP_PKG_NAME/$FSP_PKG_VPD_NAME.dsc  $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV  $FSP_PKG_NAME/Bsf/$FSP_BASENAME.bsf  $BD_MACRO"
echo $python_command
$python_command
echo BSF file was generated successfully !

copy_command="cp $OUT_DIR/$FSP_PKG_NAME/$BD_TARGET"_"$toolchain/FV/FspUpdVpd.h  $FSP_PKG_NAME/Include/FspUpdVpd.h"
echo $copy_command
$copy_command

#echo build $BD_ARGS
build $BD_ARGS

