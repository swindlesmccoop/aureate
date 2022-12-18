#include <curl/curl.h>
#include <dirent.h>
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
#define PARSE_URL BASE_URL "rpc/?v=5&type=info&arg="
#define SUFFIX ".git"

int download(char *argv[]) {
	//define vars
	const char* syscache = getenv("XDG_CACHE_HOME");
	const char* pkg = argv[2];

	//check if XDG_CACHE_HOME exists - if not, create it
	struct stat st;
	if (stat(syscache, &st) == -1) {
		mkdir(syscache, 0700);
	}

	//construct aureate cache var
	char cache[strlen(syscache)+strlen("aureate")+1];
	memset(cache, 0, strlen(syscache)+strlen("aureate")+1);
	sprintf(cache, "%s/%s", syscache, "aureate");
	
	//do the same thing as syscache with aureate cache
	if (stat(cache, &st) == -1) {
		mkdir(cache, 0700);
	}

	//construct clone URL
	char clone_url[strlen(BASE_URL)+strlen(pkg)+strlen(SUFFIX)+1];
	memset(clone_url, 0, strlen(BASE_URL)+strlen(pkg)+strlen(SUFFIX)+1);
	sprintf(clone_url, "%s%s%s", BASE_URL, pkg, SUFFIX);

	//clone or pull the repo
	chdir(cache);
	git_libgit2_init();

	//if directory for package already exists, clone
	if (stat(pkg, &st) == -1) {
		printf(BLUE ":: " RESET "Fetching repo..."); fflush(stdout);
		git_repository *repo = NULL;
		git_clone(&repo, clone_url, pkg, NULL);
		chdir(pkg);
		if (access("PKGBUILD", F_OK) != -1) {
			printf("done.\n"); fflush(stdout);
		} else {
			printf("error. Package name invalid or you are not connected to the internet.\n"); fflush(stdout);
			//remove failed clone
			return 1;
		}
	//else, pull latest changes
	} else {
		printf(BLUE ":: " RESET "Fetching latest changes..."); fflush(stdout);
		git_repository *repo;
		git_remote *remote;

		git_repository_open(&repo, pkg);
		git_remote_lookup(&remote, repo, "origin");
		git_remote_fetch(remote, NULL, NULL, "pull");
		git_remote_free(remote);
		git_repository_free(repo);
		
		printf("done.\n"); fflush(stdout);
		chdir(pkg);
	}

	//hand off the rest to pacman
	system("makepkg -si");
	return 0;
}

int uninstall(char *argv[]) {
	const char* pkg = argv[2];
	char cmd[strlen(SUDO)+strlen("pacman -R")+strlen(pkg)+1];
	memset(cmd, 0, strlen(SUDO)+strlen("pacman -R")+strlen(pkg)+1);
	sprintf(cmd, "%s pacman -R %s", SUDO, pkg);
	system(cmd);
}

void help() {
	printf(BLUE "AUReate" RESET ": AUR helper in the C programming language\n"
	"Usage: aureate [arguments] <package>\n\n"
	"Arguments:\n"
	GREEN "  -S: " RESET "Sync package from remote respository\n"
	GREEN "  -R: " RESET "Remove local package\n");
}

void search() {
	printf("Not yet implemented\n");
}

void flags(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		/* -S  */ if (strcmp(argv[i], "-S") == 0) { download(argv); }
		/* -Ss */ else if (strcmp(argv[i], "-Ss") == 0) { search(argv); }
		/* -R  */ else if (strcmp(argv[i], "-R") == 0) { uninstall(argv); }
	}
}


int main(int argc, char* argv[]) {
	//kill if root is executing
	if (geteuid() == 0) {
		fprintf(stderr, RED "Error: do not run as root.\n" RESET);
		return 1;
	}
	
	//require args
	if (argc < 1) {
		help();
		return 1;
	}
	
	//run program
	flags(argc, argv);
	return 0;
}
