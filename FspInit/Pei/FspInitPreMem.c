/** @file
This FspInitPreMem PEIM initialize system before MRC, following action is performed,
1. Get boot mode from boot loader.
2. Initialize MRC
3. TODO

Copyright (c) 2015 - 2016 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FspInitPreMem.h"
#include "CommonHeader.h"
#include "MrcWrapper.h"

EFI_PEI_NOTIFY_DESCRIPTOR mMemoryDiscoveredNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemoryDiscoveredPpiNotifyCallback
  }
};

STATIC EFI_PEI_STALL_PPI mStallPpi = {
  PEI_STALL_RESOLUTION,
  Stall
};

STATIC EFI_PEI_RESET_PPI mResetPpi = { ResetSystem };

STATIC EFI_PEI_PPI_DESCRIPTOR mPpiList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI),
    &gEfiPeiStallPpiGuid,
    &mStallPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiResetPpiGuid,
    &mResetPpi
  }
};

/**
  This function reset the entire platform, including all processor and devices, and
  reboots the system.

  @param  PeiServices General purpose services available to every PEIM.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
ResetSystem (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  ResetCold();
  return EFI_SUCCESS;
}

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
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI  *This,
  IN UINTN                    Microseconds
  )
{
  MicroSecondDelay (Microseconds);
  return EFI_SUCCESS;
}

/**
  Peform the boot mode determination logic
  If the box is closed, then
    1. If it's first time to boot, it's boot with full config .
    2. If the ChassisIntrution is selected, force to be a boot with full config
    3. Otherwise it's boot with no change.
  
  @param  PeiServices General purpose services available to every PEIM.   
  
  @param  BootMode The detected boot mode.
  
  @retval EFI_SUCCESS if the boot mode could be set
**/
EFI_STATUS
UpdateBootMode (
  IN  EFI_PEI_SERVICES     **PeiServices,
  OUT EFI_BOOT_MODE        *BootModePtr
  )
{
  EFI_STATUS          Status;
  FSP_INIT_RT_COMMON_BUFFER  *FspInitRtBuffer;
  CHAR16               *BootModeDescStr;

  //
  // Detect BootMode here
  //
  FspInitRtBuffer = (FSP_INIT_RT_COMMON_BUFFER *)((FSP_MEMORY_INIT_PARAMS *)GetFspApiParameter())->RtBufferPtr;
  
  if (FspInitRtBuffer != NULL) {
    //
    // Get BootMode from Boot Loader
    //
    *BootModePtr = (EFI_BOOT_MODE)FspInitRtBuffer->BootMode;

    //
    //Detect BootMode
    //
    switch (*BootModePtr) {
    case BOOT_WITH_FULL_CONFIGURATION:
	    BootModeDescStr = L"BOOT_WITH_FULL_CONFIGURATION";
	    break;
    case BOOT_WITH_MINIMAL_CONFIGURATION:
	    BootModeDescStr = L"BOOT_WITH_MINIMAL_CONFIGURATION";
	    break;
    case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
	    BootModeDescStr = L"BOOT_ASSUMING_NO_CONFIGURATION_CHANGES";
	    break;
    case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
	    BootModeDescStr = L"BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS";
	    break;
    case BOOT_WITH_DEFAULT_SETTINGS:
	    BootModeDescStr = L"BOOT_WITH_DEFAULT_SETTINGS";
	    break;
    case BOOT_ON_S4_RESUME:
	    BootModeDescStr = L"BOOT_ON_S4_RESUME";
	    break;
    case BOOT_ON_S5_RESUME:
	    BootModeDescStr = L"BOOT_ON_S5_RESUME";
	    break;
    case BOOT_WITH_MFG_MODE_SETTINGS:
	    BootModeDescStr = L"BOOT_WITH_MFG_MODE_SETTINGS";
	    break;
    case BOOT_ON_S2_RESUME:
	    BootModeDescStr = L"BOOT_ON_S2_RESUME";
	    break;
    case BOOT_ON_S3_RESUME:
	    BootModeDescStr = L"BOOT_ON_S3_RESUME";
	    break;
    case BOOT_ON_FLASH_UPDATE:
	    BootModeDescStr = L"BOOT_ON_FLASH_UPDATE";
	    break;
    case BOOT_IN_RECOVERY_MODE:
	    BootModeDescStr = L"BOOT_IN_RECOVERY_MODE";
	    break;
    default:
	    BootModeDescStr = L"UNKOWN";
    }
  } else {
    //
    // Fail to get BootMode from Boot Loader
    //
    DEBUG((EFI_D_ERROR, "No valid BootMode, using default!\n"));

    //
    // Set FULL CONFIGRATION
    //
    BootModeDescStr = L"FullCfg";
    *BootModePtr = BOOT_WITH_FULL_CONFIGURATION;
  }

  if (BootModeDescStr != NULL) {
    DEBUG ((EFI_D_INFO, "BootMode: %s\n", BootModeDescStr));
  }

  Status = PeiServicesSetBootMode (*BootModePtr);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;  
}

