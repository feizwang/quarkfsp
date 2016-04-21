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

//
// USB Phy Registers
//
#define USB2_GLOBAL_PORT  0x4001
#define USB2_PLL1         0x7F02
#define USB2_PLL2         0x7F03
#define USB2_COMPBG       0x7F04

//
// The global indicator, the FvFileLoader callback will modify it to TRUE after loading PEIM into memory
//
BOOLEAN ImageInMemory = FALSE;

BOARD_LEGACY_GPIO_CONFIG      mBoardLegacyGpioConfigTable[]  = { PLATFORM_LEGACY_GPIO_TABLE_DEFINITION };
UINTN                         mBoardLegacyGpioConfigTableLen = (sizeof(mBoardLegacyGpioConfigTable) / sizeof(BOARD_LEGACY_GPIO_CONFIG));
BOARD_GPIO_CONTROLLER_CONFIG  mBoardGpioControllerConfigTable[]  = { PLATFORM_GPIO_CONTROLLER_CONFIG_DEFINITION };
UINTN                         mBoardGpioControllerConfigTableLen = (sizeof(mBoardGpioControllerConfigTable) / sizeof(BOARD_GPIO_CONTROLLER_CONFIG));
UINT8                         ChipsetDefaultMac [6] = {0xff,0xff,0xff,0xff,0xff,0xff};

/**
  Set Mac address on chipset ethernet device.

  @param  Bus      PCI Bus number of chipset ethernet device.
  @param  Device   Device number of chipset ethernet device.
  @param  Func     PCI Function number of chipset ethernet device.
  @param  MacAddr  MAC Address to set.

**/
VOID
EFIAPI
SetLanControllerMacAddr (
  IN CONST UINT8                          Bus,
  IN CONST UINT8                          Device,
  IN CONST UINT8                          Func,
  IN CONST UINT8                          *MacAddr,
  IN CONST UINT32                         Bar0
  )
{
  UINT32                            Data32;
  UINT16                            PciVid;
  UINT16                            PciDid;
  UINT32                            Addr;
  UINT32                            MacVer;
  volatile UINT8                    *Wrote;
  UINT32                            DevPcieAddr;
  UINT16                            SaveCmdReg;
  UINT32                            SaveBarReg;

  DevPcieAddr = PCI_LIB_ADDRESS (
                  Bus,
                  Device,
                  Func,
                  0
                  );

  //
  // Do nothing if not a supported device.
  //
  PciVid = PciRead16 (DevPcieAddr + PCI_VENDOR_ID_OFFSET);
  PciDid = PciRead16 (DevPcieAddr + PCI_DEVICE_ID_OFFSET);
  if((PciVid != V_IOH_MAC_VENDOR_ID) || (PciDid != V_IOH_MAC_DEVICE_ID)) {
    return;
  }

  //
  // Save current settings for PCI CMD/BAR registers
  //
  SaveCmdReg = PciRead16 (DevPcieAddr + PCI_COMMAND_OFFSET);
  SaveBarReg = PciRead32 (DevPcieAddr + R_IOH_MAC_MEMBAR);

  //
  // Use predefined tempory memory resource
  //
  PciWrite32 ( DevPcieAddr + R_IOH_MAC_MEMBAR, Bar0);
  PciWrite8 ( DevPcieAddr + PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_MEMORY_SPACE);

  Addr =  Bar0 + R_IOH_MAC_GMAC_REG_8;
  MacVer = *((volatile UINT32 *) (UINTN)(Addr));

  DEBUG ((EFI_D_INFO, "Ioh MAC [B:%d, D:%d, F:%d] VER:%04x ADDR:",
    (UINTN) Bus,
    (UINTN) Device,
    (UINTN) Func,
    (UINTN) MacVer
    ));

  //
  // Set MAC Address0 Low Register (GMAC_REG_17) ADDRLO bits.
  //
  Addr =  Bar0 + R_IOH_MAC_GMAC_REG_17;
  Data32 = *((UINT32 *) (UINTN)(&MacAddr[0]));
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;
  Wrote = (volatile UINT8 *) (UINTN)(Addr);
  DEBUG ((EFI_D_INFO, "%02x-%02x-%02x-%02x-",
    (UINTN) Wrote[0],
    (UINTN) Wrote[1],
    (UINTN) Wrote[2],
    (UINTN) Wrote[3]
    ));

  //
  // Set MAC Address0 High Register (GMAC_REG_16) ADDRHI bits
  // and Address Enable (AE) bit.
  //
  Addr =  Bar0 + R_IOH_MAC_GMAC_REG_16;
  Data32 =
    ((UINT32) MacAddr[4]) |
    (((UINT32)MacAddr[5]) << 8) |
    B_IOH_MAC_AE;
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;
  Wrote = (volatile UINT8 *) (UINTN)(Addr);

  DEBUG ((EFI_D_INFO, "%02x-%02x\n", (UINTN) Wrote[0], (UINTN) Wrote[1]));

  //
  // Restore settings for PCI CMD/BAR registers
  //
  PciWrite32 ((DevPcieAddr + R_IOH_MAC_MEMBAR), SaveBarReg);
  PciWrite16 (DevPcieAddr + PCI_COMMAND_OFFSET, SaveCmdReg);
}

