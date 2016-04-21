;------------------------------------------------------------------------------
;
; Copyright (c) 2013-2016 Intel Corporation.
;
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
; Module Name:
;
;  Flat32.asm
;
; Abstract:
;
;  This is the code that goes from real-mode to protected mode.
;  It consumes the reset vector, configures the stack.
;
;
;------------------------------------------------------------------------------


;
; Define assembler characteristics
;
.586p
.model flat, c

;
; Include processor definitions
;

INCLUDE Platform.inc

;
;Pull FSP related routines into build.
;
EXTRN   TempRamInitApi:NEAR
EXTRN   FspInitApi:NEAR

;
; Contrary to the name, this file contains 16 bit code as well.
;
_TEXT_REALMODE      SEGMENT PARA PUBLIC USE16 'CODE'
                    ASSUME  CS:_TEXT_REALMODE, DS:_TEXT_REALMODE

;----------------------------------------------------------------------------
;
; Procedure:    _ModuleEntryPoint
;
; Input:        None
;
; Output:       None
;
; Destroys:     Assume all registers
;
; Description:
;
;   Transition to non-paged flat-model protected mode from a
;   hard-coded GDT that provides exactly two descriptors.
;   This is a bare bones transition to protected mode only
;   used for a while in PEI and possibly DXE.
;
;   After enabling protected mode, a far jump is executed to
;   transfer to PEI using the newly loaded GDT.
;
; Return:       None
;
;----------------------------------------------------------------------------
align 16
_ModuleEntryPoint      PROC C PUBLIC

  jmp $

_ModuleEntryPoint   ENDP

_TEXT_REALMODE      ENDS

.code

;-----------------------------------------------------------------------
; FUNCTION:
;    This fucntion will be called by TempRamInit FSP API to do platfrom
;    specific NEM initialization.
;
; IN:
;    EAX:  Return address
;    ESP:  TempRamInit fake stack pointer
;      [ESP+00h]:  TempRamInit API return address
;      [ESP+04h]:  TempRamInit parameter block pointer
;
; OUT:
;    EAX:  Error code
;               0:  Success
;          Others:  Failure
;
;-----------------------------------------------------------------------
SecCarInit PROC NEAR PUBLIC
  ;
  ; coreboot should initialize ESRAM before TempRamInit API,
  ; do nothing here.
  ;
  mov	edx, eax
  xor   eax, eax	
  jmp  edx
SecCarInit ENDP

END
