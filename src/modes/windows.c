#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../compositor.h"
#include "../mode.h"
#include "../xmalloc.h"

static bool windows_mode_check_deps(void)
{
	return active_backend != NULL;
}

static void windows_mode_populate(struct wl_list *results, const char *input)
{
	(void)input;

	struct wl_list windows;
	if (!compositor_get_windows(&windows)) {
		return;
	}

	struct window_info *win;
	wl_list_for_each(win, &windows, link) {
		char label[MAX_RESULT_LABEL_LEN];
		char info[MAX_RESULT_INFO_LEN];

		if (win->app_id[0]) {
			if (win->title[0] && strcmp(win->title, win->app_id) != 0) {
				char trimmed[16] = {0};
				strncpy(trimmed, win->title, 10);
				if (strlen(win->title) > 10) {
					strcat(trimmed, "...");
				}
				snprintf(label, sizeof(label), "%s - %s", win->app_id, trimmed);
			} else {
				strncpy(label, win->app_id, sizeof(label) - 1);
				label[sizeof(label) - 1] = '\0';
			}
		} else if (win->title[0]) {
			strncpy(label, win->title, sizeof(label) - 1);
			label[sizeof(label) - 1] = '\0';
		} else {
			strcpy(label, "Unknown");
		}

		snprintf(info, sizeof(info), "WIN:%s", win->address);

		struct result *r = result_create(label, info, "", 10, MODE_BIT_WINDOWS);
		if (r) {
			wl_list_insert(results, &r->link);
		}
	}

	windows_list_destroy(&windows);
}

static void windows_mode_execute(const char *info)
{
	if (strncmp(info, "WIN:", 4) != 0) {
		return;
	}
	compositor_focus_window(info + 4);
}

struct mode windows_mode = {
	.bit = MODE_BIT_WINDOWS,
	.name = "windows",
	.dep_binary = NULL,
	.default_display_prefix = "Show",
	.check_deps = windows_mode_check_deps,
	.populate = windows_mode_populate,
	.execute = windows_mode_execute,
};
