// allocnozero.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "mylogging.h"

#include <string.h>

#include "mylastheader.h"

static int  obtain_priv(LPCTSTR lpName) {
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

	LookupPrivilegeValue(NULL, lpName, &luid);

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

static char human_size(double *pSize) {
	static const char units[] = "BKMGT";
	const char *unit = units;

	while (*pSize > 1800 && *(unit + 1) != '\0') {
		*pSize /= 1024;
		unit++;
	}
	return *unit;
}

static void clear_compression_flag(HANDLE hFile) {
	static USHORT nocompress = COMPRESSION_FORMAT_NONE;
	DWORD nb;
	if (!DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &nocompress, sizeof(nocompress), NULL, 0, &nb, NULL)) {
		pWin32Error(WARN, "DeviceIoControl(FSCTL_SET_COMPRESSION, 0) failed");
	}
}

static long long cwd_free_space() {
	DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
	if (!GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters)) {
		pWin32Error(ERR, "GetDiskFreeSpace() failed");
		return 0;
	}
	return (long long)SectorsPerCluster * BytesPerSector * NumberOfFreeClusters;
}

static size_t dirname_len(LPCTSTR path)
{
	LPCTSTR p = path;
	size_t len = 0, sublen = 0;

	for (;; p++) {
		if (*p == 0) return len;
		sublen++;
		if (*p == '/' || *p == '\\') {
			len += sublen;
			sublen = 0;
		}
	}
}

enum { CONWIDTH = 80 };

typedef struct zerofree_data_t {
	DWORD lastticks;
	double progressdivizor;
	long long curfilepos;
	LARGE_INTEGER filesize;
} zerofree_data_t;

static void print_progress(zerofree_data_t *progress, int force) {
	DWORD ticks = GetTickCount();

	if (force || ticks - progress->lastticks > 1000) {
		FILE *out = stdout;
		int k;
		int progr;

		progress->lastticks = ticks;

		putc('\r', out);
		putc('|', out);
		if (progress->curfilepos - progress->filesize.QuadPart < 0) {
			progr = (int)(progress->curfilepos / progress->progressdivizor);
		} else {
			progr = CONWIDTH - 3;
		}
		for (k = 0; k < progr; k++) {
			putc('=', out);
		}
		for (; k < (CONWIDTH - 3); k++) {
			putc(' ', out);
		}
		putc('|', out);
		fflush(out);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	typedef char block_t[512];
	static const LARGE_INTEGER zeropos = { 0 };
	static const block_t zeroblock = { 0 };

	int rc;
	DWORD nb;
	size_t sz;

	LPTSTR filename;

	long long freespace, byteswrote = 0;

	double humansize;

	HANDLE hFile;

	block_t buf[8192 / sizeof(block_t)];

	zerofree_data_t data;

	FILE *out = stdout;


	setlocale(LC_ALL, ".ACP"); // correct code page for localized system messages
	setvbuf(out, NULL, _IOFBF, BUFSIZ); // no autoflush stdout

	if (argc != 2) {
		log(ERR, "bad args. usage: progname FILE");
		return 1;
	}
	filename = argv[1];

	rc = obtain_priv(SE_MANAGE_VOLUME_NAME);
	if (rc != 0)
		return rc;

	if (!DeleteFile(filename)) {
		if (GetLastError() != ERROR_FILE_NOT_FOUND) {
			pWin32Error(ERR, "DeleteFile() failed");
			return 1;
		}
	}

	// chdir to file's directory
	sz = dirname_len(filename);
	if (sz != 0) {
		filename[sz-1] = '\0';
		if (!SetCurrentDirectory(filename)) {
			pWin32Error(ERR, "SetCurrentDirectory('%S') failed: ", filename);
			return 1;
		}
		filename += sz;
	}

	freespace = cwd_free_space();

	log(INFO, "free space: %lld bytes (%.1f%c)", freespace, humansize, human_size(&(humansize = (double)freespace)));

	// leave untouched at most 10Mb, at least 10 percent
	data.filesize.QuadPart = freespace;
	freespace = freespace / 10;
	if (freespace > 10 * 1024 * 1024)
		freespace = 10 * 1024 * 1024;

	// round to block size
	data.filesize.QuadPart = (data.filesize.QuadPart - freespace) / 512 * 512;

	if (data.filesize.QuadPart == 0) {
		log(ERR, "not enough free space");
		return 1;
	}

	log(INFO, "creating file: %lld bytes", data.filesize.QuadPart);

	hFile = CreateFile(filename
		, GENERIC_READ | GENERIC_WRITE
		, 0 /* others can't read */
		, NULL, CREATE_ALWAYS
		, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE
		, NULL);
	if (INVALID_HANDLE_VALUE == hFile) {
		pWin32Error(ERR, "CreateFile('%S') failed", filename);
		return 1;
	}

	clear_compression_flag(hFile);

	SetFilePointerEx(hFile, data.filesize, NULL, FILE_BEGIN);

	rc = 1;

	if (!SetEndOfFile(hFile)) {
		pWin32Error(ERR, "SetEndOfFile() failed");
		goto ennd;
	}
	if (!SetFileValidData(hFile, data.filesize.QuadPart)) {
		pWin32Error(ERR, "SetFileValidData() failed");
		goto ennd;
	}
	SetFilePointerEx(hFile, zeropos, NULL, FILE_BEGIN);
	log(INFO, "file created");
	log(INFO, "start rewriting non-zero blocks");

	data.curfilepos = 0;
	data.progressdivizor = (double)data.filesize.QuadPart / (CONWIDTH - 3);

	print_progress(&data, 1);
	for (;;) {
		ReadFile(hFile, buf, sizeof(buf), &nb, NULL);

		print_progress(&data, nb == 0);

		if (nb == 0)
			break;
		data.curfilepos += nb;

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

	log(INFO, "wrote: %lld bytes (%.1f%c)", byteswrote, humansize, human_size(&(humansize = (double)byteswrote)));

	rc = 0;
ennd:
	CloseHandle(hFile);
	return rc;
}

