/*
 * myid3tag.c
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

#include "myid3tag.h"

#include "myutils.h"
#include "sqlite.h"

static char *genre[] =
{	"Blues",
	"Classic Rock",
	"Country",
	"Dance",
	"Disco",
	"Funk",
	"Grunge",
	"Hip-Hop",
	"Jazz",
	"Metal",
	"New Age",
	"Oldies",
	"Other",
	"Pop",
	"R&B",
	"Rap",
	"Reggae",
	"Rock",
	"Techno",
	"Industrial",
	"Alternative",
	"Ska",
	"Death Metal",
	"Pranks",
	"Soundtrack",
	"Euro-Techno",
	"Ambient",
	"Trip-Hop",
	"Vocal",
	"Jazz+Funk",
	"Fusion",
	"Trance",
	"Classical",
	"Instrumental",
	"Acid",
	"Acid",
	"Game",
	"Sound Clip",
	"Gospel",
	"Noise",
	"Alternative Rock",
	"Bass",
	"Soul",
	"Punk",
	"Space",
	"Meditative",
	"Instrumental Pop",
	"Instrumental Rock",
	"Ethnic",
	"Gothic",
	"Darkwave",
	"Techno-Industrial",
	"Electronic",
	"Pop-Folk",
	"Eurodance",
	"Dream",
	"Southern Rock",
	"Comedy",
	"Comedy",
	"Gangsta",
	"Top 40",
	"Christian Rap",
	"Pop/Funk",
	"Jungle",
	"Native US",
	"Cabaret",
	"New Wave",
	"Psychadelic",
	"Rave",
	"Showtunes",
	"Trailer",
	"Lo-Fi",
	"Tribal",
	"Acid Punk",
	"Acid Jazz",
	"Polka",
	"Retro",
	"Musical",
	"Rock & Roll",
	"Hard Rock",
	"Folk",
	"Folk-Rock",
	"National Folk",
	"Swing",
	"Fast Fusion",
	"Bebob",
	"Latin",
	"Revival",
	"Celtic",
	"Bluegrass",
	"Avantgarde",
	"Gothic Rock",
	"Progressive Rock",
	"Psychedelic Rock",
	"Symphonic Rock",
	"Slow Rock",
	"Big Band",
	"Chorus",
	"Easy Listening",
	"Acoustic",
	"Humour",
	"Speech",
	"Chanson",
	"Opera",
	"Chamber Music",
	"Sonata",
	"Symphony",
	"Booty Bass",
	"Primus",
	"Porn Groove",
	"Satire",
	"Slow Jam",
	"Club",
	"Tango",
	"Samba",
	"Folklore",
	"Ballad",
	"Power Ballad",
	"Rhythmic Soul",
	"Freestyle",
	"Duet",
	"Punk Rock",
	"Drum Solo",
	"Acapella",
	"Euro-House",
	"Dance Hall",
	"Goa",
	"Drum & Bass",
	"Club - House",
	"Hardcore",
	"Terror",
	"Indie",
	"BritPop",
	"Negerpunk",
	"Polsk Punk",
	"Beat",
	"Christian Gangsta Rap",
	"Heavy Metal",
	"Black Metal",
	"Crossover",
	"Contemporary Christian",
	"Christian Rock",
	"Merengue",
	"Salsa",
	"Thrash Metal",
	"Anime",
	"JPop",
	"Synthpop",
	"World Music",
	"Bongo Flava",
	"Nerdcore"
};
int get_mp3_tags(const char *file, int rowid)
{
	struct id3_tag *tag;
	struct id3_file *id3;
	id3 = id3_file_open(file, ID3_FILE_MODE_READONLY);
	if (id3 == NULL)
		return -1;
	tag = id3_file_tag(id3);
	if (tag)
	{
		struct id3_frame *frame;
		union id3_field *field;
		int j = 0;
		id3_latin1_t *old_tag;
		char *id;
		const id3_ucs4_t *string;
//		const id3_byte_t *bin;
//		id3_length_t *len;
//		char *binfile, *pext;
//		FILE *fp;
		while ((frame = id3_tag_findframe(tag, NULL, j++)) != NULL)
		{
			int n, m;
			for (n = 0; n < frame->nfields; n++)
			{
//				printf("\t***FRAME %d, %d fields\t", j, frame->nfields);
				field = &frame->fields[n];
				switch (field->type)
				{
				case ID3_FIELD_TYPE_TEXTENCODING:
//					printf("TEXTENCODING\n");
				case ID3_FIELD_TYPE_LATIN1FULL:
//					printf("LATIN1FULL\n");
				case ID3_FIELD_TYPE_LATIN1LIST:
//					printf("LATIN1LIST\n");
				case ID3_FIELD_TYPE_STRING:
//					printf("STRING\n");
				case ID3_FIELD_TYPE_STRINGFULL:
//					printf("STRINGFULL\n");
				case ID3_FIELD_TYPE_LATIN1:
//					printf("LATIN1\n");
				case ID3_FIELD_TYPE_STRINGLIST:
//					printf("STRINGLIST\n");
					for (m = 0; m < id3_field_getnstrings(field); m++)
					{
						string = id3_field_getstrings(field, m);
						if (string == NULL)
							return -1;
						old_tag = id3_ucs4_latin1duplicate(string);
						if (old_tag == NULL)
							return -1;
//						printf("tag: %s\t", old_tag);
						id = frame->id;
						if (strcmp("TCON", id) == 0)
						{
							if (isdigit(old_tag[0]))
							{
								int g = atoi((char*) old_tag);
								printf("\t%s = %s\n", id, genre[g]);
								set_tag_data(id, genre[g], rowid);
							}
							else
							{
								printf("\t%s = %s\n", id, old_tag);
								set_tag_data(id, (char*) old_tag, rowid);
							}
						}
						else
						{
							printf("\t%s = %s\n", id, old_tag);
							set_tag_data(id, (char*) old_tag, rowid);
						}
						free(old_tag);
					}
					break;
				case ID3_FIELD_TYPE_BINARYDATA:
	/*				len = &field->binary.length;
					if (*len >= 4096)
					{
						printf("Found image: size = %d\n",(int) *len);
						bin = id3_field_getbinarydata(field, len);
						binfile = malloc(strlen(file) + 4);
						strcpy(binfile, file);
						pext = strrchr(binfile, '.');
						if (pext == NULL)
							break;
						*++pext = 'j';
						*++pext = 'p';
						*++pext = 'g';
						*++pext = '\0';

						if ((fp = fopen(binfile, "w")) == NULL)
							printf("can't open %s for writing\n", binfile);
						else
						{
							fwrite(bin, sizeof(*len), *len, fp);
							fclose(fp);
						}
					}*/
					break;
				default:
					break;
				}
			}
		}
	}
	else
		printf("no tag\n");
	id3_file_close(id3);
	return 0;
}
int write_mp3_tag(const char *file, char *match_field, char *new_tag)
{
	printf("write: %s %s\n", match_field, new_tag);
	struct id3_tag *tag;
	struct id3_file *id3;
	id3 = id3_file_open(file, ID3_FILE_MODE_READWRITE);
	if (id3 == NULL)
	{
		printf("Can't write to %s\n", file);
		return -1;
	}
	tag = id3_file_tag(id3);
	if (tag)
	{
		struct id3_frame *frame;
		union id3_field *field;
		int j = 0;
		int match = 0;
		id3_latin1_t *old_tag = NULL;
		char *id;
		while ((frame = id3_tag_findframe(tag, NULL, j++)) != NULL)
		{
			int m;
			printf("\t***FRAME %d, %d fields\n", j, frame->nfields);
			field = &frame->fields[1];
			printf("field type %d\n", field->type);
			switch (field->type)
			{
			case ID3_FIELD_TYPE_TEXTENCODING:
			case ID3_FIELD_TYPE_LATIN1:
			case ID3_FIELD_TYPE_LATIN1FULL:
			case ID3_FIELD_TYPE_LATIN1LIST:
			case ID3_FIELD_TYPE_STRING:
			case ID3_FIELD_TYPE_STRINGFULL:
			case ID3_FIELD_TYPE_STRINGLIST:
			case ID3_FIELD_TYPE_LANGUAGE:
			case ID3_FIELD_TYPE_DATE:
				id = frame->id;
				printf("id = %s\n", id);
				char *back_field = malloc(sizeof(char) * 5);
				back_translate(match_field, &back_field);
				printf("match_field = %s\tback_field = %s\n", match_field, back_field);
				if (! strcasecmp(id, match_field) || ! strcasecmp(id, back_field))
				{
					printf("id = %s\n", id);
					for (m = 0; m < id3_field_getnstrings(field); m++)
					{
						const id3_ucs4_t *string = id3_field_getstrings(field, m);
/*						if (string == NULL)//frame but no tag, just write it and return
						{
							const id3_ucs4_t *ptr = id3_latin1_ucs4duplicate((id3_latin1_t const*) new_tag);
							int ret = id3_field_addstring(field, ptr);
							printf("fail write = %d\n", ret);
							if (ret == 0)
								update_tag_data(file, id, new_tag);
							id3_file_update(id3);
							id3_file_close(id3);
							return 0;
						}*/
						if (string != NULL)
						{
							old_tag = id3_ucs4_latin1duplicate(string);
							//special case for GENRE tags which have an unweildy numeric list
							if (strcmp("TCON", id) == 0)
							{
								if (new_tag == NULL) {
									int fail = id3_field_setstrings(field, m, NULL);
									if (fail)
										printf("fail write = %d\n", fail);
									if (fail == 0)
										update_tag_data(file, id, new_tag);
								}
								else if (isdigit(old_tag[0]))
								{
									int g = atoi((char*) old_tag);
									printf("g = %d %s\n", g, genre[g]);
									const id3_ucs4_t *genre = id3_genre_name(id3_genre_index((unsigned int)g));
									printf("genre = %s\n", (char*) genre);
								}
								else if (strcmp((char*)old_tag, new_tag))//they differ
								{
									printf("set %s to %s\n", id, new_tag);
									int fail = id3_field_setstrings(field, m, NULL);
									if (fail)
										printf("fail clear = %d\n", fail);
									const id3_ucs4_t *ptr = id3_latin1_ucs4duplicate((id3_latin1_t const*) new_tag);
									fail = id3_field_addstring(field, ptr);
									if (fail)
										printf("fail write = %d\n", fail);
									if (fail == 0)
										update_tag_data(file, id, new_tag);
								}
							}
							else
							{
								if (field->type == ID3_FIELD_TYPE_TEXTENCODING)
									printf("textencoding\n");
								else if (field->type == ID3_FIELD_TYPE_LATIN1)
									printf("latin1\n");
								else if (field->type == ID3_FIELD_TYPE_LATIN1FULL)
									printf("latinfull\n");
								else if (field->type == ID3_FIELD_TYPE_LATIN1LIST)
									printf("latinlist\n");
								else if (field->type == ID3_FIELD_TYPE_STRING)
									printf("string\n");
								else if (field->type == ID3_FIELD_TYPE_STRINGFULL)
									printf("stringfull\n");
								else if (field->type == ID3_FIELD_TYPE_STRINGLIST)
								{
									printf("stringlist\n");
									if (new_tag == NULL) {
										int fail = id3_field_setstrings(field, m, NULL);
										if (fail)
											printf("fail clear = %d\n", fail);
										else
											update_tag_data(file, id, NULL);
									}
									else if (strcmp((char*)old_tag, new_tag))//they differ
									{
										printf("set %s to %s\n", id, new_tag);
										int fail = id3_field_setstrings(field, m, NULL);
										if (fail)
											printf("fail clear = %d\n", fail);
										const id3_ucs4_t *ptr = id3_latin1_ucs4duplicate((id3_latin1_t const*) new_tag);
										fail = id3_field_addstring(field, ptr);
										if (fail)
											printf("fail write = %d\n", fail);
										if (fail == 0)
											update_tag_data(file, id, new_tag);
									}
								}
								else if (field->type == ID3_FIELD_TYPE_LANGUAGE)
									printf("language\n");
								else if (field->type == ID3_FIELD_TYPE_DATE)
									printf("date\n");
							}
						}
					}
					match++;
					if (old_tag != NULL)
						free(old_tag);
					break;
				default:
					break;
				}
				free(back_field);
			}
		}
		//no match, need to make a new frame
		if ((match == 0) && (new_tag != NULL))
		{
			char *back_field = malloc(sizeof(char) * 5);
			back_translate(match_field, &back_field);
			printf("no match for %s\n", match_field);
			printf("creating frame %s with tag %s\n", back_field, new_tag);
			struct id3_frame *new_frame;
			if (! (new_frame = id3_frame_new(back_field)))
			{
				printf("id3_frame_new FAIL\n");
				return -1;
			}
			if (id3_tag_attachframe(tag, new_frame))
			{
				printf("id3 tag attachframe FAIL\n");
				return -1;
			}
			const id3_ucs4_t *ptag = id3_latin1_ucs4duplicate((id3_latin1_t const*) new_tag);
			int fail = id3_field_addstring(&new_frame->fields[1], ptag);
			if (fail)
			{
				printf("id3 field addstring FAIL\n");
				return -1;
			}
			printf("SUCCESS\n");
			update_tag_data(file, match_field, new_tag);
			free(back_field);
		}
		int ret = id3_file_update(id3);
		printf("update return = %d\n", ret);
	}
	else
	{
		printf("no tag, creating\n");
		struct id3_tag *new_id3_tag;
		new_id3_tag = id3_tag_new();
		if (new_id3_tag == NULL)
		{
			printf("making new tag failed\n");
			return -1;
		}
	}
	id3_file_close(id3);
	return 0;
}
int write_mp3_tags(char *file, int num_tags, char **fields, char **tags)
{
	printf("write mp3 tags\n");
	int i;
	for (i = 0; i < num_tags; i++)
	{
		printf("%s\t=> %s\n", fields[i], tags[i]);
		write_mp3_tag(file, fields[i], tags[i]);
	}
	return 0;
}
int clear_mp3_tag(const char *file, char *match_field)
{
	printf("clear: %s %s\n", file, match_field);
	struct id3_tag *tag;
	struct id3_file *id3;
	id3 = id3_file_open(file, ID3_FILE_MODE_READWRITE);
	if (id3 == NULL)
	{
		printf("Can't write to %s\n", file);
		return -1;
	}
	tag = id3_file_tag(id3);
	if (tag)
	{
		struct id3_frame *frame;
		union id3_field *field;
		int j = 0;
		int match = 0;
		char *id;
		while ((frame = id3_tag_findframe(tag, NULL, j++)) != NULL)
		{
			int m;
//			printf("\t***FRAME %d, %d fields\n", j, frame->nfields);
			field = &frame->fields[1];
			switch (field->type)
			{
			case ID3_FIELD_TYPE_TEXTENCODING:
			case ID3_FIELD_TYPE_LATIN1:
			case ID3_FIELD_TYPE_LATIN1FULL:
			case ID3_FIELD_TYPE_LATIN1LIST:
			case ID3_FIELD_TYPE_STRING:
			case ID3_FIELD_TYPE_STRINGFULL:
			case ID3_FIELD_TYPE_STRINGLIST:
			case ID3_FIELD_TYPE_LANGUAGE:
			case ID3_FIELD_TYPE_DATE:
				id = frame->id;
				if (! strcasecmp(id, match_field))
				{
					printf("id = %s\n", id);
					for (m = 0; m < id3_field_getnstrings(field); m++)
					{
						const id3_ucs4_t *string = id3_field_getstrings(field, m);
						if (string != NULL)
						{
							if (strcmp("TCON", id) == 0)
							{
								//deal with v1 genre number
							}
							else
							{
								if (field->type == ID3_FIELD_TYPE_TEXTENCODING)
									printf("textencoding\n");
								else if (field->type == ID3_FIELD_TYPE_LATIN1)
									printf("latin1\n");
								else if (field->type == ID3_FIELD_TYPE_LATIN1FULL)
									printf("latinfull\n");
								else if (field->type == ID3_FIELD_TYPE_LATIN1LIST)
									printf("latinlist\n");
								else if (field->type == ID3_FIELD_TYPE_STRING)
									printf("string\n");
								else if (field->type == ID3_FIELD_TYPE_STRINGFULL)
									printf("stringfull\n");
								else if (field->type == ID3_FIELD_TYPE_STRINGLIST)
								{
									printf("stringlist\n");
									int fail = id3_field_setstrings(field, 0, NULL);
									printf("fail clear = %d\n", fail);
									if (fail == 0)
										delete_tag_data(file, id);
								}
								else if (field->type == ID3_FIELD_TYPE_LANGUAGE)
									printf("language\n");
								else if (field->type == ID3_FIELD_TYPE_DATE)
									printf("date\n");
							}
						}
					}
					match++;
					break;
				default:
					break;
				}
			}
		}
		id3_file_update(id3);
	}
	else
	{
		printf("no tag, creating\n");
		struct id3_tag *new_id3_tag;
		new_id3_tag = id3_tag_new();
		if (new_id3_tag == NULL)
		{
			printf("making new tag failed\n");
			return -1;
		}
	}
	id3_file_close(id3);
	return 0;
}
int check_for_mp3_image(const char *file)
{
	char *image_file = "/tmp/weighty-tag-album-art.jpg";

	struct id3_tag *tag;
	struct id3_file *id3;
	id3 = id3_file_open(file, ID3_FILE_MODE_READONLY);
	tag = id3_file_tag(id3);
	if (tag)
	{
		struct id3_frame *frame;
		union id3_field *field;
		int j = 0;
		id3_length_t *len;
		FILE *fp;
		//const id3_byte_t *bin;
		unsigned char const *bin;//wtf?
		while ((frame = id3_tag_findframe(tag, NULL, j++)) != NULL)
		{
//			printf("\t***FRAME %d, %d fields\n", j, frame->nfields);
			int k;
			for(k = 0; k < frame->nfields; k++)
			{
				field = &frame->fields[k];
				switch (field->type)
				{
				case ID3_FIELD_TYPE_BINARYDATA:
					len = &field->binary.length;
					if (*len >= 4096)
					{
						printf("%s\n", file);
						printf("\tFound image: size = %d\n",(int) *len);
						bin = id3_field_getbinarydata(field, len);
						if (*bin == 0)//empty tag
							printf("empty tag\n");
						else if ((fp = fopen(image_file, "w")) == NULL)
							printf("can't open %s for writing\n", image_file);
						else
						{
							printf("\tbin = %u\n",*bin);
							fwrite(bin, 1, *len, fp);
							fclose(fp);
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}
	id3_file_close(id3);
	return 0;
}
