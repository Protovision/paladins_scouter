#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CURL *curl;
struct curl_slist *players;

enum player_status {
	PLAYER_OFFLINE,
	PLAYER_LOBBY,
	PLAYER_MATCH_LOBBY
};

void load_player_list(const char *filename)
{
	FILE *f;
	char *p;
	char line[256];

	f = fopen(filename, "rb");
	players = 0;
	while (fgets(line, 256, f)) {
		p = strchr(line, '\n');
		if (p != 0) *p = 0;
		if (line[0] == 0) break;
		players = curl_slist_append(players, line);
	}
	fclose(f);
}

size_t write_string_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	size_t len;

	len = strlen((char*)userdata);
	memcpy((char*)userdata + len, ptr, size * nmemb);
	return size * nmemb;
}

int fetch_player_online(const char *player)
{
	char url[256];
	char response[512];

	response[0] = 0;
	strcpy(url, "https://theevie.club/api/player/status/PC/");
	strcat(url, player);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_perform(curl);
	if (strstr(response, "Offline") != 0) return 0;
	return 1;
}

int fetch_player_live_match(const char *player)
{
	char url[256];
	char response[512];

	response[0] = 0;
	strcpy(url, "http://api.paladins.guru/v2/player/live-match?player=");
	strcat(url, player);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_string_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_perform(curl);
	if (strstr(response, "\"success\":false") != 0) return 0;
	return 1;

}

enum player_status fetch_player_status(const char *player)
{
	enum player_status result;

	if (fetch_player_online(player)) {
		result = (fetch_player_live_match(player) ? 
				PLAYER_MATCH_LOBBY : PLAYER_LOBBY);
	} else {
		result = PLAYER_OFFLINE;
	}
	return result;
}

void quit(int sig)
{
	curl_slist_free_all(players);
	curl_easy_cleanup(curl);
	exit(0);
}

int main(int argc, char *argv[])
{
	struct curl_slist *p;
	int safe;
	enum player_status status;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0");
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	if (argc != 2) {
		printf("Paladins Player Tracker\n"
			"Arguments: PLAYERLISTFILE\n");
		return 0;
	}
	load_player_list(argv[1]);
	if (players == 0) return 1;
	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	//for (;;) {
		safe = 1;
		for (p = players; p != 0; p = p->next) {
			status = fetch_player_status(p->data);
			switch (status) {
			case PLAYER_OFFLINE:
				printf("%s: Offline\n", p->data);
				break;
			case PLAYER_LOBBY:
				printf("%s: In Lobby\n", p->data);
				safe = 0;
				break;
			case PLAYER_MATCH_LOBBY:
				printf("%s: In Match Lobby\n", p->data);
				break;
			}
		}
		printf(safe == 0 ? "\nUnsafe to queue\n" : "\nSafe to queue\n");
	//	system("sleep 5");
	//}
	quit(0);
	return 0;
}
