/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include "sii_utils.h"
# include "sii_enums.h"
# include "help_swpfi.h"
# include <gdk/gdkkeysyms.h>

# include <solo_window_structs.h>
# include <sed_shared_structs.h>
# include <seds.h>
# include <sii_widget_content_structs.h>

/* Sweepfiles Menu Actions  */

enum {
   SWPFI_ZERO,
   SWPFI_CANCEL,
   SWPFI_CLOSE,
   SWPFI_OK,

   SWPFI_ZAP_SWPFIS,
   SWPFI_LIST_SWPFIS,
   SWPFI_SWPFI_LIST,
   SWPFI_RADAR_LIST,
   SWPFI_LAYOUT,
   SWPFI_SCROLLED,

   SWPFI_LINKS,
   LOCKSTEP_LINKS,
   SWPFI_TIMESYNC_LINKS,

   SWPFI_REPLOT_THIS,
   SWPFI_REPLOT_LINKS,
   SWPFI_REPLOT_ALL,

   SWPFI_ELECTRIC,		/* check button */
   SWPFI_RESCAN,
   SWPFI_CURRENT_SWPFI,
   SWPFI_DIR,
   SWPFI_RADAR_NAME,
   SWPFI_START_TIME,
   SWPFI_STOP_TIME,
   SWPFI_FILTER,
   SWPFI_SCAN_MODES,
   SWPFI_FIXED_INFO,
   SWPFI_TIME_SYNC,

   SWPFI_OVERVIEW,
   SWPFI_HLP_TIMES,
   SWPFI_HLP_TSYNC,
   SWPFI_HLP_LINKS,
   SWPFI_HLP_LOCKSTEP,
   SWPFI_HLP_FILTER,

   SWPFI_LAST_ENUM,
};

# define SWPFI_MAX_WIDGETS SWPFI_LAST_ENUM


/* c---------------------------------------------------------------------- */

typedef struct {
   gboolean changed;

   guint precision;

   GtkWidget *data_widget[SWPFI_MAX_WIDGETS];

   gboolean entry_flag[SWPFI_MAX_WIDGETS];
   guint num_values[SWPFI_MAX_WIDGETS];
   gdouble orig_values[SWPFI_MAX_WIDGETS][2];
   gdouble values[SWPFI_MAX_WIDGETS][2];

   GString *orig_txt[SWPFI_MAX_WIDGETS];
   GString *txt[SWPFI_MAX_WIDGETS];

   gboolean toggle[SWPFI_MAX_WIDGETS];
   gboolean electric_swpfi;

   LinksInfo *swpfi_links;
   LinksInfo *lockstep_links;

   guint label_height;
   guint label_char_width;

   guint layout_label_count;
   guint swpfi_list_popup_count;

   gdouble radar_lat;
   gdouble radar_lon;
   gdouble radar_alt;

   gdouble start_time;
   gdouble stop_time;

   GtkWidget **label_items;

} SwpfiData;

static GSList *radio_group = NULL;


/* c---------------------------------------------------------------------- */

void sii_swpfi_menubar2( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num);
void sii_swpfi_menubar( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num);
void sii_swpfi_widget( guint frame_num );

void show_swpfi_widget (GtkWidget *text, gpointer data );

void sii_swpfi_list_widget( guint frame_num );

const gchar *sii_return_swpfi_radar (guint frame_num);
const gchar *sii_return_swpfi_dir (guint frame_num);
double sii_return_swpfi_time_stamp (guint frame_num);
const char *sii_string_from_dtime (double dtime);
const struct solo_list_mgmt *sii_return_swpfi_list (guint frame_num);
const gchar *solo_list_entry (const struct solo_list_mgmt *slm, int item_num);
const gchar *sii_return_swpfi_label (int frame_num);
gboolean sii_set_swpfi_info (guint frame_num, gint sweep_num);
double dd_relative_time (const char *str);

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

