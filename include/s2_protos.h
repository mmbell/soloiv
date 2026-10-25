/*
 * s2_protos.h -- prototypes for the s2 library (the translate/ sources).
 *
 * Convention (applies to translate/, and to perusal/, editor/ and tests/
 * as they are converted):
 *
 *   - Every non-static function defined in a translate/ source file has its
 *     prototype here, grouped by source file.  When you add or change
 *     a function in translate/, add or change its prototype here too.
 *     Exceptions: radar_io.c's prototypes live in radar_io.h, and the
 *     piraq_dd.c functions whose parameters are piraq.h types are
 *     declared at the top of piraq_dd.c (nothing else calls them).
 *   - Functions defined in other libraries and called from translate/
 *     get their prototypes from that library's GTK-free header
 *     (soloiv_core_protos.h for perusal/, soloedit_protos.h for
 *     editor/), never from local declarations.  Each library's
 *     header follows this same layout.
 *   - No K&R declarations ("int foo();") anywhere, at file scope or
 *     inside functions.  Static functions get a full prototype near
 *     the top of their file if they are used before they are defined.
 *   - Prototypes take only GTK-free types; structs are forward-declared
 *     below so this header needs nothing but dd_defines.h, dd_time.h
 *     and the C library.  It also includes the C library headers that
 *     the old function_decl.h supplied (stdio, stdlib, string, unistd...);
 *     <math.h> is not among them, include it where it is needed.
 *
 * The s2 target builds with -Werror=deprecated-non-prototype,
 * -Werror=strict-prototypes and -Werror=implicit-function-declaration
 * (clang) or the GCC equivalents, so a missing prototype is a compile
 * error.  The lists below were generated from the definitions with
 * cproto (cproto -E "clang -E" -I include -I translate -DUNIX
 * -DLITTLENDIAN -DNETCDF -DSOLOIV_IO_BACKEND_RADX
 * -DSOLOIV_DORADE_VIA_RADX file.c) and can be checked the same way.
 *
 * function_decl.h and dgi_func_decl.h, the old K&R declaration
 * lists, have been removed; these headers replace them.
 */

#ifndef INCs2_protosh
#define INCs2_protosh

/* the C library headers the old function_decl.h supplied */
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <stdio.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <ctype.h>
# include <time.h>
# include <dd_defines.h>
# include <dd_time.h>

struct ads_data;
struct ads_raw_data_que;
struct catalog_info;
struct cell_d;
struct cfac_wrap;
struct comment_d;
struct correction_d;
struct d_limits;
struct d_time_struct;
struct dd_comm_info;
struct dd_file_name_v3;
struct dd_general_info;
struct dd_input_filters;
struct dd_input_sweepfiles_v3;
struct dd_radar_name_info_v3;
struct dd_ray_sector;
struct dd_stats;
struct ddir_info_v3;
struct dds_structs;
struct derived_fields;
struct field_radar_i;
struct generic_radar_info;
struct input_read_info;
struct io_packet;
struct letvar_info;
struct lidar_parameters;
struct mrd_production;
struct parameter_d;
struct platform_i;
struct point_in_space;
struct radar_angles;
struct radar_d;
struct ray_i;
struct rename_str;
struct rnc_info;
struct rot_ang_table;
struct rot_table_entry;
struct run_sum;
struct running_avg_que;
struct solo_list_mgmt;
struct super_SWIB;
struct sweepinfo_d;
struct time_dependent_fixes;
struct type5_stuff;
struct uf_production;
struct unique_sweepfile_info_v3;
struct ve_que_val;
struct volume_d;
struct wsrqc_stuff_ptrs;

/* identical to the typedef in dd_general_info.h (C11 allows the repeat) */
typedef struct dd_general_info *DGI_PTR;

/* gneric_dd.c */
void dd_celv_update(struct dds_structs *dds);
void dd_init_sweep(struct dd_general_info *dgi, int new_vol);
void dd_parms_update(struct dd_general_info *dgi);
void dd_radd_update(struct dds_structs *dds);
void dd_stuff_ray(void);
void dd_vold_update(struct dd_general_info *dgi);
struct generic_radar_info *return_gri_ptr(void);

/* sigm_dd.c */
void sigmet_dd_conv(void);
void sigmet_ini(void);
void sigmet_nab_data(void);
void sigmet_nab_info(void);
int sigmet_next_ray(void);
int sigmet_select_ray(void);
void sigmet_print_stat_line(int count);
int ingest_ingest_headers(char *tbuf);
void grab_summary_header(char *buf);
int sig_read(int fid, char *buf, int size);
short sig_swap2(void *vp);
long sig_swap4(fourb *ov);

