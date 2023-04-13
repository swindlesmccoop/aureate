#include "config.h"
#include <curl/curl.h>
#include <dirent.h>
#include <fcntl.h>
#include <git2.h>
#include <json-c/json.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BASE_URL "https://aur.archlinux.org/"
#define PARSE_URL BASE_URL "rpc/?v=5&type=info&arg="
#define SEARCH_URL BASE_URL "rpc/?v=5&type=search&arg=%s"
#define SUFFIX ".git"

int vasprintf(char **restrict strp, const char *restrict fmt, va_list ap) {
	va_list args;
	va_copy(args, ap);
	int size = vsnprintf(NULL, 0, fmt, args);

	/* if negative number is returned return error */
	if(size < 0)
		return -1;
	*strp = (char *)malloc(size + 1);
	if(*strp == NULL)
		return -1;

	va_end(args);
	return size = vsprintf(*strp, fmt, ap);
}

int asprintf(char **restrict strp, const char *restrict fmt, ...) {
	va_list args;
	int size = 0;
	va_start(args, fmt);
	size = vasprintf(strp, fmt, args);
	va_end(args);
	return size;
}


//for API parsing
struct MemoryStruct {
		char *memory;
		size_t size;
};

int eputs(const char *s) {
	return fprintf(stderr, "%s\n", s);
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
		size_t realsize = size * nmemb;
		struct MemoryStruct *mem = (struct MemoryStruct *)userp;
		mem->memory = realloc(mem->memory, mem->size + realsize + 1);
		if (mem->memory == NULL) {
				printf("Not enough memory (realloc returned NULL)\n");
				return 0;
		}
		memcpy(&(mem->memory[mem->size]), contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
		return realsize;
}

void search(const char *pkg) {
	CURL *curl;
	CURLcode res;
	struct MemoryStruct chunk;

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
			char *url;
			asprintf(&url, SEARCH_URL, pkg);
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

			res = curl_easy_perform(curl);

			if (res != CURLE_OK) {
					fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			} else {
					json_object *parsed_json, *results;
					parsed_json = json_tokener_parse(chunk.memory);
					json_object_object_get_ex(parsed_json, "results", &results);

					int n_results = json_object_array_length(results);

					for (int i = 0; i < n_results; i++) {
							json_object *pkg_obj = json_object_array_get_idx(results, i);
							json_object *pkg_name, *pkg_desc, *pkg_ver;

							json_object_object_get_ex(pkg_obj, "Name", &pkg_name);
							json_object_object_get_ex(pkg_obj, "Description", &pkg_desc);
							json_object_object_get_ex(pkg_obj, "Version", &pkg_ver);

							printf(BOLDCYAN "aur" RESET "/" BOLDWHITE "%s" BOLDGREEN " %s" RESET "\n", json_object_get_string(pkg_name), json_object_get_string(pkg_ver));
							printf("	%s\n", json_object_get_string(pkg_desc));
					}
			json_object_put(parsed_json);
			}
	curl_easy_cleanup(curl);
	free(url);
	}
	free(chunk.memory);
	curl_global_cleanup();
}

int download(int argc, char *argv[]) {
	//define vars
	 char* syscache = getenv("XDG_CACHE_HOME");
	if(syscache == NULL) 
		syscache = ".cache";

	for (int i = 2; i < argc; i++) {
		const char* pkg = argv[i];
	
		//check if XDG_CACHE_HOME exists - if not, create it
		struct stat st;
		if (stat(syscache, &st) == -1) {
			mkdir(syscache, 0700);
		}

		//construct aureate cache var
		char *cache;
		asprintf(&cache, "%s/%s", syscache, "aureate");
		
		//do the same thing as syscache with aureate cache
		if (stat(cache, &st) == -1) {
			mkdir(cache, 0700);
		}

		//construct clone URL
		char *clone_url = NULL;
		asprintf(&clone_url, "%s%s%s", BASE_URL, pkg, SUFFIX);

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
		free(clone_url);
		//hand off the rest to pacman
		system("makepkg -si");
	}

	return 0;
}

int uninstall(char *argv[]) {
	const char* pkg = argv[2];
	char *cmd;
	asprintf(&cmd, "%s pacman -R %s", SUDO, pkg);
	system(cmd);
	free(cmd);
	return 0;
}

void help() {
	printf(BLUE "AUReate" RESET ": AUR helper in the C programming language\n"
	"Usage: aureate [arguments] <package>\n\n"
	"Arguments:\n"
	GREEN "  -S: " RESET "Sync package from remote respository\n"
	GREEN "  -Ss: " RESET "Search for package in remote respository\n"
	GREEN "  -R: " RESET "Remove local package\n"
	GREEN "  -h, --help: " RESET "Print this help message\n"
	RESET "\nYou can find more information by running " BLUE "man aureate" RESET ".\n");
}

void flags(int argc, char* argv[]) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-S") == 0) { download(argc, argv); }
		else if (strcmp(argv[i], "-Ss") == 0) { search(argv[2]);; }
		else if (strcmp(argv[i], "-h") == 0) { help(); }
		else if (strcmp(argv[i], "--help") == 0) { help(); }
		else if (strcmp(argv[i], "-R") == 0) { uninstall(argv); } /* This prototype sucks, why do you
																   * pass an array to a function instead
																   * of the name of the package you want
																   * to uninstall? */
	}
}

int main(int argc, char* argv[]) {
	//kill if root is executing
	if (geteuid() == 0) {
		fprintf(stderr, RED "Error: do not run as root.\n" RESET);
		return 1;
	}
	
	//require args
	if (argc < 2) {
		help();
		return 1;
	}
	
	//run program
	flags(argc, argv);
	return 0;
}

