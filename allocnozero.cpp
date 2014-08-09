// allocnozero.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "mylogging.h"

#include <string.h>

#include "mylastheader.h"

static int  obtainPriv(LPCTSTR lpName) {
	HANDLE hToken;
	LUID     luid;
	TOKEN_PRIVILEGES tp;    /* token provileges */
	TOKEN_PRIVILEGES oldtp;    /* old token privileges */
	DWORD    dwSize = sizeof (TOKEN_PRIVILEGES);

	int rc = 1;

	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	LookupPrivilegeValue(NULL, lpName, &luid);
	ZeroMemory(&tp, sizeof (tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize);
	rc = 0;
//ennd:
	CloseHandle(hToken);
	return rc;
}

static char humanSize(double *pSize) {
	static const char units[] = "BKMGT";
	const char *unit = units;

	while (*pSize > 1800 && *(unit + 1) != '\0') {
		*pSize /= 1024;
		unit++;
	}
	return *unit;
}

//#define FL512(typ, x) ( (x) & (~(512 / sizeof(typ))) )
#define FL512(x) ( (x) & (~(512 - 1)) )

int _tmain(int argc, _TCHAR* argv[])
{
	FILE *out = stdout;

	LPTSTR filename;
	LPTSTR rest;
	TCHAR savec;

	DWORD SectorsPerCluster;
	DWORD BytesPerSector;
	DWORD NumberOfFreeClusters;
	DWORD TotalNumberOfClusters;

	long long freespace;
	LARGE_INTEGER filesize2;
	static const LARGE_INTEGER zeropos = { 0 };
	double human;

	HANDLE hFile;

	int rc;

	setlocale(LC_ALL, ".ACP");
	setvbuf(out, NULL, _IOFBF, BUFSIZ);

	if (argc != 2) {
		log(ERR, "bad args");
		return 1;
	}
	filename = argv[1];

	rc = obtainPriv(SE_MANAGE_VOLUME_NAME);
	if (rc != 0)
		return rc;

	if (!DeleteFile(filename)) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
			pWin32Error(ERR, "DeleteFile() failed");
			return 1;
		}
	}

	rest = _tcsrchr(filename, '\\');
	if (rest) {
		savec = rest[0];
		rest[0] = '\0';
		if (!SetCurrentDirectory(filename)) {
			pWin32Error(ERR, "SetCurrentDirectory('%S') failed: ", filename);
			return 1;
		}
		filename = rest + 1;
		//rest[0] = savec;
	}
	if (!GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters)) {
		pWin32Error(ERR, "GetDiskFreeSpace() failed");
		return 1;
	}
	freespace = (long long)SectorsPerCluster * BytesPerSector * NumberOfFreeClusters;

	log(INFO, "free space: %lld bytes (%.1f%c)\n", freespace, human, humanSize(&(human = (double)freespace)));

	// leave 10 percent or 10Mb untouched
	filesize2.QuadPart = freespace;
	freespace = freespace / 10;
	if (freespace > 10 * 1024 * 1024)
		freespace = 10 * 1024 * 1024;
	filesize2.QuadPart = FL512(filesize2.QuadPart - freespace);

	log(INFO, "creating file: %lld bytes\n", filesize2.QuadPart);
	fflush(out);

	hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE
		, FILE_SHARE_READ
		, NULL, CREATE_ALWAYS
		, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE
		, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		pWin32Error(ERR, "CreateFile('%S') failed", filename);
		return 1;
	}
	static USHORT nocompress = COMPRESSION_FORMAT_NONE;
	DWORD nb;
	BOOL b;
	b = DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &nocompress, sizeof(nocompress), NULL, 0, &nb, NULL);

	SetFilePointerEx(hFile, filesize2, NULL, FILE_BEGIN);

	rc = 1;

	if (!SetEndOfFile(hFile)) {
		pWin32Error(ERR, "SetEndOfFile() failed");
		goto ennd;
	}
	if (!SetFileValidData(hFile, filesize2.QuadPart)) {
		pWin32Error(ERR, "SetFileValidData() failed");
		goto ennd;
	}
	SetFilePointerEx(hFile, zeropos, NULL, FILE_BEGIN);
	log(INFO, "file created");
	log(INFO, "start reading blocks");

	typedef char block_t[512];
	static const block_t zeroblock = { 0 };

	//enum { LONGS_IN_BLOCK = 512 / sizeof(long) };
	//typedef long block_t[LONGS_IN_BLOCK];

	//enum { BLOCKS_IN_BUF = 8192 / sizeof(block_t) };
	block_t buf[8192 / sizeof(block_t)];

	//enum { LONGS_IN_BUF = 8192 / sizeof(long) };
	//long buf[LONGS_IN_BUF];
	//long buf[BUFCOUNT];
	DWORD t1 = GetTickCount(), t2;
	enum { CONWIDTH = 80 };
	long long curfilepos = 0, byteswrote = 0;
	double progressdivizor = (double)filesize2.QuadPart / (CONWIDTH - 3);

	for (;;) {
		ReadFile(hFile, buf, sizeof(buf), &nb, NULL);

		t2 = GetTickCount();
		if (t2 - t1 > 1000 || nb == 0) {
			int k;
			t1 = t2;
			putc('\r', out);
			putc('|', out);
			int progress = curfilepos < filesize2.QuadPart ? (int)(curfilepos / progressdivizor) : (CONWIDTH - 3);
			for (k = 0; k < progress; k++) {
				putc('=', out);
			}
			for (; k < (CONWIDTH - 3); k++) {
				putc(' ', out);
			}
			putc('|', out);
			fflush(out);
		}

		if (nb == 0)
			break;
		curfilepos += nb;

		int nblocks = nb / sizeof(block_t);
		int curbufblock = nblocks;
		LARGE_INTEGER dist;
		for (int i = 0; i < nblocks; i++) {
			if (0 != memcmp(buf[i], zeroblock, sizeof(block_t))) {
				dist.QuadPart = (i - curbufblock) * (int)sizeof(block_t);
				if (dist.QuadPart != 0) {
					SetFilePointerEx(hFile, dist, NULL, FILE_CURRENT);
				}
				WriteFile(hFile, zeroblock, sizeof(zeroblock), &nb, NULL);
				byteswrote += nb;
				curbufblock = i + 1;
			}
		}
		dist.QuadPart = (nblocks - curbufblock) * (int)sizeof(block_t);
		if (dist.QuadPart != 0) {
			SetFilePointerEx(hFile, dist, NULL, FILE_CURRENT);
		}
	}

	putc('\n', out);
	fflush(out);

	log(INFO, "wrote: %lld bytes (%.1f%c)\n", byteswrote, human, humanSize(&(human = (double)byteswrote)));
	fflush(out);

	rc = 0;
ennd:
	CloseHandle(hFile);
	return rc;
}

