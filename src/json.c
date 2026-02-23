#include "json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void json_parser_init(json_parser_t *p, const char *json)
{
	p->pos = json;
	p->error = NULL;
}

const char *json_get_error(json_parser_t *p)
{
	return p->error;
}

static void set_error(json_parser_t *p, const char *msg)
{
	if (!p->error) {
		p->error = msg;
	}
}

bool json_skip_ws(json_parser_t *p)
{
	while (*p->pos == ' ' || *p->pos == '\t' || *p->pos == '\n' || *p->pos == '\r') {
		p->pos++;
	}
	return true;
}

bool json_peek_char(json_parser_t *p, char c)
{
	json_skip_ws(p);
	return *p->pos == c;
}

bool json_expect_char(json_parser_t *p, char c)
{
	json_skip_ws(p);
	if (*p->pos == c) {
		p->pos++;
		return true;
	}
	set_error(p, "unexpected character");
	return false;
}

static int hex_digit(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return 10 + c - 'a';
	if (c >= 'A' && c <= 'F') return 10 + c - 'A';
	return -1;
}

static int parse_hex16(const char *s, unsigned int *out)
{
	int d0 = hex_digit(s[0]);
	int d1 = hex_digit(s[1]);
	int d2 = hex_digit(s[2]);
	int d3 = hex_digit(s[3]);
	if (d0 < 0 || d1 < 0 || d2 < 0 || d3 < 0) return 0;
	*out = (d0 << 12) | (d1 << 8) | (d2 << 4) | d3;
	return 1;
}

