#include <curl/curl.h>
#include <string.h>

#define PATH "\\$HOME/.local/"

int download(char *argv[]) {
	//get URL
	const char* pkg = argv[2];
	const char* baseurl = "https://aur.archlinux.org/cgit/aur.git/snapshot/";
	char url[100] = "";
	strcat(url, baseurl);
	strcat(url, pkg);
	char final_url[100] = "";
	strcat(final_url, url);
	strcat(final_url, ".tar.gz");
	printf("Final URL: %s\n", final_url);
	
	//curl file	
	CURL *curl;
	CURLcode res;
	FILE *fp;
	curl = curl_easy_init();
	if (curl) {
		char file_path[100];
		sprintf(file_path, "%s%s", PATH, pkg);
		fp = fopen(file_path, "wb");
		curl_easy_setopt(curl, CURLOPT_URL, final_url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);
		return (int)res;
	}
	return 1;
}

int help() {
	printf("Usage: aureate -S <package>\n");
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		help();
		return 1;
	}
	
	if (strcmp(argv[1], "-S") == 0 && strcmp(argv[2], "") != 0) {
		download(argv);
	} else {
		help();
	}
	
	return 0;
}