/**
  This function will initialize USB Phy registers associated with QuarkSouthCluster.

  @param  VOID                  No Argument

  @retval EFI_SUCCESS           All registers have been initialized
**/
VOID
EFIAPI
InitializeUSBPhy (
    VOID
   )
{
    UINT32 RegData32;

    /** In order to configure the PHY to use clk120 (ickusbcoreclk) as PLL reference clock
     *  and Port2 as a USB device port, the following sequence must be followed
     *
     **/

    // Sideband register write to USB AFE (Phy)
    RegData32 = QNCAltPortRead (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_GLOBAL_PORT);
    RegData32 &= ~(BIT1);
    //
    // Sighting #4930631 PDNRESCFG [8:7] of USB2_GLOBAL_PORT = 11b.
    // For port 0 & 1 as host and port 2 as device.
    //
    RegData32 |= (BIT8 | BIT7);
    QNCAltPortWrite (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_GLOBAL_PORT, RegData32);

    //
    // Sighting #4930653 Required BIOS change on Disconnect vref to change to 600mV.
    //
    RegData32 = QNCAltPortRead (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_COMPBG);
    RegData32 &= ~(BIT10 | BIT9 | BIT8 | BIT7);
    RegData32 |= (BIT10 | BIT7);
    QNCAltPortWrite (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_COMPBG, RegData32);

    // Sideband register write to USB AFE (Phy)
    // (pllbypass) to bypass/Disable PLL before switch
    RegData32 = QNCAltPortRead (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL2);
    RegData32 |= BIT29;
    QNCAltPortWrite (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL2, RegData32);

    // Sideband register write to USB AFE (Phy)
    // (coreclksel) to select 120MHz (ickusbcoreclk) clk source.
    // (Default 0 to select 96MHz (ickusbclk96_npad/ppad))
    RegData32 = QNCAltPortRead (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL1);
    RegData32 |= BIT1;
    QNCAltPortWrite (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL1, RegData32);

    // Sideband register write to USB AFE (Phy)
    // (divide by 8) to achieve internal 480MHz clock
    // for 120MHz input refclk.  (Default: 4'b1000 (divide by 10) for 96MHz)
    RegData32 = QNCAltPortRead (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL1);
    RegData32 &= ~(BIT5 | BIT4 | BIT3);
    RegData32 |= BIT6;
    QNCAltPortWrite (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL1, RegData32);

    // Sideband register write to USB AFE (Phy)
    // Clear (pllbypass)
    RegData32 = QNCAltPortRead (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL2);
    RegData32 &= ~BIT29;
    QNCAltPortWrite (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL2, RegData32);

    // Sideband register write to USB AFE (Phy)
    // Set (startlock) to force the PLL FSM to restart the lock
    // sequence due to input clock/freq switch.
    RegData32 = QNCAltPortRead (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL2);
    RegData32 |= BIT24;
    QNCAltPortWrite (QUARK_SC_USB_AFE_SB_PORT_ID, USB2_PLL2, RegData32);

    // At this point the PLL FSM and COMP FSM will complete

}

