/*
 * soloiv_core_protos.h -- prototypes for the soloiv_core library
 * (the perusal/ sources) whose signatures need no GTK types.
 *
 * Convention (same as s2_protos.h, which describes it in full):
 *
 *   - Every non-static function defined in perusal/ has its prototype
 *     here or, if its signature uses GTK/GLib/cairo types, in
 *     perusal/soloiv_core_gtk_protos.h, grouped by source file.  New
 *     functions must be added to one of the two.  Exceptions:
 *     sii_log_handler.c's prototypes live in sii_log_handler.h, and
 *     the sii_param_widgets.c functions that take its private
 *     SiiPalette type are declared at the top of that file.
 *   - No K&R declarations ("int foo();"), at file scope or inside
 *     functions.
 *   - This header must stay GTK-free: translate/ and editor/ include
 *     it, and they are compiled without GTK types in scope.  Structs
 *     are forward-declared below; XColor and XImage (typedefs in
 *     solo_window_structs.h, which includes GTK) are named by their
 *     struct tags.  Functions that editor/ calls are written with plain C
 *     types (unsigned int, int, char) even where perusal's own code
 *     uses the GLib names for them.
 *
 * The lists were generated from the definitions (one prototype per
 * definition, parameter names kept) and are checked by the compiler:
 * soloiv_core builds with -Werror=strict-prototypes and
 * -Werror=implicit-function-declaration, and every perusal source
 * includes this header, so a stale entry is a conflicting-types error.
 */

#ifndef INCsoloiv_core_protosh
#define INCsoloiv_core_protosh

# include <stdio.h>

struct bnd_point_mgmt;
struct boundary_stuff;
struct centering_widget_info;
struct dd_general_info;
struct examine_widget_info;
struct landmark_widget_info;
struct line_segment;
struct linked_frames;
struct parameter_widget_info;
struct point_in_space;
struct sed_command_files;
struct solo_click_info;
struct solo_field_vals;
struct solo_frame_state_files;
struct solo_list_mgmt;
struct solo_palette;
struct solo_palette_ptr;
struct solo_perusal_info;
struct solo_window_ptrs;
struct sweep_widget_info;
struct swp_file_input_control;
struct ts_linked_frames;
struct ts_ray_info;
struct ui_command;
struct unique_sweepfile_info_v3;
struct view_widget_info;
struct xyras;

/* tagged in solo_window_structs.h so that they can be named here */
struct XColor;
struct XImage;

/* identical to the typedef in solo_window_structs.h (C11 allows the repeat) */
typedef struct solo_window_ptrs *WW_PTR;

/* soloii.c */
int sii_return_colors_filesel_wid(void);
void sii_initialization_widget(void);
void sii_image_gen_widget(void);
void sp_change_cursor(int normal);

/* sii_colors_stuff.c */
void sii_set_default_colors(void);
void set_all_X_named_colors(void);

/* sii_config_stuff.c */
void sii_check_def_widget_sizes(void);
void sii_init_frame_configs(void);
void sii_reset_config_cells(void);
void sii_new_frames(void);
void sii_apply_table_allocation(int width, int height);
void sii_schedule_table_allocation(int width, int height);

/* sii_edit_widgets.c */
void se_invalidate_read_caches(unsigned int frame_num);
int se_point_frame_at_written(unsigned int frame_num);
void se_undo_discard_top(void);
void sii_edit_zap_last_bnd_point(void);
int se_edit_start_stop(int frame_num, struct swp_file_input_control *sfic);

/* sii_utils.c */
void sii_message(const char *message);
void sii_dump_debug_stuff(void);
int sii_str_seek(char **sptrs, int count, const char *sought);
char *sii_glom_tokens(char *str, char **sptrs, int count, char *joint);

/* sii_param_widgets.c */
void param_set_cb_loc(int frame_num, int loc);
void sii_param_add_ctbl(const char **at, int nn);
int sii_param_dump_ctbls(FILE *stream);
void sii_param_dup_opal(struct solo_palette *opal);
void sii_param_set_plot_field(int frame_num, char *fname);
void solo_get_palette(char *name, int frame_num);
int solo_palette_color_numbers(int frame_num);

/* sii_view_widgets.c */
void sii_blanket_zoom_change(double zoom_factor);

/* sii_exam_widgets.c */
void se_dump_examine_widget(int frame_num, struct examine_widget_info *ewi);
void se_refresh_examine_list(int frame_num, struct solo_list_mgmt *slm);
void se_refresh_examine_widgets(int frame_num, struct solo_list_mgmt *slm);
void sii_exam_finalize_radio_state(void);

/* sii_links_widget.c */
void sii_set_links_from_solo_struct(int frame_num, int links_id, long *linked_windows);

/* sii_X_stuff.c */
int sii_png_image(char *fname);

