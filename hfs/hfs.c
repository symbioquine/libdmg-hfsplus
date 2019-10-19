#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <hfs/hfsplus.h>
#include <dirent.h>

#include <hfs/hfslib.h>
#include "abstractfile.h"
#include <inttypes.h>

char endianness;


void cmd_ls(Volume* volume, int argc, const char *argv[]) {
	if(argc > 1)
		hfs_ls(volume, argv[1]);
	else
		hfs_ls(volume, "/");
}

void cmd_cat(Volume* volume, int argc, const char *argv[]) {
	HFSPlusCatalogRecord* record;
	AbstractFile* stdoutFile;

	record = getRecordFromPath(argv[1], volume, NULL, NULL);

	stdoutFile = createAbstractFileFromFile(stdout);
	
	if(record != NULL) {
		if(record->recordType == kHFSPlusFileRecord)
			writeToFile((HFSPlusCatalogFile*)record, stdoutFile, volume);
		else
			printf("Not a file\n");
	} else {
		printf("No such file or directory\n");
	}
	
	free(record);
	free(stdoutFile);
}

void cmd_extract(Volume* volume, int argc, const char *argv[]) {
	HFSPlusCatalogRecord* record;
	AbstractFile *outFile;
	
	if(argc < 3) {
		printf("Not enough arguments");
		return;
	}
	
	outFile = createAbstractFileFromFile(fopen(argv[2], "wb"));
	
	if(outFile == NULL) {
		printf("cannot create file");
	}
	
	record = getRecordFromPath(argv[1], volume, NULL, NULL);
	
	if(record != NULL) {
		if(record->recordType == kHFSPlusFileRecord)
			writeToFile((HFSPlusCatalogFile*)record, outFile, volume);
		else
			printf("Not a file\n");
	} else {
		printf("No such file or directory\n");
	}
	
	outFile->close(outFile);
	free(record);
}

void cmd_mv(Volume* volume, int argc, const char *argv[]) {
	if(argc > 2) {
		move(argv[1], argv[2], volume);
	} else {
		printf("Not enough arguments");
	}
}

void cmd_symlink(Volume* volume, int argc, const char *argv[]) {
	if(argc > 2) {
		makeSymlink(argv[1], argv[2], volume);
	} else {
		printf("Not enough arguments");
	}
}

void cmd_mkdir(Volume* volume, int argc, const char *argv[]) {
	if(argc > 1) {
		newFolder(argv[1], volume);
	} else {
		printf("Not enough arguments");
	}
}

void cmd_add(Volume* volume, int argc, const char *argv[]) {
	AbstractFile *inFile;
	
	if(argc < 3) {
		printf("Not enough arguments");
		return;
	}
	
	inFile = createAbstractFileFromFile(fopen(argv[1], "rb"));
	
	if(inFile == NULL) {
		printf("file to add not found");
	}

	add_hfs(volume, inFile, argv[2]);
}

void cmd_rm(Volume* volume, int argc, const char *argv[]) {
	if(argc > 1) {
		removeFile(argv[1], volume);
	} else {
		printf("Not enough arguments");
	}
}

void cmd_chmod(Volume* volume, int argc, const char *argv[]) {
	int mode;
	
	if(argc > 2) {
		sscanf(argv[1], "%o", &mode);
		chmodFile(argv[2], mode, volume);
	} else {
		printf("Not enough arguments");
	}
}

void cmd_extractall(Volume* volume, int argc, const char *argv[]) {
	HFSPlusCatalogRecord* record;
	char cwd[1024];
	char* name;
	
	ASSERT(getcwd(cwd, 1024) != NULL, "cannot get current working directory");
	
	if(argc > 1)
		record = getRecordFromPath(argv[1], volume, &name, NULL);
	else
		record = getRecordFromPath("/", volume, &name, NULL);
	
	if(argc > 2) {
		ASSERT(chdir(argv[2]) == 0, "chdir");
	}

	if(record != NULL) {
		if(record->recordType == kHFSPlusFolderRecord)
			extractAllInFolder(((HFSPlusCatalogFolder*)record)->folderID, volume);  
		else
			printf("Not a folder\n");
	} else {
		printf("No such file or directory\n");
	}
	free(record);
	
	ASSERT(chdir(cwd) == 0, "chdir");
}


void cmd_rmall(Volume* volume, int argc, const char *argv[]) {
	HFSPlusCatalogRecord* record;
	char* name;
	char initPath[1024];
	int lastCharOfPath;
	
	if(argc > 1) {
		record = getRecordFromPath(argv[1], volume, &name, NULL);
		strcpy(initPath, argv[1]);
		lastCharOfPath = strlen(argv[1]) - 1;
		if(argv[1][lastCharOfPath] != '/') {
			initPath[lastCharOfPath + 1] = '/';
			initPath[lastCharOfPath + 2] = '\0';
		}
	} else {
		record = getRecordFromPath("/", volume, &name, NULL);
		initPath[0] = '/';
		initPath[1] = '\0';	
	}
	
	if(record != NULL) {
		if(record->recordType == kHFSPlusFolderRecord) {
			removeAllInFolder(((HFSPlusCatalogFolder*)record)->folderID, volume, initPath);
		} else {
			printf("Not a folder\n");
		}
	} else {
		printf("No such file or directory\n");
	}
	free(record);
}