gboolean
sii_set_swpfi_info (guint frame_num, gint sweep_num)
{
  LinksInfo *li;
  SwpfiData *sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;
  struct sweep_widget_info swi;
  char str[256], *sptrs[32];
  const gchar *aa, *bb;
  gint nt, nn, jj;
  WW_PTR wwptr = solo_return_wwptr(frame_num);



  memset (&swi, 0, sizeof (swi));

  swi.frame_num = frame_num;

				/* dump swpfi links info */
				/* dump lockstep links info */
				/* dump timesync links info */

  swi.sweep_num = sweep_num;	/* >= 0 => swpfi list selection */

  aa = gtk_editable_get_text (GTK_EDITABLE (sd->data_widget[SWPFI_DIR]));
  strcpy (swi.directory_name, aa);

  aa = gtk_editable_get_text (GTK_EDITABLE (sd->data_widget[SWPFI_RADAR_NAME]));
  strcpy (str, aa);
  nt = dd_tokenz (str, sptrs, sii_item_delims());
  swi.radar_name[0] = '\0';
  if (nt)
    { strcpy (swi.radar_name, sptrs[0]); }

  aa = gtk_editable_get_text (GTK_EDITABLE (sd->data_widget[SWPFI_START_TIME]));
  strcpy (swi.start_time_text, aa);

  aa = gtk_editable_get_text (GTK_EDITABLE (sd->data_widget[SWPFI_STOP_TIME]));
  strcpy (swi.stop_time_text, aa);

  wwptr->swpfi_time_sync_set =
    swi.timesync_flag = (sd->toggle[SWPFI_TIME_SYNC]) ? 1 : 0;

  swi.filter_flag = (sd->toggle[SWPFI_FILTER]) ? frame_num+1 : 0;
  if (swi.filter_flag) {
     sii_return_filter_info (frame_num, wwptr->filter_scan_modes
			     , &wwptr->filter_fxd_ang
			     , &wwptr->filter_tolerance);
  }

  li = frame_configs[frame_num]->link_set[LI_SWPFI];
  for (jj=0; jj < maxFrames; jj++) {
    swi.linked_windows[jj] = (li->link_set[jj]) ? 1 : 0;
  }
  li = frame_configs[frame_num]->link_set[LI_LOCKSTEP];
  for (jj=0; jj < maxFrames; jj++) {
    swi.lockstep_links[jj] = (li->link_set[jj]) ? 1 : 0;
  }

  nn = solo_set_sweep_info (&swi);
  return (nn) ? FALSE : TRUE;
}

/* c---------------------------------------------------------------------- */

gint sii_return_filter_info (gint frame_num, char *scan_modes
			     , float *fixed_angle, float *tolerance)
{
  int nt, jj = 0, kk, ok = 1;
  SwpfiData *sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;
  GtkWidget *widget;
  gchar str[256], *sptrs[16];
  const gchar *aa;
  float ff;

  widget = sii_get_widget_ptr (frame_num, FRAME_SWPFI_MENU);
  if (!sd || !widget || !sd->toggle[SWPFI_FILTER])
    { return 0; }

  aa = gtk_editable_get_text (GTK_EDITABLE (sd->data_widget[SWPFI_SCAN_MODES]));
  g_string_assign (sd->orig_txt[SWPFI_SCAN_MODES], aa);
  strcpy (scan_modes, aa);

  aa = gtk_editable_get_text (GTK_EDITABLE (sd->data_widget[SWPFI_FIXED_INFO]));
  g_string_assign (sd->orig_txt[SWPFI_FIXED_INFO], aa);
  strcpy (str, aa);

  nt = dd_tokenz (str, sptrs, sii_item_delims());
  if (nt > 0 ) {
    kk = sscanf (sptrs[0], "%f", &ff);
    if (kk == 1)
      { *fixed_angle = ff; }
    else
      { ok = 0; }
    if (nt > 1) {
      kk = sscanf (sptrs[1], "%f", &ff);
      if (kk == 1)
	{ *tolerance = ff; }
      else
	{ ok = 0; }
    }
    else
      { ok = 0; }
  }
  else
    { ok = 0; }

  if (!ok) {
    sprintf (str, "Unusable swpfi filter info for frame %d", frame_num);
    sii_message (str);
  }
  return ok;
}

/* c---------------------------------------------------------------------- */

void sii_update_swpfi_widget (guint frame_num)
{
  LinksInfo *li;
  WW_PTR wwptr = solo_return_wwptr(frame_num);
  SwpfiData *sd;
  SwpfiData *sd2;
  GtkWidget *check_item, *widget;
  gdouble dtime;
  gchar str[64];
  const gchar *cc, *aa;
  guint wid, jj;
  gboolean active;



  if( !frame_configs[frame_num]->swpfi_data ) {
     sd = frame_configs[frame_num]->swpfi_data =
       g_malloc0 (sizeof( SwpfiData ));

     frame_configs[frame_num]->link_set[LI_SWPFI] =
       sd->swpfi_links = sii_new_links_info
	 ( "Sweepfile Links", frame_num, FRAME_SWPFI_LINKS, FALSE );

     frame_configs[frame_num]->link_set[LI_LOCKSTEP] =
       sd->lockstep_links = sii_new_links_info
	 ( "Lockstep Links", frame_num, FRAME_LOCKSTEP_LINKS, FALSE );
  }
  else
    { sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data; }

  sd->toggle[SWPFI_TIME_SYNC] = (wwptr->swpfi_time_sync_set) ? TRUE : FALSE;
  sd->toggle[SWPFI_FILTER] = (wwptr->swpfi_filter_set) ? TRUE : FALSE;



  if( !sii_get_widget_ptr (frame_num, FRAME_SWPFI_MENU) )
    { return; }


  wid = SWPFI_CURRENT_SWPFI;
  cc = wwptr->show_file_info;
  gtk_label_set_text (GTK_LABEL (sd->data_widget[wid]), cc );
  g_string_assign (sd->orig_txt[wid], cc);

  wid = SWPFI_DIR;
  cc = wwptr->sweep->directory_name;
  gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[wid]), cc);
  g_string_assign (sd->orig_txt[wid], cc);

  wid = SWPFI_RADAR_NAME;
  cc = wwptr->sweep->radar_name;
  gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[wid]), cc);
  g_string_assign (sd->orig_txt[wid], cc);

  wid = SWPFI_START_TIME;
  aa = gtk_editable_get_text (GTK_EDITABLE (sd->data_widget[wid]));
  if (!dd_relative_time(aa)) {
    dtime = wwptr->sweep->start_time;
    cc = sii_string_from_dtime (dtime);
    gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[wid]), cc);
    g_string_assign (sd->orig_txt[wid], cc);
  }
  wid = SWPFI_STOP_TIME;
  dtime = wwptr->sweep->stop_time;
  cc = sii_string_from_dtime (dtime);
  gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[wid]), cc);
  g_string_assign (sd->orig_txt[wid], cc);

  wid = SWPFI_FILTER;
  active = (wwptr->swpfi_filter_set) ? TRUE : FALSE;
  gtk_check_button_set_active
    (GTK_CHECK_BUTTON (sd->data_widget[wid]), active);

  cc = wwptr->filter_scan_modes;
  g_string_assign (sd->orig_txt[SWPFI_SCAN_MODES], cc);
  gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[SWPFI_SCAN_MODES]), cc);

  g_string_printf (sd->orig_txt[SWPFI_FIXED_INFO], "%.2f,%.3f"
		    , wwptr->filter_fxd_ang, wwptr->filter_tolerance);
  gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[SWPFI_FIXED_INFO])
		      , sd->orig_txt[SWPFI_FIXED_INFO]->str);