/**
  This function provides early platform Thermal sensor initialisation.
**/
VOID
EFIAPI
EarlyPlatformThermalSensorInit (
  VOID
  )
{
  DEBUG ((EFI_D_INFO, "Early Platform Thermal Sensor Init\n"));

  //
  // Set Thermal sensor mode.
  //
  QNCThermalSensorSetRatiometricMode ();

  //
  // Enable RMU Thermal sensor with a Catastrophic Trip point.
  //
  QNCThermalSensorEnableWithCatastrophicTrip (PLATFORM_CATASTROPHIC_TRIP_CELSIUS);

  //
  // Lock all RMU Thermal sensor control & trip point registers.
  //
  QNCThermalSensorLockAllRegisters ();
}

/**
  This function provides early platform Legacy GPIO initialisation.

  @param  PlatformType  Platform type for GPIO init.

**/
VOID
EFIAPI
EarlyPlatformLegacyGpioInit (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  BOARD_LEGACY_GPIO_CONFIG          *LegacyGpioConfig;
  UINT32                            NewValue;
  UINT32                            GpioBaseAddress;

  //
  // Assert if platform type outside table range.
  //
  ASSERT ((UINTN) PlatformType < mBoardLegacyGpioConfigTableLen);
  LegacyGpioConfig = &mBoardLegacyGpioConfigTable[(UINTN) PlatformType];

  GpioBaseAddress = (UINT32)PcdGet16 (PcdGbaIoBaseAddress);

  NewValue     = 0x0;
  //
  // Program QNC GPIO Registers.
  //
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGEN_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGEN_CORE_WELL, NewValue );
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGIO_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellIoSelect;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGIO_CORE_WELL, NewValue);
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGLVL_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellLvlForInputOrOutput;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGLVL_CORE_WELL, NewValue);
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGTPE_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellTriggerPositiveEdge;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGTPE_CORE_WELL, NewValue );
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGTNE_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellTriggerNegativeEdge;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGTNE_CORE_WELL, NewValue);
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGGPE_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellGPEEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGGPE_CORE_WELL, NewValue);
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGSMI_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellSMIEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGSMI_CORE_WELL, NewValue );
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CGTS_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellTriggerStatus;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CGTS_CORE_WELL, NewValue);
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_CNMIEN_CORE_WELL) & 0xFFFFFFFC) | LegacyGpioConfig->CoreWellNMIEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_CNMIEN_CORE_WELL, NewValue);

  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGEN_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGEN_RESUME_WELL, NewValue );
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGIO_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellIoSelect;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGIO_RESUME_WELL, NewValue) ;
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGLVL_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellLvlForInputOrOutput;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGLVL_RESUME_WELL, NewValue);
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGTPE_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellTriggerPositiveEdge;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGTPE_RESUME_WELL, NewValue );
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGTNE_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellTriggerNegativeEdge;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGTNE_RESUME_WELL, NewValue) ;
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGGPE_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellGPEEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGGPE_RESUME_WELL, NewValue);
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGSMI_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellSMIEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGSMI_RESUME_WELL, NewValue );
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RGTS_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellTriggerStatus;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RGTS_RESUME_WELL, NewValue) ;
  NewValue = (IoRead32 (GpioBaseAddress + R_QNC_GPIO_RNMIEN_RESUME_WELL) & 0xFFFFFFC0) | LegacyGpioConfig->ResumeWellNMIEnable;
  IoWrite32 (GpioBaseAddress + R_QNC_GPIO_RNMIEN_RESUME_WELL, NewValue);
}

/**
  Performs any early platform specific Legacy GPIO manipulation.

  @param  PlatformType  Platform type GPIO manipulation.

**/
VOID
EFIAPI
EarlyPlatformLegacyGpioManipulation (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  if (PlatformType == CrossHill) {

    //
    // Pull TPM reset low for 80us (equivalent to cold reset, Table 39
    // Infineon SLB9645 Databook), then pull TPM reset high and wait for
    // 150ms to give time for TPM to stabilise (Section 4.7.1 Infineon
    // SLB9645 Databook states TPM is ready to receive command after 30ms
    // but section 4.7 states some TPM commands may take longer to execute
    // upto 150ms after test).
    //

    PlatformLegacyGpioSetLevel (
      R_QNC_GPIO_RGLVL_RESUME_WELL,
      PLATFORM_RESUMEWELL_TPM_RST_GPIO,
      FALSE
      );
    MicroSecondDelay (80);

    PlatformLegacyGpioSetLevel (
      R_QNC_GPIO_RGLVL_RESUME_WELL,
      PLATFORM_RESUMEWELL_TPM_RST_GPIO,
      TRUE
      );
    MicroSecondDelay (150000);
  }

}

