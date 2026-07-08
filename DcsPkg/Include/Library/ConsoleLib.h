/** @file
Console abstraction library

Copyright (c) 2019-2026. DiskCryptor, David Xanatos

This program and the accompanying materials are licensed and made available
under the terms and conditions of the GNU Lesser General Public License, version 3.0 (LGPL-3.0).

The full text of the license may be found at
https://opensource.org/licenses/LGPL-3.0
**/

#ifndef __CONSOLELIB_H__
#define __CONSOLELIB_H__

#include <Uefi.h>
#include <Library/GraphLib.h>

//////////////////////////////////////////////////////////////////////////
// Console Interface
//////////////////////////////////////////////////////////////////////////

// Forward declaration
typedef struct _CONSOLE_INTERFACE CONSOLE_INTERFACE;

// Console interface structure with function pointers
typedef struct _CONSOLE_INTERFACE {
    //------------------------------------------------------------------------
    // Output Functions
    //------------------------------------------------------------------------

    // Standard print (format string, variadic)
    VOID (*Print)(IN CONST CHAR16 *Format, ...);

    // Error print (red text)
    VOID (*PrintError)(IN CONST CHAR16 *Format, ...);

    // Warning print (orange/yellow text)
    VOID (*PrintWarning)(IN CONST CHAR16 *Format, ...);

    // Print at specific position (Col=-1, Row=-1 for current position)
    VOID (*PrintAt)(IN INT32 Col, IN INT32 Row, IN CONST CHAR16 *Format, ...);

    //------------------------------------------------------------------------
    // Screen Control Functions
    //------------------------------------------------------------------------

    // Clear screen/console area
    VOID (*Clear)(VOID);

    // Set cursor position
    VOID (*SetCursor)(IN INT32 Col, IN INT32 Row);

    // Enable/disable cursor
    VOID (*EnableCursor)(IN BOOLEAN Enable);

    // Get current cursor position
    VOID (*GetCursor)(OUT INT32 *Col, OUT INT32 *Row);

    // Get console dimensions (columns and rows)
    VOID (*GetSize)(OUT INT32 *Cols, OUT INT32 *Rows);

    //------------------------------------------------------------------------
    // Input Functions
    //------------------------------------------------------------------------

    // Wait for key and return it
    EFI_INPUT_KEY (*GetKey)(VOID);

    // Wait for key with timeout (returns key or timeout indication)
    EFI_INPUT_KEY (*KeyWait)(IN CHAR16 *Prompt, IN UINTN MilliDelay,
                              IN UINT16 DefaultScanCode, IN UINT16 DefaultChar);

    // Flush input buffer with delay
    VOID (*FlushInput)(IN UINTN DelayMicroseconds);

    // Read a line of input
    BOOLEAN (*ReadLine)(OUT UINTN *Length, OUT CHAR16 *Line,
                        OUT CHAR8 *AsciiLine, IN UINTN MaxLen, IN UINT8 ShowChars);

    // Ask yes/no question
    UINT8 (*AskYesNo)(IN CHAR8 *Prompt, IN UINT8 Visible);

    //------------------------------------------------------------------------
    // Context/State
    //------------------------------------------------------------------------

    VOID *PrivateData;  // Implementation-specific context

} CONSOLE_INTERFACE;

//////////////////////////////////////////////////////////////////////////
// Global Console Interface
//////////////////////////////////////////////////////////////////////////

// Global console interface pointer
extern CONSOLE_INTERFACE *g_Con;

//////////////////////////////////////////////////////////////////////////
// Initialization Functions
//////////////////////////////////////////////////////////////////////////

// Auto-detect and initialize appropriate console based on gDCryptTouchInput
EFI_STATUS
ConsoleInit(VOID);

// Force text console initialization
EFI_STATUS
ConsoleInitText(VOID);

// Force touch console initialization (returns EFI_UNSUPPORTED if not available)
EFI_STATUS
ConsoleInitTouch(VOID);

// Cleanup and shutdown console
VOID
ConsoleShutdown(VOID);

// Check if touch console is active
BOOLEAN
ConsoleIsTouchMode(VOID);

//////////////////////////////////////////////////////////////////////////
// Convenience Macros
//////////////////////////////////////////////////////////////////////////

#define CON_PRINT(fmt, ...)       g_Con->Print(fmt, ##__VA_ARGS__)
#define CON_ERR(fmt, ...)         g_Con->PrintError(fmt, ##__VA_ARGS__)
#define CON_WARN(fmt, ...)        g_Con->PrintWarning(fmt, ##__VA_ARGS__)
#define CON_PRINT_AT(c,r,fmt,...) g_Con->PrintAt(c, r, fmt, ##__VA_ARGS__)
#define CON_CLEAR()               g_Con->Clear()
#define CON_GETKEY()              g_Con->GetKey()

#endif // __CONSOLELIB_H__
