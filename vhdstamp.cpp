#include <winsock2.h>
#include <stdio.h>
#include <tchar.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <time.h>
#include <locale.h>
#include <stddef.h>
#include <windows.h>

typedef char vhd_unique_id_t[16];

struct vhd_footer {
	char Cookie[8];
	char Features[4];
	char File_Format_Version[4];
	char Data_Offset[8];
	u_long Time_Stamp;
	char Creator_Application[4];
	char Creator_Version[4];
	char Creator_Host_OS[4];
	char Original_Size[8];
	char Current_Size[8];
	char Disk_Geometry[4];
	char Disk_Type[4];
	u_long Checksum;
	vhd_unique_id_t Unique_Id;
	char Saved_State[1];
	char Reserved[427];
};

enum { driveFooterSize = sizeof(struct vhd_footer) };

enum { Parent_Unicode_Name_nchars = 512 / sizeof(u_short) };

struct vhd_header {
	char Cookie[8];
	char Data_Offset[8];
	char Table_Offset[8];
	char Header_Version[4];
	char Max_Table_Entries[4];
	char Block_Size[4];
	char Checksum[4];
	vhd_unique_id_t Parent_Unique_ID;
	u_long Parent_Time_Stamp;
	char Reserved[4];
	u_short Parent_Unicode_Name[Parent_Unicode_Name_nchars];
	char Parent_Locator_Entry_1[24];
	char Parent_Locator_Entry_2[24];
	char Parent_Locator_Entry_3[24];
	char Parent_Locator_Entry_4[24];
	char Parent_Locator_Entry_5[24];
	char Parent_Locator_Entry_6[24];
	char Parent_Locator_Entry_7[24];
	char Parent_Locator_Entry_8[24];
	char Reserved2[256];
};

u_long vhd_footer_checksum(struct vhd_footer *driveFooter) {
	int counter;
	u_long save = driveFooter->Checksum;
	u_long checksum = 0;
	driveFooter->Checksum = 0;
	for (counter = 0; counter < driveFooterSize; counter++)
	{
		checksum += ((unsigned char*)driveFooter)[counter];
	}
	driveFooter->Checksum = save;
	checksum = ~checksum;
	return checksum;
}

#define Y2KSTAMP 946684800

time_t vhd_get_time_t(u_long Time_Stamp) {
	return ntohl(Time_Stamp) + Y2KSTAMP;
}

void time_t_asctime(char buf1[100], time_t timestamp)
{
	struct tm tm;
	localtime_s(&tm, &timestamp);

	asctime_s(buf1, 100, &tm);
}

time_t getmtime(int fd) {
	struct stat st;
	if (0 != fstat(fd, &st)) {
		perror("fstat() failed");
		exit(1);
	}
	return st.st_mtime;
}

void vhd_validate(struct vhd_footer *footer, _TCHAR *hint)
{
	u_long checksum = ntohl(footer->Checksum);
	u_long calculated = vhd_footer_checksum(footer);

	int badchksum = 0;

	if (checksum != calculated) {
		_ftprintf(stderr, _T("bad checksum: \n"), hint);
		exit(1);
	}
}

void vhd_read_footer(FILE *f, struct vhd_footer *footer, _TCHAR *hint) {
	int rc;
	rc = _fseeki64(f, -(off_t)sizeof(struct vhd_footer), SEEK_END);
	if (rc != 0) {
		perror("_fseeki64() failed");
		exit(1);
	}

	rc = fread(footer, 1, sizeof(struct vhd_footer), f);
	if (rc != sizeof(struct vhd_footer)) {
		fprintf(stderr, "read failed\n");
		exit(1);
	}

	vhd_validate(footer, hint);
}

