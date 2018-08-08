#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USERAGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0"

enum paladins_player_status {
	PALADINS_PLAYER_OFFLINE,
	PALADINS_PLAYER_IN_LOBBY,
	PALADINS_PLAYER_IN_MATCH	
};

const char *paladins_player_status_strings[] = {
	"Offline",
	"In Lobby",
	"In Match"
};

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
	char player[256], *p;

	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	while (fgets(player, sizeof(player), stdin)) {
		p = strchr(player, '\n');
		if (p) *p = 0;
		if (player[0] == 0) continue;
		printf("%s: %s\n", player, paladins_player_status_strings[
					fetch_paladins_player_status(curl, 
					player)]);
	}
	curl_easy_cleanup(curl);
	return 0;
}
