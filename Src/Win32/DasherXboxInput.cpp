
#include "Common\WinCommon.h"

#include ".\dasherxboxinput.h"

using namespace Dasher;

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG
#define DEBUG_NEW new( _NORMAL_BLOCK, THIS_FILE, __LINE__ )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif


// XInput support
// copied from https://github.com/walbourn/directx-sdk-samples/tree/master/XInput
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <commdlg.h>
#include <basetsd.h>
#include <objbase.h>

#ifdef USE_DIRECTX_SDK
#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\include\xinput.h>
#pragma comment(lib,"xinput.lib")
#elif (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#include <XInput.h>
#pragma comment(lib,"xinput.lib")
#else
#include <XInput.h>
#pragma comment(lib,"xinput9_1_0.lib")
#endif

//-----------------------------------------------------------------------------
// Function-prototypes
//-----------------------------------------------------------------------------
HRESULT UpdateControllerState(int &x, int &y, int& buttonId);

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers 
#define INPUT_DEADZONE  ( 0.24f * FLOAT(0x7FFF) )  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

struct CONTROLLER_STATE
{
    XINPUT_STATE state;
    bool bConnected;
};

CONTROLLER_STATE g_Controllers[MAX_CONTROLLERS];
WCHAR g_szMessage[4][1024] = { 0 };
HWND    g_hWnd;
bool    g_bDeadZoneOn = true;

//-----------------------------------------------------------------------------
HRESULT UpdateControllerState(int &x, int &y, int& buttonId, int& rightTrigger)
{
    DWORD dwResult;
    for (DWORD i = 0; i < MAX_CONTROLLERS; i++)
    {
        // Simply get the state of the controller from XInput.
        dwResult = XInputGetState(i, &g_Controllers[i].state);

        if (dwResult == ERROR_SUCCESS)
            g_Controllers[i].bConnected = true;
        else
            g_Controllers[i].bConnected = false;
    }

    x = 1; y = 0; buttonId = -1; rightTrigger = -1;
    // only use the first controller
    DWORD i = 0;
    if (g_Controllers[i].bConnected) {
        WORD wButtons = g_Controllers[i].state.Gamepad.wButtons;

        if (wButtons & XINPUT_GAMEPAD_A) buttonId = 0;
        else if (wButtons & XINPUT_GAMEPAD_X) buttonId = 2;

        if (g_bDeadZoneOn)
        {
            // Zero value if thumbsticks are within the dead zone 
            if ((g_Controllers[i].state.Gamepad.sThumbLX < INPUT_DEADZONE &&
                g_Controllers[i].state.Gamepad.sThumbLX > -INPUT_DEADZONE) &&
                (g_Controllers[i].state.Gamepad.sThumbLY < INPUT_DEADZONE &&
                    g_Controllers[i].state.Gamepad.sThumbLY > -INPUT_DEADZONE))
            {
                g_Controllers[i].state.Gamepad.sThumbLX = 0;
                g_Controllers[i].state.Gamepad.sThumbLY = 0;
            }
        }

        x = g_Controllers[i].state.Gamepad.sThumbLX;
        y = g_Controllers[i].state.Gamepad.sThumbLY;

        rightTrigger = g_Controllers[i].state.Gamepad.bRightTrigger;
    }

    return S_OK;
}

// CDashserXboxInput definition
CDasherXboxInput::CDasherXboxInput(HWND _hwnd)
: CScreenCoordInput(0, "Mouse Input"), m_hwnd(_hwnd), m_buttonId(-1), m_rightTrigger(-1) {
    // Init state
    ZeroMemory(g_Controllers, sizeof(CONTROLLER_STATE) * MAX_CONTROLLERS);
}

CDasherXboxInput::~CDasherXboxInput(void) {
}

bool CDasherXboxInput::GetScreenCoords(screenint &iX, screenint &iY, CDasherView *pView) {
    int x, y, buttonId = -1, rightTrigger = -1;

    UpdateControllerState(x, y, buttonId, rightTrigger);
    iX = x; iY = y;

    if (buttonId != m_buttonId) {
        if (buttonId == 0) pView->KeyDown(100); // A
        else if (buttonId == 2) pView->KeyDown(101); // X
        m_buttonId = buttonId;
    }

    if (rightTrigger != m_rightTrigger) {
        if (rightTrigger == 255) pView->KeyDown(102); // right trigger (RT)
        m_rightTrigger = rightTrigger;
    }

    return true;
}
