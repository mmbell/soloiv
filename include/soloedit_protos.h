/*
 * soloedit_protos.h -- prototypes for the soloedit library (the editor/
 * sources).
 *
 * Convention: as s2_protos.h describes.  editor/ has not been converted
 * yet, so for now this holds only the editor functions that perusal/
 * calls; it is to be completed when editor/ is converted to prototypes.
 * GTK-free, like the other libraries' headers.
 */

#ifndef INCsoloedit_protosh
#define INCsoloedit_protosh

struct bnd_point_mgmt;
struct boundary_stuff;
struct one_boundary;
struct sed_command_files;
struct solo_click_info;
struct solo_edit_stuff;
struct solo_frame_state_files;
struct solo_list_mgmt;
struct swp_file_input_control;
struct ui_command;

/* se_bnd.c */
struct boundary_stuff *return_se_bnd_struct(void);
void xse_add_bnd_pt(struct solo_click_info *sci, struct one_boundary *ob);
void se_clr_all_bnds(void);
void se_clr_bnd(struct one_boundary *ob);
void se_clr_current_bnd(void);
void se_cycle_bnd(void);
void se_draw_bnd(struct bnd_point_mgmt *bpm, int num, int erase);
void se_erase_all_bnds(void);
int xse_find_intxns(double angle, double range, struct one_boundary *ob);
struct one_boundary *se_malloc_one_bnd(void);
void se_prev_bnd_set(void);
void se_redraw_all_bnds(void);
void se_redraw_all_bnds_for_frame(int frame_num);
void se_unpack_bnd_buf(char *buf, int size);
struct bnd_point_mgmt *se_zap_last_bpm(struct bnd_point_mgmt **top_bpm);
char *absorb_zmap_bnd(char *ifile, int skip, int *nbytes);
int se_rain_gauge_info(double dlat, double dlon);

/* se_proc_data.c */
int se_process_data(int arg, struct ui_command *cmds, int time_series, int automatic, int down, double d_ctr, int frame_num);
int se_really_readin_cmds(void);

/* se_utils.c */
void se_fix_comment(char *comment);
struct sed_command_files *se_return_cmd_files_struct(void);
struct solo_frame_state_files *se_return_state_files_struct(void);
char *se_string_time(double t, char *str);
struct solo_click_info *clear_click_info(void);
struct solo_edit_stuff *return_sed_stuff(void);
struct swp_file_input_control *return_swp_fi_in_cntrl(void);

/* se_wgt_stf.c */
void se_process_click(struct solo_click_info *sci);
struct solo_list_mgmt *se_update_list(int list_id);
int se_nab_files(const char *dir, struct solo_list_mgmt *slm, char *type, int nt);

/* sxm_examine.c */
void sxm_click_in_data(struct solo_click_info *sci);
void sxm_close_log_stream(void);
void sxm_flush_log_stream(void);
void sxm_set_log_dir(const char *dir);
void sxm_toggle_log_dir(int active);
void sxm_process_click(struct solo_click_info *sci);
void sxm_refresh_list(struct solo_click_info *sci);
void sxm_set_click_frame(int frme, double theta, double range);
void sxm_update_examine_data(int frme, int wgt_btn);

#endif /* INCsoloedit_protosh */