/* solo2.c */
void sii_png_image_prep(char *dir);
int d_inSector(double ang, double ang1, double ang2);
int se_dump_sfic_widget(struct swp_file_input_control *sfic, int frame_num);
void sii_check_colors(void);
int sii_return_frame_count(void);
void sii_solo_set_filter(int frame_num, int set_it);
int sii_split_file_comment(const char *filename, float *fxd, char *scan_mode);
double sii_nearest_file_time(int frame_num, double target_time);
double sii_dtime_from_string(char *tstr);
const char *sii_string_from_dtime(double dtime);
double sii_range_in_pixels(int frame_num, double dist_km);
void print_fieldvalues(int frame_num, struct solo_list_mgmt *slm);
void popup_message(struct solo_list_mgmt *slm);
void yes_exit(void);
void sii_set_boundary_pt_count(int frame_num, int num_points);
void se_clear_segments(int frame_num);
void se_erase_segments(int frame_num, int num_points, struct bnd_point_mgmt *bpm);
void se_draw_segments(int frame_num, int num_points, struct bnd_point_mgmt *bpm);
void sp_do_lock_color(int frame);
void uii_printf(char *message, ...);
void solo_message(char *message, ...);
void solo_def_params(struct dd_general_info *dgi, int *list, int num_parms);
void solo_data_color_lut(int frme);

/* sp_clkd.c */
void solo_new_list_field_vals(int frme);
void solo_put_field_vals(struct dd_general_info *dgi, int param_num, double angle, double range, struct solo_field_vals *sfv, int state);
void sp_sweep_file_check(char *dir, char *file);
void sp_ts_seek_field_vals(int frme);
int sp_ts_ray_list(int frme, int xpos, int nr);
void sp_ts_data_click(int frme, int xpos, int ypos);
void sp_data_click(int frme, double x, double y);
void sp_locate_this_point(struct solo_click_info *sci, struct bnd_point_mgmt *bpm);
void sp_rtheta_screen_to_xyz(int frme, struct point_in_space *radar, struct point_in_space *pisp);
void sp_seek_field_vals(int frme);
void sp_xyz_to_rtheta_screen(int frme, struct point_in_space *radar, struct point_in_space *pisp);

/* sp_basics.c */
int solo_ensure_cell_capacity(WW_PTR wwptr, int ncells);
char *sp_ts_ascii_time(int frme, int xpos, char *atime);
void sp_ui_plot_cmds(int arg, struct ui_command *cmds);
int sp_ts_service(int arg, struct ui_command *cmds);
void solo_clear_busy_signal(void);
void solo_set_busy_signal(void);
int solo_busy(void);
void solo_return_radar_name(int frme, char *name);
struct point_in_space *solo_return_radar_pisp(int frme);
void solo_activate_frame(int dst_frme);
int solo_read_open(struct solo_window_ptrs *wwptr);
int solo_return_state(void);
struct solo_perusal_info *solo_return_winfo_ptr(void);
void sp_copy_tsri(struct ts_ray_info *a, struct ts_ray_info *b);
struct solo_window_ptrs *solo_return_wwptr(int ww_num);
void se_set_sfic_from_frame(int frme);
int solo_set_state(int new_state);
void solo_trigger_dir_rescans(char *dir);
void solo_update_active_info(void);
void solo_ww_top_line(int frme);
int sp_max_frames(void);

/* sp_accepts.c */
void sp_cpy_lmrk_info(int src_frme, int dst_frme);
void sp_cpy_ctr_info(int src_frme, int dst_frme);
void solo_cpy_lock_info(int src_frme, int dst_frme);
void solo_cpy_parameter_info(int src_frme, int dst_frme);
void solo_cpy_sweep_info(int src_frme, int dst_frme);
void solo_cpy_view_info(int src_frme, int dst_frme);
char *solo_gen_file_info(int frme);
int solo_set_lock_info(struct sweep_widget_info *swi);
int solo_set_parameter_info(struct parameter_widget_info *pwi);
int solo_set_sweep_info(struct sweep_widget_info *swi);
void sp_set_landmark_info(struct landmark_widget_info *lwi);
void sp_set_center_of_view_info(struct view_widget_info *pvi, struct centering_widget_info *cwi);
int solo_set_view_info(struct view_widget_info *pvi, struct landmark_widget_info *lwi, struct centering_widget_info *cwi);

