#ifndef MODE_H
#define MODE_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-util.h>

#define MAX_MODE_NAME_LEN 32
#define MAX_DISPLAY_PREFIX_LEN 16
#define MAX_TRIGGER_PREFIX_LEN 16
#define MAX_RESULT_INFO_LEN 512
#define MAX_RESULT_LABEL_LEN 256

#define MODE_BIT_DRUN         (1 << 0)
#define MODE_BIT_HYPRWIN      (1 << 1)
#define MODE_BIT_HYPRWS       (1 << 2)
#define MODE_BIT_TMUX_FRIDGE  (1 << 3)
#define MODE_BIT_TMUX_ATTACH  (1 << 4)
#define MODE_BIT_PROMPT       (1 << 5)
#define MODE_BIT_URL          (1 << 6)
#define MODE_BIT_ALL          0x7F

struct result {
	struct wl_list link;
	char label[MAX_RESULT_LABEL_LEN];
	char info[MAX_RESULT_INFO_LEN];
	char icon[MAX_RESULT_LABEL_LEN];
	int priority;
	uint32_t mode_bit;
};

struct mode {
	uint32_t bit;
	const char *name;
	const char *dep_binary;
	const char *default_display_prefix;
	bool (*check_deps)(void);
	void (*populate)(struct wl_list *results, const char *input);
	void (*execute)(const char *info);
};

struct mode_config {
	uint32_t enabled_modes;
	char display_prefix_drun[MAX_DISPLAY_PREFIX_LEN];
	char display_prefix_hyprwin[MAX_DISPLAY_PREFIX_LEN];
	char display_prefix_hyprws[MAX_DISPLAY_PREFIX_LEN];
	char display_prefix_tmux_fridge[MAX_DISPLAY_PREFIX_LEN];
	char display_prefix_tmux_attach[MAX_DISPLAY_PREFIX_LEN];
	char display_prefix_prompt[MAX_DISPLAY_PREFIX_LEN];
	char display_prefix_calc[MAX_DISPLAY_PREFIX_LEN];
	char display_prefix_url[MAX_DISPLAY_PREFIX_LEN];
	char prefix_math[MAX_TRIGGER_PREFIX_LEN];
	char prefix_prompt[MAX_TRIGGER_PREFIX_LEN];
	bool show_display_prefixes;
	uint32_t calc_debounce_ms;
	bool calc_history;
	char prompt_command[256];
	char tmux_fridge_dir[256];
};

extern struct mode_config mode_config;

void mode_config_init(void);
void mode_config_set_defaults(void);

uint32_t mode_parse_modes_string(const char *str);
bool mode_check_deps(uint32_t mode_bit);
void mode_populate_results(struct wl_list *results, const char *input, uint32_t enabled_modes);
void mode_execute_result(struct result *result);
const char *mode_get_display_prefix(uint32_t mode_bit);

struct result *result_create(const char *label, const char *info, const char *icon, int priority, uint32_t mode_bit);
void result_destroy(struct result *result);
void results_destroy(struct wl_list *results);

bool drun_mode_check_deps(void);
void drun_mode_populate(struct wl_list *results, const char *input);
void drun_mode_execute(const char *info);

#endif
