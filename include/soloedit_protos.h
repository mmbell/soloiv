/*
 * soloedit_protos.h -- prototypes for the soloedit library (the editor/
 * sources).
 *
 * Convention: as s2_protos.h describes.  Every non-static function
 * defined in an editor/ source file has its prototype here, grouped by
 * source file; when you add or change one, change it here too.  None
 * of them needs GTK/GLib types, so this header stays GTK-free like the
 * other libraries' headers: structs are forward-declared below, and
 * FILE comes from <stdio.h>.
 *
 * The lists were generated from the definitions with cproto (cproto -E
 * "clang -E" -I include -I translate -I editor -DUNIX -DLITTLENDIAN
 * -DNETCDF -DSOLOIV_IO_BACKEND_RADX -DSOLOIV_DORADE_VIA_RADX
 * `pkg-config --cflags gtk4` file.c) and are checked by the compiler:
 * soloedit builds with -Werror=strict-prototypes and
 * -Werror=implicit-function-declaration, and every editor source
 * includes this header, so a stale entry is a conflicting-types error.
 */

#ifndef INCsoloedit_protosh
#define INCsoloedit_protosh

# include <stdio.h>

struct bnd_point_mgmt;
struct boundary_stuff;
struct dd_general_info;
struct one_boundary;
struct point_in_space;
struct se_changez;
struct sed_command_files;
struct solo_click_info;
struct solo_edit_stuff;
struct solo_frame_state_files;
struct solo_list_mgmt;
struct solo_str_mgmt;
struct swp_file_input_control;
struct ui_cmd_mgmt;
struct ui_command;
struct zmap_points;

/* se_bnd.c */
struct boundary_stuff *return_se_bnd_struct(void);
int xse_absorb_bnd(void);
void xse_add_bnd_pt(struct solo_click_info *sci, struct one_boundary *ob);
void se_append_bpm(struct bnd_point_mgmt **top_bpm, struct bnd_point_mgmt *bpm);
void se_ascii_bnd(FILE *stream, int num, struct point_in_space *pisp);
void se_bnd_pt_atts(struct bnd_point_mgmt *bpm);
int xse_ccw(double x0, double y0, double x1, double y1);
int se_ccw(long x0, long y0, long x1, long y1);
void se_clr_all_bnds(void);
void se_clr_bnd(struct one_boundary *ob);
void se_clr_current_bnd(void);
void se_cycle_bnd(void);
void se_delete_bnd_pt(struct bnd_point_mgmt *bpm);
void se_draw_bnd_for_frame(int frame_num, struct bnd_point_mgmt *bpm, int num, int erase);
void se_draw_bnd(struct bnd_point_mgmt *bpm, int num, int erase);
void se_erase_all_bnds(void);
void se_ts_find_intxns(double radar_alt, double d_max_range, struct one_boundary *ob, double d_time, double d_pointing, int automatic, int down, double d_ctr);
int se_merge_intxn(struct bnd_point_mgmt *bpm, struct one_boundary *ob);
int xse_find_intxns(double angle, double range, struct one_boundary *ob);
struct one_boundary *se_malloc_one_bnd(void);
void se_mid_insert(struct bnd_point_mgmt *bpm, struct one_boundary *ob);
void se_nab_segment(int num, double *r0, double *r1, struct one_boundary *ob);
int xse_num_segments(struct one_boundary *ob);
int se_compare_bnds(char *aa, char *bb, int size);
void se_pack_bnd_set(char *buf);
struct bnd_point_mgmt *se_pop_bpms(void);
void se_prev_bnd_set(void);
void se_push_all_bpms(struct bnd_point_mgmt **bpmptr);
void se_push_bpm(struct bnd_point_mgmt *bpm);
int se_radar_inside_bnd(struct one_boundary *ob);
void se_redraw_all_bnds(void);
void se_redraw_all_bnds_for_frame(int frame_num);
struct one_boundary *se_return_next_bnd(void);
void xse_save_bnd(void);
int xse_set_intxn(double x, double y, double slope, struct bnd_point_mgmt *bpm, struct one_boundary *ob);
void se_shift_bnd(struct one_boundary *ob, struct point_in_space *boundary_radar, struct point_in_space *current_radar, int scan_mode, double current_tilt);
int se_sizeof_bnd_set(void);
void se_unpack_bnd_buf(char *buf, int size);
int se_use_bnd(int arg, struct ui_command *cmds);
void xse_x_insert(struct bnd_point_mgmt *bpm, struct one_boundary *ob);
void xse_y_insert(struct bnd_point_mgmt *bpm, struct one_boundary *ob);
struct bnd_point_mgmt *se_zap_last_bpm(struct bnd_point_mgmt **top_bpm);
char *absorb_zmap_bnd(char *ifile, int skip, int *nbytes);
int absorb_zmap_pts(struct zmap_points **top, char *ifile);
int se_rain_gauge_info(double dlat, double dlon);
int list_zmap_values(char *fname, int frame);

