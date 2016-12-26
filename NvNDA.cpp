/* MSI LED wrapper (NDA.DLL)
 * tpruvot - 2016
 */

#include <Windows.h>
#include <errno.h>
#include <stdio.h>
#include <inttypes.h>
#include <string>

using namespace std;

static HMODULE library = NULL;
static int64_t cards = 0;

#define LED_LOGO 0
#define LED_SLI 1

#define MSI_LOGO  1
#define MSI_BACK  2 // unused
#define MSI_FRONT 4 // red only on Gaming 10xx
#define MSI_ALL   7

typedef bool (__stdcall*NDA_Initialize)();
typedef bool (__stdcall*NDA_Unload)();
typedef bool (__stdcall*NDA_GetGPUCounts)(int64_t &GpuCounts);
typedef bool (__stdcall*NDA_CheckLEDFunction)(int iAdapterIndex, byte &Date);
typedef bool (__stdcall*NDA_QueryIlluminationSupport)(int iAdapterIndex, int Attribute);
typedef bool (__stdcall*NDA_GetIlluminationParm)(int iAdapterIndex, int Attribute, int &Value);
typedef bool (__stdcall*NDA_SetIlluminationParm)(int iAdapterIndex, int Attribute, int Value);
typedef bool (__stdcall*NDA_SetIlluminationParmColor_RGB)(int iAdapterIndex, int cmd, int led1, int led2, int ontime, int offtime, int time, int darktime, int bright, int r, int g, int b, bool One/*=0*/);

#define internal /* C# definition */
typedef struct {
	internal int iAdapterIndex;
	internal bool IsPrimaryDisplay;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string DisplayName;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_PNP;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_pDeviceId;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_pSubSystemId;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_pRevisionId;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_FullName;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_BIOS_Date;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_BIOS_PartNumber;
	//[MarshalAs(UnmanagedType.BStr)]
	internal string Card_BIOS_Version;
	internal int GPU_Usage;
	internal int GPU_Clock_Current;
	internal int GPU_Clock_Base;
	internal int GPU_Clock_Set;
	internal int GPU_Clock_Max;
	internal int GPU_Clock_Min;
	internal int VRAM_Usage;
	internal int VRAM_Clock_Current;
	internal int VRAM_Clock_Base;
	internal int VRAM_Clock_Max;
	internal int VRAM_Clock_Min;
	internal int GPU_Temperature_Current;
	internal float GPU_Voltage_Current;
	internal int GPU_FanPercent_Current;
	internal int Memory_TotalSize;
} NDA_GraphicsInfo;

typedef bool (*NDA_GetGraphicsInfo)(int iAdapterIndex, NDA_GraphicsInfo* graphicsInfo);

int nda_init()
{
	library = LoadLibraryA("NDA.DLL");
	if (library == NULL) {
		fprintf(stderr, "Unable to find NDA.DLL!");
		return ENOENT;
	}
	// bool NDA_Initialize();
	NDA_Initialize init = (NDA_Initialize) GetProcAddress(library, "_NDA_Initialize@0");
	bool res = init ? init() : false;
	if (!res)
		return ENOSYS;

	NDA_GetGPUCounts fn = (NDA_GetGPUCounts) GetProcAddress(library, "_NDA_GetGPUCounts@4");
	if (!fn)
		return ENOSYS;
 	fn(cards);
	if (!fn)
		return ENOSYS;
	printf("%d card%s found\n", (int)cards, cards>1?"s":"");

	NDA_QueryIlluminationSupport query;
	query = (NDA_QueryIlluminationSupport)GetProcAddress(library, "_NDA_QueryIlluminationSupport@8");
	for (int c = 0; c < cards && query; c++) {
		res = query(c, LED_LOGO);
		if (!res) continue;
		int val = 0;
		NDA_GetIlluminationParm getled = (NDA_GetIlluminationParm)
			GetProcAddress(library, "_NDA_GetIlluminationParm@12");
		res = getled ? getled(c, LED_LOGO, val) : false;
		//printf(" Card %d led at %d %%\n", c, val);
	}
	return 0;
}

int nda_setled(int percent, int color)
{
	if (library == NULL)
		return ENOENT;
	NDA_SetIlluminationParmColor_RGB setrgb = (NDA_SetIlluminationParmColor_RGB)
		GetProcAddress(library, "_NDA_SetIlluminationParmColor_RGB@52");
	NDA_SetIlluminationParm setled = (NDA_SetIlluminationParm)
		GetProcAddress(library, "_NDA_SetIlluminationParm@12"); // ok on evga pascal (std nv brightness)
	NDA_GetIlluminationParm getled = (NDA_GetIlluminationParm)
		GetProcAddress(library, "_NDA_GetIlluminationParm@12");
	if (NULL == setled)
		return ENOSYS;
	for (int c = 2; c < cards; c++) {
		uint8_t r, g, b;
		bool res;
		if (percent != -1) {
			r = g = b = percent * 0xFF;
			res = setled(c, LED_LOGO, (int)percent);
			if (!res) continue;
		} else {
			r = (color >> 16) & 0xFF;
			g = (color >> 8) & 0xFF;
			b = (color & 0xFF);
		}
		if (!color || !percent)
			res = setrgb(c, 24, MSI_ALL, 0, 0, 0, 4, 0, 0, 0x20, 0x20, 0x20, false); // turn off
		else
			res = setrgb(c, 21, MSI_ALL, 0, 0, 0, 4, 0, 0, (int)r, (int)g, (int)b, false);
		if (res) {
			int val = 0;
			getled(c, LED_LOGO, val);
			printf(" Card %d led set at %d %% R:%d G:%d B:%d, res=%d\n", c, val, r,g,b, (int)res);
		}
	}
	return 0;
}

void nda_close()
{
	if (library != NULL) {
		void *pfn = GetProcAddress(library, "_NDA_Unload@0");
		if (pfn) (NDA_Unload)(pfn);
		FreeLibrary(library);
		library = NULL;
	}
}