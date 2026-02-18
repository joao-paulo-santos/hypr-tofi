#ifndef INPUT_H
#define INPUT_H

#include <xkbcommon/xkbcommon.h>
#include "tofi.h"

void input_handle_keypress(struct tofi *tofi, xkb_keycode_t keycode);
void input_scroll_up(struct tofi *tofi);
void input_scroll_down(struct tofi *tofi);
void input_refresh_results(struct tofi *tofi);
const char *get_calc_value(void);
const char *get_calc_expr(void);
void calc_mark_dirty(struct tofi *tofi);
bool calc_update_if_ready(struct tofi *tofi);
void calc_force_update(struct tofi *tofi);
void calc_add_to_history(void);
int get_calc_history_count(void);
const char *get_calc_history_entry(int index);
void calc_clear_history(void);
void clear_calc_input(struct tofi *tofi);

#endif /* INPUT_H */