static const char DIGITS_LOWER[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

static char *encodeHex(const char data[], size_t datalen, char out[]) {
	for (int i = 0, j = 0; i < datalen; i++) {
		unsigned char c = (unsigned char)data[i];
		out[j++] = DIGITS_LOWER[c >> 4];
		out[j++] = DIGITS_LOWER[0x0F & c];
		c = 0;
	}
	return out;
}

static void print_vhd_unique_id(const char *prefix, vhd_unique_id_t uid) {
	char buf1[100];
	encodeHex(uid, sizeof(vhd_unique_id_t), buf1);
	printf("%s{%.8s-%.4s-%.4s-%.4s-%.12s}\n", prefix, buf1, buf1 + 8, buf1 + 12, buf1 + 16, buf1 + 20);
}

static void print_timestamp(const char *prefix, time_t timestamp) {
	char buf1[100];
	size_t sz;

	time_t_asctime(buf1, timestamp);

	sz = strlen(buf1) - 1;
	if (buf1[sz] == '\n') buf1[sz] = '\0';

	printf("%s%s\n", prefix, buf1);
}

int _tmain(int argc, _TCHAR* argv[])
{
	int rc;
	struct vhd_footer parent_footer, child_footer;
	struct vhd_header child_header;

	setlocale(LC_ALL, ".ACP");

	int fix_header = 0, fix_mtime = 0;

	if (argc < 3) {
		fprintf(stderr, "usage: progname vhdparent vhdchild [--fix-header] [--fix-mtime]\n");
		return 1;
	}

	_TCHAR *sParent = argv[1];
	_TCHAR *sChild = argv[2];

	for (int i = 3; i < argc; i++) {
		if (0 == _tcscmp(argv[i], _T("--fix-header"))) {
			fix_header = 1;
		}
		else if (0 == _tcscmp(argv[i], _T("--fix-mtime"))) {
			fix_mtime = 1;
		}
	}

	FILE *vhdparent = _tfopen(sParent, _T("r+b"));
	if (!vhdparent) {
		vhdparent = _tfopen(sParent, _T("rb"));
		if (!vhdparent) {
			_tperror(sParent);
			return 1;
		}
	}

	int fdParent = _fileno(vhdparent);

	FILE *vhdchild = _tfopen(sChild, _T("rb"));
	if (!vhdchild) {
		_tperror(sChild);
		return 1;
	}

	vhd_read_footer(vhdparent, &parent_footer, sParent);
	vhd_read_footer(vhdchild, &child_footer, sChild);

	rc = _fseeki64(vhdchild, sizeof(struct vhd_footer), SEEK_SET);
	if (rc != 0) {
		_tperror(NULL);
	}

	rc = fread(&child_header, 1, sizeof(struct vhd_header), vhdchild);
	if (rc != sizeof(struct vhd_header)) {
		fprintf(stderr, "read failed\n");
		return 1;
	}

	char buf1[100], buf2[100];

	u_short native_Parent_Unicode_Name[Parent_Unicode_Name_nchars];
	for (int i = 0; i < Parent_Unicode_Name_nchars; i++) {
		native_Parent_Unicode_Name[i] = ntohs(child_header.Parent_Unicode_Name[i]);
	}
	printf("child.Parent_Unicode_Name: %.*S\n", Parent_Unicode_Name_nchars, native_Parent_Unicode_Name);
	printf("\n");

	print_vhd_unique_id("child.Parent_Unique_ID : ", child_header.Parent_Unique_ID);
	print_vhd_unique_id("parent_footer.Unique_Id: ", parent_footer.Unique_Id);
	printf("\n");

	print_timestamp("child.Parent_Time_Stamp: ", vhd_get_time_t(child_header.Parent_Time_Stamp));
	print_timestamp("parent.Time_Stamp      : ", vhd_get_time_t(parent_footer.Time_Stamp));
	time_t parent_mtime = getmtime(fdParent);
	print_timestamp("parent file mtime      : ", parent_mtime);
	printf("\n");

	fflush(stdout);

	int changed = 0;

	if (fix_header && parent_footer.Time_Stamp != child_header.Parent_Time_Stamp) {
		printf("fixing timestamp\n", buf1, buf2);
		fflush(stdout);

		changed = 1;

		parent_footer.Time_Stamp = child_header.Parent_Time_Stamp;
		parent_footer.Checksum = htonl(vhd_footer_checksum(&parent_footer));

		rc = _fseeki64(vhdparent, -(off_t)sizeof(struct vhd_footer), SEEK_END);
		if (rc != 0) {
			_tperror(NULL);
		}

		rc = fwrite(&parent_footer, 1, sizeof(struct vhd_footer), vhdparent);
		if (rc != sizeof(struct vhd_footer)) {
			fprintf(stderr, "write failed\n");
			return 1;
		}

		rc = _fseeki64(vhdparent, 0, SEEK_SET);
		if (rc != 0) {
			_tperror(NULL);
		}

		rc = fwrite(&parent_footer, 1, sizeof(struct vhd_footer), vhdparent);
		if (rc != sizeof(struct vhd_footer)) {
			fprintf(stderr, "write failed\n");
			return 1;
		}

	}

	struct _utimbuf utim;
	utim.modtime = vhd_get_time_t(child_header.Parent_Time_Stamp);

	if (changed || fix_mtime && parent_mtime > utim.modtime) {
		time(&utim.actime);
		printf("resetting parent file modification time\n");
		fflush(stdout);
		if (0 != _futime(fdParent, &utim)) {
			perror("_futime() failed");
			return 1;
		}
	}

	fclose(vhdparent);
	fclose(vhdchild);

	return 0;
}
