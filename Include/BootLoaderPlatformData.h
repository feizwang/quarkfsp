/** @file
TODO

Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BOOTLOADER_PLATFORM_DATA_H
#define _BOOTLOADER_PLATFORM_DATA_H

//
// @note
// All lines between tag EXTERNAL_BOOTLOADER_STRUCT_BEGIN and
// EXTERNAL_BOOTLOADER_STRUCT_END will be exported to the generated
// FspUpdVpd.h file by the tool.
//

#pragma pack(push, 1)

/* !EXPORT EXTERNAL_BOOTLOADER_STRUCT_BEGIN */

/** TODO - Port to fit Quark SoC.
**/

#define MAX_CHANNELS_NUM       1
#define MAX_DIMMS_NUM          1

typedef struct {
	UINT8         DimmId;
	UINT32        SizeInMb;
	UINT16        MfgId;
	/** Module part number for DDR3 is 18 bytes however for
	 DRR4 20 bytes as per JEDEC Spec, so reserving 20 bytes
	 **/
	UINT8         ModulePartNum[20];
} DIMM_INFO;

typedef struct {
	UINT8         ChannelId;
	UINT8         DimmCount;
	DIMM_INFO     DimmInfo[MAX_DIMMS_NUM];
} CHANNEL_INFO;

typedef struct {
	UINT8         Revision;
	UINT16        DataWidth;
	/** As defined in SMBIOS 3.0 spec
	Section 7.18.2 and Table 75
	**/
	UINT8         MemoryType;
	UINT16        MemoryFrequencyInMHz;
	/** As defined in SMBIOS 3.0 spec
	Section 7.17.3 and Table 72
	**/
	UINT8         ErrorCorrectionType;
	UINT8         ChannelCount;
	CHANNEL_INFO  ChannelInfo[MAX_CHANNELS_NUM];
} FSP_SMBIOS_MEMORY_INFO;

/* !EXPORT EXTERNAL_BOOTLOADER_STRUCT_END   */

#pragma pack(pop)

#endif


