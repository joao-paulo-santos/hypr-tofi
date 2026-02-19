#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log.h"
#include "plugin.h"
#include "xmalloc.h"

#define MAX_LINE_LEN 1024
#define MAX_ARRAY_ITEMS 32

static struct wl_list plugins;

void plugin_init(void)
{
	wl_list_init(&plugins);
}

void plugin_destroy(void)
{
	struct plugin *p, *tmp;
	wl_list_for_each_safe(p, tmp, &plugins, link) {
		if (p->depends) {
			for (size_t i = 0; i < p->depends_count; i++) {
				free(p->depends[i]);
			}
			free(p->depends);
		}
		
		struct plugin_action *a, *atmp;
		wl_list_for_each_safe(a, atmp, &p->actions, link) {
			wl_list_remove(&a->link);
			free(a);
		}
		
		free(p);
	}
}

static char *trim(char *str)
{
	while (isspace(*str)) str++;
	if (*str == '\0') return str;
	char *end = str + strlen(str) - 1;
	while (end > str && isspace(*end)) end--;
	*(end + 1) = '\0';
	return str;
}

static char *parse_string_value(char *value)
{
	value = trim(value);
	if (*value == '"') {
		value++;
		char *end = strrchr(value, '"');
		if (end) *end = '\0';
	}
	return value;
}

static int parse_string_array(char *value, char **out, size_t max)
{
	value = trim(value);
	if (*value != '[') return 0;
	
	value++;
	int count = 0;
	char *token = strtok(value, ",]");
	
	while (token && count < (int)max) {
		token = trim(token);
		token = parse_string_value(token);
		if (*token) {
			out[count++] = xstrdup(token);
		}
		token = strtok(NULL, ",]");
	}
	
	return count;
}

static bool parse_bool_value(char *value)
{
	value = trim(value);
	return (strcmp(value, "true") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "1") == 0);
}

static plugin_format_t parse_format(const char *value)
{
	if (strcmp(value, "json") == 0) return FORMAT_JSON;
	return FORMAT_LINES;
}

static plugin_action_type_t parse_action_type(const char *value)
{
	if (strcmp(value, "input") == 0) return ACTION_TYPE_INPUT;
	if (strcmp(value, "select") == 0) return ACTION_TYPE_SELECT;
	if (strcmp(value, "plugin") == 0) return ACTION_TYPE_PLUGIN;
	return ACTION_TYPE_EXEC;
}

static plugin_on_select_t parse_on_select(const char *value)
{
	if (strcmp(value, "input") == 0) return ON_SELECT_INPUT;
	if (strcmp(value, "plugin") == 0) return ON_SELECT_PLUGIN;
	return ON_SELECT_EXEC;
}

static bool check_dependency(const char *binary)
{
	char *path_env = getenv("PATH");
	if (!path_env) return false;
	
	char *paths = xstrdup(path_env);
	char *dir = strtok(paths, ":");
	
	while (dir) {
		char full_path[512];
		snprintf(full_path, sizeof(full_path), "%s/%s", dir, binary);
		if (access(full_path, X_OK) == 0) {
			free(paths);
			return true;
		}
		dir = strtok(NULL, ":");
	}
	
	free(paths);
	return false;
}

static bool check_dependencies(struct plugin *plugin)
{
	for (size_t i = 0; i < plugin->depends_count; i++) {
		if (!check_dependency(plugin->depends[i])) {
			log_debug("Plugin '%s' missing dependency: %s\n", plugin->name, plugin->depends[i]);
			return false;
		}
	}
	return true;
}

static struct plugin *plugin_create(void)
{
	struct plugin *p = xcalloc(1, sizeof(*p));
	p->global = true;
	p->has_provider = false;
	p->loaded = false;
	p->deps_satisfied = false;
	p->depends = NULL;
	p->depends_count = 0;
	wl_list_init(&p->actions);
	return p;
}