/* gpro_data.c */
char *eld_nimbus_fix_asib(DD_TIME *dts, struct platform_i *asib, int options, int radar_num);
void ac_nc_nab_this_time_step(void);
void ac_nc_apply_fixes(DD_TIME *dts, struct platform_i *asib, int options, int ndx);
int ac_nc_find_tag(char *tag);
int ac_nc_init(void);
void eld_gpro_fix_asib(DD_TIME *dts, struct platform_i *asib, int options);
void gpro_assemble_vals(struct ads_raw_data_que *rdq, struct ads_data *adsd, double pct);
void setup_ltvi(void);
int gpro_mount_raw(void);
void gp_header(char *buf);
int is_gp_var(char *name);
int len_arg(char *a, char *b);
int gp_get_line(char *s);
void gp_date(char *s, int *year, int *mon, int *day);
float gp_time(char *s);
char *gp_arg_ptr(char *s, int nth);
double gpro_decode(struct letvar_info *this, double pct_samp, long *at);
char *gpro_next_rec(struct ads_raw_data_que *rdq, int *status);
void gpro_position_err(struct ads_data *adsd);
struct ads_raw_data_que *gpro_sync(double time);
struct ads_raw_data_que *gpro_initialize(struct ads_data **adsd_at);

/* stdhrd.c */
int hrd_ascii_read(void);
int hrd_merge_std(struct generic_radar_info *gri);
void hrd_std_init(void);
int hrd_sync_std(struct generic_radar_info *gri);
int hrd_sync_ascii(struct generic_radar_info *gri);
void hrd_dmp_std(struct type5_stuff *t5s, struct generic_radar_info *gri);
void hrd_ac_update_std(struct generic_radar_info *gri);
void type4_new(void);
void type5_new(void);
double cvt_hp1000fp(char *at);
int hrdstd_read(int fid, char *buf, int size, int io_type);

/* xwsrqc.c */
int wsrqc_correct(struct generic_radar_info *gri, char *name);
int wsrqc_init(struct generic_radar_info *gri);
struct wsrqc_stuff_ptrs *wsr_malloc(void);
int nab_wsrqc_stuff(FILE *stream, char *buf, struct wsrqc_stuff_ptrs *wsp, char *quit_at, int num_occurs);
int wsrqc_power_cor(struct generic_radar_info *gri, char *name);

/* piraq_dd.c */
struct input_read_info *return_pir_irq(void);
int pui_next_block(void);
void piraq_dd_conv(int interactive_mode);
int piraq_isa_new_vol(void);
int piraq_isa_new_sweep(void);
int piraq_select_ray(void);
void piraq_print_stat_line(int count);
void piraq_reset(void);
void piraq_name_replace(char *name, struct rename_str *top);
void piraq_name_aliases(char *aliases, struct rename_str **top);
double pc_swap4f(fourB *ov);
long pc_swap4(fourB *ov);
void piraq_positioning(void);
int piraq_inventory(struct input_read_info *irq);
void piraq_dump_this_ray(struct solo_list_mgmt *slm);
void piraq_print_headers(struct solo_list_mgmt *slm);
int piraq_ts_stats(void);
short snarf2(char *bytes);
long snarf4(char *bytes);
float snarf4f(char *bytes);

/* by_products.c */
void by_products(struct dd_general_info *dgi, time_t ztime);

/* dd_catalog.c */
void dd_append_cat_comment(char *comm_str);
void dd_enable_cat_comments(void);
void dd_catalog(struct dd_general_info *dgi, time_t ignored_time, int flush);
void cat_volume(struct dd_general_info *dgi, struct catalog_info *cii, time_t current_time, int finish);
void cat_sweep(struct dd_general_info *dgi, struct catalog_info *cii, time_t current_time, int finish);
char *cat_time(time_t time, char *dst);
char *cat_date(time_t time, char *dst);
void cat_cat_att(char *name, int num_args, char *arg, char *cat);
void cat_cat_att_args(char *name, int num_args, char *arglist[], char *cat);
char *reverse_string(char *str, char *rev);
char *c_deblank(char *a, int n, char *clean);
void ddcat_close(int fid);
int ddcat_open(char *file_name);
void ddcat_write(struct catalog_info *cii, char *buf, int size, int func);

/* gecho.c */
void dd_gecho(DGI_PTR dgi);
void dd_gecho_intxn(DGI_PTR dgi, int fid, double min_rot_angle, double max_rot_angle, double earth_radius, int min_gates, int min_gatex, double reasonable_gecho, char *gecho_refl, char *gecho_vel, int radar_num, char *gecho_ncp);

