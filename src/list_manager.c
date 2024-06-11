/*
 * list_manager.c
 *
 *  Copyright: 2016-2017 David Morton
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

#include "list_manager.h"

#include "current_song.h"
#include "myutils.h"
#include "net_utils.h"
#include "songhash.h"
#include "three_lists.h"

static char pls[8000];//for holding the pls file for streams
static int pls_len = 0;

static size_t save_pls(void*, size_t, size_t, void*);
static void parse_pls(char*);

/*streamlist functions
 * ------------------------------------------------------
 */
void pls_check(char *url, char* mrl)
{
	char *purl = strrchr(url, '.');
	purl++;
	if (! strcmp(purl, "pls"))
	{
		pls_len = 0;
		CURL *curl;
		char string;
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_pls);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
        int success = curl_easy_perform(curl);
        if (success == 0)
        	parse_pls(mrl);
        else
        	printf("error getting pls\n");
        curl_easy_cleanup(curl);
	}
	else if (! strcmp(purl, "m3u"))
	{
		memcpy(mrl, url, strlen(url) + 1);
		return;


		printf("getting m3u\n");
		pls_len = 0;
		CURL *curl;
		char string;
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, save_pls);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &string);
        int success = curl_easy_perform(curl);
        printf("get m3u mrl = %d\n", success);
        if (success == 0)
        {
        	//for the muthafukas who use \r in m3u's >:(
        	int j = 0;
        	int i;
        	for (i = 0; i < pls_len; i++)
        	{
        		if (pls[i] == '\n')
        		{
        			pls[i] = '0';
        			if (pls[i + 1] == '\r')
        			{
        				i++;
        				pls[i] = '0';
        			}
        			if (j != 0)
        				break;
        			if (pls[i+1] != '#')
        				j = i+1;
        		}
        		else if (pls[i] == '\r')
        		{
        			pls[i] = '0';
        			if (pls[i + 1] == '\n')
        			{
        				i++;
        				pls[i] = '0';
        			}
        			if (j != 0)
        				break;
        			if (pls[i+1] != '#')
        				j = i+1;
        		}
        	}
        	memcpy(mrl, &pls[j], strlen(&pls[j]) + 1);
			print_data(mrl, strlen(mrl) + 1);
        }
        else
        	printf("error getting m3u\n");
	}
	else
		memcpy(mrl, url, strlen(url) + 1);
}
size_t save_pls(void *buffer, size_t size, size_t nmemb, void *user)
{
	memcpy(&pls[pls_len], buffer, nmemb);
	pls_len += nmemb;
    return nmemb;
}
void parse_pls(char *mrl)
{
	printf("parse %s\n", mrl);
	char *p = &pls[0];
	int i;
	for (i = 0; i < pls_len; i++)
	{
		if (strncmp(p, "File1=", 6) == 0)
		{
			p += 6;
			while (*p != '\n')
			{
				*mrl++ = *p++;
			}
			*mrl++ = '\0';
			break;
		}
		else
			p++;
	}
	pls[0] = '0';
}
