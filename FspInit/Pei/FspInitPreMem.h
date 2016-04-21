/** @file
The header file of FspInitPreMem PEIM.

Copyright (c) 2015 - 2016 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_INIT_PRE_MEM_H_
#define _FSP_INIT_PRE_MEM_H_

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/PciCf8Lib.h>
#include <Library/PciLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/CacheLib.h>
#include <Library/FspCommonLib.h>
#include <Library/FspPlatformLib.h>
#include <Library/FspSwitchStackLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>
#include <Library/ResetSystemLib.h>
#include <Guid/GuidHobFsp.h>
#include <Guid/MemoryConfigData.h>
#include <Guid/FspSiliconFv.h>
#include <Guid/FspSmbios.h>
#include <Ppi/Stall.h>
#include <Ppi/Reset.h>
#include <Ppi/MemoryDiscovered.h>
#include <FspApi.h>
#include <FspUpdVpd.h>
#include <Register/Cpuid.h>
#include <Include/IndustryStandard/SmBios.h>
#include <mrc.h>

#define PEI_STALL_RESOLUTION            1

#define DDRFREQ_UNKNOWN                 0
#define DDRFREQ_800MHZ                  800
#define DDRFREQ_1066MHZ                 1066

/**
This function reset the entire platform, including all processor and devices, and
reboots the system.

@param  PeiServices General purpose services available to every PEIM.

@retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
ResetSystem(
IN CONST EFI_PEI_SERVICES          **PeiServices
);

/**
This function provides a blocking stall for reset at least the given number of microseconds
stipulated in the final argument.

@param  PeiServices General purpose services available to every PEIM.

@param  this Pointer to the local data for the interface.

@param  Microseconds number of microseconds for which to stall.

@retval EFI_SUCCESS the function provided at least the required stall.
**/
EFI_STATUS
EFIAPI
Stall(
IN CONST EFI_PEI_SERVICES   **PeiServices,
IN CONST EFI_PEI_STALL_PPI  *This,
IN UINTN                    Microseconds
);

/**
  This function will be called when MRC is done.

  @param  PeiServices General purpose services available to every PEIM.

  @param  NotifyDescriptor Information about the notify event..

  @param  Ppi The notify context.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

#endif /* _FSP_INIT_PRE_MEM_H_ */
