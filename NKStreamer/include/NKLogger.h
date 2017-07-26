#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <3ds.h>

FILE* f = nullptr;

void InitLogger()
{
	f = fopen("romfs:/logs/dump.txt", "rw");
	if(f)
	{
		printf("Load log ok\n");
	}
}

void EndLogger()
{
	if (f == nullptr) return;
	fclose(f);
}

void Log(const char* text)
{
	if (f == nullptr) return;
	int writed = fwrite(text, sizeof(char), sizeof(text), f);
}