/* dorade_uf.c */
void produce_uf(struct dd_general_info *dgi);
int ufp_check_media(struct uf_production *ufp);
void ufp_init(struct dd_general_info *dgi, struct uf_production *ufp);
void ufp_stuff_ray(struct dd_general_info *dgi, struct uf_production *ufp);
void ufp_uf_stuff(struct dd_general_info *dgi, struct uf_production *ufp);
int ufp_write(struct uf_production *ufp, int size);

/* product_x.c */
void dd_product_x(DGI_PTR dgi);
void dd_px(DGI_PTR dgi, int fid, int rn);

/* dd_der_flds.c */
void dd_derived_fields(DGI_PTR dgi);
double dd_ac_vel(DGI_PTR dgi);
void dd_BB_unfold(DGI_PTR dgi, int fn, double ac_vel, int qsize, double f_fold_shear, struct ve_que_val *vqv);
void dd_denotch(DGI_PTR dgi, int fn, double f_notch_max, double f_notch_shear);
int dd_despeckle(DGI_PTR dgi, int fn, int a_speckle);
int dd_single_prf_vortex_vels(DGI_PTR dgi, int fn, char *dname);
int dd_fix_vortex_vels(DGI_PTR dgi, int fn, char *dname);
int dd_gp_threshold(DGI_PTR dgi, int fn, char *thr_name, double thr_val1, double thr_val2, int thr_type);
int dd_legal_pname(char *name);
void dd_lidar_db(DGI_PTR dgi, int fnd, struct lidar_parameters *lps);
int dd_new_param(DGI_PTR dgi, char *srs_name, char *dst_name);
void dd_nix_air_motion(DGI_PTR dgi, int fn, double ac_vel, int center_on_zero);
void dd_pct_stats(DGI_PTR dgi);
int dd_thr_parms(char *a, char *name, float *val);
int dder_lidar_opts(struct derived_fields *derfs, char *opts, char *dst_flds[], int num_dst);

/* nssl_mrd.c */
void dd_mrd_conv(void);
int dd_mrd_init(void);
void produce_nssl_mrd(struct dd_general_info *dgi, int flush);
void mrdp_write(struct mrd_production *mrdp, int header_only);
int mrd_find_param(struct dd_general_info *dgi, int find);

/* dd_swp_files.c */
void dd_set_unique_vol_id(void);
int dd_sizeof_prev_file(void);
void dd_dump_headers(struct dd_general_info *dgi);
void dd_dump_ray(struct dd_general_info *dgi);
void dd_flush(int flush_radar_num);
void dd_insert_headers(struct dd_general_info *dgi);
void dd_new_vol(struct dd_general_info *dgi);

/* dorade_tape.c */
void dd_tape(DGI_PTR dgi, int flush);

/* shane.c */
void dd_shane_conv(void);
void produce_shanes_data(DGI_PTR dgi);