/* se_wgt_stf.c */
void se_process_click(struct solo_click_info *sci);
struct solo_list_mgmt *se_update_list(int list_id);
int se_nab_files(const char *dir, struct solo_list_mgmt *slm, char *type, int nt);
int se_nab_all_files(char *dir, struct solo_list_mgmt *slm);

/* se_catch_all.c */
int se_readin_cmds(int arg, struct ui_command *cmds);
int se_establish_field(struct dd_general_info *dgi, char *src_name, char *dst_name, int *fns, int *fnd);
int se_once_only(int arg, struct ui_command *cmds);
int se_dir(int arg, struct ui_command *cmds);
int se_version_change(char *dir, char *radar);

/* se_utils.c */
int se_cmd_tokens(char *line, struct ui_cmd_mgmt *ucm);
struct ui_cmd_mgmt *ok_cmd(char *line, struct ui_cmd_mgmt *the_ucm);
int se_absorb_strings(char *path_name, struct solo_str_mgmt **top_ssm);
void se_util_dummy(void);
void se_fix_comment(char *comment);
int se_crack_ed_file_name(char *name, char *file, char *comment);
void se_crack_ed_path_name(char *name, char *dir, char *file, char *comment);
struct sed_command_files *se_return_cmd_files_struct(void);
struct solo_frame_state_files *se_return_state_files_struct(void);
char *se_string_time(double t, char *str);
struct solo_click_info *clear_click_info(void);
char *fgetz(char *aa, int nn, FILE *stream);
struct solo_click_info *return_solo_click_ptr(void);
int se_malloc_flagged_arrays(struct solo_edit_stuff *seds, int max_gates);
struct ui_cmd_mgmt *malloc_cmd_mgmt(void);
struct solo_edit_stuff *return_sed_stuff(void);
struct swp_file_input_control *return_swp_fi_in_cntrl(void);
void se_all_fer_cmds(struct solo_list_mgmt *which);
void se_all_input_commands(struct solo_list_mgmt *which);
void se_all_other_commands(struct solo_list_mgmt *which);
void se_append_string(struct solo_str_mgmt **top_ssm, struct solo_str_mgmt *ssm);
struct solo_str_mgmt *se_bracket_procedure(struct solo_str_mgmt **top_ssm, char *name, int *ncmds, int *item_num);
struct solo_str_mgmt *se_clone_ssm(struct solo_str_mgmt *ssm);
void se_insert_ssm_string(struct solo_str_mgmt **top_ssm, int item, struct solo_str_mgmt *ssm);
struct solo_str_mgmt *se_pop_spair_string(void);
void se_print_ssm_strings(struct solo_str_mgmt *top_ssm);
struct solo_str_mgmt *se_pull_ssm_string(struct solo_str_mgmt **top_ssm, struct solo_str_mgmt *ssmx);
void se_push_all_ssms(struct solo_str_mgmt **ssmptr);
void se_push_spair_string(struct solo_str_mgmt *ssm);
struct solo_str_mgmt *se_remove_ssm_string(struct solo_str_mgmt **top_ssm, int item);
void se_replace_prcdr_items(struct solo_str_mgmt **ssma, struct solo_str_mgmt **ssmb);
int se_replace_ssm_string(struct solo_str_mgmt **top_ssm, int item, struct solo_str_mgmt *ssm);
char *se_unquote_string(char *uqs, char *qs);
void solo_print_list(struct solo_list_mgmt *slm);

/* se_for_each.c */
int se_for_each_ray(int arg, struct ui_command *cmds);
int se_header_value(int arg, struct ui_command *cmds);
int se_rewrite(int arg, struct ui_command *cmds);
int se_remove_field(int arg, struct ui_command *cmds);
int se_cpy_field(int arg, struct ui_command *cmds);
int se_funfold(int arg, struct ui_command *cmds);
int se_hard_zap(int arg, struct ui_command *cmds);
int se_threshold_field(int arg, struct ui_command *cmds);
int se_radial_shear(int arg, struct ui_command *cmds);

/* se_defrkl.c */
int se_rescale_field(int arg, struct ui_command *cmds);
int se_ring_zap(int arg, struct ui_command *cmds);
int se_fix_vortex_vels(int arg, struct ui_command *cmds);
int se_defrekl(int arg, struct ui_command *cmds);
int se_flag_freckles(int arg, struct ui_command *cmds);
int se_flag_glitches(int arg, struct ui_command *cmds);
int se_despeckle(int arg, struct ui_command *cmds);

/* se_histog.c */
int se_histo_output(void);
void se_histo_fin_irreg_areas(void);
void se_histo_fin_irreg_counts(void);
void se_histo_fin_areas(char *name);
void se_histo_fin_counts(void);
void se_histo_init(struct solo_edit_stuff *seds);
int se_histo_ray(int arg, struct ui_command *cmds);
int se_histog_setup(int arg, struct ui_command *cmds);
int se_xy_stuff(int arg, struct ui_command *cmds);