/**
  Performs any early platform specific GPIO Controller init & manipulation.

  @param  PlatformType  Platform type for GPIO init & manipulation.

**/
VOID
EFIAPI
EarlyPlatformGpioCtrlerInitAndManipulation (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  )
{
  UINT32                            IohGpioBase;
  UINT32                            Data32;
  UINT32                            Addr;
  BOARD_GPIO_CONTROLLER_CONFIG      *GpioConfig;
  UINT32                            DevPcieAddr;
  UINT16                            SaveCmdReg;
  UINT32                            SaveBarReg;
  UINT16                            PciVid;
  UINT16                            PciDid;

  ASSERT ((UINTN) PlatformType < mBoardGpioControllerConfigTableLen);
  GpioConfig = &mBoardGpioControllerConfigTable[(UINTN) PlatformType];

  IohGpioBase = (UINT32) PcdGet64 (PcdIohGpioMmioBase);

  DevPcieAddr = PCI_LIB_ADDRESS (
                  PcdGet8 (PcdIohGpioBusNumber),
                  PcdGet8 (PcdIohGpioDevNumber),
                  PcdGet8 (PcdIohGpioFunctionNumber),
                  0
                  );

  //
  // Do nothing if not a supported device.
  //
  PciVid = PciRead16 (DevPcieAddr + PCI_VENDOR_ID_OFFSET);
  PciDid = PciRead16 (DevPcieAddr + PCI_DEVICE_ID_OFFSET);
  if((PciVid != V_IOH_I2C_GPIO_VENDOR_ID) || (PciDid != V_IOH_I2C_GPIO_DEVICE_ID)) {
    return;
  }

  //
  // Save current settings for PCI CMD/BAR registers.
  //
  SaveCmdReg = PciRead16 (DevPcieAddr + PCI_COMMAND_OFFSET);
  SaveBarReg = PciRead32 (DevPcieAddr + PcdGet8 (PcdIohGpioBarRegister));

  //
  // Use predefined tempory memory resource.
  //
  PciWrite32 ( DevPcieAddr + PcdGet8 (PcdIohGpioBarRegister), IohGpioBase);
  PciWrite8 ( DevPcieAddr + PCI_COMMAND_OFFSET, EFI_PCI_COMMAND_MEMORY_SPACE);

  //
  // Gpio Controller Init Tasks.
  //

  //
  // IEN- Interrupt Enable Register
  //
  Addr =  IohGpioBase + GPIO_INTEN;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->IntEn & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // ISTATUS- Interrupt Status Register
  //
  Addr =  IohGpioBase + GPIO_INTSTATUS;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // GPIO SWPORTA Direction Register - GPIO_SWPORTA_DR
  //
  Addr =  IohGpioBase + GPIO_SWPORTA_DR;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->PortADR & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // GPIO SWPORTA Data Direction Register - GPIO_SWPORTA_DDR - default input
  //
  Addr =  IohGpioBase + GPIO_SWPORTA_DDR;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->PortADir & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // Interrupt Mask Register - GPIO_INTMASK - default interrupts unmasked
  //
  Addr =  IohGpioBase + GPIO_INTMASK;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->IntMask & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // Interrupt Level Type Register - GPIO_INTTYPE_LEVEL - default is level sensitive
  //
  Addr =  IohGpioBase + GPIO_INTTYPE_LEVEL;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->IntType & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // Interrupt Polarity Type Register - GPIO_INT_POLARITY - default is active low
  //
  Addr =  IohGpioBase + GPIO_INT_POLARITY;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->IntPolarity & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // Interrupt Debounce Type Register - GPIO_DEBOUNCE - default no debounce
  //
  Addr =  IohGpioBase + GPIO_DEBOUNCE;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->Debounce & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // Interrupt Clock Synchronisation Register - GPIO_LS_SYNC - default no sync with pclk_intr(APB bus clk)
  //
  Addr =  IohGpioBase + GPIO_LS_SYNC;
  Data32 = *((volatile UINT32 *) (UINTN)(Addr)) & 0xFFFFFF00; // Keep reserved bits [31:8]
  Data32 |= (GpioConfig->LsSync & 0x000FFFFF);
  *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  //
  // Gpio Controller Manipulation Tasks.
  //

  if (PlatformType == (EFI_PLATFORM_TYPE) Galileo) {
    //
    // Reset Cypress Expander on Galileo Platform
    //
    Addr = IohGpioBase + GPIO_SWPORTA_DR;
    Data32 = *((volatile UINT32 *) (UINTN)(Addr));
    Data32 |= BIT4;                                 // Cypress Reset line controlled by GPIO<4>
    *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

    Data32 = *((volatile UINT32 *) (UINTN)(Addr));
    Data32 &= ~BIT4;                                // Cypress Reset line controlled by GPIO<4>
    *((volatile UINT32 *) (UINTN)(Addr)) = Data32;

  }

  //
  // Restore settings for PCI CMD/BAR registers
  //
  PciWrite32 ((DevPcieAddr + PcdGet8 (PcdIohGpioBarRegister)), SaveBarReg);
  PciWrite16 (DevPcieAddr + PCI_COMMAND_OFFSET, SaveCmdReg);
}