/* dd_crackers.c */
void nssl_crack_head(void *srs, void *dst, int limit);
void nssl_crack_head2(void *srs, void *dst, int limit);
void se_crack_bdhd(void *srs, void *dst, int limit);
void se_crack_pisp(void *srs, void *dst, int limit);
void sp_crack_sxmn(void *srs, void *dst, int limit);
void sp_crack_slmk0(void *srs, void *dst, int limit);
void sp_crack_slmk(void *srs, void *dst, int limit);
void sp_crack_sctr0(void *srs, void *dst, int limit);
void sp_crack_sctr(void *srs, void *dst, int limit);
void sp_crack_sptl0(void *srs, void *dst, int limit);
void sp_crack_sptl(void *srs, void *dst, int limit);
void sp_crack_ssfi0(void *srs, void *dst, int limit);
void sp_crack_ssfi(void *srs, void *dst, int limit);
void sp_crack_ssfiLE(void *srs, void *dst, int limit);
void sp_crack_ssfiLE0(void *srs, void *dst, int limit);
void sp_crack_spmi0(void *srs, void *dst, int limit);
void sp_crack_spmi(void *srs, void *dst, int limit);
void sp_crack_spal(void *srs, void *dst, int limit);
void sp_crack_swvi0(void *srs, void *dst, int limit);
void sp_crack_swvi(void *srs, void *dst, int limit);
void ddin_crack_rktb(void *srs, void *dst, int limit);
void ddin_crack_sswbLE_aligned8(void *srs, void *dst, int limit);
void ddin_crack_sswb_aligned8(void *srs, void *dst, int limit);
void ddin_crack_sswbLE(void *srs, void *dst, int limit);
void ddin_crack_sswb(void *srs, void *dst, int limit);
void eld_crack_cspd(void *srs, void *dst, int limit);
void eld_crack_lidr(void *srs, void *dst, int limit);
void eld_crack_frad(void *srs, void *dst, int limit);
void eld_crack_ldat(void *srs, void *dst, int limit);
void fof_crack_hsk(void *srs, void *dst, int limit);
void ddin_crack_vold(void *srs, void *dst, int limit);
void ddin_crack_cfac(void *srs, void *dst, int limit);
void ddin_crack_celv(void *srs, void *dst, int limit);
void ddin_crack_radd(void *srs, void *dst, int limit);
void ddin_crack_frib(void *srs, void *dst, int limit);
void ddin_crack_parm(void *srs, void *dst, int limit);
void ddin_crack_swib(void *srs, void *dst, int limit);
void ddin_crack_asib(void *srs, void *dst, int limit);
void ddin_crack_ryib(void *srs, void *dst, int limit);
void ddin_crack_qdat(void *srs, void *dst, int limit);
void tdwr_crack_cbdh(void *srs, void *dst, int limit);
void hrd_crack_hrh(void *srs, void *dst, int limit);
void hrd_crack_hdrh(void *srs, void *dst, int limit);
void hrd_crack_radar(void *srs, void *dst, int limit);
void hrd_crack_header(void *srs, void *dst, int limit);
void uf_crack_loc_use_ncar(void *srs, void *dst, int limit);
void uf_crack_loc_use_ucla(void *srs, void *dst, int limit);
void uf_crack_fhed(void *srs, void *dst, int limit);
void uf_crack_opt(void *srs, void *dst, int limit);
void uf_crack_man(void *srs, void *dst, int limit);
void nex_crack_drdh(void *srs, void *dst, int limit);
void nex_crack_nmh(void *srs, void *dst, int limit);
void nex_crack_rda(void *srs, void *dst, int limit);
void piraq_crack_header(void *srs, void *dst, int limit, int sparc_alignment);
void piraq_crack_radar(void *srs, void *dst, int limit, int sparc_alignment);
void crackers(char *srs, char *dst, const int item_count, const int ndx_cnt, int *tbl, int srs_ndx, int dst_ndx, int limit);
void piraq_crackers(char *srs, char *dst, int item_count, int ndx_cnt, int *tbl, int srs_ndx, int dst_ndx, int limit);
void swack_short(void *src, void *dst, int nn);
void swack2(void *src, void *dst);
void swack_long(void *src, void *dst, int nn);
void swack4(void *src, void *dst);
void swack_double(void *src, void *dst, int nn);

/* dorade_ncdf.c */
struct rnc_info *return_rnc_info(char *name);
void produce_ncdf(struct dd_general_info *dgi, int finish);

/* dd_files.c */
int craack_ddfn(char *fn, struct dd_file_name_v3 *ddfn);
char *ddfn_file_info_line(struct dd_file_name_v3 *ddfn, char *str);
void ddfn_file_name(struct dd_file_name_v3 *ddfn, char *name);
void ddfn_list_versions(struct dd_file_name_v3 *this);
struct dd_file_name_v3 *ddfn_pop_spair(void);
void ddfn_ptr_list(struct dd_file_name_v3 *this, int list_type);
void ddfn_push_spair(struct dd_file_name_v3 *ddfns);
struct dd_file_name_v3 *ddfn_search(int dir_num, int radar_num, double d_target_time, int req_type, int version);
void ddfn_sort_insert(struct dd_radar_name_info_v3 *rni, struct dd_file_name_v3 *ddfn);
double ddfn_time(struct dd_file_name_v3 *ddfn);
int ddfnp_list(int dir_num, int radar_num, int list_type);
int ddir_files_v3(int dir_num, char *dir);
int ddir_radar_num_v3(int dir_num, char *radar_name);
void ddir_rescan_urgent(int dir_num);
double d_mddir_file_info_v3(int dir_num, int radar_num, double target_time, int req_type, int version, int *true_version_num, char *info_line, char *file_name);
struct dd_file_name_v3 **mddir_entire_list_v3(int dir_num, int radar_num, int *num_swps);
long mddir_file_info_v3(int dir_num, int radar_num, long target_time, int req_type, int version, int *true_version_num, char *info_line, char *file_name);
int mddir_file_list_v3(int dir_num, char *dir);
int mddir_gen_swp_list_v3(int dir_num, int radar_num, struct solo_list_mgmt *lm);
int mddir_gen_swp_str_list_v3(int dir_num, int radar_num, int full_file_name, struct solo_list_mgmt *lm);
int mddir_num_radars_v3(int dir_num);
char *mddir_radar_name_v3(int dir_num, int radar_num);
int mddir_radar_num_v3(int dir_num, char *radar_name);
struct dd_file_name_v3 **mddir_return_swp_list_v3(int dir_num, int radar_num, int *num_swps);
double ddfnp_list_entry(int dir_num, int radar_num, int ent_num, int *version, char *line, char *file_name);
struct ddir_info_v3 *return_ddir(int dir_num);