# ifdef obsolete
  if (active && wwptr->swpfi_filter_set-1 != frame_num) {
     /* frame number where filter was last set */
     sd2 = (SwpfiData *)frame_configs[wwptr->swpfi_filter_set-1]->swpfi_data;
     cc = sd2->orig_txt[SWPFI_SCAN_MODES]->str;
     g_string_assign (sd->orig_txt[SWPFI_SCAN_MODES], cc);
     gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[SWPFI_SCAN_MODES]), cc);
     cc = sd2->orig_txt[SWPFI_FIXED_INFO]->str;
     g_string_assign (sd->orig_txt[SWPFI_FIXED_INFO], cc);
     gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[SWPFI_FIXED_INFO]), cc);
  }
# endif

  active = wwptr->swpfi_time_sync_set ? TRUE : FALSE;
  gtk_check_button_set_active
    (GTK_CHECK_BUTTON (sd->data_widget[SWPFI_TIME_SYNC]), active);

  li = frame_configs[frame_num]->link_set[LI_SWPFI];
  for (jj=0; jj < maxFrames; jj++) {
    li->link_set[jj] = (wwptr->sweep->linked_windows[jj]) ? TRUE : FALSE;
  }

  li = frame_configs[frame_num]->link_set[LI_LOCKSTEP];
  for (jj=0; jj < maxFrames; jj++) {
    li->link_set[jj] = (wwptr->lock->linked_windows[jj]) ? TRUE : FALSE;
  }
}

/* c---------------------------------------------------------------------- */

void show_swpfi_widget (GtkWidget *text, gpointer data )
{
  GtkWidget *widget;
  guint frame_num = GPOINTER_TO_UINT (data);

  widget = sii_get_widget_ptr (frame_num, FRAME_SWPFI_MENU);

  if( !widget )
    { sii_swpfi_widget( frame_num ); }
  else {
    sii_update_swpfi_widget (frame_num);
    /* GTK4: window positioning is handled by the window manager */
    gtk_widget_set_visible (widget, TRUE);
  }
}

/* c---------------------------------------------------------------------- */

