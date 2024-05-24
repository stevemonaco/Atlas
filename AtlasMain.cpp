// Atlas main entry point

#include "stdafx.h"
#include <ctime>
#include <string>
#include <vector>
#include <conio.h>
#include "AtlasCore.h"

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	clock_t StartTime, EndTime, ElapsedTime;
	int argoff = 0;
	int retcode = 0;

	Logger.SetLogStatus(false);
	StartTime = clock();

	printf("Atlas 1.12 by Klarth\n\n");
	if (argc != 3 && argc != 5)
	{
		printf("Usage: %s [switches] ROM.ext Script.txt\n", argv[0]);
		printf("Switches: -d filename or -d stdout (debugging)\n");
		printf("Arguments in brackets are optional\n\n");
		printf("Press any key to continue\n");
		(void)getch();
		return 1;
	}

	if (strcmp("-d", argv[1]) == 0)
	{
		if (strcmp("stdout", argv[2]) == 0)
			Atlas.SetDebugging(stdout);
		else
			Atlas.SetDebugging(fopen(argv[2], "w"));
		argoff += 2;
	}

	if (!Atlas.Insert(argv[1 + argoff], argv[2 + argoff]))
	{
		printf("Insertion failed\n\n");
		retcode = 2;
	}

	EndTime = clock();

	ElapsedTime = EndTime - StartTime;

	printf("Execution time: %u msecs\n", (unsigned int)ElapsedTime);

	return retcode;
}