void cmd_addall(Volume* volume, int argc, const char *argv[]) {   
	if(argc < 2) {
		printf("Not enough arguments");
		return;
	}

	if(argc > 2) {
		addall_hfs(volume, argv[1], argv[2]);
	} else {
		addall_hfs(volume, argv[1], "/");
	}
}

void cmd_grow(Volume* volume, int argc, const char *argv[]) {
	uint64_t newSize;

	if(argc < 2) {
		printf("Not enough arguments\n");
		return;
	}
	
	newSize = 0;
	sscanf(argv[1], "%" PRId64, &newSize);

	grow_hfs(volume, newSize);

	printf("grew volume: %" PRId64 "\n", newSize);
}

void cmd_getattr(Volume* volume, int argc, const char *argv[]) {
	HFSPlusCatalogRecord* record;

	if(argc < 3) {
		printf("Not enough arguments");
		return;
	}

	record = getRecordFromPath(argv[1], volume, NULL, NULL);

	if(record != NULL) {
		HFSCatalogNodeID id;
		uint8_t* data;
		size_t size;
		if(record->recordType == kHFSPlusFileRecord)
			id = ((HFSPlusCatalogFile*)record)->fileID;
		else
			id = ((HFSPlusCatalogFolder*)record)->folderID;

		size = getAttribute(volume, id, argv[2], &data);

		if(size > 0) {
			fwrite(data, size, 1, stdout);
			free(data);
		} else {
			printf("No such attribute\n");
		}
	} else {
		printf("No such file or directory\n");
	}
	
	free(record);
}

// Copied from https://stackoverflow.com/a/122721
// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *trim_whitespace(char *str) {
	if (str == NULL) {
		return NULL;
	}

	char *end;

	// Trim leading space
	while(isspace((unsigned char)*str)) str++;

	// All spaces?
	if(*str == 0) {
		return str;
	}

	// Trim trailing space
	end = str + strlen(str) - 1;
	while(end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator character
	end[1] = '\0';

	return str;
}

int getFlagOpt(char* flagOptStr, uint16_t* flagOptOut) {
	if(strcmp(flagOptStr, "HasCustomIcon") == 0) {
		*flagOptOut = kHasCustomIcon;
	} else if(strcmp(flagOptStr, "IsStationary") == 0) {
		*flagOptOut = kIsStationery;
	} else if(strcmp(flagOptStr, "NameLocked") == 0) {
		*flagOptOut = kNameLocked;
	} else if(strcmp(flagOptStr, "HasBundle") == 0) {
		*flagOptOut = kHasBundle;
	} else if(strcmp(flagOptStr, "IsInvisible") == 0) {
		*flagOptOut = kIsInvisible;
	} else if(strcmp(flagOptStr, "IsAlias") == 0) {
		*flagOptOut = kIsAlias;
	} else {
		return 1;
	}
	return 0;
}

static const char SET_FINDER_FLAGS_USAGE[] = "usage: setfinderflags /path/to/my/file +HasCustomIcon,-HasBundle\n";
static const char FINDER_FLAGS_DELIM = ',';
static const uint16_t FILE_ONLY_FLAGS = (kIsStationery & kHasBundle & kIsAlias);

