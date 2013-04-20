/**
 * Copyright (c) 2005 Zed A. Shaw
 * You can redistribute it and/or modify it under the same terms as Ruby.
 */

#include "midnight.h"

#define LEN(AT, FPC) (FPC - buffer - parser->AT)
#define MARK(M,FPC) (parser->M = (FPC) - buffer)
#define PTR_TO(F) (buffer + parser->F)

/** Machine **/

%%{
	machine http_parser;

	action mark {
		MARK(mark, fpc);
	}

	action start_field {
		MARK(field_start, fpc);
	}

	action write_field {
		parser->field_len = LEN(field_start, fpc);
	}

	action start_value {
		MARK(mark, fpc);
	}

	action write_value {
		if (parser->http_field != NULL) {
		  parser->http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action request_method {
		if (parser->request_method != NULL) {
			parser->request_method(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action request_uri {
		if (parser->request_uri != NULL) {
			parser->request_uri(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action fragment {
		if (parser->fragment != NULL) {
			parser->fragment(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action start_query {
		MARK(query_start, fpc);
	}

	action query_string {
		if (parser->query_string != NULL) {
			parser->query_string(parser->data, PTR_TO(query_start), LEN(query_start, fpc));
		}
	}

	action http_version {
		if (parser->http_version != NULL) {
		  parser->http_version(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action request_path {
		if (parser->request_path != NULL) {
		  parser->request_path(parser->data, PTR_TO(mark), LEN(mark,fpc));
		}
	}

	action done {
		parser->body_start = fpc - buffer + 1;
		if (parser->header_done != NULL) {
			parser->header_done(parser->data, fpc + 1, pe - fpc - 1);
		}
		fbreak;
	}

	include http_parser_common "http_common.rl";

}%%

/** Data **/
%% write data;

void http_parser_init(http_parser *parser)  {
	parser->http_field = md_http_field;
	parser->request_method = md_request_method;
	parser->request_uri = md_request_uri;
	parser->fragment = md_fragment;
	parser->request_path = md_request_path;
	parser->query_string = md_query_string;
	parser->http_version = md_http_version;
	parser->header_done = md_header_done;
}

void http_parser_reset(http_parser *parser) {
	int cs = 0;

	%% write init;

	parser->cs = cs;
	parser->body_start = 0;
	parser->content_len = 0;
	parser->mark = 0;
	parser->nread = 0;
	parser->field_len = 0;
	parser->field_start = 0;
}


/** exec **/
size_t http_parser_execute(http_parser *parser, const char *buffer, size_t len, size_t off)  {

	const char *p, *pe;
	int cs = parser->cs;

	assert(off <= len && "offset past end of buffer");

	p = buffer+off;
	pe = buffer+len;

	assert(*pe == '\0' && "pointer does not end on NUL");
	assert(pe - p == len - off && "pointers aren't same distance");

	%% write exec;

	parser->cs = cs;
	parser->nread += p - (buffer + off);

	assert(p <= pe && "buffer overflow after parsing execute");
	assert(parser->nread <= len && "nread longer than length");
	assert(parser->body_start <= len && "body starts after buffer end");
	assert(parser->mark < len && "mark is after buffer end");
	assert(parser->field_len <= len && "field has length longer than whole buffer");
	assert(parser->field_start < len && "field starts after buffer end");

	if(parser->body_start) {
		/* final \r\n combo encountered so stop right here */
		parser->nread++;
	}

	return(parser->nread);
}

int http_parser_finish(http_parser *parser)
{
  int cs = parser->cs;

  parser->cs = cs;

  if (http_parser_has_error(parser) ) {
	return -1;
  } else if (http_parser_is_finished(parser) ) {
	return 1;
  } else {
	return 0;
  }
}

int http_parser_has_error(http_parser *parser) {
  return parser->cs == http_parser_error;
}

int http_parser_is_finished(http_parser *parser) {
  return parser->cs == http_parser_first_final;
}

/* header parser function */
void md_http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen) {
	http_header *new_entry = malloc(sizeof(http_header));

	char *f = malloc(sizeof(char) * (flen + 1));
	strncpy(f, field, flen);
	f[flen] = '\0';

	char *v = malloc(sizeof(char) * (vlen + 1));
	strncpy(v, value, vlen);
	v[vlen] = '\0';

	new_entry->key = f;
	new_entry->value = v;
	md_log(LOGDEBUG, "f: %s, v: %s, flen: %lu, vlen: %lu", f, v, flen, vlen);

	HASH_ADD_KEYPTR(hh, ((request *) data)->table, new_entry->key, strlen(new_entry->key), new_entry);
}

void md_request_method(void *data, const char *at, size_t length) {
	request *r = (request *) data;

	char *v = malloc(sizeof(char) * (length + 1));
	strncpy(v, at, length);
	v[length] = '\0';

	r->request_method = v;
}

void md_request_uri(void *data, const char *at, size_t length) {
	request *r = (request *) data;

	char *v = malloc(sizeof(char) * (length + 1));
	strncpy(v, at, length);
	v[length] = '\0';

	r->request_uri = v;
}

void md_fragment(void *data, const char *at, size_t length) {
	request *r = (request *) data;

	char *v = malloc(sizeof(char) * (length + 1));
	strncpy(v, at, length);
	v[length] = '\0';

	r->fragment = v;
}

void md_request_path(void *data, const char *at, size_t length) {
	request *r = (request *) data;

	char *v = malloc(sizeof(char) * (length + 1));
	strncpy(v, at, length);
	v[length] = '\0';

	r->request_path = v;
}

void md_query_string(void *data, const char *at, size_t length) {
	request *r = (request *) data;

	char *v = malloc(sizeof(char) * (length + 1));
	strncpy(v, at, length);
	v[length] = '\0';

	r->query_string = v;
}

void md_http_version(void *data, const char *at, size_t length) {
	request *r = (request *) data;

	char *v = malloc(sizeof(char) * (length + 1));
	strncpy(v, at, length);
	v[length] = '\0';

	r->http_version = v;
}

void md_header_done(void *data, const char *at, size_t length) {
	#ifdef DEBUG
	md_log(LOGDEBUG, "done parsing!");
	#endif
}