/* dda_common.c */
void solo_set_halt_flag(void);
void solo_clear_halt_flag(void);
int solo_halt_flag(void);
void dd_set_control_c(void);
void dd_reset_control_c(void);
int dd_control_c(void);
void dd_cchandl(int sig);
int dd_catalog_only_flag(void);
struct dd_stats *dd_stats_reset(void);
char *dd_stats_sprintf(void);
int dd_solo_flag(void);
int dd_set_solo_flag(int val);
int dd_assign_radar_num(char *radar_name);
struct time_dependent_fixes *dd_clean_tdfs(int *fx_count);
void dd_close(int fid);
struct dd_comm_info *dd_first_comment(void);
struct dd_general_info *dd_get_structs(int radar_num);
void dd_intset(void);
struct dd_comm_info *dd_last_comment(void);
int dd_min_rays_per_sweep(void);
int dd_max_rays_per_sweep(void);
int dd_max_sweeps_per_volume(void);
int dd_min_sweeps_per_volume(void);
struct comment_d *dd_return_comment(int num);
int dd_return_num_comments(void);
void dd_reset_comments(void);
struct dd_comm_info *dd_next_comment(struct comment_d *comm, int little_endian);
struct time_dependent_fixes *dd_kill_tdf(struct time_dependent_fixes *fx, int *fx_count);
int dd_num_radars(void);
int dd_open(char *dir, char *file_name);
int dd_output_flag(int val);
int dd_position(int fid, int offset);
void dd_rename_swp_file(struct dd_general_info *dgi);
struct dd_input_filters *dd_return_difs_ptr(void);
int dd_return_radar_num(char *radar_name);
struct dd_stats *dd_return_stats_ptr(void);
struct time_dependent_fixes *dd_time_dependent_fixes(int *fx_count);
void dd_unlink(char *full_path_name);
DGI_PTR dd_window_dgi(int window_num);
void dd_write(int fid, char *buf, int size);
DGI_PTR dgi_last(void);
void do_print_dd_open_info(void);
void dont_print_dd_open_info(void);
char *return_dd_open_info(void);

