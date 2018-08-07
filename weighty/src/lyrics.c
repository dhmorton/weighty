/*
 * lyrics.c
 *
*  Copyright: 2011-2017 David Morton
 *
 *   This file is part of weighty.
 *
 *   Weighty is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Weighty is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with weighty.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LyricWikiBinding.nsmap"
#include "mylyrics.h"
#include "myutils.h"
#include "net_utils.h"
#include "soapH.h"

static char raw_lyrics[300000];
static int raw_len = 0;

static size_t save_data(void*, size_t, size_t, void*);

int get_lyrics(char *art, char *tit)
{
    CURL *curl;
    char *endpoint = "http://lyrics.wikia.com/server.php";
    char *action = "urn:LyricWiki#getSong";

    struct soap *soap = soap_new();
    struct ns1__getSongResponse result;

    if (soap_call_ns1__getSong(soap, endpoint, action, art, tit, &result) == SOAP_OK)
    {
    	raw_len = 0;
        int success;
        char string;
        //printf("%s\n", result.return_->url);
        //all this because lyrics.wikia added a /wiki/ subdir in the url reference that doesn't show up in their SOAP return
        char *purl = result.return_->url;
        int slashcount = 0;
        while (slashcount < 3)
        {
        	if(*purl == '/')
        		slashcount++;
        	purl++;
        }
        //printf("purl = %s\n", purl);
        char newurl[strlen(purl) + 30];
        snprintf(newurl, 30 + strlen(purl), "%s%s", "http://lyrics.wikia.com/wiki/", purl);
        //printf("%s\n", newurl);
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, newurl);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
        success = curl_easy_perform(curl);
        if (success == 0)
        	parse_lyrics();
        else
        	printf("lyrics failed\n");
        curl_easy_cleanup(curl);
    }
    else
        printf("lyrics failed\n");

    return 0;
}
size_t save_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
	memcpy(&raw_lyrics[raw_len], buffer, nmemb);
	raw_len += nmemb;
    return nmemb;
}
void parse_lyrics()
{
	char lyrics[24000];
	memset(lyrics, 0, 24000);
    char *p = &raw_lyrics[0];
	int store_data = 0;

	int lc = 0;
	int i;
    for (i = 0; i < raw_len; i++)
    {
        if (! strncmp(p, "lyricbox'>", 10))
            store_data = 2;
        if (store_data == 1)
        {
            if (! strncmp(p, "</div>", 5))
                store_data = 2;
        }
        else if (store_data == 2)
        {
            if (! strncmp(p, "<!--", 4))
                store_data = 0;
            else
            {
                if (*p == '&')
                {
                    p++; i++;//past the &
                    if (strncmp(p, "amp", 3) == 0)
                    {
                    	lyrics[lc++] = '&';
                    	p += 3;
                    	i += 3;
                    }
                    else
                    {
						p++; i++;//past the #
						char num[4];
						memset(num, 0, 4);
						int j = 0;
						while (*p != ';')
						{
							num[j++] = *p++;
							i++;
						}
						num[j] = '\0';
						int ascii = atoi(num);
						lyrics[lc++] = (char)ascii;
                    }
                }
                else if (*p == '<')
                {
                    p += 5; i += 5;
                    lyrics[lc++] = '\n';
                }
            }
        }
        p++;
    }
    if (lc > 0)
    {
    	lyrics[lc++] = '\0';
    	char com[lc + 1];
    	com[0] = 'X';
    	memcpy(&com[1], lyrics, lc);
    	send_command(com, lc + 1);
    }
}
