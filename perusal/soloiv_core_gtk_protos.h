/*
 * soloiv_core_gtk_protos.h -- prototypes for the soloiv_core library
 * (the perusal/ sources) whose signatures use GTK, GLib, cairo or
 * soloii.h types.  The rest are in include/soloiv_core_protos.h, which
 * this header includes; see that file for the convention.
 *
 * soloii.h includes this header at its end, so any source that
 * includes soloii.h sees every perusal prototype.
 */

# ifndef SOLOIV_CORE_GTK_PROTOS_H
# define SOLOIV_CORE_GTK_PROTOS_H

# include <gtk/gtk.h>
# include "soloii.h"
# include "sii_overlays.h"
# include <soloiv_core_protos.h>

struct bnd_point_mgmt;
struct solo_list_mgmt;

/* soloii.c */
void show_solo_init(GtkWidget *w, gpointer data);
void show_solo_image_gen(GtkWidget *w, gpointer data);
void sii_initialize_cb(GtkWidget *w, gpointer data);
void sii_main_text_click_cb(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void click_data_cb(GtkWidget *text, gpointer data);
void soloiv_activate(GtkApplication *app, gpointer user_data);
GActionGroup *soloiv_get_main_menu_actions(void);
void soloiv_build_main_menu(GtkWidget *window, GtkWidget *vbox);

/* sii_colors_stuff.c */
GdkRGBA *normalized_RGB_to_gdk(gfloat f_red, gfloat f_green, gfloat f_blue);
GdkRGBA *tone_down_gdk_color(gchar *name, gfloat atten);
GdkRGBA *sii_set_gtk_color(gchar *color_name, gfloat f_red, gfloat f_green, gfloat f_blue);
gboolean sii_print_str(gpointer a_str, gpointer nada);
void sii_print_hash_str(gpointer key, gpointer value, gpointer user_data);
void put_ascii_color_table(const gchar *name, gchar *table);

/* sii_config_stuff.c */
gboolean sii_set_config(void);

/* sii_callbacks.c */
void center_frame_cb(GtkWidget *w, gpointer data);
void sii_blow_up_draw_func(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
void sii_frame_draw_func(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
void sii_blow_up_resize_cb(GtkDrawingArea *frame, int width, int height, gpointer data);
void sii_frame_resize_cb(GtkDrawingArea *frame, int width, int height, gpointer data);
void sii_enter_cb(GtkEventControllerMotion *controller, double x, double y, gpointer data);
void sii_leave_cb(GtkEventControllerMotion *controller, gpointer data);
void sii_blow_up_mouse_click_cb(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void sii_mouse_click_cb(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
gboolean sii_blow_up_key_pressed_cb(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
gboolean sii_frame_key_pressed_cb(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
void custom_config_cb(GtkWidget *w, gpointer data);
void zoom_data_cb(GtkWidget *w, gpointer data);
void zoom_config_cb(GtkWidget *w, gpointer data);
void shape_cb(GtkWidget *w, gpointer data);
void config_cb(GtkWidget *w, gpointer data);
void test_event_cb(GtkWidget *label, gpointer data);
void test_widget(GtkWidget *w, gpointer data);
void dialog_a_cb(GtkWidget *w, gpointer data);
void checkup_cb(GtkWidget *w, gpointer data);
void image_cb(GtkWidget *w, gpointer data);
void sii_focus_in_cb(GtkEventControllerFocus *controller, gpointer data);
void sii_focus_out_cb(GtkEventControllerFocus *controller, gpointer data);
GtkWidget *sii_filesel(gint which_but, gchar *dirroot);

/* sii_edit_widgets.c */
gboolean se_undo_push(guint frame_num);
gboolean se_undo_pop_restore(void);
void sii_edit_return_start_stop(guint frame_num, gdouble *stt, gdouble *stp);
void sii_edit_reset_times(guint frame_num);
void sii_edit_menu_cb(GtkWidget *w, gpointer data);
void show_edit_cmd_help_widget(guint frame_num);
void show_edit_widget(GtkWidget *text, gpointer data);
void show_edit_files_widget(guint frame_num);
void sii_edit_click_in_data(gint frame_num);
void boundary_buttons_cb(GtkWidget *text, gpointer data);
void sii_connect_click(GtkWidget *widget, GCallback cb, gpointer data);
void edit_text_event_cb(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void sii_edit_help_event_cb(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void edit_file_text_event_cb(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void edit_files_widget(guint frame_num);
gboolean sii_edit_keyboard_event(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
void main_edit_widget(guint frame_num);
void sii_edit_menubar2(GtkWidget *window, GtkWidget **menubar, guint frame_num);
void edit_cmds_help_widget(guint frame_num);
gboolean set_edit_help_items(gpointer a_str, gpointer words, gpointer data);
void sii_cmds_help_event(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
void put_edit_cmds_help(gchar *name, GString *words);
void sii_edit_eg_split(guint frame_num, const gchar **eg, int nlines);

/* sii_utils.c */
void sii_append_debug_stuff(gchar *gch);
void sii_glom_strings(const gchar **cptrs, int nn, GString *gs);
int sii_absorb_zeb_map(gchar *path_name, siiOverlay *overlay);
guint sii_inc_master_seq_num(void);
guint sii_get_master_seq_num(void);
void sii_zap_quick_message(GtkWidget *w, gpointer data);
gchar *sii_set_gtktext_data(GSList *list);
gchar *sii_generic_gslist_insert(gpointer name, gpointer data);
void sii_set_widget_frame_num(GtkWidget *w, gpointer data);
gboolean sii_str_values(const gchar *line, guint nvals, gfloat *f1, gfloat *f2);
void sii_nullify_widget_cb(GtkWidget *widget, gpointer data);
GtkWidget *sii_get_widget_ptr(guint frame_num, gint window_id);
void sii_set_widget_ptr(guint frame_num, gint window_id, GtkWidget *widget);
gchar *sii_nab_line_from_text(const gchar *txt, guint position);
gboolean sii_nab_region_from_text(const gchar *txt, guint position, gint *start, gint *end);
SiiLinkedList *sii_ll_queue(SiiLinkedList **list, SiiLinkedList *link);
SiiLinkedList *sii_ll_dequeue(SiiLinkedList **list);
SiiLinkedList *sii_ll_push(SiiLinkedList **list, SiiLinkedList *link);
SiiLinkedList *sii_ll_remove(SiiLinkedList **list, SiiLinkedList *link);
SiiLinkedList *sii_ll_pop(SiiLinkedList **list);
SiiLinkedList *sii_init_linked_list(guint length);
SiiLinkedList *sii_ll_malloc_item(void);
SiiLinkedList *sii_init_circ_que(guint length);
SiiLinkedList *insert_clicks_que(SiiPoint *click);
gchar *sii_item_delims(void);
gchar *sii_set_string_from_vals(GString *gs, guint nvals, gfloat f1, gfloat f2, guint precision);
void sii_bad_entry_message(const gchar *ee, guint items);
GtkWidget *sii_create_menu_button(const gchar *label_text, GMenuModel *menu_model);
GtkWidget *sii_submenu(gchar *name, GtkWidget *mbar);
GtkWidget *sii_submenu_item(gchar *name, GtkWidget *submenu, guint widget_id, GCallback sigfun, guint frame_num);
GtkWidget *sii_toggle_submenu_item(gchar *name, GtkWidget *submenu, guint widget_id, GCallback sigfun, guint frame_num, guint radio_item, GSList **radio_group_ptr);

/* sii_frame_menu.c */
void sii_set_click_data_text(guint frame_num, GString *gs);
void frame_menu_cb(GtkWidget *frame, gpointer data);
void show_frame_menu(GtkWidget *text, gpointer data);
void show_all_click_data_widgets(gboolean really);
void show_click_data_widget(guint frame_num);
void frame_click_data_widget(guint frame_num);
void frame_menu_widget(guint frame_num);

/* sii_param_widgets.c */
void sii_param_colors_filesel(const gchar *str, GtkWidget *fs);
void param_entry_widget(guint frame_num, const gchar *prompt, const gchar *dirroot);
gboolean sii_param_absorb_ctbl(guint frame_num, const gchar *filename);
void param_text_event_cb(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
const gchar *set_cb_labeling_info(guint frame_num, gdouble *relative_locs);
void set_color_bar(guint frame_num, guint font_height);
void show_param_widget(GtkWidget *text, gpointer data);
GdkRGBA *sii_annotation_color(guint frame_num, gint exposed);
GdkRGBA *sii_background_color(guint frame_num, gint exposed);
GdkRGBA *sii_boundary_color(guint frame_num, gint exposed);
gboolean sii_color_tables_lister(gpointer name, gpointer value, gpointer data);
void sii_colorize_image(guint frame_num);
gboolean sii_colors_lister(gpointer name, gpointer value, gpointer data);
void sii_ctr_inc_from_min_max(guint ncolors, gfloat *ctr, gfloat *inc, gfloat *min, gfloat *max);
gchar *sii_default_palettes_list(void);
void sii_do_annotation(guint frame_num, gint exposed, gboolean blow_up, cairo_t *cr);
void sii_double_colorize_image(guint frame_num);
void sii_get_clip_rectangle(guint frame_num, Rectangle *clip, gboolean blow_up);
gint sii_get_ctr_and_inc(guint frame_num, float *ctr, float *inc);
gint sii_get_emph_min_max(guint frame_num, float *zmin, float *zmax);
GdkRGBA *sii_grid_color(guint frame_num, gint exposed);
void sii_initialize_parameter(guint frame_num, gchar *name);
void sii_min_max_from_ctr_inc(guint ncolors, gfloat *ctr, gfloat *inc, gfloat *min, gfloat *max);
SiiLinkedList *sii_new_palette_for_param(const gchar *p_name, const gchar *name);
gchar *sii_new_color_tables_list(void);
gchar *sii_new_colors_list(void);
gchar *sii_new_palettes_list(void);
SiiLinkedList *sii_palette_for_param(const gchar *name);
SiiLinkedList *sii_palette_seek(const gchar *p_name);
void sii_param_dup_entries(guint frame_num);
void sii_param_dup_orig(guint frame_num);
void sii_param_dup_pal(gpointer sii_pal, gpointer old_pal);
void sii_param_check_changes(guint frame_num);
void sii_param_entry_info(GtkWidget *w, guint wid, guint frame_num, guint entry_flag);
void sii_param_entry_paste_cb(GtkWidget *w, gpointer data);
gchar *sii_param_fix_color(gchar *old_color);
void sii_param_menu_cb(GtkWidget *w, gpointer data);
void sii_param_menubar2(GtkWidget *window, GtkWidget **menubar, guint frame_num);
const gchar *sii_param_palette_name(guint frame_num);
void sii_param_process_changes(guint frame_num);
void sii_param_reset_changes(guint frame_num);
gboolean sii_apply_color_table_by_name(guint frame_num, const gchar *name);
gboolean sii_palette_get_state(guint frame_num, gfloat minmax[2], gfloat ctrinc[2], guint *ncolors);
gboolean sii_param_widget_simulate_minmax(guint frame_num, gfloat min_val, gfloat max_val);
void sii_param_toggle_field(guint frame_num);
void sii_param_widget(guint frame_num);
void sii_redisplay_list(guint frame_num, guint widget_id);
void sii_reset_image(guint frame_num);
void sii_set_entries_from_palette(guint frame_num, const gchar *name, const gchar *p_name);
gboolean sii_set_param_info(guint frame_num);
gchar *sii_set_param_names_list(guint frame_num, GString *gs_list, guint count);
int solo_hardware_color_table(gint frame_num);
SiiLinkedList *sii_solo_palette_stack(void);
void sii_update_param_widget(guint frame_num);

/* sii_view_widgets.c */
gboolean sii_view_tic_info(guint frame_num, gdouble *xinc, gdouble *yinc, gboolean *xtics, gboolean *ytics, gboolean *annot);
void sii_default_center(gint frame_num);
void sii_center_on_clicks(gint num_clicks);
gboolean sii_set_view_info(guint frame_num);
void sii_update_linked_view_widgets(guint frame_num);
void sii_view_update_links(guint frame_num, int li_type);
void sii_update_view_info(guint frame_num);
void sii_update_view_widget(guint frame_num);
gint sii_radio_inventory(GtkWidget *rmi, gint *item_count);
void sii_initialize_view(guint frame_num);
void sii_view_dup_orig(guint frame_num);
void sii_view_entry_info(GtkWidget *w, guint wid, guint frame_num, guint entry_flag);
void sii_view_menu_cb(GtkWidget *w, gpointer data);
void sii_view_entry_paste_cb(GtkWidget *w, gpointer data);
void show_view_widget(GtkWidget *text, gpointer data);
void sii_view_widget(guint frame_num);
void sii_view_menubar2(GtkWidget *window, GtkWidget **menubar, guint frame_num);

/* sii_swpfi_widgets.c */
gboolean sii_set_swpfi_info(guint frame_num, gint sweep_num);
gint sii_return_filter_info(gint frame_num, char *scan_modes, float *fixed_angle, float *tolerance);
void sii_update_swpfi_widget(guint frame_num);
void show_swpfi_widget(GtkWidget *text, gpointer data);
void sii_swpfi_menu_cb(GtkWidget *w, gpointer data);
void sii_swpfi_entry_paste_cb(GtkWidget *w, gpointer data);
void sii_swpfi_list_widget(guint frame_num);
void sii_swpfi_widget(guint frame_num);
void sii_swpfi_menubar2(GtkWidget *window, GtkWidget **menubar, guint frame_num);

/* sii_exam_widgets.c */
void sii_exam_return_click_indices(guint frame_num, gint *list_index, gint *char_index);
void se_update_exam_widgets(gint frame_num);
void show_exam_widget(GtkWidget *text, gpointer data);
void sii_exam_list_event(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
gboolean sii_exam_keyboard_event(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
void sii_exam_click_in_data(guint frame_num);
void sii_exam_menu_cb(GtkWidget *w, gpointer data);
void sii_exam_entry_paste_cb(GtkWidget *w, gpointer data);
guint exam_list_cb(GtkWidget *label, gpointer data);
guint exam_text_event_cb(GtkWidget *text, GdkEvent *event, gpointer data);
void sii_exam_entry_info(GtkWidget *w, guint wid, guint frame_num, guint num_values);
void sii_exam_widget(guint frame_num);
void sii_exam_menubar2(GtkWidget *window, GtkWidget **menubar, guint frame_num);

/* sii_links_widget.c */
void sii_links_set_foreach(GtkWidget *button, gpointer data);
void sii_links_set_from_struct(GtkWidget *button, gpointer data);
void sii_links_get_foreach(GtkWidget *button, gpointer data);
LinksInfo *sii_new_links_info(gchar *name, guint frame_num, guint widget_id, gboolean this_frame_only);
void show_links_widget(LinksInfo *linkfo);
void sii_links_widget_cb(GtkWidget *w, gpointer data);
void sii_links_cb(GtkWidget *w, gpointer data);
void sii_links_widget(guint frame_num, guint widget_id, LinksInfo *linkfo);

/* solo2.c */
gboolean sii_reset_frame_sizes(void);
guint sii_frame_sync_num(guint frame_num);
guint sii_config_sync_num(guint frame_num);
void sii_set_debug_flag(gint flag);
gboolean sii_debug_flag(void);
gboolean sii_time_series(guint frame_num, gdouble *tstart, gdouble *tspan);
const gchar *sii_return_swpfi_label(int frame_num);
const GString *sii_return_config_files(const char *dir);
gboolean sii_nab_state_set(int frame_num, const char *dir, const char *filename);
gboolean sii_save_commands(int frame_num, const char *dir, const char *comment);
gboolean sii_save_boundary(int frame_num, const char *dir, const char *comment);
gboolean sii_nab_command_set(int frame_num, const char *dir, const char *filename);
gboolean sii_nab_boundary(int frame_num, const char *dir, const char *filename);
double sii_return_swpfi_time_stamp(guint frame_num);
const gchar *sii_return_swpfi_dir(guint frame_num);
const gchar *sii_return_swpfi_radar(guint frame_num);
const struct solo_list_mgmt *sii_return_swpfi_radar_list(guint frame_num);
const struct solo_list_mgmt *sii_return_swpfi_list(guint frame_num);
const GString *sii_return_edit_cmd_files(guint frame_num, const char *dir);
const GString *sii_return_edit_bnd_files(guint frame_num, const char *dir);
const GString *sii_return_llb_bnd_files(guint frame_num, const char *dir);
void sii_set_geo_coords(int frame_num, gdouble dx, gdouble dy);
void sp_cell_range_info(int frame_num, gfloat *r0, gfloat *gs, gint *ncells);
gfloat sp_nyquist_vel(int frame_num);
const GString *sii_slm_to_gs(struct solo_list_mgmt *slm);
int sii_radar_coords_to_frame_coords(int frame_num, gdouble xx_km, gdouble yy_km, gint *x, gint *y);
gdouble sii_tic1(gdouble tic, gdouble ticinc);
int sii_lmrk_coords_to_frame_coords(int frame_num, gdouble xx_km, gdouble yy_km, gint *x, gint *y);
void se_draw_segments_cairo(cairo_t *cr, int frame_num, int num_points, struct bnd_point_mgmt *bpm);
gboolean sii_blow_up(guint frame_num);
void sii_xfer_images(int frame_num, GdkRectangle *area);
void sii_really_xfer_images(int frame_num, GdkRectangle *area, gboolean blow_up);
void sii_really_xfer_images_cairo(cairo_t *cr, int frame_num, GdkRectangle *area, gboolean blow_up);
gboolean sii_lmrk_inside(guint frame_num);
gint sii_rng_ring_arcs(guint frame_num, gdouble range);
void sii_update_frame_info(guint frame_num);
void sii_check_image_size(guint frame_num);
void sii_plot_data2(guint frame_num, guint plot_function);
void sii_plot_data(guint frame_num, guint plot_function);
gboolean sii_batch(gpointer argu);
gchar *sii_get_swpfi_dir(gchar *dir);
int sii_default_startup(const gchar *swpfi_dir);

/* sii_gtk4_compat.c */
SiiFont *sii_font_new(const char *family, int size_points, gboolean monospace);
void sii_font_free(SiiFont *font);
gint sii_font_string_width(SiiFont *font, const gchar *text);
void sii_cairo_draw_text(cairo_t *cr, SiiFont *font, double x, double y, const gchar *text, int len);
void sii_cairo_set_color(cairo_t *cr, GdkRGBA *color);
void sii_widget_destroyed_cb(GtkWidget *widget, GtkWidget **widget_pointer);
GdkRGBA *sii_color_new(gfloat red, gfloat green, gfloat blue);


# endif /* SOLOIV_CORE_GTK_PROTOS_H */
