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

int download(char *argv[]) {
	//get cache dir
	char syscache[100];
	strcpy(syscache, getenv("XDG_CACHE_HOME"));
	struct stat st;
	if (stat(syscache, &st) == -1) {
		mkdir(syscache, 0700);
	}
	strcat(syscache, "/aureate/");
	char cache[100];
	strcpy(cache, syscache);
	if (stat(cache, &st) == -1) {
		mkdir(cache, 0700);
	}

	//get URL
	const char* pkg = argv[2];
	const char* pkgname = argv[2];
	strcpy (pkgname, pkg);
	//const char* baseurl = "https://aur.archlinux.org/cgit/aur.git/snapshot/";
	const char* baseurl = "https://aur.archlinux.org/";
	char url[100] = "";
	strcat(url, baseurl);
	strcat(url, pkg);
	char final_url[100] = "";
	strcat(final_url, url);
	strcat(final_url, ".git");

	chdir(cache);
	if (stat(pkgname, &st) == -1) {
		git_repository *repo;
		git_repository_open(&repo, pkgname);
		git_remote *remote;
		git_remote_lookup(&remote, repo, "origin");
		git_strarray refspecs;
		refspecs.count = 1;
		refspecs.strings = (char**) malloc(refspecs.count * sizeof(char*));
		refspecs.strings[0] = "refs/heads/*:refs/heads/*";

		git_remote_fetch(remote, &refspecs, NULL, NULL);

		git_oid remote_head, local_head;
		git_remote_head(remote, &remote_head);
		git_reference_name_to_id(&local_head, repo, "HEAD");

		if (git_oid_cmp(&remote_head, &local_head) == 0) {
			printf("No changes\n");
			return 1;
		}
	}
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
	bool isCaseSensitive = true;
	enum { CHARACTER_MODE, WORD_MODE, LINE_MODE } mode = CHARACTER_MODE;

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
