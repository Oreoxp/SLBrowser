#pragma once

#define ISOLATION_AWARE_ENABLED		1

#define GDIPVER	0x0110

//#define WIN32_LEAN_AND_MEAN

#include "targetver.h"
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <litehtml.h>
#include <winhttp.h>
#include <algorithm>

// include TxDIB project
#include <TxDIB.h>
#pragma comment(lib, "txdib.lib")

// include CAIRO project
#include <cairo.h>
#include <cairo-win32.h>
#pragma comment(lib, "cairo.lib")

// include SIMPLEDIB project
#include <dib.h>
#pragma comment(lib, "simpledib.lib")

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Msimg32.lib")

extern cairo_surface_t* dib_to_surface(CTxDIB& img);