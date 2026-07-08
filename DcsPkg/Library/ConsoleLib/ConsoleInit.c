/** @file
Console initialization and mode selection

Copyright (c) 2026. DiskCryptor, David Xanatos

This program and the accompanying materials are licensed and made available
under the terms and conditions of the GNU Lesser General Public License, version 3.0 (LGPL-3.0).

The full text of the license may be found at
https://opensource.org/licenses/LGPL-3.0
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/CommonLib.h>
#include <Library/GraphLib.h>
#include <Library/ConsoleLib.h>

//////////////////////////////////////////////////////////////////////////
// External declarations
//////////////////////////////////////////////////////////////////////////

// From DiskCryptorLib - touch input configuration
extern int gDCryptTouchInput;

//////////////////////////////////////////////////////////////////////////
// Text Console Implementation (defined in TextConsole.c)
//////////////////////////////////////////////////////////////////////////

extern VOID TextConsolePrint(IN CONST CHAR16 *Format, ...);
extern VOID TextConsolePrintError(IN CONST CHAR16 *Format, ...);
extern VOID TextConsolePrintWarning(IN CONST CHAR16 *Format, ...);
extern VOID TextConsolePrintAt(IN INT32 Col, IN INT32 Row, IN CONST CHAR16 *Format, ...);
extern VOID TextConsoleClear(VOID);
extern VOID TextConsoleSetCursor(IN INT32 Col, IN INT32 Row);
extern VOID TextConsoleEnableCursor(IN BOOLEAN Enable);
extern VOID TextConsoleGetCursor(OUT INT32 *Col, OUT INT32 *Row);
extern VOID TextConsoleGetSize(OUT INT32 *Cols, OUT INT32 *Rows);
extern EFI_INPUT_KEY TextConsoleGetKey(VOID);
extern EFI_INPUT_KEY TextConsoleKeyWait(IN CHAR16 *Prompt, IN UINTN MilliDelay, IN UINT16 ScanCode, IN UINT16 Char);
extern VOID TextConsoleFlushInput(IN UINTN DelayUs);
extern BOOLEAN TextConsoleReadLine(OUT UINTN *Length, OUT CHAR16 *Line, OUT CHAR8 *AsciiLine, IN UINTN MaxLen, IN UINT8 ShowChars);
extern UINT8 TextConsoleAskYesNo(IN CHAR8 *Prompt, IN UINT8 Visible);

//////////////////////////////////////////////////////////////////////////
// Touch Console Implementation (defined in TouchConsole.c)
//////////////////////////////////////////////////////////////////////////

extern EFI_STATUS TouchConsoleInitialize(VOID);
extern VOID TouchConsoleShutdown(VOID);
extern VOID TouchConsolePrint(IN CONST CHAR16 *Format, ...);
extern VOID TouchConsolePrintError(IN CONST CHAR16 *Format, ...);
extern VOID TouchConsolePrintWarning(IN CONST CHAR16 *Format, ...);
extern VOID TouchConsolePrintAt(IN INT32 Col, IN INT32 Row, IN CONST CHAR16 *Format, ...);
extern VOID TouchConsoleClear(VOID);
extern VOID TouchConsoleSetCursor(IN INT32 Col, IN INT32 Row);
extern VOID TouchConsoleEnableCursor(IN BOOLEAN Enable);
extern VOID TouchConsoleGetCursor(OUT INT32 *Col, OUT INT32 *Row);
extern VOID TouchConsoleGetSize(OUT INT32 *Cols, OUT INT32 *Rows);
extern EFI_INPUT_KEY TouchConsoleGetKey(VOID);
extern EFI_INPUT_KEY TouchConsoleKeyWait(IN CHAR16 *Prompt, IN UINTN MilliDelay, IN UINT16 ScanCode, IN UINT16 Char);
extern VOID TouchConsoleFlushInput(IN UINTN DelayUs);
extern BOOLEAN TouchConsoleReadLine(OUT UINTN *Length, OUT CHAR16 *Line, OUT CHAR8 *AsciiLine, IN UINTN MaxLen, IN UINT8 ShowChars);
extern UINT8 TouchConsoleAskYesNo(IN CHAR8 *Prompt, IN UINT8 Visible);
extern VOID* TouchConsoleGetPrivateData(VOID);

//////////////////////////////////////////////////////////////////////////
// Global Console Interface
//////////////////////////////////////////////////////////////////////////

// Global console interface pointer
CONSOLE_INTERFACE *g_Con = NULL;

// Static console interface instances
static CONSOLE_INTERFACE gTextConsole;
static CONSOLE_INTERFACE gTouchConsole;

// Track which mode is active
static BOOLEAN gTouchModeActive = FALSE;

//////////////////////////////////////////////////////////////////////////
// Initialization Functions
//////////////////////////////////////////////////////////////////////////

/**
  Initialize text console interface.

  @retval EFI_SUCCESS    Text console initialized successfully.
**/
EFI_STATUS
ConsoleInitText(VOID)
{
    // Set up text console function pointers
    gTextConsole.Print        = TextConsolePrint;
    gTextConsole.PrintError   = TextConsolePrintError;
    gTextConsole.PrintWarning = TextConsolePrintWarning;
    gTextConsole.PrintAt      = TextConsolePrintAt;
    gTextConsole.Clear        = TextConsoleClear;
    gTextConsole.SetCursor    = TextConsoleSetCursor;
    gTextConsole.EnableCursor = TextConsoleEnableCursor;
    gTextConsole.GetCursor    = TextConsoleGetCursor;
    gTextConsole.GetSize      = TextConsoleGetSize;
    gTextConsole.GetKey       = TextConsoleGetKey;
    gTextConsole.KeyWait      = TextConsoleKeyWait;
    gTextConsole.FlushInput   = TextConsoleFlushInput;
    gTextConsole.ReadLine     = TextConsoleReadLine;
    gTextConsole.AskYesNo     = TextConsoleAskYesNo;
    gTextConsole.PrivateData  = NULL;

    g_Con = &gTextConsole;
    gTouchModeActive = FALSE;

    return EFI_SUCCESS;
}

