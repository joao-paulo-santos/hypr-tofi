#ifndef PLUGIN_H
#define PLUGIN_H

#include <stdbool.h>
#include <stddef.h>
#include <wayland-client.h>
#include "string_vec.h"

#define PLUGIN_NAME_MAX 64
#define PLUGIN_PATH_MAX 256
#define PLUGIN_LABEL_MAX 256
#define PLUGIN_EXEC_MAX 512
#define PLUGIN_PROMPT_MAX 64
#define PLUGIN_FIELD_MAX 64

typedef enum {
	ACTION_TYPE_EXEC,
	ACTION_TYPE_INPUT,
	ACTION_TYPE_SELECT,
	ACTION_TYPE_PLUGIN,
} plugin_action_type_t;

typedef enum {
	ON_SELECT_EXEC,
	ON_SELECT_INPUT,
	ON_SELECT_PLUGIN,
} plugin_on_select_t;

typedef enum {
	FORMAT_LINES,
	FORMAT_JSON,
} plugin_format_t;

struct plugin_result {
	struct wl_list link;
	char label[PLUGIN_LABEL_MAX];
	char value[PLUGIN_LABEL_MAX];
	char exec[PLUGIN_EXEC_MAX];
	char plugin_name[PLUGIN_NAME_MAX];
	plugin_action_type_t action_type;
	char prompt[PLUGIN_PROMPT_MAX];
	char plugin_ref[PLUGIN_NAME_MAX];
	char list_cmd[PLUGIN_EXEC_MAX];
	plugin_format_t format;
	plugin_on_select_t on_select;
	char label_field[PLUGIN_FIELD_MAX];
	char value_field[PLUGIN_FIELD_MAX];
};

struct plugin_action {
	struct wl_list link;
	char label[PLUGIN_LABEL_MAX];
	char display_prefix[PLUGIN_LABEL_MAX];
	plugin_action_type_t type;
	
	char prompt[PLUGIN_PROMPT_MAX];
	char exec[PLUGIN_EXEC_MAX];
	
	char list_cmd[PLUGIN_EXEC_MAX];
	plugin_format_t format;
	plugin_on_select_t on_select;
	char label_field[PLUGIN_FIELD_MAX];
	char value_field[PLUGIN_FIELD_MAX];
	
	char plugin_ref[PLUGIN_NAME_MAX];
};

struct plugin {
	struct wl_list link;
	char name[PLUGIN_NAME_MAX];
	char display_prefix[PLUGIN_LABEL_MAX];
	char context_name[PLUGIN_LABEL_MAX];
	bool global;
	
	char **depends;
	size_t depends_count;
	
	bool has_provider;
	char list_cmd[PLUGIN_EXEC_MAX];
	plugin_format_t format;
	char label_field[PLUGIN_FIELD_MAX];
	char value_field[PLUGIN_FIELD_MAX];
	char exec[PLUGIN_EXEC_MAX];
	
	struct wl_list actions;
	
	bool loaded;
	bool deps_satisfied;
};

void plugin_init(void);
void plugin_destroy(void);

void plugin_load_directory(const char *path);
struct plugin *plugin_get(const char *name);
size_t plugin_count(void);

void plugin_populate_results(struct wl_list *results, const char *filter);
void plugin_populate_action_results(struct plugin *plugin, struct wl_list *results);
void plugin_run_select_cmd(const char *list_cmd, plugin_format_t format, 
	const char *label_field, const char *value_field,
	struct wl_list *plugin_results, struct string_ref_vec *display_results);
void plugin_results_destroy(struct wl_list *results);
void plugin_rebuild_entry_results(struct wl_list *plugin_results, bool show_prefixes);

struct plugin_action *plugin_action_create(void);
void plugin_action_destroy(struct plugin_action *action);

#endif