/**
  This function will be called when MRC is done.

  @param  PeiServices General purpose services available to every PEIM.

  @retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
FspSpecificMemoryDiscoveredHook (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_BOOT_MODE                BootMode
)
{
  EFI_STATUS                            Status = EFI_SUCCESS;
  UINT32                                RegData32;
  UINT8                                 CpuAddressWidth;
  UINT32                                RegEax;

  DEBUG ((EFI_D_INFO, "FSP Specific PEIM Memory Callback\n"));

  //
  // Do QNC initialization after MRC
  //
  PeiQNCPostMemInit ();

  //
  // Set E000/F000 Routing
  //
  RegData32 = QNCPortRead (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC);
  RegData32 |= (BIT2|BIT1);
  QNCPortWrite (QUARK_NC_HOST_BRIDGE_SB_PORT_ID, QNC_MSG_FSBIC_REG_HMISC, RegData32);

  if (BootMode == BOOT_ON_S3_RESUME) {
    return EFI_SUCCESS;
  }

  //
  // Build flash HOB, it's going to be used by GCD and E820 building
  // Map full SPI flash decode range (regardless of smaller SPI flash parts installed)
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_FIRMWARE_DEVICE,
    (EFI_RESOURCE_ATTRIBUTE_PRESENT    |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE),
    (SIZE_4GB - SIZE_8MB),
    SIZE_8MB
    );

  //
  // Create a CPU hand-off information
  //
  CpuAddressWidth = 32;
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &RegEax, NULL, NULL, NULL);
    CpuAddressWidth = (UINT8) (RegEax & 0xFF);
  }
  DEBUG ((EFI_D_INFO, "CpuAddressWidth: %d\n", CpuAddressWidth));

  BuildCpuHob (CpuAddressWidth, 16);

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  This function reports and installs new FV

  @retval     EFI_SUCCESS          The function completes successfully
**/
EFI_STATUS
ReportAndInstallNewFv (
  VOID
)
{
  EFI_FIRMWARE_VOLUME_EXT_HEADER *FwVolExtHeader;
  FSP_INFO_HEADER                *FspInfoHeader;
  EFI_FIRMWARE_VOLUME_HEADER     *FvHeader;
  UINT8                          *CurPtr;
  UINT8                          *EndPtr;

  FspInfoHeader = GetFspInfoHeaderFromApiContext();
  if (FspInfoHeader->Signature != FSP_INFO_HEADER_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "The signature of FspInfoHeader getting from API context is invalid.\n"));
    FspInfoHeader = GetFspInfoHeader();
  }

  CurPtr = (UINT8 *)FspInfoHeader->ImageBase;
  EndPtr = CurPtr + FspInfoHeader->ImageSize - 1;

  while (CurPtr < EndPtr) {
    FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)CurPtr;
    if (FvHeader->Signature != EFI_FVH_SIGNATURE) {
      break;
    }

    if (FvHeader->ExtHeaderOffset != 0) {
      //
      // Searching for the silicon FV in the FSP image.
      //
      FwVolExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) ((UINT8 *) FvHeader + FvHeader->ExtHeaderOffset);
      if (CompareGuid(&FwVolExtHeader->FvName, &gFspSiliconFvGuid)) {
        PeiServicesInstallFvInfoPpi (
          NULL,
          (VOID *)FvHeader,
          (UINTN) FvHeader->FvLength,
          NULL,
          NULL
          );
      }
    }
    CurPtr += FvHeader->FvLength;
  }

  return EFI_SUCCESS;
}

