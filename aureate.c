#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <tar.h>

int download(char *argv[]) {
	//get cache dir
	char cache_dir[100];
    strcpy(cache_dir, getenv("XDG_CACHE_HOME"));
	struct stat st;
	if (stat(cache_dir, &st) == -1) {mkdir(cache_dir, 0700);}
    strcat(cache_dir, "/aureate/");
    char cache[100];
    strcpy(cache, cache_dir);
	if (stat(cache, &st) == -1) {mkdir(cache, 0700);}

	//get URL
	const char* pkg = argv[2];
	const char* baseurl = "https://aur.archlinux.org/cgit/aur.git/snapshot/";
	char url[100] = "";
	strcat(url, baseurl);
	strcat(url, pkg);
	char final_url[100] = "";
	strcat(final_url, url);
	strcat(final_url, ".tar.gz");
	//printf("Final URL: %s\n", final_url);
	
	//curl file	
	CURL *curl;
	CURLcode res;
	FILE *fp;
	curl = curl_easy_init();
	if (curl) {
		char file_path[100];
		sprintf(file_path, "%s%s%s", cache, pkg, ".tar.gz");
		fp = fopen(file_path, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, final_url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
	}

	const char* tarfile = strcat(cache, pkg);
	tar_open(&tarfile, tarfile, NULL, O_RDONLY, 0, 0);
	tar_extract_all(tarfile, "./");
	tar_close(tarfile);
	
	return 0;
}

void help() {
	printf("Usage: aureate -S <package>\n");
}

int main(int argc, char* argv[]) {
	//needs to have two arguments, else print help
	if (argc < 3) {
		help();
		return 1;
	}
	
	//download if args are valid, print help otherwise	
	if (strcmp(argv[1], "-S") == 0 && strcmp(argv[2], "") != 0) {
		download(argv);
	} else {
		help();
	}
	
	return 0;
}