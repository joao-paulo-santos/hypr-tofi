#ifndef JSON_H
#define JSON_H

#include <stdbool.h>
#include <stddef.h>

typedef struct json_parser {
	const char *pos;
	const char *error;
} json_parser_t;

typedef struct json_builder {
	char *buf;
	size_t len;
	size_t capacity;
	bool error;
	bool need_comma;
	int depth;
} json_builder_t;

void json_parser_init(json_parser_t *p, const char *json);
const char *json_get_error(json_parser_t *p);

bool json_skip_ws(json_parser_t *p);
bool json_peek_char(json_parser_t *p, char c);
bool json_expect_char(json_parser_t *p, char c);
bool json_skip_value(json_parser_t *p);

bool json_parse_string(json_parser_t *p, char *out, size_t max_len);
bool json_parse_bool(json_parser_t *p, bool *out);
bool json_parse_number(json_parser_t *p, double *out);
bool json_parse_null(json_parser_t *p);
bool json_parse_int(json_parser_t *p, long *out);

bool json_object_begin(json_parser_t *p);
bool json_object_next(json_parser_t *p, char *key, size_t key_max, bool *has_more);
bool json_object_end(json_parser_t *p);

bool json_array_begin(json_parser_t *p);
bool json_array_next(json_parser_t *p, bool *has_more);
bool json_array_end(json_parser_t *p);

void json_builder_init(json_builder_t *b, size_t initial_capacity);
void json_builder_free(json_builder_t *b);
char *json_builder_steal(json_builder_t *b);
const char *json_builder_get(json_builder_t *b);
size_t json_builder_len(json_builder_t *b);

void json_builder_object_begin(json_builder_t *b);
void json_builder_object_end(json_builder_t *b);
void json_builder_array_begin(json_builder_t *b);
void json_builder_array_end(json_builder_t *b);
void json_builder_key(json_builder_t *b, const char *key);
void json_builder_string(json_builder_t *b, const char *val);
void json_builder_bool(json_builder_t *b, bool val);
void json_builder_number(json_builder_t *b, double val);
void json_builder_int(json_builder_t *b, long val);
void json_builder_null(json_builder_t *b);
void json_builder_raw(json_builder_t *b, const char *str, size_t len);

size_t json_unescape_string(const char *src, size_t src_len, char *dest, size_t dest_size);
size_t json_escape_string(const char *src, char *dest, size_t dest_size);

#endif
