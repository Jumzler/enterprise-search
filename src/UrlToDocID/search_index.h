
#ifndef _SEARCH_URLTODOCID_INDEX_
#define _SEARCH_URLTODOCID_INDEX_


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

typedef struct
{
    FILE	*indexf, *dbf;
    off_t	index_size;
    off_t	one_unit;
} urldocid_data;


// init:
urldocid_data* urldocid_search_init(char *index_filename, const char *db_filename);
// exit:
void urldocid_search_exit(urldocid_data *data);

// search:
unsigned int urldocid_search_index(urldocid_data *data, unsigned char *sha1);

char getDocIDFromUrl(urldocid_data *data, char *url, unsigned int *DocID);


#endif	// _SEARCH_URLTODOCID_INDEX_