/**
  Build FSP SMBIOS memory info HOB

  @param[in] VOID
**/
VOID
BuildFspSmbiosMemoryInfoHob (
  VOID
  )
{
  FSP_SMBIOS_MEMORY_INFO      FspSmbiosMemoryInfo;
  UINT8                       ChannelIndex;
  UINT8                       ChannelCount;
  UINT8                       DimmIndex;
  UINT8                       DimmCount;

  FspSmbiosMemoryInfo.Revision = 0x01;
  FspSmbiosMemoryInfo.MemoryType = MemoryTypeDdr3;
  FspSmbiosMemoryInfo.MemoryFrequencyInMHz = DDRFREQ_800MHZ;
  FspSmbiosMemoryInfo.ErrorCorrectionType = ErrorDetectingMethodNone;

  ChannelCount = 0;
  for (ChannelIndex = 0; ChannelIndex < NUM_CHANNELS; ChannelIndex++) {
    DimmCount = 0;
    FspSmbiosMemoryInfo.ChannelInfo[ChannelIndex].ChannelId = ChannelIndex;
    for (DimmIndex = 0; DimmIndex < MAX_SOCKETS; DimmIndex++) {
      FspSmbiosMemoryInfo.ChannelInfo[ChannelIndex].DimmInfo[DimmIndex].DimmId = DimmIndex;
      //TODO: Update SizeInMb
      FspSmbiosMemoryInfo.ChannelInfo[ChannelIndex].DimmInfo[DimmIndex].SizeInMb = 128;
        ///
        /// Dimm is present in slot
        /// Get the Memory DataWidth info
        /// SPD Offset 8 Bits [2:0] DataWidth aka Primary Bus Width
        ///
        FspSmbiosMemoryInfo.DataWidth = 16;
      DimmCount++;
    }
    FspSmbiosMemoryInfo.ChannelInfo[ChannelIndex].DimmCount = DimmCount;
    ChannelCount++;
  }
  FspSmbiosMemoryInfo.ChannelCount = ChannelCount;

  //
  // Build HOB for FspSmbiosMemoryInfo
  //
  BuildGuidDataHob (
    &gFspSmbiosMemoryInfoHobGuid,
    &FspSmbiosMemoryInfo,
    sizeof (FSP_SMBIOS_MEMORY_INFO)
    );
}

