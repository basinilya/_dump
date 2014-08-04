// allocnozero.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "mylogging.h"
#include "mylastheader.h"

static int  obtainPriv(LPCTSTR lpName) {
	HANDLE hToken;
	LUID     luid;
	TOKEN_PRIVILEGES tp;    /* token provileges */
	TOKEN_PRIVILEGES oldtp;    /* old token privileges */
	DWORD    dwSize = sizeof (TOKEN_PRIVILEGES);

	int rc = 1;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		pWin32Error(ERR, "OpenProcessToken() failed");
		return 1;
	}
	if (!LookupPrivilegeValue(NULL, lpName, &luid)) {
		pWin32Error(ERR, "LookupPrivilegeValue() failed");
		goto ennd;
	}
	ZeroMemory(&tp, sizeof (tp));
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &oldtp, &dwSize)) {
		pWin32Error(ERR, "AdjustTokenPrivileges() failed");
		goto ennd;
	}
	rc = 0;
ennd:
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

int _tmain(int argc, _TCHAR* argv[])
{
	LPTSTR filename;
	LPTSTR rest;
	TCHAR savec;

	DWORD SectorsPerCluster;
	DWORD BytesPerSector;
	DWORD NumberOfFreeClusters;
	DWORD TotalNumberOfClusters;

	long long freespace;
	long long filesize;
	LARGE_INTEGER filesize2;
	double human;

	HANDLE hFile;
	HANDLE hToken;

	int rc;

	setlocale(LC_ALL, ".ACP");

	if (argc != 2) {
		log(ERR, "bad args");
		return 1;
	}
	filename = argv[1];
	rest = _tcsrchr(filename, '\\');
	if (rest) {
		savec = rest[0];
		rest[0] = '\0';
		if (!SetCurrentDirectory(filename)) {
			pWin32Error(ERR, "SetCurrentDirectory('%S') failed: ", filename);
			return 1;
		}
		rest[0] = savec;
	}
	if (!GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters)) {
		pWin32Error(ERR, "GetDiskFreeSpace() failed");
		return 1;
	}
	freespace = (long long)SectorsPerCluster * BytesPerSector * NumberOfFreeClusters;

	printf("free space: %lld bytes (%.1f%c)\n", freespace, human, humanSize(&(human = freespace)));
	filesize = freespace - (freespace / 10);
	printf("creating file: %lld bytes\n", filesize);

	rc = obtainPriv(SE_MANAGE_VOLUME_NAME);
	if (rc != 0)
		return rc;
	
	hFile = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		pWin32Error(ERR, "CreateFile('%S') failed", filename);
		return 1;
	}
	filesize2.QuadPart = filesize;
	if (!SetFilePointerEx(hFile, filesize2, NULL, FILE_BEGIN)) {
		pWin32Error(ERR, "SetFilePointerEx() failed", filename);
		goto err;
	}
	if (!SetEndOfFile(hFile)) {
		pWin32Error(ERR, "SetEndOfFile() failed", filename);
		goto err;
	}
	if (!SetFileValidData(hFile, filesize)) {
		pWin32Error(ERR, "SetFileValidData() failed", filename);
		goto err;
	}
	filesize2.QuadPart = 0;
	if (!SetFilePointerEx(hFile, filesize2, NULL, FILE_BEGIN)) {
		pWin32Error(ERR, "SetFilePointerEx() failed", filename);
		goto err;
	}
	log(INFO, "file created");

	CloseHandle(hFile);
	return 0;
	err:
	CloseHandle(hFile);
	DeleteFile(filename);
	return 1;
}