/* se_flag_ops.c */
int se_assert_bad_flags(int arg, struct ui_command *cmds);
int se_flagged_add(int arg, struct ui_command *cmds);
int se_bad_flags_logic(int arg, struct ui_command *cmds);
int se_clear_bad_flags(int arg, struct ui_command *cmds);
int se_copy_bad_flags(int arg, struct ui_command *cmds);
void se_do_clear_bad_flags_array(int nn);
int se_set_bad_flags(int arg, struct ui_command *cmds);

/* se_add_mult.c */
int se_absolute_value(int arg, struct ui_command *cmds);
int se_assign_value(int arg, struct ui_command *cmds);
int se_add_const(int arg, struct ui_command *cmds);
int se_add_fields(int arg, struct ui_command *cmds);
int se_mult_const(int arg, struct ui_command *cmds);
int se_mangle_field(int arg, struct ui_command *cmds);

/* se_proc_data.c */
int se_ucm_errlen(void);
int se_ucm_cmdlen(void);
void se_cull_setup_input(struct solo_str_mgmt **top_ssm);
void dd_edd(int time_series, int automatic, int down, double d_ctr);
void se_edit_summary(void);
int se_gen_sweep_list(void);
struct ui_cmd_mgmt *se_malloc_ucm(void);
int se_interpret_commands(struct solo_str_mgmt *first_ssm, struct ui_cmd_mgmt *first_ucm, int *cmds_count);
int se_text_to_ssms(char *txt, struct solo_str_mgmt **top_ssm);
int sii_cmd_tmplt_tokens(struct ui_cmd_mgmt *the_ucm);
int se_perform_cmds(struct ui_cmd_mgmt *the_ucm, int num_cmds);
int se_process_data(int arg, struct ui_command *cmds, int time_series, int automatic, int down, double d_ctr, int frame_num);
int se_really_readin_cmds(void);
void se_setup_input_cmds(void);
void se_shift_setup_inputs(struct solo_str_mgmt *ssm);
void se_tack_on_cmds(char **where, int *how_much);
double ts_pointing_angle(struct dd_general_info *dgi);
int se_write_sed_cmds(int arg, struct ui_command *cmds);

/* se_BB_unfold.c */
int alt_gecho(struct dd_general_info *dgi, double min_grad, int *zmax_cell);
int se_ac_surface_tweak(int arg, struct ui_command *cmds);
int se_BB_ac_unfold(int arg, struct ui_command *cmds);
int se_BB_setup(int arg, struct ui_command *cmds);
int se_remove_ac_motion(int arg, struct ui_command *cmds);
int se_remove_storm_motion(int arg, struct ui_command *cmds);

/* sxm_examine.c */
void sxm_apply_changez(int frme);
int sxm_append_to_log_stream(char *stuff, int nchar);
void sxm_change_cell_in_list(struct se_changez *chz, int data_row_num, double val);
void sxm_clear_changes(int frme);
void sxm_click_in_data(struct solo_click_info *sci);
void sxm_click_in_list(struct solo_click_info *sci);
void sxm_close_log_stream(void);
void sxm_flush_log_stream(void);
void sxm_set_log_dir(const char *dir);
void sxm_set_tmp_nyq_vel(double nyqvel);
void sxm_toggle_log_dir(int active);
void sxm_gen_all_files_list(int frme, struct solo_list_mgmt *slm);
void sxm_gen_delete_lists(struct solo_click_info *sci);
void sxm_get_widget_info(int frme);
void sxm_list_beams(int frme);
void sxm_list_descriptors(int frme);
void sxm_list_edit_hist(int frme);
void sxm_list_to_log(struct solo_list_mgmt *slm, int entry, int num);
void sxm_log_stat_line(struct dd_general_info *dgi);
void sxm_open_log_stream(void);
struct se_changez *sxm_pop_change(struct se_changez **topptr);
struct se_changez *sxm_pop_spair_change(void);
void sxm_print_list(struct solo_list_mgmt *slm);
void sxm_process_click(struct solo_click_info *sci);
void sxm_push_change(struct se_changez *chz, struct se_changez **topptr);
void sxm_push_spair_change(struct se_changez *chz);
void sxm_refresh_list(struct solo_click_info *sci);
struct se_changez *sxm_remove_change(struct se_changez **topptr, struct se_changez *this);
void sxm_scroll_list(struct solo_click_info *sci);
void sxm_set_click_frame(int frme, double theta, double range);
void sxm_stack_spair_change(struct se_changez **topptr);
void sxm_stat_line(struct dd_general_info *dgi, int verbosity, char *preamble, char *postamble, char *line);
void sxm_ui_cmd(int arg, struct ui_command *cmds, int key);
void sxm_undo_last(int frme);
void sxm_unlink_files_list(struct solo_list_mgmt *slm, char *dir, int frme);
int sxm_ray_list(int frme, int wgt_btn);
void sxm_update_examine_data(int frme, int wgt_btn);

#endif /* INCsoloedit_protosh */
