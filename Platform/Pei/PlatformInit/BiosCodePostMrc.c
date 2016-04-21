/** @file
This PEIM initialize platform for MRC, following action is performed,
1. Initizluize GMCH
2. Detect boot mode
3. Detect video adapter to determine whether we need pre allocated memory
4. Calls MRC to initialize memory and install a PPI notify to do post memory initialization.
This file contains the main entrypoint of the PEIM.

Copyright (c) 2013 - 2016 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "CommonHeader.h"
#include "PlatformEarlyInit.h"

//
// Function prototypes to routines implemented in other source modules
// within this component.
//

EFI_STATUS
EFIAPI
PlatformErratasPostMrc (
  VOID
  );

//
// The global indicator, the FvFileLoader callback will modify it to TRUE after loading PEIM into memory
//
BOOLEAN ImageInMemory = FALSE;

/**
  Initialize state of I2C GPIO expanders.

  @param  PlatformType  Platform type for GPIO expander init.

**/
EFI_STATUS
EarlyPlatformConfigGpioExpanders (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  EFI_STATUS              Status;
  EFI_I2C_DEVICE_ADDRESS  I2CSlaveAddress;
  UINTN                   Length;
  UINTN                   ReadLength;
  UINT8                   Buffer[2];

  //
  // Configure GPIO expanders for Galileo Gen 2
  // Route I2C pins to Arduino header
  // Set all GPIO expander pins connected to the Reset Button as inputs
  //
  if (PlatformType == GalileoGen2) {
    //
    // Configure AMUX1_IN (EXP2.P1_4) as an output
    //
    PlatformPcal9555GpioSetDir (
      GALILEO_GEN2_IOEXP2_7BIT_SLAVE_ADDR,  // IO Expander 2.
      12,                                   // P1-4.
      FALSE                                 // Configure as output
      );

    //
    // Set AMUX1_IN(EXP2.P1_4) low to route I2C to Arduino Shield connector
    //
    PlatformPcal9555GpioSetLevel (
      GALILEO_GEN2_IOEXP2_7BIT_SLAVE_ADDR,  // IO Expander 2.
      12,                                   // P1-4.
      FALSE                                 // Set pin low
      );

    //
    // Configure Reset Button(EXP1.P1_7) as an input
    //
    PlatformPcal9555GpioSetDir (
      GALILEO_GEN2_IOEXP1_7BIT_SLAVE_ADDR,  // IO Expander 1.
      15,                                   // P1-7.
      TRUE
      );

    //
    // Disable pullup on Reset Button(EXP1.P1_7)
    //
    PlatformPcal9555GpioDisablePull (
      GALILEO_GEN2_IOEXP1_7BIT_SLAVE_ADDR,  // IO Expander 1.
      15                                    // P1-7.
      );

    //
    // Configure Reset Button(EXP2.P1_7) as an input
    //
    PlatformPcal9555GpioSetDir (
      GALILEO_GEN2_IOEXP2_7BIT_SLAVE_ADDR,  // IO Expander 2.
      15,                                   // P1-7.
      TRUE
      );

    //
    // Disable pullup on Reset Button(EXP2.P1_7)
    //
    PlatformPcal9555GpioDisablePull (
      GALILEO_GEN2_IOEXP2_7BIT_SLAVE_ADDR,  // IO Expander 2.
      15                                    // P1-7.
      );
  }

  //
  // Configure GPIO expanders for Galileo Gen 2
  // Set all GPIO expander pins connected to the Reset Button as inputs
  // Route I2C pins to Arduino header
  //
  if (PlatformType == Galileo) {
    //
    // Detect the I2C Slave Address of the GPIO Expander
    //
    if (PlatformLegacyGpioGetLevel (R_QNC_GPIO_RGLVL_RESUME_WELL, GALILEO_DETERMINE_IOEXP_SLA_RESUMEWELL_GPIO)) {
      I2CSlaveAddress.I2CDeviceAddress = GALILEO_IOEXP_J2HI_7BIT_SLAVE_ADDR;
    } else {
      I2CSlaveAddress.I2CDeviceAddress = GALILEO_IOEXP_J2LO_7BIT_SLAVE_ADDR;
    }
    DEBUG ((EFI_D_INFO, "Galileo GPIO Expender Slave Address = %02x\n", I2CSlaveAddress.I2CDeviceAddress));

    //
    // Set I2C_MUX (GPORT1_BIT5) low to route I2C to Arduino Shield connector
    //

    //
    // Select GPIO Expander GPORT1
    //
    Length = 2;
    Buffer[0] = 0x18; //sub-address
    Buffer[1] = 0x01; //data
    Status = I2cWriteMultipleByte (
      I2CSlaveAddress,
      EfiI2CSevenBitAddrMode,
      &Length,
      &Buffer
      );
    ASSERT_EFI_ERROR (Status);

    //
    // Read "Pin Direction" of GPIO Expander GPORT1
    //
    Length = 1;
    ReadLength = 1;
    Buffer[1] = 0x1C;
    Status = I2cReadMultipleByte (
      I2CSlaveAddress,
      EfiI2CSevenBitAddrMode,
      &Length,
      &ReadLength,
      &Buffer[1]
      );
    ASSERT_EFI_ERROR (Status);

    //
    // Configure GPIO Expander GPORT1_BIT5 as an output
    //
    Length = 2;
    Buffer[0] = 0x1C; //sub-address
    Buffer[1] = (UINT8)(Buffer[1] & (~BIT5)); //data

    Status = I2cWriteMultipleByte (
      I2CSlaveAddress,
      EfiI2CSevenBitAddrMode,
      &Length,
      &Buffer
      );
    ASSERT_EFI_ERROR (Status);

    //
    // Set GPIO Expander GPORT1_BIT5 low
    //
    Length = 2;
    Buffer[0] = 0x09; //sub-address
    Buffer[1] = (UINT8)(~BIT5); //data

    Status = I2cWriteMultipleByte (
      I2CSlaveAddress,
      EfiI2CSevenBitAddrMode,
      &Length,
      &Buffer
      );
    ASSERT_EFI_ERROR (Status);

    //
    // Configure RESET_N_SHLD (GPORT5_BIT0) and SW_RESET_N_SHLD (GPORT5_BIT1) as inputs
    //

    //
    // Select GPIO Expander GPORT5
    //
    Length = 2;
    Buffer[0] = 0x18;
    Buffer[1] = 0x05;
    Status = I2cWriteMultipleByte (
      I2CSlaveAddress,
      EfiI2CSevenBitAddrMode,
      &Length,
      &Buffer
      );
    ASSERT_EFI_ERROR (Status);

    //
    // Read "Pin Direction" of GPIO Expander GPORT5
    //
    Length = 1;
    ReadLength = 1;
    Buffer[1] = 0x1C;
    Status = I2cReadMultipleByte (
      I2CSlaveAddress,
      EfiI2CSevenBitAddrMode,
      &Length,
      &ReadLength,
      &Buffer[1]
      );
    ASSERT_EFI_ERROR (Status);

    //
    // Configure GPIO Expander GPORT5_BIT0 and GPORT5_BIT1 as inputs
    //
    Length = 2;
    Buffer[0] = 0x1C;
    Buffer[1] = Buffer[1] | BIT0 | BIT1;
    Status = I2cWriteMultipleByte (
      I2CSlaveAddress,
      EfiI2CSevenBitAddrMode,
      &Length,
      &Buffer
      );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

/**
  This is the entrypoint of PEIM

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
BiosAfterMemoryInit(
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                              Status = EFI_SUCCESS;
  EFI_BOOT_MODE                           BootMode;
  EFI_PLATFORM_TYPE                       PlatformType;

  MEMORY_INIT_UPD  *MemoryInitUpdPtr = (MEMORY_INIT_UPD *)GetFspMemoryInitUpdDataPointer();
  PlatformType = MemoryInitUpdPtr->PcdPlatformType;

  //
  // Do platform specific logic to create a boot mode
  //
  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status);

  //
  // Do Early PCIe init.
  //
  DEBUG ((EFI_D_INFO, "Early PCIe controller initialization\n"));
  PlatformPciExpressEarlyInit (PlatformType);


  DEBUG ((EFI_D_INFO, "Platform Erratas After MRC\n"));
  PlatformErratasPostMrc ();

  DEBUG ((EFI_D_INFO, "EarlyPlatformConfigGpioExpanders ()\n"));
  EarlyPlatformConfigGpioExpanders (PlatformType);

  return Status;
}
