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
	char Parent_Unicode_Name[512];
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

void vhd_asctime(char buf1[100], u_long Time_Stamp)
{
	time_t timestamp = vhd_get_time_t(Time_Stamp);
	time_t_asctime(buf1, timestamp);
}

time_t getmtime(int fd) {
	struct stat st;
	if (0 != fstat(fd, &st)) {
		perror("fstat() failed");
		exit(1);
	}
	char buf[100];
	time_t_asctime(buf, st.st_mtime);
	printf("parent file mtime      : %s\n", buf);
	fflush(stdout);
	return st.st_mtime;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int rc;
	struct vhd_footer parent_footer;
	struct vhd_header child_header;

	setlocale(LC_ALL, ".ACP");

	int fix_header = 0;

	if (!(argc == 3 || argc == 4 && 0 == _tcscmp(_T("--fix-header"), argv[3]) && (fix_header = 1))) {
		fprintf(stderr, "usage: progname vhdparent vhdchild [--fix-header]\n");
		return 1;
	}

	FILE *vhdparent = _tfopen(argv[1], _T("r+b"));
	if (!vhdparent) {
		vhdparent = _tfopen(argv[1], _T("rb"));
		if (!vhdparent) {
			_tperror(argv[1]);
			return 1;
		}
	}

	int fdParent = _fileno(vhdparent);

	FILE *vhdchild = _tfopen(argv[2], _T("rb"));
	if (!vhdchild) {
		_tperror(argv[2]);
		return 1;
	}

	rc = _fseeki64(vhdparent, -(off_t)sizeof(struct vhd_footer), SEEK_END);
	if (rc != 0) {
		_tperror(NULL);
	}

	rc = fread(&parent_footer, 1, sizeof(struct vhd_footer), vhdparent);
	if (rc != sizeof(struct vhd_footer)) {
		fprintf(stderr, "read failed\n");
		return 1;
	}

	u_long checksum = ntohl(parent_footer.Checksum);
	u_long calculated = vhd_footer_checksum(&parent_footer);

	int badchksum = 0;

	if (checksum != calculated) {
		fprintf(stderr, "bad checksum in parent vhd\n");
		badchksum = 1;
	}

	//Parent_Unique_ID

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
	vhd_asctime(buf1, child_header.Parent_Time_Stamp);
	vhd_asctime(buf2, parent_footer.Time_Stamp);

	printf("child.Parent_Time_Stamp: %s\n" "parent.Time_Stamp      : %s\n", buf1, buf2);
	fflush(stdout);

	if (badchksum) {
		return 1;
	}
	
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

	if (changed || getmtime(fdParent) > utim.modtime) {
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

