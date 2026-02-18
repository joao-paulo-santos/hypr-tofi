#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../compositor.h"
#include "../log.h"
#include "../xmalloc.h"

static bool hyprland_is_available(void)
{
	return access("/tmp/hypr", F_OK) == 0 || getenv("HYPRLAND_INSTANCE_SIGNATURE") != NULL;
}

static char *run_hyprctl(const char *args)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "hyprctl %s 2>/dev/null", args);

	FILE *fp = popen(cmd, "r");
	if (!fp) {
		return NULL;
	}

	size_t capacity = 4096;
	size_t len = 0;
	char *output = xmalloc(capacity);
	output[0] = '\0';

	char buf[1024];
	while (fgets(buf, sizeof(buf), fp)) {
		size_t buf_len = strlen(buf);
		if (len + buf_len + 1 > capacity) {
			capacity *= 2;
			output = xrealloc(output, capacity);
		}
		strcpy(output + len, buf);
		len += buf_len;
	}

	pclose(fp);
	return output;
}

static char *skip_whitespace(char *p)
{
	while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
		p++;
	}
	return p;
}

static char *find_key_value_start(char *json, const char *key)
{
	char search[128];
	snprintf(search, sizeof(search), "\"%s\":", key);

	char *pos = strstr(json, search);
	if (!pos) {
		snprintf(search, sizeof(search), "\"%s\" :", key);
		pos = strstr(json, search);
	}
	return pos;
}

static bool parse_string_value(char *json, const char *key, char *out, size_t max_len)
{
	char *pos = find_key_value_start(json, key);
	if (!pos) {
		return false;
	}

	pos = strchr(pos + strlen(key) + 3, '"');
	if (!pos) {
		return false;
	}
	pos++;

	char *end = strchr(pos, '"');
	if (!end) {
		return false;
	}

	size_t len = end - pos;
	if (len >= max_len) {
		len = max_len - 1;
	}
	strncpy(out, pos, len);
	out[len] = '\0';
	return true;
}

static bool parse_int_value(char *json, const char *key, int *out)
{
	char *pos = find_key_value_start(json, key);
	if (!pos) {
		return false;
	}

	pos = strchr(pos + strlen(key) + 2, ':');
	if (!pos) {
		return false;
	}
	pos = skip_whitespace(pos + 1);

	*out = atoi(pos);
	return true;
}

static char *find_object_end(char *start)
{
	int depth = 0;
	bool in_string = false;

	for (char *p = start; *p; p++) {
		if (*p == '"' && (p == start || *(p - 1) != '\\')) {
			in_string = !in_string;
		} else if (!in_string) {
			if (*p == '{') {
				depth++;
			} else if (*p == '}') {
				depth--;
				if (depth == 0) {
					return p;
				}
			}
		}
	}
	return NULL;
}

static bool hyprland_get_windows(struct wl_list *windows)
{
	wl_list_init(windows);

	char *output = run_hyprctl("-j clients");
	if (!output || !*output) {
		free(output);
		return false;
	}

	char *p = output;
	while ((p = strchr(p, '{')) != NULL) {
		char *obj_end = find_object_end(p);
		if (!obj_end) {
			break;
		}

		char saved = *obj_end;
		*obj_end = '\0';

		struct window_info *win = xcalloc(1, sizeof(*win));

		if (!parse_string_value(p, "address", win->address, sizeof(win->address))) {
			free(win);
			*obj_end = saved;
			p = obj_end + 1;
			continue;
		}

		parse_string_value(p, "title", win->title, sizeof(win->title));
		parse_string_value(p, "class", win->app_id, sizeof(win->app_id));

		if (win->title[0] == '\0') {
			strncpy(win->title, win->app_id[0] ? win->app_id : "Unknown", sizeof(win->title) - 1);
		}

		wl_list_insert(windows, &win->link);

		*obj_end = saved;
		p = obj_end + 1;
	}

	free(output);
	return !wl_list_empty(windows);
}

static bool hyprland_get_workspaces(struct wl_list *workspaces)
{
	wl_list_init(workspaces);

	char *output = run_hyprctl("-j workspaces");
	if (!output || !*output) {
		free(output);
		return false;
	}

	char *active_output = run_hyprctl("-j activeworkspace");
	int active_id = -999;
	if (active_output) {
		parse_int_value(active_output, "id", &active_id);
		free(active_output);
	}

	char *p = output;
	while ((p = strchr(p, '{')) != NULL) {
		char *obj_end = strchr(p, '}');
		if (!obj_end) {
			break;
		}

		*obj_end = '\0';

		struct workspace_info *ws = xcalloc(1, sizeof(*ws));

		if (!parse_int_value(p, "id", (int*)&ws->id)) {
			free(ws);
			p = obj_end + 1;
			continue;
		}

		parse_string_value(p, "name", ws->name, sizeof(ws->name));
		ws->focused = (ws->id == active_id);

		wl_list_insert(workspaces, &ws->link);

		*obj_end = '}';
		p = obj_end + 1;
	}

	free(output);
	return !wl_list_empty(workspaces);
}

static void hyprland_focus_window(const char *address)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "hyprctl dispatch focuswindow address:%s &", address);
	system(cmd);
}

static void hyprland_switch_workspace(const char *name)
{
	char cmd[128];
	snprintf(cmd, sizeof(cmd), "hyprctl dispatch workspace name:%s &", name);
	system(cmd);
}

const struct compositor_backend hyprland_backend = {
	.name = "hyprland",
	.is_available = hyprland_is_available,
	.get_windows = hyprland_get_windows,
	.get_workspaces = hyprland_get_workspaces,
	.focus_window = hyprland_focus_window,
	.switch_workspace = hyprland_switch_workspace,
};
