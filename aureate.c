#include "config.h"
#include <curl/curl.h>
#include <dirent.h>
#include <fcntl.h>
#include <git2.h>
#include <json-c/json.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BASE_URL "https://aur.archlinux.org/"
//#define PARSE_URL BASE_URL "rpc/?v=5&type=info&arg="
#define SEARCH_URL BASE_URL "rpc/?v=5&type=search&arg=%s"
#define SUFFIX ".git"
#define URL_MAX 256
#ifndef PATH_MAX
#define PATH_MAX 256
#endif /* PATH_MAX */

//for API parsing
struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
static void pretty_print(const char *str);
static void search(const char *pkg);
static int download(int argc, char *argv[], int install);
static void help(const char *progname);

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
	if (size != 0 && nmemb > -1 / size)
		errx(1, "realloc: %s", strerror(ENOMEM));

	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;
	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL)
		err(1, "realloc");
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = '\0';

	return realsize;
}

void pretty_print(const char *str) {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

	int term_width = w.ws_col;
	int indent = 4;
	int wrap_width = term_width - indent;

	int cur_width = indent;
	int start_idx = 0;
	int wrapped = 0;

	//add the initial 4 spaces
	printf("    ");

	for (int i = 0; i < (int)strlen(str); ++i) {
		if (str[i] == ' ' && cur_width >= wrap_width) {
			printf("%.*s", i - start_idx, &str[start_idx]);
			start_idx = i + 1;
			cur_width = indent;
			wrapped = 1;
			//add 4 spaces for wrapped lines
			printf("\n    ");
		} else {
			//set wrapped to 0 for non-wrapped lines
			wrapped = 0;
		}
		++cur_width;
	}

	//only print extra 4 spaces if wrapped
	if (wrapped)
		printf("    %s\n", &str[start_idx]);
	else
		printf("%s\n", &str[start_idx]);
}

void search(const char *pkg) {
	CURL *curl;
	CURLcode res;
	char url[URL_MAX];
	struct MemoryStruct chunk = {
		.memory = NULL,
		.size = 0
	};

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl == NULL)
		errx(1, "curl_easy_init: failed");

	snprintf(url, URL_MAX, SEARCH_URL, pkg);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
		errx(1, "curl_easy_perform() failed: %s", curl_easy_strerror(res));

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

		printf(BOLDCYAN "aur" RESET "/" BOLDWHITE "%s" BOLDGREEN " %s" RESET "\n",
		       json_object_get_string(pkg_name), json_object_get_string(pkg_ver));

		//printf("    %s\n", json_object_get_string(pkg_desc));
		//printf("    %s\n");

		const char *desc_str = json_object_get_string(pkg_desc);
		pretty_print(desc_str);
	}
	json_object_put(parsed_json);

	curl_easy_cleanup(curl);
	free(chunk.memory);
	curl_global_cleanup();
}

/* Get path for cache. Also use $HOME var */
char *cache_path(const char *env_name)
{
        char *syscache = getenv(env_name); // ? getenv(env_name) : ".cache";   
        if (syscache == NULL) {
                char *home_dir = getenv("HOME");
                static char full_path[PATH_MAX];

                snprintf(full_path, PATH_MAX, "%s/.cache", home_dir);

                return full_path;
        }
        return syscache;
}

