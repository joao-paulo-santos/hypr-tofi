#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

#define MAX_WINDOW_TITLE 256
#define MAX_WINDOW_APP_ID 64
#define MAX_WINDOW_ADDRESS 32
#define MAX_WORKSPACE_NAME 64

struct window_info {
	struct wl_list link;
	char title[MAX_WINDOW_TITLE];
	char app_id[MAX_WINDOW_APP_ID];
	char address[MAX_WINDOW_ADDRESS];
};

struct workspace_info {
	struct wl_list link;
	int32_t id;
	char name[MAX_WORKSPACE_NAME];
	bool focused;
};

struct compositor_backend {
	const char *name;
	bool (*is_available)(void);
	bool (*get_windows)(struct wl_list *windows);
	bool (*get_workspaces)(struct wl_list *workspaces);
	void (*focus_window)(const char *address);
	void (*switch_workspace)(const char *name);
};

extern const struct compositor_backend *active_backend;

bool compositor_init(const char *name);
void compositor_cleanup(void);

bool compositor_get_windows(struct wl_list *windows);
bool compositor_get_workspaces(struct wl_list *workspaces);
void compositor_focus_window(const char *address);
void compositor_switch_workspace(const char *name);

void windows_list_destroy(struct wl_list *windows);
void workspaces_list_destroy(struct wl_list *workspaces);

extern const struct compositor_backend hyprland_backend;

#endif
