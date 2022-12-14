#include <archive.h>
#include <archive_entry.h>
#include <curl/curl.h>
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

//OLD CURL METHOD, HERE AS A BACKUP JUST IN CASE
	//strcat(final_url, ".tar.gz");
	
	//printf("Downloading tarball...");
	//fflush(stdout);
	//CURL *curl;
	//CURLcode res;
	//FILE *fp;
	//curl = curl_easy_init();
	//if (curl) {
	//	char file_path[100];
	//	sprintf(file_path, "%s%s%s", cache, pkg, ".tar.gz");
	//	fp = fopen(file_path, "wb");
	//	curl_easy_setopt(curl, CURLOPT_URL, final_url);
	//	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
	//	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
	//	res = curl_easy_perform(curl);
	//	curl_easy_cleanup(curl);
	//	fclose(fp);
	//}

	//[ -d pkgname ] && cd into and git pull || git clone final_url

	//TODO: find a better way to do this
	//archive_entry_set_pathname(cache, pkg);
	
	//char* tarfile = strcat(cache, pkg);
	//strcat(tarfile, ".tar.gz");
	//struct archive *a = archive_read_new();
	//archive_read_support_filter_gzip(a);
	//archive_read_support_format_tar(a);
	//int r = archive_read_open_filename(a, tarfile, 10240);
	//if (r != ARCHIVE_OK) {
	//	fprintf(stderr, RED "error.\nEither the package name is invalid or you are not connected to the internet.\n" RESET);
	//	remove(tarfile);
	//	return 1;
	//} else {
	//	printf(GREEN "done!\n" RESET);
	//	remove(tarfile);
	//}

	//struct archive *ext = archive_write_disk_new();
	//archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS | ARCHIVE_EXTRACT_PERM);
	//archive_write_disk_set_standard_lookup(ext);

	//struct archive_entry *entry;
	//printf("Extracting tarball...");
	//while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
	//	r = archive_read_extract(a, entry, 0);
	//	if (r != ARCHIVE_OK) {
	//		fprintf(stderr, RED "error.\n" RESET);
	//		return 1;
	//	}
	//}
	//printf(GREEN "done!\n" RESET);

	//archive_read_close(a);
	//archive_read_free(a);
	//archive_write_close(ext);
	//archive_write_free(ext);

	//char *pkgdir = strcat(syscache, argv[2]);
	//chdir(pkgdir);
	//system("makepkg -si");

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
