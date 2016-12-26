// main.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <windows.h>
#include <inttypes.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

int nda_init();
int nda_setled(int percent, int color);
void nda_close();

#define DELAY_1S 1000

int main(int argc, char* const *argv)
{
	int color = 0;
	const char* opt_color;
	if (argc < 2) {
		printf("msi-led for MSI cards with RGB logo - tpruvot@github 2016\n");
		printf("usage: msi-led 0xFF00FF (RGB color)\n");
		return 1;
	}
	opt_color = argv[1];
	color = (int) strtol(opt_color, NULL, 16);
	//printf("color set to 0x%06X\n", color);

	nda_init();
	nda_setled(-1, color);
	nda_close();

	return 0;
}
