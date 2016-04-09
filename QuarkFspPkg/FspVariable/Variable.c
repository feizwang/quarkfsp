/** @file
  Implement ReadOnly Variable Services required by PEIM and install
  PEI ReadOnly Varaiable2 PPI. These services operates the non volatile storage space.

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Variable.h"

#define HOB_DATA_SIZE_ALIGNMENT 8
#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
	((ActualSize)+(((Alignment)-((ActualSize)& ((Alignment)-1))) & ((Alignment)-1)))

//
// Module globals
//
EFI_PEI_READ_ONLY_VARIABLE2_PPI mVariablePpi = {
  FspPeiGetVariable,
  FspPeiGetNextVariableName
};

EFI_PEI_PPI_DESCRIPTOR     mPpiListVariable = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariable2PpiGuid,
  &mVariablePpi
};


/**
  Provide the functionality of the variable services.

  @param  FileHandle   Handle of the file being invoked.
                       Type EFI_PEI_FILE_HANDLE is defined in FfsFindNextFile().
  @param  PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS  If the interface could be successfully installed
  @retval Others       Returned from PeiServicesInstallPpi()
**/
EFI_STATUS
EFIAPI
PeimInitializeVariableServices (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  return PeiServicesInstallPpi (&mPpiListVariable);
}