void sii_swpfi_menu_cb ( GtkWidget *w, gpointer   data )
{
   guint num = GPOINTER_TO_UINT (data);
   guint jj, mm, nn, frame_num, task, wid, width, height;
   gchar *aa, *bb, *line, str[128], *sptrs[16];
   const gchar *cc;
   SwpfiData *sd;
   GtkWidget *widget, *check_item, *rmi, *entry;
   gint flag;
   const struct solo_list_mgmt *slm;
   struct solo_perusal_info *spi, *solo_return_winfo_ptr();
   GString *gs;


   frame_num = num/TASK_MODULO;
   wid = task = num % TASK_MODULO;
   sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;
   widget =
     check_item =
       rmi = sd->data_widget[task];


   switch (task) {

    case SWPFI_OK:
      sii_set_swpfi_info (frame_num, -1);
    case SWPFI_CANCEL:
    case SWPFI_CLOSE:
      widget = sii_get_widget_ptr (frame_num, FRAME_SWPFI_MENU);
      if( widget ) {
	gtk_widget_set_visible (widget, FALSE);
      }
      widget = sii_get_widget_ptr (frame_num, FRAME_SWPFI_DISPLAY );
      if( widget ) {
	gtk_widget_set_visible (widget, FALSE);
      }
      widget = sii_get_widget_ptr (frame_num, FRAME_SWPFI_LINKS );
      if( widget ) {
	gtk_widget_set_visible (widget, FALSE);
      }
      if (sd->swpfi_list_popup_count & 1) {
	 sd->swpfi_list_popup_count++;
      }
      widget = sii_get_widget_ptr (frame_num, FRAME_LOCKSTEP_LINKS );
      if( widget ) {
	gtk_widget_set_visible (widget, FALSE);
      }
      /* clear out the start time in case we've been doing relative time */
      gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[SWPFI_START_TIME]), "");
      break;

    case SWPFI_LIST_SWPFIS:
      if (!sd->swpfi_list_popup_count || !(sd->swpfi_list_popup_count & 1)) {
	 /* Get directory and radar name and generate a list
	  */
	sii_set_swpfi_info (frame_num, -1);
	sii_update_swpfi_widget (frame_num);
	sii_swpfi_list_widget (frame_num);
      }
      else {
	 widget = sii_get_widget_ptr (frame_num, FRAME_SWPFI_DISPLAY );
	 if( widget ) {
	   gtk_widget_set_visible (widget, FALSE);
	 }
      }
      sd->swpfi_list_popup_count++;
      break;

    case SWPFI_ZAP_SWPFIS:
      break;

   case SWPFI_RADAR_LIST:
     solo_gen_radar_list(frame_num);
     spi = solo_return_winfo_ptr();
     slm = spi->list_radars;
     wid = SWPFI_RADAR_NAME;
     gs = sd->txt[wid];
     g_string_truncate (gs, 0);

     for (jj = 0; jj < slm->num_entries; jj++) {
       cc = solo_list_entry (slm, jj);
       if (jj)
	 { g_string_append (gs, " "); }
       g_string_append (gs, cc);
     }
     entry = sd->data_widget[wid];
     gtk_editable_set_text (GTK_EDITABLE (entry), gs->str);
     break;

    case SWPFI_RESCAN:
      break;

   case SWPFI_REPLOT_THIS:
   case SWPFI_REPLOT_LINKS:
   case SWPFI_REPLOT_ALL:
     flag = REPLOT_THIS_FRAME;
     if (task == SWPFI_REPLOT_LINKS)
       { flag = REPLOT_LOCK_STEP; }
     else if (task == SWPFI_REPLOT_ALL)
       { flag = REPLOT_ALL; }
     if (sii_set_swpfi_info (frame_num, -1))
       { sii_plot_data (frame_num, flag); }
       sii_update_swpfi_widget (frame_num);
     break;

   case SWPFI_ELECTRIC:
     sd->toggle[task] = gtk_check_button_get_active
       (GTK_CHECK_BUTTON (rmi));
     if (!gtk_check_button_get_active (GTK_CHECK_BUTTON (rmi)))
       { break; }
     break;

   case SWPFI_TIME_SYNC:
   case SWPFI_FILTER:
     sd->toggle[task] = gtk_check_button_get_active
       (GTK_CHECK_BUTTON (rmi));
     sii_set_swpfi_info (frame_num, -1);
     break;

   case SWPFI_SCAN_MODES:
   case SWPFI_FIXED_INFO:
     break;

   case SWPFI_STOP_TIME:
     if (sii_set_swpfi_info (frame_num, -1))
       { sii_plot_data (frame_num, REPLOT_LOCK_STEP); }
       sii_update_swpfi_widget (frame_num);
       break;

   case SWPFI_DIR:
   case SWPFI_RADAR_NAME:
   case SWPFI_START_TIME:
     if (sii_set_swpfi_info (frame_num, -1)) {
       sii_plot_data (frame_num, REPLOT_LOCK_STEP);
       sii_update_swpfi_widget (frame_num);
     }
     break;

    case SWPFI_LINKS:
      show_links_widget (sd->swpfi_links);
      break;

    case LOCKSTEP_LINKS:
      show_links_widget (sd->lockstep_links);
      break;

   case SWPFI_OVERVIEW:
     nn = sizeof (hlp_swpfi_overview)/sizeof (char *);
     sii_glom_strings (hlp_swpfi_overview, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case SWPFI_HLP_LINKS:
     nn = sizeof (hlp_swpfi_links)/sizeof (char *);
     sii_glom_strings (hlp_swpfi_links, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case SWPFI_HLP_LOCKSTEP:
     nn = sizeof (hlp_swpfi_lockstep)/sizeof (char *);
     sii_glom_strings (hlp_swpfi_lockstep, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case SWPFI_HLP_TIMES:
     nn = sizeof (hlp_swpfi_times)/sizeof (char *);
     sii_glom_strings (hlp_swpfi_times, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case SWPFI_HLP_FILTER:
     nn = sizeof (hlp_swpfi_filter)/sizeof (char *);
     sii_glom_strings (hlp_swpfi_filter, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case SWPFI_HLP_TSYNC:
     nn = sizeof (hlp_swpfi_tsync)/sizeof (char *);
     sii_glom_strings (hlp_swpfi_tsync, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   };
}

/* c---------------------------------------------------------------------- */

static void
sii_swpfi_list_click_cb (GtkGestureClick *gesture, int n_press,
                         double x, double y, gpointer data)
{
  GtkWidget *label = GTK_WIDGET (data);
  guint frame_num, swpfi_index;
  SwpfiData *sd;

  frame_num = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(label),
			  "frame_num" ));
  sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;
  swpfi_index = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(label),
			  "swpfi_index" ));

# ifdef obsolete
   g_message( "layout widget label fn: %d  swpfi_index: %d"
	     , frame_num, swpfi_index );
# endif

  sii_set_swpfi_info (frame_num, swpfi_index);

  if (sd->toggle[SWPFI_ELECTRIC]) {
    sii_plot_data (frame_num, REPLOT_LOCK_STEP);;
  }
  sii_update_swpfi_widget (frame_num);
}