/**
This function will be called when MRC is done.

@param  PeiServices General purpose services available to every PEIM.

@param  NotifyDescriptor Information about the notify event..

@param  Ppi The notify context.

@retval EFI_SUCCESS If the function completed successfully.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback(
IN EFI_PEI_SERVICES           **PeiServices,
IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
IN VOID                       *Ppi
)
{
  EFI_STATUS             Status;
  UINT8                  ApiMode;
  EFI_BOOT_MODE          BootMode;
  EFI_HOB_GUID_TYPE      *GuidHob;
  FSP_MEMORY_INIT_PARAMS   *FspMemoryInitParams;
  UINT32                 BootLoaderTolumSize;

  DEBUG((DEBUG_INFO, "Memory Discovered Notify invoked ...\n"));

  // 
  // Get pointer of FSP_MEMORY_INIT_PARAMS(FSP_INIT_PARAMS) structure
  //
  FspMemoryInitParams = (FSP_MEMORY_INIT_PARAMS *)GetFspApiParameter();

  // 
  // Migrate bootloader data before destroying CAR
  //
  FspMigrateTemporaryMemory();

  // 
  // Get Boot Mode
  //
  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status);

  // 
  // FSP specific hook
  //
  FspSpecificMemoryDiscoveredHook(PeiServices, BootMode);

  //
  // FSP Reserved Memory - TODO: Porting it for Quark SoC, cbmem_top is forced to 0xAFD0000.
  //
  BuildResourceDescriptorWithOwnerHob(
	EFI_RESOURCE_MEMORY_RESERVED,
	(
	EFI_RESOURCE_ATTRIBUTE_PRESENT |
	EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
	EFI_RESOURCE_ATTRIBUTE_TESTED |
	EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
	EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
	EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
	EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
	),
	0xAECE000,
	PcdGet32(PcdFspReservedMemoryLength),
	&gFspReservedMemoryResourceHobGuid
  );

  //
  // FSP BOOTLOADER TOLUM HOB - TODO: Porting it for Quark SoC.
  //
  BootLoaderTolumSize = ((FSP_INIT_RT_COMMON_BUFFER *)FspMemoryInitParams->RtBufferPtr)->BootLoaderTolumSize;

  if (BootLoaderTolumSize > 0) {
    BuildResourceDescriptorWithOwnerHob (
      EFI_RESOURCE_MEMORY_RESERVED,            // MemoryType,
      (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_TESTED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
      ),
      0xAFCE000,
      BootLoaderTolumSize,
      &gFspBootLoaderTolumHobGuid
      );
  }

  //
  // Build FSP Non-Volatile Storage Hob
  //
  if (BootMode  != BOOT_ON_S3_RESUME) {
    GuidHob = GetFirstGuidHob (&gEfiMemoryConfigDataGuid);
    if (GuidHob != NULL) {
      //
      // Include the EFI_HOB_GENERIC_HEADER header in HOB.
      //
      BuildGuidDataHob (&gFspNonVolatileStorageHobGuid, (void *)((UINTN)GET_GUID_HOB_DATA (GuidHob) - sizeof (EFI_HOB_GUID_TYPE)), GET_GUID_HOB_DATA_SIZE (GuidHob) + sizeof (EFI_HOB_GUID_TYPE));
    }
  }

  // Create SMBIOS Memory Info HOB
  DEBUG((DEBUG_INFO | DEBUG_INIT, "BuildFspSmbiosMemoryInfoHob\n"));
  BuildFspSmbiosMemoryInfoHob ();

  ApiMode = GetFspApiCallingMode();
  if (ApiMode != 0) {
    //
    // Calling use FspMemoryInit API
    // Return the control directly
    //
    if ((FspMemoryInitParams->HobListPtr) != NULL) {
      *(FspMemoryInitParams->HobListPtr) = (VOID *)GetHobList();
    }
    //
    // This is the end of the FspMemoryInit API
    // Give control back to the boot loader
    //
    DEBUG((DEBUG_INFO | DEBUG_INIT, "FspMemoryInitApi() - End\n"));
    SetFspApiReturnStatus(EFI_SUCCESS);
    Pei2LoaderSwitchStack();
  }

  //
  // Disable CAR and resets fixed and variable MTRR values.
  //
  //ResetCacheAttributes(); // TODO - Porting it for Quark SoC.
  if (ApiMode != 0) {
    //
    // This is the end of the TempRamExit API
    // Give control back to the boot loader
    //
    DEBUG((DEBUG_INFO | DEBUG_INIT, "TempRamExitApi() - End\n"));
    SetFspApiReturnStatus(EFI_SUCCESS);
    Pei2LoaderSwitchStack();
  }

  //
  // Install FSP silicon FV
  //
  ReportAndInstallNewFv();

  DEBUG((DEBUG_INFO, "Memory Discovered Notify completed ...\n"));

  return EFI_SUCCESS;
}

/**

Do FSP Pre-Memory initialization

@param  FfsHeader    Not used.
@param  PeiServices  General purpose services available to every PEIM.

@return EFI_SUCCESS  Memory initialization completed successfully.
All other error conditions encountered result in an ASSERT.

**/
EFI_STATUS
PeimFspInitPreMem(
IN       EFI_PEI_FILE_HANDLE  FileHandle,
IN CONST EFI_PEI_SERVICES     **PeiServices
)
{
  EFI_STATUS                 Status;
  UPD_DATA_REGION        *UpdDataRgnPtr;
  MEMORY_INIT_UPD        *MemoryInitUpd;
  SILICON_INIT_UPD       *SiliconInitUpd;
  EFI_BOOT_MODE          BootMode;
  UINT8                  ApiMode;

  //
  // Get the UPD pointer.
  //
  ApiMode = GetFspApiCallingMode();
  UpdDataRgnPtr = (UPD_DATA_REGION *)GetFspUpdDataPointer();
  if (ApiMode == 0) {
    //
    // FSP v1.0 mode, we update the MemoryInitUpd from Upd
    //
    MemoryInitUpd = (MEMORY_INIT_UPD *)(&(UpdDataRgnPtr->MemoryInitUpd));
    SiliconInitUpd = (SILICON_INIT_UPD *)(&(UpdDataRgnPtr->SiliconInitUpd));
    SetFspMemoryInitUpdDataPointer(MemoryInitUpd);
    SetFspSiliconInitUpdDataPointer(SiliconInitUpd);
  }
  else {
    MemoryInitUpd = (MEMORY_INIT_UPD *)UpdDataRgnPtr;
    SetFspMemoryInitUpdDataPointer(MemoryInitUpd);
    SetFspSiliconInitUpdDataPointer((void *)NULL);
  }

  //
  // Now that all of the pre-permament memory activities have
  // been taken care of, post a call-back for the permament-memory
  // resident services, such as HOB construction.
  // PEI Core will switch stack after this PEIM exit.  After that the MTRR
  // can be set.
  //
  Status = (**PeiServices).NotifyPpi(PeiServices, &mMemoryDiscoveredNotifyList[0]);
  ASSERT_EFI_ERROR(Status);

  //
  // Install serivce PPIs
  //
  Status = (**PeiServices).InstallPpi(PeiServices, mPpiList);
  ASSERT_EFI_ERROR(Status);

  //
  // Get boot mode from boot loader
  //
  Status = UpdateBootMode ((EFI_PEI_SERVICES**)PeiServices, &BootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // Do SOC Init Pre memory init.
  //
  PeiQNCPreMemInit ();

  //
  // Make legacy SPI READ/WRITE enabled if not a secure build
  //
  LpcPciCfg32And (R_QNC_LPC_BIOS_CNTL, ~B_QNC_LPC_BIOS_CNTL_BIOSWE);

  DEBUG((EFI_D_INFO, "MRC Entry\n"));
  MemoryInit((EFI_PEI_SERVICES**)PeiServices);

  return Status;
}