/* ddb_common.c */
int dd_get_scan_mode(char *mne);
struct cfac_wrap *dd_absorb_cfacs(char *list, char *radar_name);
void dd_absorb_cfac(char *list, char *radar_name, struct correction_d *cfac);
char *dd_align_structs(char *c, int n);
double dd_altitude(struct dd_general_info *dgi);
double dd_altitude_agl(struct dd_general_info *dgi);
void dd_alloc_data_field(DGI_PTR dgi, int pn);
double dd_azimuth_angle(struct dd_general_info *dgi);
int dd_cell_num(struct dds_structs *dds, int parameter_num, double range);
void dd_clear_pisp(struct point_in_space *pisp);
int dd_clip_gate(DGI_PTR dgi, double elev, double alt, double lower, double upper);
int dd_compress(unsigned short *src, unsigned short *dst, int flag, int n);
void dd_copy_pisp(struct point_in_space *p0, struct point_in_space *p1);
int dd_datum_size(int binary_format);
char *dd_delimit(char *c);
double dd_drift(struct dd_general_info *dgi);
double dd_earthr(double lat);
double dd_elevation_angle(struct dd_general_info *dgi);
char *dd_find_desc(char *a, void *b, char *desc);
int dd_find_field(struct dd_general_info *dgi, char *name);
void dd_get_cfac(FILE *stream, struct correction_d *cfac);
void dd_get_difs(struct dd_input_filters *difs);
void dd_get_lims(char *str_ptrs[], int ii, int nt, struct d_limits **dlims);
int dd_givfld(DGI_PTR dgi, int *ndx, int *g1, int *n, float *dd, float *badval);
double dd_heading(struct dd_general_info *dgi);
struct dd_general_info *dd_ini(int rn, char *radar_name);
void dd_input_strings(void);
void dgi_interest_really(struct dd_general_info *dgi, int verbosity, char *preamble, char *postamble, struct sweepinfo_d *swib);
int dd_itsa_physical_dev(char *s);
double dd_latitude(struct dd_general_info *dgi);
void dd_latlon_relative(struct point_in_space *p0, struct point_in_space *p1);
int loop_ll2xy_v3(double *plat, double *plon, double *palt, double *x, double *y, double *z, double olat, double olon, double oalt, double R_earth, int num_pts);
void dd_latlon_shift(struct point_in_space *p0, struct point_in_space *p1);
int loop_xy2ll_v3(double *plat, double *plon, double *palt, double *x, double *y, double *z, double olat, double olon, double oalt, double R_earth, int num_pts);
char *dd_link_dev_name(char *radar_name, char *dev_links, char *str);
double dd_longitude(struct dd_general_info *dgi);
struct radar_d *dd_malloc_radd(DGI_PTR dgi, char *config_name);
double dd_nav_rotation_angle(struct dd_general_info *dgi);
double dd_nav_tilt_angle(struct dd_general_info *dgi);
int dd_ndx_name(DGI_PTR dgi, char *name2);
double dd_pitch(struct dd_general_info *dgi);
void dor_print_asib(struct platform_i *asib, struct solo_list_mgmt *slm);
void dor_print_celv(struct cell_d *celv, struct solo_list_mgmt *slm);
void dor_print_cfac(struct correction_d *cfac, struct solo_list_mgmt *slm);
void dor_print_parm(struct parameter_d *parm, struct solo_list_mgmt *slm);
void dor_print_frib(struct field_radar_i *frib, struct solo_list_mgmt *slm);
void dor_print_radd(struct radar_d *radd, struct solo_list_mgmt *slm);
void dor_print_rktb(struct rot_ang_table *rktb, struct solo_list_mgmt *slm);
void dor_print_ryib(struct ray_i *ryib, struct solo_list_mgmt *slm);
void dor_print_swib(struct sweepinfo_d *swib, struct solo_list_mgmt *slm);
void dor_print_sswb(struct super_SWIB *sswb, struct solo_list_mgmt *slm);
void dor_print_vold(struct volume_d *vold, struct solo_list_mgmt *slm);
void dd_radar_angles(struct platform_i *asib, struct correction_d *cfac, struct radar_angles *ra, struct dd_general_info *dgi);
char *dd_radar_name(struct dds_structs *dds);
char *dd_radar_namec(char *name0);
void dd_radar_selected(char *radar_name, int radar_num, struct dd_input_filters *difs);
void dd_range_gate(DGI_PTR dgi, float *req_range, int *gate, float *val);
struct dd_ray_sector *dd_ray_coverage(DGI_PTR dgi, struct rot_ang_table *rat, int rn, double expansion);
void dd_razel_to_xyz(struct point_in_space *p0);
void dd_reset_d_limits(struct d_limits **at);
int dd_return_id_num(char *name);
struct rot_table_entry *dd_return_rotang1(struct rot_ang_table *rat);
double dd_roll(struct dd_general_info *dgi);
int dd_rotang_seek(struct rot_ang_table *rat, double rotang);
void dd_rotang_table(DGI_PTR dgi, int func);
double dd_rotation_angle(struct dd_general_info *dgi);
void dd_rng_info(DGI_PTR dgi, int *g1, int *m, float *rngs, int *ng);
void dd_set_uniform_cells(struct dds_structs *dds);
void dd_site_name(char *proj_name, char *site_name);
int dd_src_fid(int *io_type);
double dd_tilt_angle(struct dd_general_info *dgi);
int dd_tokens(char *att, char *str_ptrs[]);
int dd_tokenz(char *att, char *str_ptrs[], char *dlims);
void dd_uniform_cell_spacing(float src_cells[], int num_cells, double cell_spacing, int *cell_lut, double r0, int nuc);
char *dd_whiteout(char *c);
int dd_xy_plane_horizontal(struct dd_general_info *dgi);
int fp_bin_search(float *f, int nf, double val);
int dgi_buf_rewind(DGI_PTR dgi);
int HRD_recompress(unsigned short *src, unsigned short *dst, int flag, int n, int compression);
int RDP_compress_bytes(unsigned char *src, unsigned char *dst, int flag, int n, int compression);
short swap2(char *ov);
long swap4(char *ov);
long xswap4(char *v_lower, char *v_upper);