/* c---------------------------------------------------------------------- */

void sii_swpfi_entry_paste_cb ( GtkWidget *w, gpointer   data )
{
   GtkWidget *widget;
   guint num = GPOINTER_TO_UINT (data);
   guint frame_num, wid;
   SwpfiData *sd;

   frame_num = num/TASK_MODULO;
   wid = num % TASK_MODULO;
   sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;

				/* clear out the entry before pasting */
   gtk_editable_set_text (GTK_EDITABLE (sd->data_widget[wid]), "");
}


/* c---------------------------------------------------------------------- */

static gboolean
sii_swpfi_key_pressed (GtkEventControllerKey *controller,
                       guint keyval, guint keycode,
                       GdkModifierType state, gpointer data)
{
   guint num = GPOINTER_TO_UINT (data);
   guint frame_num, task, wid;
   gint value, height;
   SwpfiData *sd;
   GtkWidget *widget;
   GtkAdjustment *adj;

   frame_num = num/TASK_MODULO;
   wid = task = num % TASK_MODULO;

   sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;

   switch( keyval ) {

    case GDK_KEY_Up:
      /* shift up one screen */
    case GDK_KEY_Down:
      /* shift down one screen */

      widget = sd->data_widget[SWPFI_SCROLLED];
      height = gtk_widget_get_height (widget) - sd->label_height;

      adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (widget));
      value = gtk_adjustment_get_value (adj);
      value += (keyval == GDK_KEY_Up) ? -height : height;

      if (value < 1)
	{ value = 1; }
      if (value + height >= gtk_adjustment_get_upper (adj))
	{ value = gtk_adjustment_get_upper (adj) - height; }

      gtk_adjustment_set_value (adj, value);

      return TRUE;

   };
   return FALSE;
}

/* c---------------------------------------------------------------------- */

void sii_swpfi_list_widget( guint frame_num )
{
  GtkWidget *label;

  GtkWidget *window;
  GtkWidget *widget;
  GtkWidget *scrolledwindow;
  GtkWidget *vbox;
  GtkWidget *list_box;

  SiiFont *font;

  GtkWidget *button;

  gchar str[256], *aa, *bb, *ee;
  const gchar *cc, *tt = "00/00/2000 00:00:00 HHHHHHHH 000.0 HHH HHHHHHHH";
  gint length, width, height, mm, nn, mark, jj;
  gint row;
  GString *gstr = g_string_new ("");
  guint widget_id = FRAME_SWPFI_MENU, arg_id;
  LinksInfo *linkfo;
  const GString *cgs;
  gdouble dtime;
  const struct solo_list_mgmt *slm;
  SwpfiData *sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;
  GtkEventController *key_controller;
  GtkGesture *click_gesture;

				/* c...code */

  sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;

  /* separate window for the list of sweepfiles */

  if (!sd->data_widget[SWPFI_SCROLLED]) { /* first time */

    widget_id = FRAME_SWPFI_DISPLAY;
    window = gtk_window_new();

    sii_set_widget_ptr ( frame_num, widget_id, window );
    /* GTK4: window positioning handled by window manager */

    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);

    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num*TASK_MODULO+widget_id));

    nn = frame_num * TASK_MODULO + widget_id;
    key_controller = gtk_event_controller_key_new ();
    g_signal_connect (key_controller, "key-pressed",
                      G_CALLBACK (sii_swpfi_key_pressed),
                      GUINT_TO_POINTER(nn));
    gtk_widget_add_controller (window, key_controller);

    bb = g_strdup_printf ("Frame %d  Sweepfiles List Widget", frame_num+1 );
    gtk_window_set_title (GTK_WINDOW (window), bb);
    g_free( bb );

    sd->data_widget[SWPFI_SCROLLED] =
      scrolledwindow = gtk_scrolled_window_new ();
    gtk_window_set_child (GTK_WINDOW (window), scrolledwindow);
