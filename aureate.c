#include <fcntl.h>
#include <git2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "config.h"

#define BASE_URL "https://aur.archlinux.org/"
#define SUFFIX ".git"

int download(char *argv[]) {
	const char* syscache = getenv("XDG_CACHE_HOME");
	const char* pkg = argv[2];

	struct stat st;
	
	if (stat(syscache, &st) == -1) {
		mkdir(syscache, 0700);
	}

	char cache[strlen(syscache)+strlen("/aureate")+1];
	memset(cache, 0, strlen(syscache)+strlen("/aureate")+1);
	strncat(cache, syscache, strlen(syscache));
	strncat(cache, "/aureate", strlen("/aureate"));
	if (stat(cache, &st) == -1) {
		mkdir(cache, 0700);
	}

	//get URL
	char clone_url[strlen(BASE_URL)+strlen(pkg)+strlen(SUFFIX)+1];
	memset(clone_url, 0, strlen(BASE_URL)+strlen(pkg)+strlen(SUFFIX)+1);
	strncat(clone_url, BASE_URL, strlen(BASE_URL));
	strncat(clone_url, pkg, strlen(pkg));
	strncat(clone_url, SUFFIX, strlen(SUFFIX));

	chdir(cache);
	git_libgit2_init();
	printf(BLUE ":: " RESET "Fetching repo...");
	fflush(stdout);
	if (stat(pkg, &st) == -1) {
		git_repository *repo = NULL;
		git_clone(&repo, clone_url, pkg, NULL);
		chdir(pkg);
		if (access("PKGBUILD", F_OK) != -1) {
			printf("done.\n");
			fflush(stdout);
		} else {
			printf("error. Package name invalid or you are not connected to the internet.\n");
			fflush(stdout);
			return 1;
		}
	} else {
		chdir(pkg);
		//git fetch code
	}

	system("makepkg -si");

	return 0;
}

void help() {
	printf(BLUE "AUReate" RESET ": AUR helper in the C programming language\n"
	"Usage: aureate [arguments] <package>\n\n"
	"Arguments:\n"
	GREEN "  -S: " RESET "Sync package from remote respository\n"
	GREEN "  -R: " RESET "Remove local package\n");
}

int main(int argc, char* argv[]) {
	if (geteuid() == 0) {
		fprintf(stderr, RED "Error: do not run as root.\n" RESET);
		return 1;
	}

	if (argc > 1) {
		char *arg = argv[1];
		switch (arg[1]) {
			case 'S':
				if (argc == 3) {
					download(argv);
					break;
				} else {
					help();
					break;
				}
			case 'h':
				help();
				break;
			default:
				printf(RED "Invalid argument: %s\n" RESET, arg);
			help();
				break;
		}
	} else {
		help();
	}
	
	return 0;
}
