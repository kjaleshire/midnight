/**
 * Copyright (c) 2005 Zed A. Shaw
 * You can redistribute it and/or modify it under the same terms as Ruby.
 */

#include "parser.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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
		  parser->md_http_field(parser->data, PTR_TO(field_start), parser->field_len, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action request_method {
		if (parser->request_method != NULL) {
			parser->md_request_method(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action request_uri {
		if (parser->request_uri != NULL) {
			parser->md_request_uri(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action fragment {
		if (parser->fragment != NULL) {
			parser->md_fragment(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action start_query {
		MARK(query_start, fpc);
	}

	action query_string {
		if (parser->query_string != NULL) {
			parser->md_query_string(parser->data, PTR_TO(query_start), LEN(query_start, fpc));
		}
	}

	action http_version {
		if (parser->http_version != NULL) {
		  parser->md_http_version(parser->data, PTR_TO(mark), LEN(mark, fpc));
		}
	}

	action request_path {
		if (parser->request_path != NULL) {
		  parser->md_request_path(parser->data, PTR_TO(mark), LEN(mark,fpc));
		}
	}

	action done {
		parser->body_start = fpc - buffer + 1;
		if (parser->header_done != NULL) {
			parser->md_header_done(parser->data, fpc + 1, pe - fpc - 1);
		}
		fbreak;
	}

	include http_parser_common "midnight_common.rl";

}%%

/** Data **/
%% write data;

void http_parser_alloc(http_parser *parser)  {
	parser->http_field = md_http_field;
	parser->request_method = md_request_method;
	parser->request_uri = md_request_uri;
	parser->fragment = md_fragment;
	parser->request_path = md_request_path;
	parser->query_string = md_query_string;
	parser->http_version = md_http_version;
	parser->header_done = md_header_done;
}

void http_parser_init(http_parser *parser) {
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

static void http_field(void *data, const char *field, size_t flen, const char *value, size_t vlen)
{
  char *ch, *end;
  VALUE req = (VALUE)data;
  VALUE v = Qnil;
  VALUE f = Qnil;

  VALIDATE_MAX_LENGTH(flen, FIELD_NAME);
  VALIDATE_MAX_LENGTH(vlen, FIELD_VALUE);

  v = rb_str_new(value, vlen);
  f = rb_str_dup(global_http_prefix);
  f = rb_str_buf_cat(f, field, flen);

  for(ch = RSTRING_PTR(f) + RSTRING_LEN(global_http_prefix), end = RSTRING_PTR(f) + RSTRING_LEN(f); ch < end; ch++) {
	if(*ch == '-') {
	  *ch = '_';
	} else {
	  *ch = toupper(*ch);
	}
  }

  rb_hash_aset(req, f, v);
}

void md_request_method(void *data, const char *at, size_t length) {
  VALUE req = (VALUE)data;
  VALUE val = Qnil;

  val = rb_str_new(at, length);
  rb_hash_aset(req, global_request_method, val);
}

static void request_uri(void *data, const char *at, size_t length)
{
  VALUE req = (VALUE)data;
  VALUE val = Qnil;

  VALIDATE_MAX_LENGTH(length, REQUEST_URI);

  val = rb_str_new(at, length);
  rb_hash_aset(req, global_request_uri, val);
}

static void fragment(void *data, const char *at, size_t length)
{
  VALUE req = (VALUE)data;
  VALUE val = Qnil;

  VALIDATE_MAX_LENGTH(length, FRAGMENT);

  val = rb_str_new(at, length);
  rb_hash_aset(req, global_fragment, val);
}

static void request_path(void *data, const char *at, size_t length)
{
  VALUE req = (VALUE)data;
  VALUE val = Qnil;

  VALIDATE_MAX_LENGTH(length, REQUEST_PATH);

  val = rb_str_new(at, length);
  rb_hash_aset(req, global_request_path, val);
  rb_hash_aset(req, global_path_info, val);
}

static void query_string(void *data, const char *at, size_t length)
{
  VALUE req = (VALUE)data;
  VALUE val = Qnil;

  VALIDATE_MAX_LENGTH(length, QUERY_STRING);

  val = rb_str_new(at, length);
  rb_hash_aset(req, global_query_string, val);
}

static void http_version(void *data, const char *at, size_t length)
{
  VALUE req = (VALUE)data;
  VALUE val = rb_str_new(at, length);
  rb_hash_aset(req, global_http_version, val);
}

/** Finalizes the request header to have a bunch of stuff that's
  needed. */

static void header_done(void *data, const char *at, size_t length)
{
  VALUE req = (VALUE)data;
  VALUE temp = Qnil;
  VALUE ctype = Qnil;
  VALUE clen = Qnil;
  VALUE body = Qnil;
  char *colon = NULL;

  clen = rb_hash_aref(req, global_http_content_length);
  if(clen != Qnil) {
	rb_hash_aset(req, global_content_length, clen);
	rb_hash_delete(req, global_http_content_length);
  }

  ctype = rb_hash_aref(req, global_http_content_type);
  if(ctype != Qnil) {
	rb_hash_aset(req, global_content_type, ctype);
	rb_hash_delete(req, global_http_content_type);
  }

  rb_hash_aset(req, global_gateway_interface, global_gateway_interface_value);
  if((temp = rb_hash_aref(req, global_http_host)) != Qnil) {
	/* ruby better close strings off with a '\0' dammit */
	colon = strchr(RSTRING_PTR(temp), ':');
	if(colon != NULL) {
	  rb_hash_aset(req, global_server_name, rb_str_substr(temp, 0, colon - RSTRING_PTR(temp)));
	  rb_hash_aset(req, global_server_port,
		  rb_str_substr(temp, colon - RSTRING_PTR(temp)+1,
			RSTRING_LEN(temp)));
	} else {
	  rb_hash_aset(req, global_server_name, temp);
	  rb_hash_aset(req, global_server_port, global_port_80);
	}
  }

  /* grab the initial body and stuff it into the hash */
  if(length > 0) {
	body = rb_hash_aref(req, global_http_body);
	rb_io_write(body, rb_str_new(at, length));
  }

  /* according to Rack specs, query string must be empty string if none */
  if (rb_hash_aref(req, global_query_string) == Qnil) {
	rb_hash_aset(req, global_query_string, global_empty);
  }
	if (rb_hash_aref(req, global_path_info) == Qnil) {
		rb_hash_aset(req, global_path_info, global_empty);
	}

	/* set some constants */
	rb_hash_aset(req, global_server_protocol, global_server_protocol_value);
	rb_hash_aset(req, global_url_scheme, global_url_scheme_value);
	rb_hash_aset(req, global_script_name, global_empty);
}