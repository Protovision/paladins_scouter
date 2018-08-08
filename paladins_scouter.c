#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0"

enum paladins_player_status {
	PALADINS_PLAYER_OFFLINE,
	PALADINS_PLAYER_IN_LOBBY,
	PALADINS_PLAYER_IN_MATCH	
};

struct curl_slist *load_player_list(const char *filename)
{
	FILE *f;
	char *p;
	struct curl_slist *players;
	char line[256];

	f = fopen(filename, "r");
	if (f == 0) return 0;
	players = 0;
	while (fgets(line, 256, f)) {
		p = strchr(line, '\n');
		if (p != 0) *p = 0;
		if (line[0] == 0) continue;
		players = curl_slist_append(players, line);
	}
	fclose(f);
	return players;
}

size_t string_write(char *ptr, 
			size_t size, 
			size_t nmemb, 
			void *string)
{
	size_t slen, tlen;

	slen = strlen((char*)string);
	tlen = size * nmemb;
	memcpy((char*)string + slen, ptr, tlen);
	return tlen;
}

int fetch_paladins_player_online_status(CURL *curl, const char *player)
{
	char url[256];
	char response[512];

	response[0] = 0;
	strcpy(url, "https://theevie.club/api/player/status/PC/");
	strcat(url, player);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_perform(curl);
	return !(strstr(response, "Offline") != 0);
}

int fetch_paladins_player_in_match_status(CURL *curl, const char *player)
{
	char url[256];
	char response[512];

	response[0] = 0;
	strcpy(url, "http://api.paladins.guru/v2/player/live-match?region=0&player=");
	strcat(url, player);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
	curl_easy_perform(curl);
	return !(strstr(response, "\"success\":false") != 0);

}

enum paladins_player_status fetch_paladins_player_status(CURL *curl, 
						const char *player)
{
	enum paladins_player_status result;

	if (fetch_paladins_player_online_status(curl, player)) {
		if (fetch_paladins_player_in_match_status(curl, player)) {
			result = PALADINS_PLAYER_IN_MATCH;
		} else {
			result = PALADINS_PLAYER_IN_LOBBY;
		}
	} else {
		result = PALADINS_PLAYER_OFFLINE;
	}
	return result;
}

int main(int argc, char *argv[])
{
	CURL *curl;
	struct curl_slist *p, *players;
	int safe;
	enum paladins_player_status status;

	if (argc != 2) {
		printf("Paladins Player Tracker\n"
			"Arguments: PLAYERLISTFILE\n");
		return 0;
	}
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	players = load_player_list(argv[1]);
	if (players == 0) {
		printf("Failed to load: %s\n", argv[1]);
		goto exit_main;
	}
	safe = 1;
	for (p = players; p != 0; p = p->next) {
		status = fetch_paladins_player_status(curl, p->data);
		switch (status) {
		case PALADINS_PLAYER_OFFLINE:
			printf("%s: Offline\n", p->data);
			break;
		case PALADINS_PLAYER_IN_LOBBY:
			printf("%s: In Lobby\n", p->data);
			safe = 0;
			break;
		case PALADINS_PLAYER_IN_MATCH:
			printf("%s: In Match Lobby\n", p->data);
			break;
		}
	}
	printf(safe == 0 ? "\nUnsafe to queue\n" : "\nSafe to queue\n");
exit_main:
	curl_slist_free_all(players);
	curl_easy_cleanup(curl);
	return 0;
}
