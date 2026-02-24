#ifndef RENDERER_H
#define RENDERER_H

#include "view.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct renderer {
	const char *name;
	void *priv;
	
	bool (*init)(struct renderer *r, struct view_theme *theme,
	             uint32_t width, uint32_t height, double scale);
	void (*destroy)(struct renderer *r);
	void (*resize)(struct renderer *r, uint32_t width, uint32_t height);
	void (*update_theme)(struct renderer *r, struct view_theme *theme);
	void (*begin_frame)(struct renderer *r);
	void (*measure)(struct renderer *r, struct view_state *state,
	                struct view_theme *theme, struct view_layout *layout);
	void (*render)(struct renderer *r, struct view_state *state,
	               struct view_theme *theme, struct view_layout *layout);
	void (*end_frame)(struct renderer *r);
	void (*present)(struct renderer *r);
	
	uint8_t *(*get_buffer)(struct renderer *r);
	size_t (*get_buffer_size)(struct renderer *r);
};

struct renderer *renderer_cairo_create(void);

#endif