int cmd_setfinderflags(Volume* volume, int argc, const char *argv[]) {
	HFSPlusCatalogRecord* record;

	if(argc < 2) {
		printf("Please specify the file/folder path to set finder flags on\n");
		printf(SET_FINDER_FLAGS_USAGE);
		return 1;
	}

	if(argc < 3) {
		printf("Please specify the finder flags to set %d\n", argc);
		printf(SET_FINDER_FLAGS_USAGE);
		return 1;
	}

	record = getRecordFromPath(argv[1], volume, NULL, NULL);

	if(record == NULL) {
		printf("No such file or directory\n");
		free(record);
		return 1;
	}

    int isFolder = 0;
	uint16_t recordFinderFlags;
	if (record->recordType == kHFSPlusFileRecord) {
		recordFinderFlags = ((HFSPlusCatalogFile*)record)->userInfo.finderFlags;
	} else {
		isFolder = 1;
		recordFinderFlags = ((HFSPlusCatalogFolder*)record)->userInfo.finderFlags;
	}

	uint16_t flagOptsUsed = 0x0000;

	char* flagOptsStr = strdup(argv[2]);
	char *flagOptStr;

	flagOptStr = trim_whitespace(strtok(flagOptsStr, &FINDER_FLAGS_DELIM));
	while (flagOptStr) {
		if (flagOptStr[0] != '\0' && flagOptStr[0] != '-' && flagOptStr[0] != '+') {
			printf("Invalid flag option '%s'. Please prefix each flag with either '+' to set the flag or '-' to clear the flag\n", flagOptStr);
			printf(SET_FINDER_FLAGS_USAGE);
			free(flagOptsStr);
			free(record);
			return 1;
		}

		if (flagOptStr[0] != '\0') {
			uint16_t flagOpt;
			int res = getFlagOpt(flagOptStr + 1, &flagOpt);

			if (res != 0) {
				printf("Unrecognized finder flag '%s'. Supported flags: HasCustomIcon, IsStationary, NameLocked, HasBundle, IsInvisible, IsAlias\n", flagOptStr);
				printf(SET_FINDER_FLAGS_USAGE);
				free(flagOptsStr);
				free(record);
				return 1;
			}

			if (flagOpt) {
				if (isFolder && (flagOpt & FILE_ONLY_FLAGS)) {
					printf("Cannot set flag %s on folder '%s' since is only supported on files\n", flagOptStr + 1, argv[1]);
					printf(SET_FINDER_FLAGS_USAGE);
					free(flagOptsStr);
					free(record);
					return 1;
				}

				if (flagOptsUsed & flagOpt) {
					printf("Please remove duplicate occurrence of flag: %s\n", flagOptStr + 1);
					printf(SET_FINDER_FLAGS_USAGE);
					free(flagOptsStr);
					free(record);
					return 1;
				}
				flagOptsUsed |= flagOpt;

				if (flagOptStr[0] == '+') {
					recordFinderFlags |= flagOpt;
				} else {
					recordFinderFlags &= ~flagOpt;
				}
			}
		}
		flagOptStr = trim_whitespace(strtok(NULL, &FINDER_FLAGS_DELIM));
	}

	if (record->recordType == kHFSPlusFileRecord) {
		((HFSPlusCatalogFile*)record)->userInfo.finderFlags = recordFinderFlags;
	} else {
		((HFSPlusCatalogFolder*)record)->userInfo.finderFlags = recordFinderFlags;
	}

	updateCatalog(volume, record);
	free(flagOptsStr);
	free(record);
	return 0;
}

void TestByteOrder()
{
	short int word = 0x0001;
	char *byte = (char *) &word;
	endianness = byte[0] ? IS_LITTLE_ENDIAN : IS_BIG_ENDIAN;
}

static const char TOOL_USAGE[] = "usage: %s <image-file> <ls|cat|mv|mkdir|add|rm|chmod|extract|extractall|rmall|addall|getattr|setfinderflags|debug> <arguments>\n";

int main(int argc, const char *argv[]) {
	io_func* io;
	Volume* volume;
	
	TestByteOrder();
	
	if(argc < 3) {
		printf("%s: Please specify an image file and command\n", argv[0]);
		printf(TOOL_USAGE, argv[0]);
		return 1;
	}
	
	io = openFlatFile(argv[1]);
	if(io == NULL) {
		fprintf(stderr, "error: Cannot open image-file.\n");
		return 1;
	}
	
	volume = openVolume(io); 
	if(volume == NULL) {
		fprintf(stderr, "error: Cannot open volume.\n");
		CLOSE(io);
		return 1;
	}

	int result = 0;
	if(strcmp(argv[2], "ls") == 0) {
		cmd_ls(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "cat") == 0) {
		cmd_cat(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "mv") == 0) {
		cmd_mv(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "symlink") == 0) {
		cmd_symlink(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "mkdir") == 0) {
		cmd_mkdir(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "add") == 0) {
		cmd_add(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "rm") == 0) {
		cmd_rm(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "chmod") == 0) {
		cmd_chmod(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "extract") == 0) {
		cmd_extract(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "extractall") == 0) {
		cmd_extractall(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "rmall") == 0) {
		cmd_rmall(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "addall") == 0) {
		cmd_addall(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "grow") == 0) {
		cmd_grow(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "getattr") == 0) {
		cmd_getattr(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "setfinderflags") == 0) {
		result = cmd_setfinderflags(volume, argc - 2, argv + 2);
	} else if(strcmp(argv[2], "debug") == 0) {
		if(argc > 3 && strcmp(argv[3], "verbose") == 0) {
			debugBTree(volume->catalogTree, TRUE);
		} else {
			debugBTree(volume->catalogTree, FALSE);
		}
	} else {
		printf("%s: Please specify a supported command\n", argv[0]);
		printf(TOOL_USAGE, argv[0]);
		result = 1;
	}
	
	closeVolume(volume);
	CLOSE(io);
	
	return result;
}
