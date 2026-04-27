
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

/*
void c------------------------------() {}
*/

int main(int argc, char *argv[])
{
	return KeenMain(argc, argv);
}

void platform_msgbox(const char *message)
{
	lprintf("%s\n", message);
}

/*
void c------------------------------() {}
*/

struct DirectoryEntry
{
	char *name;
	bool is_directory;
};

static DirectoryEntry *filelist = NULL;
static int num_files = 0;


int LoadDirectoryListing(char *path)
{
	FreeDirectoryListing();
	num_files = 0;

	DIR *dir = opendir(path);
	if (!dir)
	{
		lprintf("LoadDirectoryEntry: failure opening path '%s'", path);
		return 0;
	}

	int alloc_size = 0;
	struct dirent *entry;

	while((entry = readdir(dir)))
	{
		if (entry->d_name[0] == '.')
		{
			if ((entry->d_name[1] == '.' && !entry->d_name[2]) || \
				entry->d_name[1] == 0)
			{
				continue;
			}
		}

		if (num_files >= alloc_size)
		{
			alloc_size = (num_files + 256);
			int requiredBytes = (alloc_size * sizeof(DirectoryEntry));

			if (!filelist)
				filelist = (DirectoryEntry *)malloc(requiredBytes);
			else
				filelist = (DirectoryEntry *)realloc(filelist, requiredBytes);
		}

		filelist[num_files].name = strdup(entry->d_name);
		filelist[num_files].is_directory = (entry->d_type == DT_DIR);
		num_files++;
	}

	closedir(dir);
	return num_files;
}


const char *GetFileAtIndex(int fno)
{
	if (fno >= num_files || fno < 0 || !filelist) return "<>";
	return filelist[fno].name;
}


char GetIsDirectory(int fno)
{
	if (fno >= num_files || fno < 0 || !filelist) return false;
	return filelist[fno].is_directory;
}

void FreeDirectoryListing(void)
{
	if (filelist)
	{
		for(int i=0;i<num_files;i++)
		{
			if (filelist[i].name)
				free(filelist[i].name);
		}

		free(filelist);
		filelist = NULL;
	}

	num_files = 0;
}