/* sp_pals.c */
struct solo_palette_ptr *solo_palette_insert(struct solo_palette_ptr *this, struct solo_palette_ptr *here, int before);
void solo_free_palette(struct solo_palette_ptr *this);
void solo_gen_palette_list(void);
struct solo_palette_ptr *xsolo_get_palette(char *name, int frme);
void solo_init_palette(void);
struct solo_palette_ptr *solo_malloc_palette(char *name);
struct solo_palette *solo_malloc_old_palette(void);
void solo_merge_usual_parm(struct solo_palette_ptr *this, char *name);
int xsolo_palette_color_numbers(int frme);
void solo_palette_message(int frme);
struct solo_palette_ptr *solo_palette_remove(struct solo_palette_ptr *this);
struct solo_palette_ptr *solo_push_palette(struct solo_palette_ptr *this);
int solo_same_palette(struct solo_palette *this, struct solo_palette *that);
void solo_subscribe_palette(int frme, struct solo_palette_ptr *this, char *parameter_name);
void solo_check_zap_parms(char *name, struct solo_palette_ptr *except);
struct solo_palette_ptr *solo_this_palette(char *name, struct solo_palette_ptr *next);
struct solo_palette_ptr *solo_usual_parms(char *name, struct solo_palette_ptr *next);
struct solo_palette_ptr *solo_topof_palettes(void);
void solo_zap_usual_parm(struct solo_palette_ptr *this, char *name);

/* sii_perusal.c */
int displayq(int click_frme, int command);
void sp_nex_sweep_set(int ns, struct linked_frames *sweep_list[]);
int sp_data_loop(struct linked_frames *flink0);
int sp_diag_flag(void);
void sp_set_diag_flag(int flag);
void sp_replot_all(void);
int solo_worksheet(void);
int solo_nab_next_file(int frme, int file_action, int version, int sweep_skip, int replot);
int sp_nab_fixed_angle_from_fn(char *fn, double *fxd);
int solo_parameter_setup(int frme);
void sp_landmark_offsets(int frme);
void sp_align_center(int frme);

/* sii_xyraster.c */
int frme_intxns(struct xyras *rxy, struct line_segment *edge);
void init_xyraster(int frme);
int ray_raster_setup(int frme, double angle0, double angle1, struct xyras *rxy);
struct xyras *return_xyras(int frme);
double sp_meters_per_pixel(void);
void swap_edges(struct line_segment *edge0, struct line_segment *edge1);
void swap_ends(struct line_segment *edge);
void xx_inner_loop(struct xyras *rxy);
void yy_inner_loop(struct xyras *rxy);

/* sii_fixme.c */
void se_dump_bnd_files_widget(struct boundary_stuff *sebs);
void se_refresh_bnd_files_widget(struct boundary_stuff *sebs);
void se_dump_state_widget(struct solo_frame_state_files *sfsf);
void sp_refresh_delete_sweeps(void);
void se_dump_sed_files_widget(struct sed_command_files *scf);
void solo_refresh_list(int list_id, struct solo_list_mgmt *lm);
void se_refresh_sed_files_widget(struct sed_command_files *scf);
void se_refresh_sfic_widget(struct swp_file_input_control *sfic);
void ui_perform(char *aa);
void solo_refresh_command_buffer(struct solo_click_info *sci);
void solo_changed_parameter_info(int frame_num);
void config_from_frame_states_file(void);
void se_refresh_state_widget(struct solo_frame_state_files *sfsf);
void slm_list_ui(struct solo_list_mgmt *slm);

/* sp_save_frms.c */
int solo_absorb_window_info(const char *dir, const char *fname, int ignore_swpfi_info);
void solo_save_window_info(const char *dir, const char *a_comment);

/* sp_dorade.c */
void solo_cell_lut(int frme);
void solo_color_cells(int frme);

/* sp_colors.c */
int solo_absorb_color_table(int frme, char *dir, char *file);
void solo_color_bar_pixels(int frme, int xlen, int ylen, char *buf);
void xsolo_color_bar_pixels(struct XImage *cb_image, int height, int frme);
int solo_color_check(char *theColorName, struct XColor *theHardwareColor);
int xsolo_hardware_color_table(int frme, char *directory, char *filename);
int solo_pixelate_color(struct XColor *theHardwareColor, char *theColorName);

/* sp_lists.c */
void solo_gen_parameter_list(int frme);
void solo_gen_radar_list(int frme);
void solo_gen_swp_list(int frme);
int solo_get_files(char *dir, struct solo_list_mgmt *lm);
struct solo_list_mgmt *solo_return_radar_list(void);
struct solo_list_mgmt *solo_return_sweep_list(void);

/* sii_bscan.c */
int sp_ts_data_loop(struct ts_linked_frames *tsLink0, struct unique_sweepfile_info_v3 *usi);
void sp_ts_line(struct ts_linked_frames *tsl, int top_down);
void sp_ts_column(int frme, unsigned char *image, int x_pxl, int top_down);
int tsDisplay(int click_frme, int command);
void ts_ray_clone(unsigned char *aa, unsigned char *cc, int y_count, int frame_width, int nlines, int top_down, int right_left);
void sp_tsYpixel_info(struct ts_linked_frames *tsLink, int top_down);


#endif /* INCsoloiv_core_protosh */