/**
  This service retrieves a variable's value using its name and GUID.

  Read the specified variable from the UEFI variable store. If the Data
  buffer is too small to hold the contents of the variable, the error
  EFI_BUFFER_TOO_SMALL is returned and DataSize is set to the required buffer
  size to obtain the data.

  @param  This                  A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.
  @param  VariableName          A pointer to a null-terminated string that is the variable's name.
  @param  VariableGuid          A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                VariableGuid and VariableName must be unique.
  @param  Attributes            If non-NULL, on return, points to the variable's attributes.
  @param  DataSize              On entry, points to the size in bytes of the Data buffer.
                                On return, points to the size of the data returned in Data.
  @param  Data                  Points to the buffer which will hold the returned variable value.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the resulting data.
                                DataSize is updated with the size required for
                                the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid, DataSize or Data is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
FspPeiGetVariable (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
  IN CONST  CHAR16                          *VariableName,
  IN CONST  EFI_GUID                        *VariableGuid,
  OUT       UINT32                          *Attributes,
  IN OUT    UINTN                           *DataSize,
  OUT       VOID                            *Data
  )
{
  UINTN                   VarDataSize;
  VOID                    *VarDataPtr;
  EFI_HOB_GUID_TYPE       *FspInitNvBufferPtr;

  if (VariableName == NULL || VariableGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (CompareGuid (VariableGuid, &gEfiMemoryConfigDataGuid)) {

    //
    // A typical MrcTimings_t structure caculated by MRC.
    //
    //MrcTimings->rcvn[0][0][0] = 0x1D2;
    //MrcTimings->rdqs[0][0][0] = 0x1D;
    //MrcTimings->wdqs[0][0][0] = 0x128;
    //MrcTimings->wdq[0][0][0] = 0x108;
    //MrcTimings->rcvn[0][0][1] = 0x1D0;
    //MrcTimings->rdqs[0][0][1] = 0x1D;
    //MrcTimings->wdqs[0][0][1] = 0x130;
    //MrcTimings->wdq[0][0][1] = 0x111;
    //MrcTimings->rcvn[0][0][2] = 0x0;
    //MrcTimings->rdqs[0][0][2] = 0x18;
    //MrcTimings->wdqs[0][0][2] = 0x0;
    //MrcTimings->wdq[0][0][2] = 0x0;
    //MrcTimings->rcvn[0][0][3] = 0x0;
    //MrcTimings->rdqs[0][0][3] = 0x18;
    //MrcTimings->wdqs[0][0][3] = 0x0;
    //MrcTimings->wdq[0][0][3] = 0x0;
    //MrcTimings->rcvn[0][1][0] = 0x1D2;
    //MrcTimings->rdqs[0][1][0] = 0x1D;
    //MrcTimings->wdqs[0][1][0] = 0x128;
    //MrcTimings->wdq[0][1][0] = 0x108;
    //MrcTimings->rcvn[0][1][1] = 0x1D0;
    //MrcTimings->rdqs[0][1][1] = 0x1D;
    //MrcTimings->wdqs[0][1][1] = 0x130;
    //MrcTimings->wdq[0][1][1] = 0x111;
    //MrcTimings->rcvn[0][1][2] = 0x0;
    //MrcTimings->rdqs[0][1][2] = 0x18;
    //MrcTimings->wdqs[0][1][2] = 0x0;
    //MrcTimings->wdq[0][1][2] = 0x0;
    //MrcTimings->rcvn[0][1][3] = 0x0;
    //MrcTimings->rdqs[0][1][3] = 0x18;
    //MrcTimings->wdqs[0][1][3] = 0x0;
    //MrcTimings->wdq[0][1][3] = 0x0;
    //MrcTimings->vref[0][0] = 0x1F;
    //MrcTimings->vref[0][1] = 0x1F;
    //MrcTimings->vref[0][2] = 0x20;
    //MrcTimings->vref[0][3] = 0x20;
    //MrcTimings->wctl[0][0] = 0xDE;
    //MrcTimings->wctl[0][1] = 0xDE;
    //MrcTimings->wcmd[0] = 0xDE;
    //MrcTimings->scrambler_seed = 0x75EC60;
    //MrcTimings->ddr_speed = 0x0;

    //
    // Memory Config Data passed from boot loader
    //
    FspInitNvBufferPtr = (EFI_HOB_GUID_TYPE *)((FSP_INIT_PARAMS *)GetFspApiParameter ())->NvsBufferPtr;
    if (FspInitNvBufferPtr == NULL)
      return EFI_INVALID_PARAMETER;

    //
    // Verify Memory Config Data - Boot loader passes Memory Config Data HOB.
    //
    if (GET_HOB_TYPE(FspInitNvBufferPtr) != EFI_HOB_TYPE_GUID_EXTENSION)
      return EFI_INVALID_PARAMETER;

    if (!CompareGuid (VariableGuid, &FspInitNvBufferPtr->Name))
      return EFI_INVALID_PARAMETER;

    //
    // Get data size and pointer
    //
    VarDataPtr = GET_GUID_HOB_DATA(FspInitNvBufferPtr);
    VarDataSize = GET_GUID_HOB_DATA_SIZE(FspInitNvBufferPtr);

  } else {
    //
    // ONLY support gEfiMemoryConfigDataGuid
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Since HOB data size (VarDataSize) always be 8 byte alignment, it would cause an assert 
  // if *DataSize is not 8 aligment structure. The solution is to force the HOB data size (VarDataSize) 
  // to *DataSize.
  //
  if ((VarDataSize > *DataSize) && (VarDataSize == GET_OCCUPIED_SIZE(*DataSize, HOB_DATA_SIZE_ALIGNMENT))) {
    VarDataSize = *DataSize;
  }

  //
  // Get data size
  //
  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    CopyMem (Data, VarDataPtr, VarDataSize);

    if (Attributes != NULL) {
      *Attributes = 0;  // Force to zero
    }

    *DataSize = VarDataSize;
    return EFI_SUCCESS;
  } else {
    *DataSize = VarDataSize;
    return EFI_BUFFER_TOO_SMALL;
  }
}

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName
  and VariableGuid of all variables currently available in the system.
  On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next
  interface. When the entire variable list has been returned,
  EFI_NOT_FOUND is returned.

  @param  This              A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.

  @param  VariableNameSize  On entry, points to the size of the buffer pointed to by VariableName.
                            On return, the size of the variable name buffer.
  @param  VariableName      On entry, a pointer to a null-terminated string that is the variable's name.
                            On return, points to the next variable's null-terminated name string.
  @param  VariableGuid      On entry, a pointer to an EFI_GUID that is the variable's GUID.
                            On return, a pointer to the next variable's GUID.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the resulting
                                data. VariableNameSize is updated with the size
                                required for the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid or
                                VariableNameSize is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
FspPeiGetNextVariableName (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
  IN OUT UINTN                              *VariableNameSize,
  IN OUT CHAR16                             *VariableName,
  IN OUT EFI_GUID                           *VariableGuid
  )
{
  return EFI_UNSUPPORTED;
}