static size_t encode_utf8(unsigned int cp, char *out)
{
	if (cp < 0x80) {
		out[0] = (char)cp;
		return 1;
	} else if (cp < 0x800) {
		out[0] = (char)(0xC0 | (cp >> 6));
		out[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	} else if (cp < 0x10000) {
		out[0] = (char)(0xE0 | (cp >> 12));
		out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	} else if (cp < 0x110000) {
		out[0] = (char)(0xF0 | (cp >> 18));
		out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		out[3] = (char)(0x80 | (cp & 0x3F));
		return 4;
	}
	return 0;
}

bool json_parse_string(json_parser_t *p, char *out, size_t max_len)
{
	json_skip_ws(p);
	if (*p->pos != '"') {
		set_error(p, "expected string");
		return false;
	}
	p->pos++;

	size_t j = 0;
	while (*p->pos && *p->pos != '"' && j + 4 < max_len) {
		if (*p->pos == '\\') {
			p->pos++;
			switch (*p->pos) {
			case '"': out[j++] = '"'; p->pos++; break;
			case '\\': out[j++] = '\\'; p->pos++; break;
			case '/': out[j++] = '/'; p->pos++; break;
			case 'b': out[j++] = '\b'; p->pos++; break;
			case 'f': out[j++] = '\f'; p->pos++; break;
			case 'n': out[j++] = '\n'; p->pos++; break;
			case 'r': out[j++] = '\r'; p->pos++; break;
			case 't': out[j++] = '\t'; p->pos++; break;
			case 'u': {
				p->pos++;
				if (!p->pos[0] || !p->pos[1] || !p->pos[2] || !p->pos[3]) {
					set_error(p, "truncated unicode escape");
					return false;
				}
				unsigned int cp;
				if (!parse_hex16(p->pos, &cp)) {
					set_error(p, "invalid unicode escape");
					return false;
				}
				p->pos += 4;
				
				if (cp >= 0xD800 && cp <= 0xDBFF) {
					if (p->pos[0] == '\\' && p->pos[1] == 'u') {
						p->pos += 2;
						if (!p->pos[0] || !p->pos[1] || !p->pos[2] || !p->pos[3]) {
							set_error(p, "truncated surrogate pair");
							return false;
						}
						unsigned int low;
						if (!parse_hex16(p->pos, &low)) {
							set_error(p, "invalid surrogate pair");
							return false;
						}
						p->pos += 4;
						if (low >= 0xDC00 && low <= 0xDFFF) {
							cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
						}
					}
				}
				j += encode_utf8(cp, out + j);
				break;
			}
			default:
				set_error(p, "invalid escape sequence");
				return false;
			}
		} else {
			out[j++] = *p->pos++;
		}
	}
	out[j] = '\0';

	if (*p->pos != '"') {
		set_error(p, "unterminated string");
		return false;
	}
	p->pos++;
	return true;
}

bool json_parse_bool(json_parser_t *p, bool *out)
{
	json_skip_ws(p);
	if (strncmp(p->pos, "true", 4) == 0) {
		p->pos += 4;
		if (out) *out = true;
		return true;
	}
	if (strncmp(p->pos, "false", 5) == 0) {
		p->pos += 5;
		if (out) *out = false;
		return true;
	}
	set_error(p, "expected boolean");
	return false;
}

bool json_parse_null(json_parser_t *p)
{
	json_skip_ws(p);
	if (strncmp(p->pos, "null", 4) == 0) {
		p->pos += 4;
		return true;
	}
	set_error(p, "expected null");
	return false;
}

bool json_parse_number(json_parser_t *p, double *out)
{
	json_skip_ws(p);
	char *end;
	double val = strtod(p->pos, &end);
	if (end == p->pos) {
		set_error(p, "expected number");
		return false;
	}
	p->pos = end;
	if (out) *out = val;
	return true;
}

bool json_parse_int(json_parser_t *p, long *out)
{
	json_skip_ws(p);
	char *end;
	long val = strtol(p->pos, &end, 10);
	if (end == p->pos) {
		set_error(p, "expected integer");
		return false;
	}
	p->pos = end;
	if (out) *out = val;
	return true;
}

bool json_skip_value(json_parser_t *p)
{
	json_skip_ws(p);
	
	if (*p->pos == '"') {
		char tmp[256];
		return json_parse_string(p, tmp, sizeof(tmp));
	}
	if (*p->pos == '{') {
		if (!json_object_begin(p)) return false;
		while (!json_peek_char(p, '}')) {
			char key[256];
			bool has_more;
			if (!json_object_next(p, key, sizeof(key), &has_more)) return false;
			if (!json_skip_value(p)) return false;
			if (json_peek_char(p, ',')) {
				p->pos++;
			}
		}
		return json_object_end(p);
	}
	if (*p->pos == '[') {
		if (!json_array_begin(p)) return false;
		while (!json_peek_char(p, ']')) {
			bool has_more;
			if (!json_array_next(p, &has_more)) return false;
			if (!json_skip_value(p)) return false;
			if (json_peek_char(p, ',')) {
				p->pos++;
			}
		}
		return json_array_end(p);
	}
	if (*p->pos == 't' || *p->pos == 'f') {
		return json_parse_bool(p, NULL);
	}
	if (*p->pos == 'n') {
		return json_parse_null(p);
	}
	if (*p->pos == '-' || isdigit((unsigned char)*p->pos)) {
		return json_parse_number(p, NULL);
	}
	set_error(p, "unexpected value");
	return false;
}

bool json_object_begin(json_parser_t *p)
{
	return json_expect_char(p, '{');
}

bool json_object_end(json_parser_t *p)
{
	return json_expect_char(p, '}');
}

bool json_object_next(json_parser_t *p, char *key, size_t key_max, bool *has_more)
{
	json_skip_ws(p);
	
	if (*p->pos == '}') {
		*has_more = false;
		return true;
	}
	
	if (!json_parse_string(p, key, key_max)) return false;
	
	if (!json_expect_char(p, ':')) return false;
	
	*has_more = true;
	return true;
}

bool json_array_begin(json_parser_t *p)
{
	return json_expect_char(p, '[');
}

bool json_array_end(json_parser_t *p)
{
	return json_expect_char(p, ']');
}

bool json_array_next(json_parser_t *p, bool *has_more)
{
	json_skip_ws(p);
	
	if (*p->pos == ']') {
		*has_more = false;
		return true;
	}
	
	*has_more = true;
	return true;
}

void json_builder_init(json_builder_t *b, size_t initial_capacity)
{
	b->buf = malloc(initial_capacity);
	b->capacity = b->buf ? initial_capacity : 0;
	b->len = 0;
	b->error = false;
	b->need_comma = false;
	b->depth = 0;
	if (b->buf) b->buf[0] = '\0';
}

void json_builder_free(json_builder_t *b)
{
	free(b->buf);
	b->buf = NULL;
	b->capacity = 0;
	b->len = 0;
}

char *json_builder_steal(json_builder_t *b)
{
	char *result = b->buf;
	b->buf = NULL;
	b->capacity = 0;
	b->len = 0;
	return result;
}

const char *json_builder_get(json_builder_t *b)
{
	return b->buf ? b->buf : "";
}

size_t json_builder_len(json_builder_t *b)
{
	return b->len;
}

static bool builder_ensure(json_builder_t *b, size_t needed)
{
	if (b->len + needed + 1 <= b->capacity) return true;
	
	size_t new_cap = b->capacity * 2;
	if (new_cap < b->len + needed + 1) {
		new_cap = b->len + needed + 1;
	}
	
	char *new_buf = realloc(b->buf, new_cap);
	if (!new_buf) {
		b->error = true;
		return false;
	}
	b->buf = new_buf;
	b->capacity = new_cap;
	return true;
}

static void builder_append(json_builder_t *b, const char *str, size_t len)
{
	if (!builder_ensure(b, len)) return;
	memcpy(b->buf + b->len, str, len);
	b->len += len;
	b->buf[b->len] = '\0';
}

static void builder_char(json_builder_t *b, char c)
{
	if (!builder_ensure(b, 1)) return;
	b->buf[b->len++] = c;
	b->buf[b->len] = '\0';
}

static void builder_maybe_comma(json_builder_t *b)
{
	if (b->need_comma) {
		builder_char(b, ',');
		b->need_comma = false;
	}
}

void json_builder_object_begin(json_builder_t *b)
{
	builder_maybe_comma(b);
	builder_char(b, '{');
	b->need_comma = false;
	b->depth++;
}

void json_builder_object_end(json_builder_t *b)
{
	builder_char(b, '}');
	b->need_comma = true;
	b->depth--;
}

void json_builder_array_begin(json_builder_t *b)
{
	builder_maybe_comma(b);
	builder_char(b, '[');
	b->need_comma = false;
	b->depth++;
}

void json_builder_array_end(json_builder_t *b)
{
	builder_char(b, ']');
	b->need_comma = true;
	b->depth--;
}

void json_builder_key(json_builder_t *b, const char *key)
{
	builder_maybe_comma(b);
	json_builder_string(b, key);
	builder_char(b, ':');
	b->need_comma = false;
}

void json_builder_string(json_builder_t *b, const char *val)
{
	builder_maybe_comma(b);
	builder_char(b, '"');
	
	for (const char *p = val; *p; p++) {
		unsigned char c = (unsigned char)*p;
		switch (c) {
		case '"': builder_append(b, "\\\"", 2); break;
		case '\\': builder_append(b, "\\\\", 2); break;
		case '\b': builder_append(b, "\\b", 2); break;
		case '\f': builder_append(b, "\\f", 2); break;
		case '\n': builder_append(b, "\\n", 2); break;
		case '\r': builder_append(b, "\\r", 2); break;
		case '\t': builder_append(b, "\\t", 2); break;
		default:
			if (c < 0x20) {
				char buf[8];
				snprintf(buf, sizeof(buf), "\\u%04x", c);
				builder_append(b, buf, 6);
			} else {
				builder_char(b, c);
			}
		}
	}
	
	builder_char(b, '"');
	b->need_comma = true;
}

void json_builder_bool(json_builder_t *b, bool val)
{
	builder_maybe_comma(b);
	builder_append(b, val ? "true" : "false", val ? 4 : 5);
	b->need_comma = true;
}

void json_builder_number(json_builder_t *b, double val)
{
	builder_maybe_comma(b);
	char buf[32];
	int len = snprintf(buf, sizeof(buf), "%g", val);
	builder_append(b, buf, len);
	b->need_comma = true;
}

void json_builder_int(json_builder_t *b, long val)
{
	builder_maybe_comma(b);
	char buf[24];
	int len = snprintf(buf, sizeof(buf), "%ld", val);
	builder_append(b, buf, len);
	b->need_comma = true;
}

void json_builder_null(json_builder_t *b)
{
	builder_maybe_comma(b);
	builder_append(b, "null", 4);
	b->need_comma = true;
}

void json_builder_raw(json_builder_t *b, const char *str, size_t len)
{
	builder_append(b, str, len);
	b->need_comma = true;
}

size_t json_unescape_string(const char *src, size_t src_len, char *dest, size_t dest_size)
{
	json_parser_t p;
	json_parser_init(&p, src);
	
	size_t j = 0;
	if (*p.pos == '"') p.pos++;
	
	while (*p.pos && *p.pos != '"' && j + 4 < dest_size) {
		if (*p.pos == '\\') {
			p.pos++;
			switch (*p.pos) {
			case '"': dest[j++] = '"'; p.pos++; break;
			case '\\': dest[j++] = '\\'; p.pos++; break;
			case '/': dest[j++] = '/'; p.pos++; break;
			case 'b': dest[j++] = '\b'; p.pos++; break;
			case 'f': dest[j++] = '\f'; p.pos++; break;
			case 'n': dest[j++] = '\n'; p.pos++; break;
			case 'r': dest[j++] = '\r'; p.pos++; break;
			case 't': dest[j++] = '\t'; p.pos++; break;
			case 'u': {
				p.pos++;
				unsigned int cp;
				if (parse_hex16(p.pos, &cp)) {
					p.pos += 4;
					j += encode_utf8(cp, dest + j);
				}
				break;
			}
			default: dest[j++] = *p.pos++; break;
			}
		} else {
			dest[j++] = *p.pos++;
		}
	}
	dest[j] = '\0';
	return j;
}

size_t json_escape_string(const char *src, char *dest, size_t dest_size)
{
	size_t j = 0;
	if (j < dest_size - 1) dest[j++] = '"';
	
	for (const char *p = src; *p && j < dest_size - 7; p++) {
		unsigned char c = (unsigned char)*p;
		switch (c) {
		case '"': dest[j++] = '\\'; dest[j++] = '"'; break;
		case '\\': dest[j++] = '\\'; dest[j++] = '\\'; break;
		case '\n': dest[j++] = '\\'; dest[j++] = 'n'; break;
		case '\r': dest[j++] = '\\'; dest[j++] = 'r'; break;
		case '\t': dest[j++] = '\\'; dest[j++] = 't'; break;
		default:
			if (c < 0x20) {
				j += snprintf(dest + j, dest_size - j, "\\u%04x", c);
			} else {
				dest[j++] = c;
			}
		}
	}
	
	if (j < dest_size - 1) dest[j++] = '"';
	dest[j] = '\0';
	return j;
}