/**
  Initialize touch console interface.

  @retval EFI_SUCCESS       Touch console initialized successfully.
  @retval EFI_UNSUPPORTED   Touch mode not available.
**/
EFI_STATUS
ConsoleInitTouch(VOID)
{
    EFI_STATUS Status;

    // Check if touch mode prerequisites are met
    if (gGraphOut == NULL) {
        return EFI_UNSUPPORTED;
    }

    if (gTouchPointer == NULL && gTouchSimulate == 0) {
        return EFI_UNSUPPORTED;
    }

    // Initialize touch console data structures
    Status = TouchConsoleInitialize();
    if (EFI_ERROR(Status)) {
        return Status;
    }

    // Set up touch console function pointers
    gTouchConsole.Print        = TouchConsolePrint;
    gTouchConsole.PrintError   = TouchConsolePrintError;
    gTouchConsole.PrintWarning = TouchConsolePrintWarning;
    gTouchConsole.PrintAt      = TouchConsolePrintAt;
    gTouchConsole.Clear        = TouchConsoleClear;
    gTouchConsole.SetCursor    = TouchConsoleSetCursor;
    gTouchConsole.EnableCursor = TouchConsoleEnableCursor;
    gTouchConsole.GetCursor    = TouchConsoleGetCursor;
    gTouchConsole.GetSize      = TouchConsoleGetSize;
    gTouchConsole.GetKey       = TouchConsoleGetKey;
    gTouchConsole.KeyWait      = TouchConsoleKeyWait;
    gTouchConsole.FlushInput   = TouchConsoleFlushInput;
    gTouchConsole.ReadLine     = TouchConsoleReadLine;
    gTouchConsole.AskYesNo     = TouchConsoleAskYesNo;
    gTouchConsole.PrivateData  = TouchConsoleGetPrivateData();

    g_Con = &gTouchConsole;
    gTouchModeActive = TRUE;

    return EFI_SUCCESS;
}

/**
  Auto-detect and initialize appropriate console.

  Checks gDCryptTouchInput configuration and available hardware
  to determine whether to use text or touch console.

  @retval EFI_SUCCESS    Console initialized successfully.
**/
EFI_STATUS
ConsoleInit(VOID)
{
    // Check if touch mode is enabled and prerequisites are met
    if (gDCryptTouchInput == 1 &&
        gGraphOut != NULL &&
        (gTouchPointer != NULL || gTouchSimulate != 0)) {

        // Try to initialize touch console
        EFI_STATUS Status = ConsoleInitTouch();
        if (!EFI_ERROR(Status)) {
            return EFI_SUCCESS;
        }
        // Fall through to text console on failure
    }

    // Default to text console
    return ConsoleInitText();
}

/**
  Shutdown and cleanup console resources.
**/
VOID
ConsoleShutdown(VOID)
{
    if (gTouchModeActive) {
        TouchConsoleShutdown();
        RestoreConsoleControl();
    }

    g_Con = NULL;
    gTouchModeActive = FALSE;
}

/**
  Check if touch console mode is active.

  @retval TRUE   Touch console is active.
  @retval FALSE  Text console is active.
**/
BOOLEAN
ConsoleIsTouchMode(VOID)
{
    return gTouchModeActive;
}

//////////////////////////////////////////////////////////////////////////
// Library Constructor
//////////////////////////////////////////////////////////////////////////

/**
  Library constructor - initializes g_Con to text console by default.

  This ensures g_Con is never NULL, even before ConsoleInit() is called.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
ConsoleLibConstructor(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    )
{
    // Initialize to text console by default
    // This ensures g_Con is never NULL
    ConsoleInitText();
    return EFI_SUCCESS;
}