int download(int argc, char *argv[], int install) {
	//define vars
	char* syscache = cache_path("XDG_CACHE_HOME");
    //construct makepkg options
    char makepkg_opts[4];
    snprintf(makepkg_opts, 4, "-s%s", install == 1 ? "i" : "");

	for (int i = 2; i < argc; i++) {
		const char* pkg = argv[i];

		//check if XDG_CACHE_HOME exists - if not, create it
		struct stat st;
		if (stat(syscache, &st) == -1) {
			mkdir(syscache, 0700);
		}

		//construct aureate cache var
		char cache[PATH_MAX];
		snprintf(cache, PATH_MAX, "%s/aureate", syscache);


		//do the same thing as syscache with aureate cache
		if (stat(cache, &st) == -1) {
			mkdir(cache, 0700);
		}

		//construct clone URL
		char clone_url[URL_MAX];
		snprintf(clone_url, URL_MAX, "%s%s%s", BASE_URL, pkg, SUFFIX);

		//clone or pull the repo
		chdir(cache);
        git_libgit2_init();

		//if directory for package already exists, clone
		if (stat(pkg, &st) == -1) {
			printf(BLUE ":: " RESET "Fetching repo...");
			fflush(stdout);
			git_repository *repo = NULL;
			git_clone(&repo, clone_url, pkg, NULL);
			chdir(pkg);
			if (access("PKGBUILD", F_OK) != -1) {
				printf("done.\n"); fflush(stdout);
			} else {
				fprintf(stderr, "\n%s: " RED "Error: " RESET
				       "Package name invalid or you are not"
				       "connected to the internet.\n", argv[0]);
				fflush(stdout);
				//remove failed clone
				return 1;
			}
		//else, pull latest changes
		} else {
			printf(BLUE ":: " RESET "Fetching latest changes...");
			fflush(stdout);
			git_repository *repo;
			git_remote *remote;

			git_repository_open(&repo, pkg);
			git_remote_lookup(&remote, repo, "origin");
			git_remote_fetch(remote, NULL, NULL, "pull");
			git_remote_free(remote);
			git_repository_free(repo);

			printf("done.\n");
			fflush(stdout);
			chdir(pkg);
		}

		//handle -e
		if (i + 1 < argc && strcmp(argv[i + 1], "-e") == 0) {
			//skip -e as an arg so download() doesn't try to run it as a pkg
			char cmd[sizeof(EDITOR) + PATH_MAX + 1];
			i++;
			snprintf(cmd, sizeof(EDITOR) + PATH_MAX,
			         "%s %s/aureate/%s/PKGBUILD",
			         EDITOR, syscache, pkg);
			system(cmd);
		}

		//hand off the rest to pacman
		execlp("makepkg", "makepkg", makepkg_opts, NULL);
	}

	return 0;
}

void help(const char *progname) {
	printf(BLUE "AUReate" RESET ": AUR helper in the C programming language\n"
	"Usage: %s [arguments] <package>\n\n"
	"Arguments:\n"
	GREEN "  -S: "  RESET "Sync package from remote respository\n"
	GREEN "  -Ss: " RESET "Search for package in remote respository\n"
    GREEN "  -Sc: " RESET "Clean local cache\n"
    GREEN "  -b: "  RESET "Only build package\n"
	GREEN "  -R: "  RESET "Remove local package\n"
	GREEN "  -h, --help: " RESET "Print this help message\n"
	RESET "\nYou can find more information by running " BLUE "man aureate" RESET ".\n", progname);
}

void die(const char *progname, const char *message)
{
        fprintf(stderr, "%s: " RED "Error: " RESET "%s\n", progname, message);
        exit(1);
}

int main(int argc, char* argv[]) {
	//kill if root is executing
	if (geteuid() == 0)
                die(argv[0], "Do not run as root!");

	//require args
	if (argc < 2) {
		help(argv[0]);
		return 1;
	}


        /* Initialize flags */
        int Sflag = 0, Rflag = 0, sflag = 0, cflag = 0, bflag = 0, ch;

        /* now check options */
        while ((ch = getopt(argc, argv, "hSRbsc")) != -1) {
                switch (ch) {
                        case 'h':
                                help(argv[0]);
                                return 0;
                        case 'S':
                                Sflag = 1;
                                break;
                        case 'R':
                                Rflag = 1;
                                break;
                        case 's':
                                sflag = 1;
                                break;
                        case 'c':
                                cflag = 1;
                                break;
                        case 'b':
                                bflag = 1;
                                break;
                        default:
                                help(argv[0]);
                                return 1;
                }
        }

        /* run program */

        if (Sflag && cflag) {
                char path[PATH_MAX];
                snprintf(path, PATH_MAX, "%s/aureate", cache_path("XDG_CACHE_HOME"));

                printf("Are you sure you want to clean cache\nIn '%s' [y/n] ", path);

                char user_input = getchar();
                if ( user_input == 'y' || user_input == 'Y')
                        execlp("rm", "rm", "-rf", path, NULL);

                return 1;
        }

        if (Rflag && !Sflag) 
                return execlp(SUDO, SUDO, "pacman", "-R", argv[2], NULL); 
        else if (Sflag && sflag && !Rflag)
                search(argv[2]);
        else if (Sflag && !bflag && !Rflag) 
                return download(argc, argv, 1);
        else if (Sflag && bflag && !Rflag)
                return download(argc, argv, 0);
        else
                die(argv[0], "Unknown option!");



	return 0;
}
