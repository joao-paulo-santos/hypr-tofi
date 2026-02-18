#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mode.h"
#include "compositor.h"
#include "log.h"
#include "xmalloc.h"

struct mode_config mode_config;

extern struct mode windows_mode;
extern struct mode workspaces_mode;

static struct mode *registered_modes = NULL;
static int registered_modes_count = 0;
static int registered_modes_capacity = 0;

static void register_mode(struct mode *mode)
{
	if (registered_modes_count >= registered_modes_capacity) {
		int new_capacity = registered_modes_capacity == 0 ? 8 : registered_modes_capacity * 2;
		registered_modes = xrealloc(registered_modes, new_capacity * sizeof(struct mode));
		registered_modes_capacity = new_capacity;
	}
	registered_modes[registered_modes_count] = *mode;
	registered_modes_count++;
}

static void register_builtin_modes(void)
{
	static struct mode drun_mode = {
		.bit = MODE_BIT_DRUN,
		.name = "drun",
		.dep_binary = NULL,
		.default_display_prefix = "launch",
		.check_deps = drun_mode_check_deps,
		.populate = drun_mode_populate,
		.execute = drun_mode_execute,
	};
	register_mode(&drun_mode);
	register_mode(&windows_mode);
	register_mode(&workspaces_mode);
}

void mode_config_init(void)
{
	mode_config_set_defaults();
	register_builtin_modes();
}

void mode_config_set_defaults(void)
{
	mode_config.enabled_modes = MODE_BIT_ALL;
	mode_config.show_display_prefixes = true;
	mode_config.calc_debounce_ms = 400;
	mode_config.calc_history = true;

	strcpy(mode_config.compositor, "auto");
	strcpy(mode_config.display_prefix_drun, "Launch");
	strcpy(mode_config.display_prefix_windows, "Show");
	strcpy(mode_config.display_prefix_workspaces, "Workspace");
	strcpy(mode_config.display_prefix_tmux_fridge, "Fridge");
	strcpy(mode_config.display_prefix_tmux_attach, "Tmux");
	strcpy(mode_config.display_prefix_prompt, "Ask");
	strcpy(mode_config.display_prefix_calc, "Calc");
	strcpy(mode_config.display_prefix_url, "Open");

	strcpy(mode_config.prefix_math, "=");
	strcpy(mode_config.prefix_prompt, "?");

	strcpy(mode_config.prompt_command, "opencode run");

	const char *home = getenv("HOME");
	if (home) {
		snprintf(mode_config.tmux_fridge_dir, sizeof(mode_config.tmux_fridge_dir),
			"%s/.config/tmux/tmux-workspaces", home);
	} else {
		strcpy(mode_config.tmux_fridge_dir, "");
	}
}

uint32_t mode_parse_modes_string(const char *str)
{
	if (!str || !*str) {
		return MODE_BIT_DRUN;
	}

	if (strcmp(str, "all") == 0) {
		return MODE_BIT_ALL;
	}

	uint32_t modes = 0;
	bool excluding = false;

	char *copy = xstrdup(str);
	char *saveptr = NULL;
	char *token = strtok_r(copy, ",", &saveptr);

	while (token != NULL) {
		while (*token == ' ') token++;

		bool is_exclude = (token[0] == '-');
		if (is_exclude) {
			token++;
			excluding = true;
		}

		if (strcmp(token, "all") == 0) {
			if (is_exclude) {
				modes = 0;
			} else {
				modes = MODE_BIT_ALL;
			}
		} else if (strcmp(token, "drun") == 0) {
			if (is_exclude) modes &= ~MODE_BIT_DRUN;
			else modes |= MODE_BIT_DRUN;
		} else if (strcmp(token, "windows") == 0) {
			if (is_exclude) modes &= ~MODE_BIT_WINDOWS;
			else modes |= MODE_BIT_WINDOWS;
		} else if (strcmp(token, "workspaces") == 0) {
			if (is_exclude) modes &= ~MODE_BIT_WORKSPACES;
			else modes |= MODE_BIT_WORKSPACES;
		} else if (strcmp(token, "tmux-fridge") == 0) {
			if (is_exclude) modes &= ~MODE_BIT_TMUX_FRIDGE;
			else modes |= MODE_BIT_TMUX_FRIDGE;
		} else if (strcmp(token, "tmux-attach") == 0) {
			if (is_exclude) modes &= ~MODE_BIT_TMUX_ATTACH;
			else modes |= MODE_BIT_TMUX_ATTACH;
		} else if (strcmp(token, "prompt") == 0) {
			if (is_exclude) modes &= ~MODE_BIT_PROMPT;
			else modes |= MODE_BIT_PROMPT;
		} else if (strcmp(token, "url") == 0) {
			if (is_exclude) modes &= ~MODE_BIT_URL;
			else modes |= MODE_BIT_URL;
		} else {
			log_error("Unknown mode: %s\n", token);
		}

		token = strtok_r(NULL, ",", &saveptr);
	}

	free(copy);

	if (modes == 0 && !excluding) {
		modes = MODE_BIT_DRUN;
	}

	return modes;
}