struct plugin_action *plugin_action_create(void)
{
	struct plugin_action *a = xcalloc(1, sizeof(*a));
	a->type = ACTION_TYPE_EXEC;
	a->on_select = ON_SELECT_EXEC;
	a->format = FORMAT_LINES;
	return a;
}

void plugin_action_destroy(struct plugin_action *action)
{
	free(action);
}

static struct plugin *parse_toml_file(const char *path)
{
	FILE *fp = fopen(path, "r");
	if (!fp) {
		log_error("Failed to open plugin file: %s\n", path);
		return NULL;
	}
	
	struct plugin *plugin = plugin_create();
	struct plugin_action *current_action = NULL;
	char line[MAX_LINE_LEN];
	
	while (fgets(line, sizeof(line), fp)) {
		char *trimmed = trim(line);
		
		if (*trimmed == '\0' || *trimmed == '#') continue;
		
		if (strcmp(trimmed, "[[action]]") == 0) {
			current_action = plugin_action_create();
			wl_list_insert(&plugin->actions, &current_action->link);
			continue;
		}
		
		char *eq = strchr(trimmed, '=');
		if (!eq) continue;
		
		*eq = '\0';
		char *key = trim(trimmed);
		char *value = trim(eq + 1);
		
		if (current_action) {
			if (strcmp(key, "label") == 0) {
				snprintf(current_action->label, sizeof(current_action->label), "%s", parse_string_value(value));
			} else if (strcmp(key, "display_prefix") == 0) {
				snprintf(current_action->display_prefix, sizeof(current_action->display_prefix), "%s", parse_string_value(value));
			} else if (strcmp(key, "type") == 0) {
				current_action->type = parse_action_type(parse_string_value(value));
			} else if (strcmp(key, "prompt") == 0) {
				snprintf(current_action->prompt, sizeof(current_action->prompt), "%s", parse_string_value(value));
			} else if (strcmp(key, "exec") == 0) {
				snprintf(current_action->exec, sizeof(current_action->exec), "%s", parse_string_value(value));
			} else if (strcmp(key, "list_cmd") == 0) {
				snprintf(current_action->list_cmd, sizeof(current_action->list_cmd), "%s", parse_string_value(value));
			} else if (strcmp(key, "format") == 0) {
				current_action->format = parse_format(parse_string_value(value));
			} else if (strcmp(key, "on_select") == 0) {
				current_action->on_select = parse_on_select(parse_string_value(value));
			} else if (strcmp(key, "label_field") == 0) {
				snprintf(current_action->label_field, sizeof(current_action->label_field), "%s", parse_string_value(value));
			} else if (strcmp(key, "value_field") == 0) {
				snprintf(current_action->value_field, sizeof(current_action->value_field), "%s", parse_string_value(value));
			} else if (strcmp(key, "plugin") == 0) {
				snprintf(current_action->plugin_ref, sizeof(current_action->plugin_ref), "%s", parse_string_value(value));
			}
		} else {
			if (strcmp(key, "name") == 0) {
				snprintf(plugin->name, sizeof(plugin->name), "%s", parse_string_value(value));
			} else if (strcmp(key, "display_prefix") == 0) {
				snprintf(plugin->display_prefix, sizeof(plugin->display_prefix), "%s", parse_string_value(value));
			} else if (strcmp(key, "context_name") == 0) {
				snprintf(plugin->context_name, sizeof(plugin->context_name), "%s", parse_string_value(value));
			} else if (strcmp(key, "global") == 0) {
				plugin->global = parse_bool_value(value);
			} else if (strcmp(key, "depends") == 0) {
				plugin->depends = xcalloc(MAX_ARRAY_ITEMS, sizeof(char *));
				plugin->depends_count = parse_string_array(value, plugin->depends, MAX_ARRAY_ITEMS);
			} else if (strcmp(key, "list_cmd") == 0) {
				snprintf(plugin->list_cmd, sizeof(plugin->list_cmd), "%s", parse_string_value(value));
				plugin->has_provider = true;
			} else if (strcmp(key, "format") == 0) {
				plugin->format = parse_format(parse_string_value(value));
			} else if (strcmp(key, "label_field") == 0) {
				snprintf(plugin->label_field, sizeof(plugin->label_field), "%s", parse_string_value(value));
			} else if (strcmp(key, "value_field") == 0) {
				snprintf(plugin->value_field, sizeof(plugin->value_field), "%s", parse_string_value(value));
			} else if (strcmp(key, "exec") == 0) {
				snprintf(plugin->exec, sizeof(plugin->exec), "%s", parse_string_value(value));
				plugin->has_provider = true;
			}
		}
	}
	