# ifdef notyet
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
# endif
  } /* first time */

  font = med_fxd_font;

  slm = sii_return_swpfi_list (frame_num);
  mm = strlen (solo_list_entry (slm, 0)) +4;
  nn = (((slm->num_entries -1)/500) +1) * 500;

  if (nn > sd->layout_label_count && sd->data_widget[SWPFI_LAYOUT] ) {
    /* Remove old box from scrolled window */
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (sd->data_widget[SWPFI_SCROLLED]), NULL);
    sd->data_widget[SWPFI_LAYOUT] = NULL;
    g_free (sd->label_items);
  }
  else if (nn < sd->layout_label_count)
    { nn = sd->layout_label_count; }



  if (!sd->data_widget[SWPFI_LAYOUT]) {
    sd->label_height =
      height = font->ascent + font->descent+1;
    sd->label_char_width =
      width = sii_font_string_width (font, "0123456789")/10;

    scrolledwindow = sd->data_widget[SWPFI_SCROLLED];
    gtk_widget_set_size_request (scrolledwindow, mm*width, 500);

    /* GTK4: Use a GtkBox instead of GtkLayout */
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    sd->data_widget[SWPFI_LAYOUT] = vbox;
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrolledwindow), vbox);

    sd->label_items = (GtkWidget **)g_malloc0 (nn * sizeof (GtkWidget *));
    sd->layout_label_count = nn;

    for (mm=0; mm < nn; mm++ ) {

      /* GTK4: No event_box needed; attach GtkGestureClick directly to label */
      label = gtk_label_new (" ");
      sd->label_items[mm] = label;
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);

      click_gesture = gtk_gesture_click_new ();
      g_signal_connect (click_gesture, "pressed",
                        G_CALLBACK (sii_swpfi_list_click_cb),
                        (gpointer)label);
      gtk_widget_add_controller (label, GTK_EVENT_CONTROLLER (click_gesture));

      gtk_box_append (GTK_BOX (vbox), label);

      g_object_set_data (G_OBJECT(label),
			   "frame_num",
			   GUINT_TO_POINTER(frame_num));

      g_object_set_data (G_OBJECT(label),
			   "swpfi_index",
			   GUINT_TO_POINTER(mm));
    }
    gtk_widget_set_visible (vbox, TRUE);
  }

  for (mm = 0; mm < nn; mm++) {
    if (mm < slm->num_entries) {
      cc = solo_list_entry (slm, mm);
      gtk_label_set_text (GTK_LABEL (sd->label_items[mm]), cc );
    }
    else {
      gtk_label_set_text (GTK_LABEL (sd->label_items[mm]), " " );
    }
  }
  widget = sii_get_widget_ptr (frame_num, FRAME_SWPFI_DISPLAY );
  /* GTK4: window positioning handled by window manager */
  gtk_widget_set_visible (widget, TRUE);

  mark = 0;
}

/* c---------------------------------------------------------------------- */