/**
  Performs any early platform init of SoC Ethernet Mac devices.

  @param  IohMac0Address  Mac address to program into Mac0 device.
  @param  IohMac1Address  Mac address to program into Mac1 device.

**/
VOID
EFIAPI
EarlyPlatformMacInit (
  IN CONST UINT8                          *IohMac0Address,
  IN CONST UINT8                          *IohMac1Address
  )
{
  BOOLEAN                           SetMacAddr;

  //
  // Set chipset MAC0 address if configured.
  //
  SetMacAddr =
    (CompareMem (ChipsetDefaultMac, IohMac0Address, sizeof (ChipsetDefaultMac))) != 0;
  if (SetMacAddr) {
    if ((*(IohMac0Address) & BIT0) != 0) {
      DEBUG ((EFI_D_ERROR, "HALT: Multicast Mac Address configured for Ioh MAC [B:%d, D:%d, F:%d]\n",
        (UINTN) IOH_MAC0_BUS_NUMBER,
        (UINTN) IOH_MAC0_DEVICE_NUMBER,
        (UINTN) IOH_MAC0_FUNCTION_NUMBER
        ));
      ASSERT (FALSE);
    } else {
      SetLanControllerMacAddr (
        IOH_MAC0_BUS_NUMBER,
        IOH_MAC0_DEVICE_NUMBER,
        IOH_MAC0_FUNCTION_NUMBER,
        IohMac0Address,
        (UINT32) PcdGet64(PcdIohMac0MmioBase)
        );
    }
  } else {
    DEBUG ((EFI_D_WARN, "WARNING: Ioh MAC [B:%d, D:%d, F:%d] NO HW ADDR CONFIGURED!!!\n",
      (UINTN) IOH_MAC0_BUS_NUMBER,
      (UINTN) IOH_MAC0_DEVICE_NUMBER,
      (UINTN) IOH_MAC0_FUNCTION_NUMBER
      ));
  }

  //
  // Set chipset MAC1 address if configured.
  //
  SetMacAddr =
    (CompareMem (ChipsetDefaultMac, IohMac1Address, sizeof (ChipsetDefaultMac))) != 0;
  if (SetMacAddr) {
    if ((*(IohMac1Address) & BIT0) != 0) {
      DEBUG ((EFI_D_ERROR, "HALT: Multicast Mac Address configured for Ioh MAC [B:%d, D:%d, F:%d]\n",
        (UINTN) IOH_MAC1_BUS_NUMBER,
        (UINTN) IOH_MAC1_DEVICE_NUMBER,
        (UINTN) IOH_MAC1_FUNCTION_NUMBER
        ));
      ASSERT (FALSE);
    } else {
        SetLanControllerMacAddr (
          IOH_MAC1_BUS_NUMBER,
          IOH_MAC1_DEVICE_NUMBER,
          IOH_MAC1_FUNCTION_NUMBER,
          IohMac1Address,
          (UINT32) PcdGet64(PcdIohMac1MmioBase)
          );
    }
  } else {
    DEBUG ((EFI_D_WARN, "WARNING: Ioh MAC [B:%d, D:%d, F:%d] NO HW ADDR CONFIGURED!!!\n",
      (UINTN) IOH_MAC1_BUS_NUMBER,
      (UINTN) IOH_MAC1_DEVICE_NUMBER,
      (UINTN) IOH_MAC1_FUNCTION_NUMBER
      ));
  }
}