/* dorade_share.c */
int dd_text_to_slm(const char *txt, struct solo_list_mgmt *slm);
int dd_strings_to_slm(const char **lines, struct solo_list_mgmt *slm, int nn);
int dd_absorb_strings(char *path_name, struct solo_list_mgmt *slm);
void solo_sort_strings(char **sptr, int ns);
int generic_sweepfiles(char *dir, struct solo_list_mgmt *lm, char *prefix, char *suffix, int not_this_suffix);
int dd_return_interpulse_periods(struct dd_general_info *dgi, int field_num, double *ipps);
int dd_return_frequencies(struct dd_general_info *dgi, int field_num, double *freqs);
int dd_alias_index_num(struct dd_general_info *dgi, char *name);
double angdiff(double a1, double a2);
double d_angdiff(double a1, double a2);
void cascdmp(char *b, int m, int n);
void chexdmp(char *b, int m, int n);
char *cpy_bfill_str(CHAR_PTR dst, CHAR_PTR srs, int n);
int dcdatime(char *str, int n, short *yy, short *mon, short *dd, short *hh, short *mm, short *ss, short *ms);
char *dd_blank_fill(CHAR_PTR srs, int n, CHAR_PTR dst);
char *dd_chop(char *aa);
int dd_crack_file_name(char *type, long *time_stamp, char *radar, int *version, char *fn);
int dd_crack_file_name_ms(char *type, double *time_stamp, char *radar, int *version, char *fn, int *ms);
void dd_file_name(char *type, long time_stamp, char *radar, int version, char *fn);
void dd_file_name_ms(char *type, long time_stamp, char *radar, int version, char *fn, int ms);
int dd_hrd16(char *buf, short *dd, int flag, int *empty_run);
int dd_hrd16_uncompressx(short *ss, short *dd, int flag, int *empty_run, int wmax);
int dd_hrd16LE_uncompressx(unsigned short *ss, unsigned short *dd, int flag, int *empty_run, int wmax);
int dd_nab_floats(char *str, float *vals);
int dd_nab_ints(char *str, int *vals);
int dd_rdp_uncompress8(unsigned char *buf, unsigned char *dd, int bad_val);
int dd_scan_mode(char *str);
char *dd_scan_mode_mne(int scan_mode, char *str);
char *dd_radar_type_ascii(int radar_type, char *str);
char *dd_unquote_string(char *uqs, char *qs);
double dorade_time_stamp(struct volume_d *vold, struct ray_i *ryib, DD_TIME *dts);
char *dts_print(DD_TIME *dts);
double eldora_time_stamp(struct volume_d *vold, struct ray_i *ryib);
int fb_read(int fin, char *buf, int count);
int fb_write(int fout, char *buf, int count);
char *fgetsx(char *aa, int nn, FILE *stream);
long file_time_stamp(char *fn);
char *fstring(char *line, int nn, FILE *stream);
char *get_tagged_string(char *tag);
int gp_read(int fid, char *buf, int size, int io_type);
int gp_write(int fid, void *buf, int size, int io_type);
int in_sector(double ang, double ang1, double ang2);
char *put_tagged_string(char *tag, char *string);
int se_free_raqs(void);
struct running_avg_que *se_return_ravgs(int nvals);
char *slash_path(char *dst, const char *srs);
void slm_dump_list(struct solo_list_mgmt *slm);
void slm_print_list(struct solo_list_mgmt *slm);
void solo_add_list_entry(struct solo_list_mgmt *which, char *entry, int len);
char *solo_list_entry(const struct solo_list_mgmt *which, int entry_num);
void solo_list_remove_dups(struct solo_list_mgmt *slm);
void solo_list_remove_entry(struct solo_list_mgmt *slm, int ii, int jj);
void solo_list_sort_file_names(struct solo_list_mgmt *slm);
struct solo_list_mgmt *solo_malloc_list_mgmt(int sizeof_entries);
struct point_in_space *solo_malloc_pisp(void);
char *solo_modify_list_entry(struct solo_list_mgmt *which, char *entry, int len, int entry_num);
void solo_reset_list_entries(struct solo_list_mgmt *which);
void solo_unmalloc_list_mgmt(struct solo_list_mgmt *slm);
void solo_sort_slm_entries(struct solo_list_mgmt *slm);
char *str_terminate(CHAR_PTR dst, CHAR_PTR srs, int n);
long time_now(void);
time_t todays_date(short date_time[]);
struct run_sum *init_run_sum(int length, int short_len);
void reset_running_sum(struct run_sum *rs);
double short_running_sum(struct run_sum *rs);
double running_sum(struct run_sum *rs, double val);
double avg_running_sum(struct run_sum *rs);
double short_avg_running_sum(struct run_sum *rs);

/* dd_time.c */
void dd_clear_dts(struct d_time_struct *dts);
double d_time_stamp(DD_TIME *dts);
DD_TIME *d_unstamp_time(DD_TIME *dts);

