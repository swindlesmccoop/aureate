#include "config.h"
#include <curl/curl.h>
#include <dirent.h>
#include <fcntl.h>
#include <git2.h>
#include <json-c/json.h>
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

//for API parsing
struct MemoryStruct {
		char *memory;
		size_t size;
};

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
			char url[strlen(SEARCH_URL)+strlen(pkg)+1];
			snprintf(url, sizeof(url), SEARCH_URL, pkg);
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
							printf("    %s\n", json_object_get_string(pkg_desc));
					}
			json_object_put(parsed_json);
			}
	curl_easy_cleanup(curl);
	}
	free(chunk.memory);
	curl_global_cleanup();
}

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
		/* -S  */ if (strcmp(argv[i], "-S") == 0) { download(argv); }
		/* -Ss */ else if (strcmp(argv[i], "-Ss") == 0) { search(argv[2]);; }
		/* -S  */ if (strcmp(argv[i], "-h") == 0) { help(); }
		/* -S  */ if (strcmp(argv[i], "--help") == 0) { help(); }
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
	if (argc < 2) {
		help();
		return 1;
	}
	
	//run program
	flags(argc, argv);
	return 0;
}