/**
  This function provides early platform initialization.

  @param  PlatformInfo  Pointer to platform Info structure.

**/
VOID
EFIAPI
EarlyPlatformInit (
  VOID
  )
{
  EFI_PLATFORM_TYPE                 PlatformType;

  //PlatformType = (EFI_PLATFORM_TYPE) PcdGet16 (PcdPlatformType);
  MEMORY_INIT_UPD  *MemoryInitUpdPtr = (MEMORY_INIT_UPD *)GetFspMemoryInitUpdDataPointer();
  PlatformType = MemoryInitUpdPtr->PcdPlatformType;

  DEBUG ((EFI_D_INFO, "EarlyPlatformInit for PlatType=0x%02x\n", (UINTN) PlatformType));

  //
  // Early Legacy Gpio Init.
  //
  EarlyPlatformLegacyGpioInit (PlatformType);

  //
  // Early platform Legacy GPIO manipulation depending on GPIOs
  // setup by EarlyPlatformLegacyGpioInit.
  //
  EarlyPlatformLegacyGpioManipulation (PlatformType);

  //
  // Early platform specific GPIO Controller init & manipulation.
  // Combined for sharing of temp. memory bar.
  //
  EarlyPlatformGpioCtrlerInitAndManipulation (PlatformType);

  //
  // Early Thermal Sensor Init.
  //
  EarlyPlatformThermalSensorInit ();

  //
  // Early Lan Ethernet Mac Init.
  //
  EarlyPlatformMacInit (
    PcdGetPtr (PcdIohEthernetMac0),
    PcdGetPtr (PcdIohEthernetMac1)
    );
}

/**
  This is the entrypoint of PEIM

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
BiosBeforeMemoryInit(
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                              Status = EFI_SUCCESS;
  EFI_BOOT_MODE                           BootMode;

  //
  // Workaround:
  //   This driver will be invoked before MemoryInitUpd pointer initailized.
  //   To avoid the system halt, just initialized it before MemoryInitUpd used.
  //
  // Background:
  //   According to current architect, pointer of UPD data region is equal to
  //   pointer of Memory Init UPD data region.
  //
  // Note:
  //   Since the whole BiosCodePreMrc and BiosCodePostMrc PEIM will be
  //   removed in the future, so it's not a problem.
  //
  UPD_DATA_REGION *UpdDataRgnPtr = (UPD_DATA_REGION *)GetFspUpdDataPointer();
  MEMORY_INIT_UPD *MemoryInitUpdPtr = (MEMORY_INIT_UPD *)UpdDataRgnPtr;
  SetFspMemoryInitUpdDataPointer(MemoryInitUpdPtr);
  SetFspSiliconInitUpdDataPointer((void *)NULL);

  //PlatformType = (EFI_PLATFORM_TYPE)PcdGet16 (PcdPlatformType);
  MemoryInitUpdPtr = (MEMORY_INIT_UPD *)GetFspMemoryInitUpdDataPointer();

  //
  // Do any early platform specific initialization.
  //
  EarlyPlatformInit ();

  // Program USB Phy
  InitializeUSBPhy();

  //
  // Do platform specific logic to create a boot mode
  //
  Status = PeiServicesGetBootMode(&BootMode);
  ASSERT_EFI_ERROR(Status);

  //
  // Signal possible dependent modules that there has been a
  // final boot mode determination
  //

  if (BootMode != BOOT_ON_S3_RESUME) {
    QNCClearSmiAndWake ();
  }

  return Status;
}
