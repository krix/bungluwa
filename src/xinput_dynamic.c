// Dynamic XInput loader — avoids hard dependency on XINPUT1_3.dll which may
// not be present on modern Windows. Tries xinput1_4 (Win8+), then xinput1_3,
// then xinput9_1_0 (Vista+). If none are found, XInputGetState simply reports
// no controller connected.

#ifdef _WIN32
#include <windows.h>
#include <xinput.h>

typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, XINPUT_STATE *);
static PFN_XInputGetState pfnXInputGetState = NULL;
static BOOL xinput_loaded = FALSE;
static BOOL xinput_init_attempted = FALSE;

static void xinput_init(void) {
	if (xinput_init_attempted) return;
	xinput_init_attempted = TRUE;

	const char *dll_names[] = {
		"xinput1_4.dll",
		"xinput1_3.dll",
		"xinput9_1_0.dll",
	};

	for (int i = 0; i < 3; i++) {
		HMODULE hmod = LoadLibraryA(dll_names[i]);
		if (hmod) {
			pfnXInputGetState = (PFN_XInputGetState)GetProcAddress(hmod, "XInputGetState");
			if (pfnXInputGetState) {
				xinput_loaded = TRUE;
				return;
			}
			FreeLibrary(hmod);
		}
	}
}

DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE *pState) {
	xinput_init();
	if (pfnXInputGetState) {
		return pfnXInputGetState(dwUserIndex, pState);
	}
	return ERROR_DEVICE_NOT_CONNECTED;
}
#endif