void sii_swpfi_widget( guint frame_num )
{
  GtkWidget *label;

  GtkWidget *window;
  GtkWidget *scrolledwindow;
  GtkWidget *vbox;
  GtkWidget *edit_menubar;
  GtkWidget *hbox;
  GtkWidget *hbbox;

  GtkWidget *vbox0;
  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *vbox3;

  GtkWidget *text;
  GtkWidget *entry;
  GtkWidget *grid;

  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;

  GtkWidget *button;

  gchar str[256], *aa, *bb, *ee;
  const gchar *cc, *tt = "00/00/2000 00:00:00 HHHHHHHH 000.0 HHH HHHHHHHH";
  gint length, width, height, mm, nn, mark, jj;
  gint row;
  GString *gstr = g_string_new ("");
  guint widget_id = FRAME_SWPFI_MENU, arg_id;
  LinksInfo *linkfo;
  const GString *cgs;
  gdouble dtime;
  const struct solo_list_mgmt *slm;
  SwpfiData *sd;
				/* c...code */

  sii_update_swpfi_widget (frame_num);
  sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;

  window = gtk_window_new();
  sii_set_widget_ptr ( frame_num, widget_id, window );
  /* GTK4: window positioning handled by window manager */

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num*TASK_MODULO+FRAME_SWPFI_MENU));

  /* --- Title and border --- */
  bb = g_strdup_printf ("Frame %d  Sweepfiles Widget", frame_num+1 );
  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );

  /* --- Create a new vertical box for storing widgets --- */
  vbox0 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW(window), vbox0);

  sii_swpfi_menubar2 (window, &menubar, frame_num);
  gtk_box_append (GTK_BOX(vbox0), menubar);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append (GTK_BOX(vbox0), vbox);
  gtk_widget_set_margin_start (vbox, 4);
  gtk_widget_set_margin_end (vbox, 4);
  gtk_widget_set_margin_top (vbox, 4);
  gtk_widget_set_margin_bottom (vbox, 4);

  hbbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append (GTK_BOX (vbox), hbbox);

  button = gtk_button_new_with_label ("Sweeps");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbbox), button);

  nn = frame_num*TASK_MODULO + SWPFI_LIST_SWPFIS;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_swpfi_menu_cb)
		      , GUINT_TO_POINTER(nn)
		      );

  button = gtk_button_new_with_label ("Replot");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbbox), button);

  nn = frame_num*TASK_MODULO + SWPFI_REPLOT_THIS;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_swpfi_menu_cb)
		      , GUINT_TO_POINTER(nn)
		      );
  button = gtk_button_new_with_label ("OK");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbbox), button);
  nn = frame_num*TASK_MODULO + SWPFI_OK;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_swpfi_menu_cb)
		      , GUINT_TO_POINTER(nn)
		      );
  button = gtk_button_new_with_label ("Cancel");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbbox), button);
  nn = frame_num*TASK_MODULO + SWPFI_CANCEL;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_swpfi_menu_cb)
		      , GUINT_TO_POINTER(nn)
		      );
  widget_id = SWPFI_CURRENT_SWPFI;
  strcpy (str, tt );
  cc = sii_return_swpfi_label (frame_num);
  label = gtk_label_new ( cc );
  sd->data_widget[widget_id] = label;
  gtk_box_append (GTK_BOX(vbox), label);
  sd->orig_txt[widget_id] = g_string_new (cc);

  grid = gtk_grid_new ();
  gtk_box_append (GTK_BOX (vbox), grid);
  row = -1;

  row++;
  label = gtk_label_new ( " Directory " );
  gtk_widget_set_hexpand (label, TRUE);
  gtk_widget_set_vexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  widget_id = SWPFI_DIR;
  entry = gtk_entry_new ();
  cc = sii_return_swpfi_dir (frame_num);
  gtk_editable_set_text (GTK_EDITABLE (entry), cc);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_widget_set_vexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 3, 1);
  sd->data_widget[widget_id] = entry;
  sd->entry_flag[widget_id] = TRUE;
  sd->num_values[widget_id] = 1;
  sd->orig_txt[widget_id] = g_string_new (cc);
  sd->txt[widget_id] = g_string_new (cc);

  row++;
  label = gtk_label_new ( " Radar " );
  gtk_widget_set_hexpand (label, TRUE);
  gtk_widget_set_vexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  widget_id = SWPFI_RADAR_NAME;
  entry = gtk_entry_new ();
  cc = sii_return_swpfi_radar (frame_num);
  gtk_editable_set_text (GTK_EDITABLE (entry), cc);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_widget_set_vexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 3, 1);
  sd->data_widget[widget_id] = entry;
  sd->entry_flag[widget_id] = TRUE;
  sd->num_values[widget_id] = 1;
  sd->orig_txt[widget_id] = g_string_new (cc);
  sd->txt[widget_id] = g_string_new (cc);


  row++;
  label = gtk_label_new ( " Start Time " );
  gtk_widget_set_hexpand (label, TRUE);
  gtk_widget_set_vexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  dtime = sii_return_swpfi_time_stamp (frame_num);
  cc = sii_string_from_dtime (dtime);
  widget_id = SWPFI_START_TIME;
  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), cc);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_widget_set_vexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 3, 1);
  sd->data_widget[widget_id] = entry;
  sd->entry_flag[widget_id] = TRUE;
  sd->num_values[widget_id] = 1;
  sd->orig_txt[widget_id] = g_string_new (cc);
  sd->txt[widget_id] = g_string_new (cc);

  row++;
  label = gtk_label_new ( " Stop Time " );
  gtk_widget_set_hexpand (label, TRUE);
  gtk_widget_set_vexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  widget_id = SWPFI_STOP_TIME;
  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), cc);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_widget_set_vexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 3, 1);
  sd->data_widget[widget_id] = entry;
  sd->entry_flag[widget_id] = TRUE;
  sd->num_values[widget_id] = 1;
  sd->orig_txt[widget_id] = g_string_new (cc);
  sd->txt[widget_id] = g_string_new (cc);

  row++;
  label = gtk_label_new ( " Scan Modes " );
  gtk_widget_set_hexpand (label, TRUE);
  gtk_widget_set_vexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 2, row, 1, 1);
  label = gtk_label_new ( " Angle, Tolerance " );
  gtk_widget_set_hexpand (label, TRUE);
  gtk_widget_set_vexpand (label, TRUE);
  gtk_grid_attach (GTK_GRID (grid), label, 3, row, 1, 1);

  row++;
  widget_id = SWPFI_FILTER;
  button = gtk_check_button_new_with_label ("Filter");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_widget_set_vexpand (button, TRUE);
  gtk_grid_attach (GTK_GRID (grid), button, 0, row, 1, 1);
  nn = frame_num * TASK_MODULO + SWPFI_FILTER;
  g_signal_connect (G_OBJECT(button)
		      ,"toggled"
		      , G_CALLBACK (sii_swpfi_menu_cb)
		      , GUINT_TO_POINTER(nn)
		      );
  sd->data_widget[widget_id] = button;


  widget_id = SWPFI_SCAN_MODES;
  entry = gtk_entry_new ();
  cc = "PPI,SUR";
  gtk_editable_set_text (GTK_EDITABLE (entry), cc);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_widget_set_vexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 2, row, 1, 1);
  sd->data_widget[widget_id] = entry;
  sd->entry_flag[widget_id] = TRUE;
  sd->num_values[widget_id] = 1;
  sd->orig_txt[widget_id] = g_string_new (cc);
  sd->txt[widget_id] = g_string_new (cc);

  widget_id = SWPFI_FIXED_INFO;
  cc = "0.5, 0.25";
  sd->precision = 3;
  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), cc);
  gtk_widget_set_hexpand (entry, TRUE);
  gtk_widget_set_vexpand (entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 3, row, 1, 1);
  sd->data_widget[widget_id] = entry;
  sd->entry_flag[widget_id] = TRUE;
  sd->num_values[widget_id] = 1;
  sd->orig_txt[widget_id] = g_string_new (cc);
  sd->txt[widget_id] = g_string_new (cc);

  widget_id = SWPFI_TIME_SYNC;
  button = gtk_check_button_new_with_label
    ("Link time to frame 1");
  sd->data_widget[widget_id] = button;
  gtk_box_append (GTK_BOX (vbox), button);
  nn = frame_num * TASK_MODULO + widget_id;
  g_signal_connect (G_OBJECT(button)
		      ,"toggled"
		      , G_CALLBACK (sii_swpfi_menu_cb)
		      , GUINT_TO_POINTER(nn)
		      );
