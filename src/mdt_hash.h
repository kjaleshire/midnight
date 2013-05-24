
/* hash structure */
typedef struct http_header {
	char* key;
	char* value;
	UT_hash_handle hh;
} http_header;