bool mode_check_deps(uint32_t mode_bit)
{
	for (int i = 0; i < registered_modes_count; i++) {
		if (registered_modes[i].bit == mode_bit) {
			if (registered_modes[i].check_deps) {
				return registered_modes[i].check_deps();
			}
			return true;
		}
	}

	if (mode_bit == MODE_BIT_DRUN) {
		return true;
	}

	return false;
}

void mode_populate_results(struct wl_list *results, const char *input, uint32_t enabled_modes)
{
	wl_list_init(results);

	for (int i = 0; i < registered_modes_count; i++) {
		if (enabled_modes & registered_modes[i].bit) {
			if (registered_modes[i].check_deps == NULL || registered_modes[i].check_deps()) {
				registered_modes[i].populate(results, input);
			}
		}
	}
}

void mode_execute_result(struct result *result)
{
	if (!result || !result->info[0]) {
		return;
	}

	if (strncmp(result->info, "APP:", 4) == 0) {
		drun_mode_execute(result->info + 4);
		return;
	}

	for (int i = 0; i < registered_modes_count; i++) {
		if (registered_modes[i].bit == result->mode_bit && registered_modes[i].execute) {
			registered_modes[i].execute(result->info);
			return;
		}
	}

	log_error("Unknown result type: %s\n", result->info);
}

const char *mode_get_display_prefix(uint32_t mode_bit)
{
	if (!mode_config.show_display_prefixes) {
		return "";
	}

	switch (mode_bit) {
		case MODE_BIT_DRUN: return mode_config.display_prefix_drun;
		case MODE_BIT_WINDOWS: return mode_config.display_prefix_windows;
		case MODE_BIT_WORKSPACES: return mode_config.display_prefix_workspaces;
		case MODE_BIT_TMUX_FRIDGE: return mode_config.display_prefix_tmux_fridge;
		case MODE_BIT_TMUX_ATTACH: return mode_config.display_prefix_tmux_attach;
		case MODE_BIT_PROMPT: return mode_config.display_prefix_prompt;
		case MODE_BIT_URL: return mode_config.display_prefix_url;
		default:
			if (mode_bit == 0 && mode_config.display_prefix_calc[0]) {
				return mode_config.display_prefix_calc;
			}
			return "";
	}
}

struct result *result_create(const char *label, const char *info, const char *icon, int priority, uint32_t mode_bit)
{
	struct result *result = xcalloc(1, sizeof(*result));

	strncpy(result->label, label ? label : "", MAX_RESULT_LABEL_LEN - 1);
	result->label[MAX_RESULT_LABEL_LEN - 1] = '\0';

	strncpy(result->info, info ? info : "", MAX_RESULT_INFO_LEN - 1);
	result->info[MAX_RESULT_INFO_LEN - 1] = '\0';

	strncpy(result->icon, icon ? icon : "", MAX_RESULT_LABEL_LEN - 1);
	result->icon[MAX_RESULT_LABEL_LEN - 1] = '\0';

	result->priority = priority;
	result->mode_bit = mode_bit;

	return result;
}

void result_destroy(struct result *result)
{
	if (result) {
		free(result);
	}
}

void results_destroy(struct wl_list *results)
{
	struct result *result, *tmp;
	wl_list_for_each_safe(result, tmp, results, link) {
		wl_list_remove(&result->link);
		result_destroy(result);
	}
}

__attribute__((unused))
static bool check_binary_exists(const char *name)
{
	if (!name || !*name) {
		return false;
	}

	char *path = getenv("PATH");
	if (!path) {
		return false;
	}

	char *path_copy = xstrdup(path);
	char *saveptr = NULL;
	char *dir = strtok_r(path_copy, ":", &saveptr);

	while (dir != NULL) {
		char full_path[512];
		snprintf(full_path, sizeof(full_path), "%s/%s", dir, name);

		if (access(full_path, X_OK) == 0) {
			free(path_copy);
			return true;
		}

		dir = strtok_r(NULL, ":", &saveptr);
	}

	free(path_copy);
	return false;
}

bool drun_mode_check_deps(void)
{
	return true;
}

void drun_mode_populate(struct wl_list *results, const char *input)
{
	(void)results;
	(void)input;
}

void drun_mode_execute(const char *info)
{
	if (!info || !*info) {
		return;
	}

	extern void drun_launch(const char *filename);
	drun_launch(info);
}
