#include <curl/curl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <csignal>
using namespace std;

CURL* curl;

#define USERAGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:61.0) Gecko/20100101 Firefox/61.0"

size_t sstream_write(char *ptr, 
			size_t size, 
			size_t nmemb, 
			void *p)
{
	size_t tlen;
	stringstream *stream;

	stream = (stringstream*)p;
	tlen = size * nmemb;
	stream->write(ptr, tlen);
	return tlen;
}

bool fetch_live_match_status(CURL *curl, const std::string& player)
{
	string url, response;
	stringstream response_stream;

	url = "http://api.paladins.guru/v2/player/live-match?region=0&player=";
	url += player;
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sstream_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_stream);
	curl_easy_perform(curl);
	response = response_stream.str();
	return response.find("\"success\":false") != string::npos;
}

void quit(int sig)
{
	curl_easy_cleanup(curl);
}

int main(int argc, char *argv[])
{
	bool status;
	unsigned int i;
	string player;
	chrono::seconds delay;

	if (argc != 3) {
		cout << 
			"Paladins Queue Dodging Tool." << endl <<
			"Usage: " << argv[0] << "PLAYER QUERY_DELAY" << endl;
		return 1;
	}
	signal(SIGINT, quit);
	signal(SIGTERM, quit);
	player = argv[1];
	delay = chrono::seconds(stoi(argv[2]));	
	curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	i = 0;
	for (;;) {
		status = fetch_live_match_status(curl, player);
		cout << setw(8) << left << i << player << " " << 
				(status ? "in live match." :
				"not in live match.") << endl;
		this_thread::sleep_for(delay);
		++i;
	}
	return 0;
}
