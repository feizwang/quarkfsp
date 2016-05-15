## @file
# Configuration file of Firmware Service Package (FSP) for Quark X1000 SoC.
#
# This package provides specific modules of FSP for Quark X1000 SoC.
# Copyright (c) 2013 - 2016 Intel Corporation.
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

[Defines]
  #
  # UPD/VPD tool definition
  #
  VPD_TOOL_GUID           = 8C3D856A-9BE6-468E-850A-24F7A8D38E08
  DEFINE UPD_TOOL_GUID           = 3E18A0B3-C3B5-492b-86B4-53E3D401C249

[PcdsDynamicVpd.Upd]
  #
  # This section is not used by the normal build process
  # Howerver, FSP will use dedicated tool to handle it and generate a
  # VPD simliar binary block (User Platform Data). This block will
  # be accessed through a generated data structure directly rather than
  # PCD services. This is for size and performance consideration.
  # Format:
  #   gQuarkFspTokenSpaceGuid.Updxxxxxxxxxxxxn        | OFFSET | LENGTH | VALUE
  # Only simple data type is supported
  #

  #
  # Comments with !BSF will be used to generate BSF file
  # Comments with !HDR will be used to generate H header file
  #

  # Global definitions in BSF
  # !BSF PAGES:{MRC:"Memory Settings", SCT:"Chipset Settings", PLT:"Platform Settings"}
  # !BSF BLOCK:{NAME:"Quark FSP", VER:"0.1"}

  # !BSF FIND:{$QRKUPD$}
  gQuarkFspTokenSpaceGuid.Signature                   | 0x0000 | 8    | 0x244450554B525124
  gQuarkFspTokenSpaceGuid.Revision                    | 0x0008 | 0x8  | 0

  #
  # MemoryInitUpdOffset must match the first UPD's offset in the structure of MemoryInitUpd
  #
  gQuarkFspTokenSpaceGuid.MemoryInitUpdOffset         | 0x0010 | 0x04 | 0x00000020
  #
  # SiliconInitUpdOffset must match the first UPD's offset in the structure of SiliconInitUpd
  #
  gQuarkFspTokenSpaceGuid.SiliconInitUpdOffset        | 0x0014 | 0x04 | 0x00000050
  gQuarkFspTokenSpaceGuid.ReservedUpd1                | 0x0018 | 0x08 | 0

  ################################################################################
  #
  # UPDs consumed in FspMemoryInit API
  #
  ################################################################################
  # !HDR EMBED:{MEMORY_INIT_UPD:MemoryInitUpd:START}
  # MemoryInitUpdSignature: {$MEMUPD$}
  gQuarkFspTokenSpaceGuid.Signature                  | 0x0020 | 0x08 | 0x244450554D454D24
  gQuarkFspTokenSpaceGuid.Revision                   | 0x0028 | 0x08 | 0x00

  gQuarkFspTokenSpaceGuid.PcdRmuBinaryBaseAddress    | 0x0030 | 0x04 | 0xFFF00000
  gQuarkFspTokenSpaceGuid.PcdRmuBinaryLen            |0x0034| 0x04 | 0x02000
  gQuarkFspTokenSpaceGuid.PcdSmmTsegSize            |0x0038| 0x01 | 0x02
  
  ######
  # Make the following PCDs go away
  ######
  gQuarkFspTokenSpaceGuid.PcdPlatformType            | 0x0039 | 0x01 | 0x8

  # !HDR EMBED:{MEMORY_INIT_UPD:MemoryInitUpd:END}
  gQuarkFspTokenSpaceGuid.ReservedMemoryInitUpd      | 0x003A | 0x16 | {0x00}

  ################################################################################
  #
  # UPDs consumed in FspSiliconInit API
  #
  ################################################################################
  # !HDR EMBED:{SILICON_INIT_UPD:SiliconInitUpd:START}
  # SiliconInitUpdSignature: {$SI_UPD$}
  gQuarkFspTokenSpaceGuid.Signature                  | 0x0050 | 0x08 | 0x244450555F495324
  gQuarkFspTokenSpaceGuid.Revision                   | 0x0058 | 0x08 | 0x00

  # !HDR EMBED:{SILICON_INIT_UPD:SiliconInitUpd:END}
  gQuarkFspTokenSpaceGuid.ReservedSiliconInitUpd     | 0x0060 | 0x20 | {0x00}

  # Note please keep "PcdRegionTerminator" at the end of the UPD region.
  # The tool will use this field to determine the actual end of the UPD data
  # structure. Anything after this field will still be in the UPD binary block,
  # but will be excluded in the final UPD_DATA_REGION data structure.
  gQuarkFspTokenSpaceGuid.PcdRegionTerminator        | 0x0080 | 2 | 0x55AA

[PcdsDynamicVpd]
  #
  # VPD Region Signature "QUK-FSP0"
  #

  # !BSF FIND:{QUK-FSP0}
  gQuarkFspTokenSpaceGuid.PcdVpdRegionSign           | 0x0000 | 0x305053462D4B5551

  #
  # VPD Region Revision
  #
  # !BSF NAME:{PcdImageRevision}  TYPE:{None}
  gQuarkFspTokenSpaceGuid.PcdImageRevision           | 0x0008 | 0x00000000

  # This is a offset pointer to the UCD regions used by FSP
  # The offset will be patched to point to the actual region during the build process
  #
  gQuarkFspTokenSpaceGuid.PcdUpdRegionOffset         | 0x000C | 0x12345678
