/*
 * myflactag.c
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

#include "myflactag.h"

#include "myutils.h"
#include "sqlite.h"

static int write_flac_tag(char*, char*, char*);

int get_flac_tags(const char *file, int rowid)
{
	FLAC__StreamMetadata streaminfo;
	FLAC__bool ret = FLAC__metadata_get_streaminfo(file, &streaminfo);
	if (ret)
	{
		int sample_rate = streaminfo.data.stream_info.sample_rate;
		int channels = streaminfo.data.stream_info.channels;
		int bits_per_sample = streaminfo.data.stream_info.total_samples;
		int total_samples = streaminfo.data.stream_info.total_samples;
		printf("sample rate = %d\tchannels = %d\tbits_per_sample = %d\ttotal_samples = %d\n", sample_rate, channels, bits_per_sample, total_samples);
	}

	FLAC__StreamMetadata *tags;
	ret = FLAC__metadata_get_tags(file, &tags);
	if (ret)
	{
		int i;
		for (i = 0; i < tags->data.vorbis_comment.num_comments; i++)
		{
			unsigned char *tag = tags->data.vorbis_comment.comments[i].entry;
			unsigned char *ptag = &tag[0];
			char field[256];
			int i = 0;
			while (*ptag != '=')
				field[i++] = *ptag++;
			field[i] = '\0';
			ptag++;
			//printf("%s\t%s\n", field, ptag);
			set_tag_data(field, (char*) ptag, rowid);
		}
	}
	else
		printf("Couldn't get tag data\n");
	if (tags != NULL)
		FLAC__metadata_object_delete(tags);

	return 0;
}
int check_for_flac_image(const char *file)
{
	FLAC__StreamMetadata *pic;
	FLAC__bool ret;
	ret = FLAC__metadata_get_picture(file, &pic, -1, NULL, NULL, -1, -1, -1, -1);
	if (ret)
	{
		printf("Found a FLAC image\n");
		int len = pic->length;
		FLAC__byte *image = pic->data.picture.data;
		FILE *fp;
		char *image_file = "/tmp/weighty-tag-album-art.jpg";
		if ((fp = fopen(image_file, "w")) != NULL) {
			fwrite(image, 1, len, fp);
			fclose(fp);
		}
		FLAC__metadata_object_delete(pic);
	}
	return 0;
}
int write_flac_tag(char *file, char *field, char *tag)
{
	FLAC__Metadata_SimpleIterator *iter = FLAC__metadata_simple_iterator_new();
	FLAC__bool ok = FLAC__metadata_simple_iterator_init(iter, file, false, true);
	if (!ok) {
		printf("flac metadeta init failed\n");
		return -1;
	}
	FLAC__bool writable = FLAC__metadata_simple_iterator_is_writable(iter);
	if (!writable) {
		printf("file %s is read only\n", file);
		return -2;
	}
	do {
		FLAC__MetadataType type = FLAC__metadata_simple_iterator_get_block_type(iter);
		if (type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
			FLAC__StreamMetadata *data = FLAC__metadata_simple_iterator_get_block(iter);
			if (data != NULL) {
				char trfield[256];
				translate_field(field, trfield);
				int i = FLAC__metadata_object_vorbiscomment_find_entry_from(data, 0, trfield);
				//the tag doesn't exist yet, create a new tag field if tag exists
				if (i == -1) {
					printf("no match found for %s\n", trfield);
					printf("tag = %s\n", tag);
					if (tag == NULL) {
						printf("empty tag not saving\n");
						break;
					}
					FLAC__StreamMetadata_VorbisComment_Entry entry;
					ok = FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, trfield, tag);
					if (ok == false)
					{
						printf("new entry failed\n");
						return -1;
					}
					ok = FLAC__metadata_object_vorbiscomment_append_comment(data, entry, false);
					if (ok == false)
						printf("FLAC append comment failed\n");
					else {
						printf("FLAC append comment success\n");
						ok = FLAC__metadata_simple_iterator_set_block(iter, data, true);
						update_tag_data(file, trfield, tag);
					}
					FLAC__metadata_object_delete(data);
					break;
				}
				else {
					char *field_name;
					char *name;
					FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(data->data.vorbis_comment.comments[i], &field_name, &name);
					printf("FLAC tag %s = %s\n", field_name, name);
					//there is a new tag so replace the existing one if tag exists or delete it if it doesn't
					if (tag == NULL) {//delete the entry
						printf("deleting entry for %s\n", trfield);
						ok = FLAC__metadata_object_vorbiscomment_delete_comment(data, i);
						if (ok == false)
							printf("failed to delete entry\n");
						else {
							ok = FLAC__metadata_simple_iterator_set_block(iter, data, true);
							int ret = delete_tag_data(file, trfield);
							if (ret)
								printf("delete failed %d\n", ret);
						}
					}
					else if (strcmp(name, tag) != 0){//tag exists and has changed
						printf("changing %s to %s\n", name, tag);
						printf("changing entry\n");
						FLAC__StreamMetadata_VorbisComment_Entry entry;
						FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&entry, trfield, tag);
						ok = FLAC__metadata_object_vorbiscomment_replace_comment(data, entry, false, false);
						if (ok == false)
							printf("failed to replace comment\n");
						else {
							ok = FLAC__metadata_simple_iterator_set_block(iter, data, true);
							update_tag_data(file, trfield, tag);
						}
					}
					free(field_name);
					free(name);
					FLAC__metadata_object_delete(data);
					break;
				}

			}
			FLAC__metadata_object_delete(data);
		}
	} while (FLAC__metadata_simple_iterator_next(iter) == true);
	FLAC__metadata_simple_iterator_delete(iter);

	return 0;
}
int write_flac_tags(char *file, int num_tags, char **fields, char **tags)
{
	int i;
	for (i = 0; i < num_tags; i++) {
		write_flac_tag(file, fields[i], tags[i]);
	}
	return 0;
}