/* swp_file_acc.c */
int dd_absorb_header_info(DGI_PTR dgi);
int dd_absorb_ray_info(DGI_PTR dgi);
int dd_absorb_rotang_info(DGI_PTR dgi, int gottaSwap);
int dd_absorb_seds(DGI_PTR dgi, int gottaSwap);
struct dd_input_sweepfiles_v3 *dd_return_sweepfile_structs_v3(void);
struct dd_input_sweepfiles_v3 *dd_setup_sweepfile_structs_v3(int *ok);
int dd_sweepfile_list_v3(struct unique_sweepfile_info_v3 *usi, double start_time, double stop_time);
double dd_ts_start(int frme, int radar_num, double start, int *version, char *info, char *name);
void dd_update_ray_info(DGI_PTR dgi);
void dd_update_sweep_info(DGI_PTR dgi);
int ddswp_last_ray(struct unique_sweepfile_info_v3 *usi);
int ddswp_new_sweep_v3(struct unique_sweepfile_info_v3 *usi);
struct cfac_wrap *ddswp_nab_cfacs(char *radar_name);
int ddswp_next_ray_v3(struct unique_sweepfile_info_v3 *usi);
void ddswp_stuff_ray(DGI_PTR dgi);
void ddswp_stuff_sweep(DGI_PTR dgi);
int dd_create_dynamic_rotang_info(DGI_PTR dgi);
int dd_read_ray_for_dynamic_angle_table(DGI_PTR dgi, int rayNum);
void dd_print_angle_table(const struct rot_ang_table *rat, FILE *out);

/* dd_io_mgmt.c */
int ffb_skip_one_fwd(FILE *strm);
int ffb_skip_one_bkw(FILE *strm);
int ffb_read(FILE *fin, char *buf, int count);
int cdcode(char *s, int n, int *ival, float *rval);
void ctype16(short *a, int m, int n);
void xctypeu16(unsigned short *a, int m, int n);
void ctypeu16(void *a, int m, int n);
void slm_ctypeu16(struct solo_list_mgmt *slm, unsigned char *a, int m, int n);
void slm_xctypeu16(struct solo_list_mgmt *slm, unsigned char *a, int m, int n);
void Xctypeu16(void *a, int m, int n);
int dd_crack_datime(char *datime, int nc, DD_TIME *dts);
char *dd_est_src_dev_names_list(char *id, struct solo_list_mgmt *slm, char *src_dir);
char *dd_establish_source_dev_names(char *id, char *names);
void dd_gen_packet_info(struct input_read_info *iri, struct io_packet *dp);
struct input_read_info *dd_init_io_structs(int index, int fmt);
void dd_input_read_close(struct input_read_info *iri);
int dd_input_read_open(struct input_read_info *iri, char *dev_name);
void dd_io_reset_offset(struct input_read_info *iri, long offset);
int dd_logical_read(struct input_read_info *iri, int direction);
char *dd_next_source_dev_name(char *id);
int dd_phys_read(struct input_read_info *iri);
int dd_really_skip_recs(struct input_read_info *iri, int direction, int skip_count);
double dd_relative_time(const char *rtm);
void dd_reset_ios(struct input_read_info *iri, int level);
struct io_packet *dd_return_current_packet(struct input_read_info *iri);
struct input_read_info *dd_return_ios(int index, int fmt);
struct io_packet *dd_return_next_packet(struct input_read_info *iri);
void dd_rewind_dev(struct input_read_info *iri);
void dd_skip_files(struct input_read_info *iri, int direction, int skip_count);
int dd_skip_recs(struct input_read_info *iri, int direction, int skip_count);
void dd_unwind_dev(struct input_read_info *iri);
void ezascdmp(char *b, int m, int n);
void slm_ezascdmp(struct solo_list_mgmt *slm, char *b, int m, int n);
void ezhxdmp(char *b, int m, int n);
void slm_ezhxdmp(struct solo_list_mgmt *slm, char *b, int m, int n);
int getreply(char *s, int lim);
int oketype(char s[], int n, float *val);
int okint(char s[], int n, int *ival);
int okreal(char s[], int n, float *val);
void gri_interest(struct generic_radar_info *gri, int verbose, char *preamble, char *postamble);
int gri_max_lines(void);
int gri_nab_input_filters(double current_time, struct dd_input_filters *difs, int less);
void gri_print_list(char **ptrs);
void gri_set_max_lines(int nlines);
int gri_start_stop_chars(int *view_char, int *view_char_count);

/* radar_io.c: prototypes are in radar_io.h */

#endif /* INCs2_protosh */
