#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../compositor.h"
#include "../mode.h"
#include "../xmalloc.h"

static bool workspaces_mode_check_deps(void)
{
	return active_backend != NULL;
}

static void workspaces_mode_populate(struct wl_list *results, const char *input)
{
	(void)input;

	struct wl_list workspaces;
	if (!compositor_get_workspaces(&workspaces)) {
		return;
	}

	struct workspace_info *ws;
	wl_list_for_each(ws, &workspaces, link) {
		char label[MAX_RESULT_LABEL_LEN];
		char info[MAX_RESULT_INFO_LEN];

		if (ws->name[0]) {
			strncpy(label, ws->name, sizeof(label) - 1);
		} else {
			snprintf(label, sizeof(label), "%d", ws->id);
		}
		label[sizeof(label) - 1] = '\0';

		snprintf(info, sizeof(info), "WS:%s", ws->name[0] ? ws->name : label);

		struct result *r = result_create(label, info, "", 10, MODE_BIT_WORKSPACES);
		if (r) {
			wl_list_insert(results, &r->link);
		}
	}

	workspaces_list_destroy(&workspaces);
}

static void workspaces_mode_execute(const char *info)
{
	if (strncmp(info, "WS:", 3) != 0) {
		return;
	}
	compositor_switch_workspace(info + 3);
}

struct mode workspaces_mode = {
	.bit = MODE_BIT_WORKSPACES,
	.name = "workspaces",
	.dep_binary = NULL,
	.default_display_prefix = "Workspace",
	.check_deps = workspaces_mode_check_deps,
	.populate = workspaces_mode_populate,
	.execute = workspaces_mode_execute,
};
