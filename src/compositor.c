#include <stdlib.h>
#include <string.h>
#include "compositor.h"
#include "log.h"

const struct compositor_backend *active_backend = NULL;

static const struct compositor_backend *backends[] = {
	&hyprland_backend,
	NULL
};

bool compositor_init(const char *name)
{
	if (name && strcmp(name, "auto") != 0) {
		for (int i = 0; backends[i]; i++) {
			if (strcmp(backends[i]->name, name) == 0) {
				if (!backends[i]->is_available()) {
					log_error("Compositor backend '%s' is not available\n", name);
					return false;
				}
				active_backend = backends[i];
				return true;
			}
		}
		log_error("Unknown compositor backend: %s\n", name);
		return false;
	}

	for (int i = 0; backends[i]; i++) {
		if (backends[i]->is_available()) {
			active_backend = backends[i];
			return true;
		}
	}

	return false;
}

void compositor_cleanup(void)
{
	active_backend = NULL;
}

bool compositor_get_windows(struct wl_list *windows)
{
	if (!active_backend || !active_backend->get_windows) {
		return false;
	}
	return active_backend->get_windows(windows);
}

bool compositor_get_workspaces(struct wl_list *workspaces)
{
	if (!active_backend || !active_backend->get_workspaces) {
		return false;
	}
	return active_backend->get_workspaces(workspaces);
}

void compositor_focus_window(const char *address)
{
	if (active_backend && active_backend->focus_window) {
		active_backend->focus_window(address);
	}
}

void compositor_switch_workspace(const char *name)
{
	if (active_backend && active_backend->switch_workspace) {
		active_backend->switch_workspace(name);
	}
}

void windows_list_destroy(struct wl_list *windows)
{
	struct window_info *win, *tmp;
	wl_list_for_each_safe(win, tmp, windows, link) {
		wl_list_remove(&win->link);
		free(win);
	}
}

void workspaces_list_destroy(struct wl_list *workspaces)
{
	struct workspace_info *ws, *tmp;
	wl_list_for_each_safe(ws, tmp, workspaces, link) {
		wl_list_remove(&ws->link);
		free(ws);
	}
}
