/** @file
  DCS TPM Driver - TPM interface for DiskCryptor

  Copyright (c) 2026 David Xanatos. All rights reserved.

**/

#ifndef __EFI_DCS_TPM_H__
#define __EFI_DCS_TPM_H__

#include <Uefi.h>

//
// Libraries
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>

//
// UEFI Driver Model Protocols
//
#include <Protocol/ComponentName2.h>
#include <Protocol/ComponentName.h>

//
// Produced Protocols
//
#include "DcsTpmProto.h"

//
// Protocol instances
//
extern EFI_COMPONENT_NAME2_PROTOCOL  gDcsTpmComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL   gDcsTpmComponentName;
extern EFI_DCS_TPM_PROTOCOL          gEfiDcsTpmProtocol;

//
// Include files with function prototypes
//
#include "ComponentName.h"

//
// Driver function prototypes
//
EFI_STATUS
EFIAPI
DcsGetStatus (
  IN      EFI_DCS_TPM_PROTOCOL  *This,
  IN      UINT32                NvIndex,
  OUT     UINT32                *Status,
  OUT     UINT32                *PcrMask OPTIONAL,
  OUT     UINT32                *Flags OPTIONAL,
  OUT     VOID                  *Info OPTIONAL,
  IN OUT  UINT32                *InfoSize OPTIONAL
  );

EFI_STATUS
EFIAPI
DcsUnsealSecret (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  IN  UINT32                NvIndex,
  OUT VOID                  *Data,
  OUT UINT32                *DataSize,
  OUT UINT32                *DataType,
  IN  CHAR16                *TpmPin OPTIONAL
  );

EFI_STATUS
EFIAPI
DcsSealSecret (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  IN  UINT32                NvIndex,
  IN  VOID                  *Data,
  IN  UINT32                DataSize,
  IN  UINT32                DataType,
  IN  DCS_TPM_SEAL_OPTIONS  *Options,
  IN  VOID                  *Info OPTIONAL,
  IN  UINT32                InfoSize
  );

EFI_STATUS
EFIAPI
DcsClearSecret (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  IN  UINT32                NvIndex,
  IN  CHAR16                *OwnerPassword
  );

EFI_STATUS
EFIAPI
DcsLockSecret (
  IN  EFI_DCS_TPM_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
DcsShowMenu (
  IN  EFI_DCS_TPM_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
DcsGetSealOptions (
  IN  EFI_DCS_TPM_PROTOCOL   *This,
  OUT DCS_TPM_SEAL_OPTIONS   *Options,
  IN  BOOLEAN                SkipOwnerPassword
  );

EFI_STATUS
EFIAPI
DcsGetRandom (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  IN  UINTN                 RandomSize,
  OUT UINT8                 *RandomData
  );

EFI_STATUS
EFIAPI
DcsGetInfo (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  OUT DCS_TPM_INFO          *Info
  );

EFI_STATUS
EFIAPI
DcsReadPcr (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  IN  UINT32                PcrIndex,
  OUT UINT8                 *PcrValue,
  OUT UINT32                *PcrSize
  );

EFI_STATUS
EFIAPI
DcsEnumNvIndices (
  IN     EFI_DCS_TPM_PROTOCOL  *This,
  OUT    UINT32                *IndexList,
  IN OUT UINT32                *IndexCount
  );

EFI_STATUS
EFIAPI
DcsGetNvIndexInfo (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  IN  UINT32                NvIndex,
  OUT UINT32                *Attributes,
  OUT UINT32                *DataSize,
  OUT UINT32                *PcrRead OPTIONAL,
  OUT UINT32                *PcrWrite OPTIONAL
  );

EFI_STATUS
EFIAPI
DcsAskOwnerPassword (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  OUT CHAR16                *OwnerPassword
  );

//
// Buffer-based SRK API (caller handles file I/O)
//
EFI_STATUS
EFIAPI
DcsSrkGetStatus (
  IN     EFI_DCS_TPM_PROTOCOL  *This,
  IN     UINT8                 *Buffer,
  IN     UINT32                BufferSize,
  OUT    UINT32                *Status,
  OUT    UINT32                *PcrMask OPTIONAL,
  OUT    UINT32                *Flags OPTIONAL,
  OUT    VOID                  *Info OPTIONAL,
  IN OUT UINT32                *InfoSize OPTIONAL
  );

EFI_STATUS
EFIAPI
DcsSrkSealPassword (
  IN     EFI_DCS_TPM_PROTOCOL  *This,
  IN     VOID                  *Password,
  IN     UINT32                PasswordSize,
  IN     UINT32                PasswordType,
  IN     DCS_TPM_SEAL_OPTIONS  *Options,
  IN     VOID                  *Info OPTIONAL,
  IN     UINT32                InfoSize,
  OUT    UINT8                 *Buffer,
  IN OUT UINT32                *BufferSize
  );

EFI_STATUS
EFIAPI
DcsSrkUnsealPassword (
  IN  EFI_DCS_TPM_PROTOCOL  *This,
  IN  UINT8                 *Buffer,
  IN  UINT32                BufferSize,
  OUT VOID                  *Password,
  OUT UINT32                *PasswordSize,
  OUT UINT32                *PasswordType,
  IN  CHAR16                *TpmPin OPTIONAL
  );

#endif