	fclose(fp);
	
	if (!plugin->name[0]) {
		log_error("Plugin missing name: %s\n", path);
		free(plugin);
		return NULL;
	}
	
	plugin->deps_satisfied = check_dependencies(plugin);
	plugin->loaded = true;
	
	return plugin;
}

void plugin_load_directory(const char *path)
{
	DIR *dir = opendir(path);
	if (!dir) {
		log_debug("Plugin directory not found: %s\n", path);
		return;
	}
	
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_type != DT_REG && entry->d_type != DT_LNK) continue;
		
		char *ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".toml") != 0) continue;
		
		char full_path[PLUGIN_PATH_MAX];
		snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
		
		struct plugin *plugin = parse_toml_file(full_path);
		if (plugin) {
			wl_list_insert(&plugins, &plugin->link);
			log_debug("Loaded plugin: %s (global=%s, deps=%s)\n",
				plugin->name,
				plugin->global ? "yes" : "no",
				plugin->deps_satisfied ? "ok" : "missing");
		}
	}
	
	closedir(dir);
}

struct plugin *plugin_get(const char *name)
{
	struct plugin *p;
	wl_list_for_each(p, &plugins, link) {
		if (strcmp(p->name, name) == 0) {
			return p;
		}
	}
	return NULL;
}

size_t plugin_count(void)
{
	size_t count = 0;
	struct plugin *p;
	wl_list_for_each(p, &plugins, link) {
		count++;
	}
	return count;
}

static char *run_command(const char *cmd)
{
	FILE *fp = popen(cmd, "r");
	if (!fp) {
		return NULL;
	}
	
	char *buffer = NULL;
	size_t buffer_size = 0;
	size_t total = 0;
	char chunk[1024];
	
	while (fgets(chunk, sizeof(chunk), fp)) {
		size_t chunk_len = strlen(chunk);
		char *new_buffer = realloc(buffer, buffer_size + chunk_len + 1);
		if (!new_buffer) {
			free(buffer);
			pclose(fp);
			return NULL;
		}
		buffer = new_buffer;
		memcpy(buffer + total, chunk, chunk_len);
		total += chunk_len;
		buffer_size += chunk_len;
	}
	
	pclose(fp);
	
	if (buffer) {
		buffer[total] = '\0';
	}
	
	return buffer;
}

void plugin_populate_results(struct wl_list *results, const char *filter)
{
	wl_list_init(results);
	
	struct plugin *p;
	wl_list_for_each(p, &plugins, link) {
		if (!p->global || !p->deps_satisfied || !p->has_provider) {
			continue;
		}
		
		char *output = run_command(p->list_cmd);
		if (!output) {
			continue;
		}
		
		if (p->format == FORMAT_LINES) {
			char *line = strtok(output, "\n");
			while (line) {
				char *trimmed = trim(line);
				if (*trimmed) {
					struct plugin_result *res = xcalloc(1, sizeof(*res));
					strncpy(res->label, trimmed, PLUGIN_LABEL_MAX - 1);
					strncpy(res->value, trimmed, PLUGIN_LABEL_MAX - 1);
					strncpy(res->exec, p->exec, PLUGIN_EXEC_MAX - 1);
					strncpy(res->plugin_name, p->name, PLUGIN_NAME_MAX - 1);
					wl_list_insert(results, &res->link);
				}
				line = strtok(NULL, "\n");
			}
		}
		
		free(output);
	}
}