# ifdef notyet
# endif


  sii_update_swpfi_widget (frame_num);


  for (jj=0; jj < SWPFI_MAX_WIDGETS; jj++ ) {

     if (sd->entry_flag[jj]) {
	nn = frame_num * TASK_MODULO + jj;
	g_signal_connect (G_OBJECT(sd->data_widget[jj])
			    ,"activate"
			    , G_CALLBACK (sii_swpfi_menu_cb)
			    , GUINT_TO_POINTER(nn) );
	/* paste_clipboard signal removed - not available in GTK4 */
     }
  }
  /* --- Make everything visible --- */
  gtk_widget_set_visible (window, TRUE);


  mark = 0;
}

/* c---------------------------------------------------------------------- */

/* GtkItemFactory menu definitions removed for GTK4 -- using button bar approach */

/* c---------------------------------------------------------------------- */

void sii_swpfi_menubar2( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num)
{
   gint jj, nn, pdi, task, active, widget_id;
   guint radio_num = 0;
   GtkWidget *check_item, *mbar, *menuitem, *submenu, *next_item;
   SwpfiData *sd = (SwpfiData *)frame_configs[frame_num]->swpfi_data;

   radio_group = NULL;

   *menubar = mbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

   submenu = sii_submenu ( "File", mbar );

   widget_id = SWPFI_CLOSE;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Close", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   widget_id = SWPFI_RADAR_LIST;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "List Radars", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

# ifdef notyet
   widget_id = SWPFI_RESCAN;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Rescan Dir", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   /* add a separator */
   menuitem = sii_submenu_item ( NULL, submenu, 0, NULL, frame_num );
# endif

   radio_num = 0;
   widget_id = SWPFI_ELECTRIC;
   sd->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Electric Swpfis", submenu, widget_id
			       , G_CALLBACK(sii_swpfi_menu_cb), frame_num, radio_num
			       , &radio_group);

# ifdef notyet
   /* add a separator */
   menuitem = sii_submenu_item ( NULL, submenu, 0, NULL, frame_num );

   widget_id = SWPFI_ZAP_SWPFIS;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Delete Swpfis", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );
# endif

   submenu = sii_submenu ( "SwpfiLinks", mbar );

   widget_id = SWPFI_LINKS;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Set Links", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   submenu = sii_submenu ( "Lockstep", mbar );

   widget_id = LOCKSTEP_LINKS;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Set Links", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   submenu = sii_submenu ( "Replot", mbar );

   widget_id = SWPFI_REPLOT_LINKS;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Replot Links", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   widget_id = SWPFI_REPLOT_ALL;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Replot All", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );


   submenu = sii_submenu ( "Help", mbar );

   widget_id = SWPFI_OVERVIEW;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Overview", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   widget_id = SWPFI_HLP_LINKS;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "With Links", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   widget_id = SWPFI_HLP_LOCKSTEP;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "With Lockstep", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   widget_id = SWPFI_HLP_TIMES;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "With Times", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   widget_id = SWPFI_HLP_FILTER;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "With the Filter", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

   widget_id = SWPFI_HLP_TSYNC;
   sd->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Time Link", submenu, widget_id
		       , G_CALLBACK(sii_swpfi_menu_cb), frame_num );

  check_item = sd->data_widget[SWPFI_ELECTRIC];
  gtk_check_button_set_active (GTK_CHECK_BUTTON (check_item), TRUE );
}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */
