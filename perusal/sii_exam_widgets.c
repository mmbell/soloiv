/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include "sii_utils.h"
# include "help_exam.h"
# include <gdk/gdkkeysyms.h>

# include <solo_window_structs.h>
# include <solo_editor_structs.h>
# include <sed_shared_structs.h>
# include <seds.h>

/* Examine Menu Actions  */

enum {
   EXAM_ZERO,
   EXAM_CANCEL,
   EXAM_CLOSE,

   EXAM_FIELDS_LIST,

   EXAM_REPLOT_THIS,
   EXAM_REPLOT_LINKS,
   EXAM_REPLOT_ALL,

   EXAM_DELETE,
   EXAM_NEG_FOLD,
   EXAM_POS_FOLD,
   EXAM_ZAP_GND_SPD,
   EXAM_DELETE_RAY,
   EXAM_NEG_FOLD_RAY,
   EXAM_POS_FOLD_RAY,
   EXAM_NEG_FOLD_SEG,
   EXAM_POS_FOLD_SEG,
   EXAM_NEG_FOLD_GT,
   EXAM_POS_FOLD_GT,
   EXAM_REPLACE,

   EXAM_DATA,
   EXAM_RAYS,
   EXAM_METADATA,
   EXAM_EDT_HIST,
   EXAM_REFRESH,

   EXAM_SCROLL_COUNT,
   EXAM_SCROLL_LEFT,
   EXAM_SCROLL_RIGHT,
   EXAM_SCROLL_UP,
   EXAM_SCROLL_DOWN,

   EXAM_UNDO,
   EXAM_CLEAR_CHANGES,
   EXAM_APPLY_CHANGES,

   EXAM_LST_RNGAZ,
   EXAM_LST_CELLRAY,

   EXAM_FORMAT,
   EXAM_RAY,
   EXAM_RAY_COUNT,
   EXAM_CELL,
   EXAM_RANGE,
   EXAM_CELL_COUNT,
   EXAM_CHANGE_COUNT,
   EXAM_ELECTRIC,

   EXAM_LAYOUT,
   EXAM_SCROLLED_WIN,
   EXAM_NYQVEL,
   EXAM_LOG_DIR,
   EXAM_LOG_ACTIVE,
   EXAM_LOG_FLUSH,
   EXAM_LOG_CLOSE,


   EXAM_OVERVIEW,
   EXAM_HLP_OPRS,
   EXAM_HLP_OPTIONS,
   EXAM_HLP_NYQVEL,
   EXAM_HLP_LOG_DIR,
   EXAM_HLP_,

   EXAM_LAST_ENUM,
};

# define EXAM_MAX_WIDGETS EXAM_LAST_ENUM


/* c---------------------------------------------------------------------- */

typedef struct {
   GtkWidget *data_widget[EXAM_MAX_WIDGETS];

   gboolean entry_flag[EXAM_MAX_WIDGETS];
   guint num_values[EXAM_MAX_WIDGETS];
   gdouble orig_values[EXAM_MAX_WIDGETS][2];
   gdouble values[EXAM_MAX_WIDGETS][2];
   guint precision[EXAM_MAX_WIDGETS];

   GString *orig_txt[EXAM_MAX_WIDGETS];
   GString *txt[EXAM_MAX_WIDGETS];

   gboolean toggle[EXAM_MAX_WIDGETS];

   gint equiv_solo_state[EXAM_MAX_WIDGETS];

   gboolean frame_active;
   guint display_state;
   guint operation_state;
   guint label_state;

   guint label_height;
   guint label_char_width;

   guint last_list_index;
   guint last_char_index;

   guint max_cells;
   guint max_possible_cells;
   guint max_chars_per_line;
   guint prior_display;
   guint prior_num_entries;

   gfloat r0_km;
   gfloat gs_km;
   gfloat clicked_range_km;
   gfloat nyq_vel;

   GString *log_dir;

   GtkWidget *label_bar0;
   GtkWidget *label_bar1;
   GtkWidget **label_items;

} ExamData;

static ExamData *xmdata;
static GSList *radio_group = NULL;

/* c---------------------------------------------------------------------- */

void sii_exam_return_click_indices (guint frame_num, gint *list_index
				    , gint *char_index);

void sii_exam_menubar( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num);
void sii_exam_menubar2( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num);
void sii_exam_widget( guint frame_num );

void show_exam_widget (GtkWidget *text, gpointer data );

void sp_cell_range_info (int frame_num, gfloat *r0, gfloat *gs, gint *ncells);

char *solo_list_entry(struct solo_list_mgmt *which, int entry_num);

struct solo_click_info *clear_click_info();

void sxm_process_click(struct solo_click_info *sci);

gfloat sp_nyquist_vel (int frame_num);

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

void sii_exam_return_click_indices (guint frame_num, gint *list_index
				    , gint *char_index)
{
   ExamData *xmd;
   xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   *list_index = xmd->last_list_index;
   *char_index = xmd->last_char_index;
}

/* c------------------------------------------------------------------------ */

void se_dump_examine_widget(int frame_num, struct examine_widget_info *ewi)
{
   gchar str[128];
   const gchar *aa;
   ExamData *xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   gint count, nn;

   aa = gtk_editable_get_text( GTK_EDITABLE (xmd->data_widget[EXAM_RAY]));
   count = sscanf (aa, "%d", &ewi->ray_num);

   aa = gtk_editable_get_text( GTK_EDITABLE (xmd->data_widget[EXAM_RAY_COUNT]));
   count = sscanf (aa, "%d", &ewi->ray_count);

   aa = gtk_editable_get_text( GTK_EDITABLE (xmd->data_widget[EXAM_FORMAT]));
   strcpy (ewi->display_format, aa);

   aa = gtk_editable_get_text( GTK_EDITABLE (xmd->data_widget[EXAM_FIELDS_LIST]));
   strcpy (ewi->fields_list, aa);

   ewi->whats_selected = xmd->equiv_solo_state[xmd->display_state];
   ewi->typeof_change = xmd->equiv_solo_state[xmd->operation_state];
   if (xmd->toggle[EXAM_LST_CELLRAY]) {
      ewi->row_annotation = EX_VIEW_CELL_NUMS;
      ewi->col_annotation = EX_VIEW_RAY_NUMS;
   }
   else {
      ewi->row_annotation = EX_VIEW_RANGES;
      ewi->col_annotation = EX_VIEW_ROT_ANGS;
   }
   ewi->at_cell = 0;
   ewi->cell_count = -1;
}

/* c------------------------------------------------------------------------ */

void se_refresh_examine_list(int frame_num, struct solo_list_mgmt  *slm)
{
   ExamData *xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   WW_PTR wwptr = solo_return_wwptr (frame_num);
   gint jj, nn = 64, gate, lim;
   gchar *aa, *bb, str[256];
   GtkWidget *label, *widget;
   GtkAdjustment *adj;
   GtkLabel *lbl;
   gint value;
   guint height;
   float f;

   label = xmd->label_items[0];
   if (!label)
     { return; }

   if (!slm) {
      slm = wwptr->examine_list;
   }
   if (!slm || !slm->num_entries)
     { return; }

   if (slm->num_entries <= xmd->max_possible_cells)
     { lim = slm->num_entries; }
   else {
      lim = xmd->max_possible_cells-8;
      sprintf (str, "Max lines:%d Lines needed:%d"
	       , xmd->max_possible_cells, slm->num_entries);
      sii_message (str);
   }

   for (jj=0; jj < lim; jj++) {
      aa = bb = solo_list_entry (slm, jj);
      if (!aa)
	{ break; }
# ifndef notyet
      if (jj < 2) {
	label = (jj == 0) ? xmd->label_bar0 : xmd->label_bar1;
	if (xmd->display_state == EXAM_DATA)
	  { bb = ""; }
	else
	  { aa = ""; }
	gtk_label_set_text (GTK_LABEL (label), aa );
	gtk_label_set_text (GTK_LABEL (xmd->label_items[jj]), bb );
      }
      else {
	gtk_label_set_text (GTK_LABEL (xmd->label_items[jj]), aa );
      }
# else
      gtk_label_set_text (GTK_LABEL (xmd->label_items[jj]), aa );
# endif
   }
   if (lim < xmd->prior_num_entries) {
     aa = "";
     for (; jj < xmd->prior_num_entries; jj++) {
       gtk_label_set_text (GTK_LABEL (xmd->label_items[jj]), aa );
     }
   }
   xmd->prior_num_entries = lim;

   if (xmd->clicked_range_km > 0 && xmd->display_state == EXAM_DATA) {
     /* shift data display to clicked range */

     widget = xmd->data_widget[EXAM_LAYOUT];
     adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widget));
     height = gtk_widget_get_height(widget);
     nn = height/xmd->label_height; /* lines visible */
     jj = (gint)((xmd->clicked_range_km -xmd->r0_km)/xmd->gs_km);
     jj -= nn/2;
     value = ( jj +2) * xmd->label_height;
     if (value < 1 )
       { value = 1; }
     if (value + nn * xmd->label_height > gtk_adjustment_get_upper(adj))
       { value = gtk_adjustment_get_upper(adj) - nn *xmd->label_height; }

     gtk_adjustment_set_value (adj, value);
   }
   xmd->clicked_range_km = 0;
}

/* c------------------------------------------------------------------------ */

void se_update_exam_widgets (gint frame_num)
{
   ExamData *   xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   struct solo_edit_stuff *seds, *return_sed_stuff();
   gfloat ff;
   guint wid;

   wid = EXAM_FORMAT;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

   wid = EXAM_RAY;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

   wid = EXAM_RAY_COUNT;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

   wid = EXAM_FIELDS_LIST;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

   wid = EXAM_CELL;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

   wid = EXAM_RANGE;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

   wid = EXAM_CHANGE_COUNT;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

   wid = EXAM_NYQVEL;
   seds = return_sed_stuff();
   ff = seds->nyquist_velocity;
   if (!ff) {
     g_string_assign (xmd->orig_txt[wid], "0");
   }
   else {
     g_string_printf (xmd->orig_txt[wid], "%.3f"
		       , seds->nyquist_velocity);
   }
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);

# ifdef obsolete
   wid = EXAM_LOG_DIR;
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid])
		       , xmd->orig_txt[wid]->str);
# endif
}

/* c------------------------------------------------------------------------ */

void se_refresh_examine_widgets (int frame_num, struct solo_list_mgmt  *slm)
{

   ExamData *xmd;
   gchar str[128], *aa;
   gint jj, nn, ncells;
   WW_PTR wwptr = solo_return_wwptr (frame_num);
   GtkWidget *widget;
   gfloat ff, r0, gs, nyq_vel;
   struct solo_edit_stuff *seds, *return_sed_stuff();

   sp_cell_range_info (frame_num, &r0, &gs, &ncells);
   nyq_vel = sp_nyquist_vel (frame_num);

   if( !frame_configs[frame_num]->exam_data ) {
      xmd =
	frame_configs[frame_num]->exam_data = g_malloc0 (sizeof( ExamData ));
     xmd->max_possible_cells = xmd->max_cells = 2064;
     xmd->max_chars_per_line = 256;
     xmd->label_items = (GtkWidget **)g_malloc0
       (xmd->max_possible_cells * sizeof (GtkWidget *));

      xmd->entry_flag[EXAM_RAY] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_RAY_COUNT] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_CHANGE_COUNT] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_CELL] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_RANGE] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_FIELDS_LIST] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_CELL_COUNT] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_FORMAT] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_LOG_DIR] = ENTRY_TXT_ONLY;
      xmd->entry_flag[EXAM_NYQVEL] = ENTRY_TXT_ONLY;

      for (jj=0; jj < EXAM_MAX_WIDGETS; jj++) {
	 if (xmd->entry_flag[jj])
	   { xmd->orig_txt[jj] = g_string_new (""); }
      }
   }
   xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   xmd->max_cells = ncells;
   xmd->r0_km = r0;
   xmd->gs_km = gs;

   se_refresh_examine_list(frame_num, slm);

   for (jj=0; jj < EXAM_MAX_WIDGETS; jj++) {
      if (xmd->entry_flag[jj]) {
	 if (xmd->orig_txt[jj])
	   { g_string_truncate (xmd->orig_txt[jj], 0); }
	 else
	   { xmd->orig_txt[jj] = g_string_new (""); }
      }
   }

   g_string_append_printf (xmd->orig_txt[EXAM_RAY]
		      ,"%d", wwptr->examine_info->ray_num);
   g_string_append_printf (xmd->orig_txt[EXAM_RAY_COUNT]
		     ,"%d", wwptr->examine_info->ray_count);
   g_string_append_printf (xmd->orig_txt[EXAM_CHANGE_COUNT]
		      ,"%d", wwptr->examine_info->change_count);

   nn = wwptr->examine_info->at_cell;
   g_string_append_printf(xmd->orig_txt[EXAM_CELL],"%d", nn);

   ff = xmd->r0_km + nn * xmd->gs_km;
   g_string_append_printf (xmd->orig_txt[EXAM_RANGE], "%.2f", ff);

   g_string_assign
     (xmd->orig_txt[EXAM_FIELDS_LIST], wwptr->examine_info->fields_list);

   g_string_append_printf(xmd->orig_txt[EXAM_CELL_COUNT]
		     ,"%d",wwptr->examine_info->cell_count);
   g_string_assign
     (xmd->orig_txt[EXAM_FORMAT],wwptr->examine_info->display_format);

   seds = return_sed_stuff();
   ff = seds->nyquist_velocity;
   if (!ff) {
     g_string_assign (xmd->orig_txt[EXAM_NYQVEL], "0");
   }
   else {
     g_string_printf (xmd->orig_txt[EXAM_NYQVEL], "%.3f"
		       , seds->nyquist_velocity);
   }
   if (xmd->data_widget[EXAM_FIELDS_LIST]) {
      se_update_exam_widgets (frame_num);
   }
}

/* c---------------------------------------------------------------------- */

void show_exam_widget (GtkWidget *text, gpointer data )
{
  GtkWidget *widget;
  guint frame_num = GPOINTER_TO_UINT (data);
  ExamData *xmd;
  gint x, y, task;
  struct solo_click_info *sci = clear_click_info();

  widget = sii_get_widget_ptr (frame_num, FRAME_EXAM_MENU);

  if( !widget )
    { sii_exam_widget( frame_num ); }
  else {
     xmd = frame_configs[frame_num]->exam_data;
     xmd->frame_active = TRUE;

# ifdef obsolete
     if (xmd->toggle[EXAM_DATA])
       { task = EXAM_DATA; }
     else if (xmd->toggle[EXAM_RAYS])
       { task = EXAM_RAYS; }
     else if (xmd->toggle[EXAM_METADATA])
       { task = EXAM_METADATA; }
     else if (xmd->toggle[EXAM_EDT_HIST])
       { task = EXAM_EDT_HIST; }
# endif
     sci->frame = frame_num;
     sxm_refresh_list (sci);
     se_update_exam_widgets (frame_num);

     gtk_widget_set_visible (widget, TRUE);

     widget = sii_get_widget_ptr (frame_num, FRAME_EXAM_DISPLAY );
     gtk_widget_set_visible (widget, TRUE);
  }
}

/* c---------------------------------------------------------------------- */

void sii_exam_list_event (GtkWidget *w, GdkEvent *event
			      , gpointer data )
{
   GtkWidget *label = GTK_WIDGET (data);
   guint num = GPOINTER_TO_UINT (data);
   guint frame_num, task, wid, exam_index;
   ExamData *xmd;
   gint xx, yy, cndx;
   GdkModifierType state;
   struct solo_click_info *sci = clear_click_info();

   sci->frame =
     frame_num =
       GPOINTER_TO_UINT (g_object_get_data
			 (G_OBJECT(label), "frame_num" ));

   xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   if (xmd->display_state != EXAM_DATA)
     { return; }

   sci->which_list_entry =
     exam_index =
       GPOINTER_TO_UINT (g_object_get_data
			 (G_OBJECT(label), "exam_index" ));

   xmd->last_list_index = exam_index;
   /* In GTK4, pointer position is obtained from the event or controller */
   xx = 0; yy = 0; state = 0;
   sci->which_character =
     xmd->last_char_index =
       cndx = xx/xmd->label_char_width;

   sci->which_widget_button = EX_CLICK_IN_LIST;
   sxm_process_click (sci);

   g_message ("Exam_list: f:%d i:%d c:%d x:%d y:%d s:%d"
	      , frame_num, exam_index, cndx, xx, yy, state);
}

/* c---------------------------------------------------------------------- */

gboolean sii_exam_keyboard_event (GtkEventControllerKey *controller,
                                  guint keyval, guint keycode,
                                  GdkModifierType state, gpointer data)
{
   guint num = GPOINTER_TO_UINT (data);
   guint frame_num, task, wid, height;
   gint value;
   ExamData *xmd;
   GtkWidget *widget;
   GtkAdjustment *adj;
   const gchar *aa;
   struct solo_click_info *sci = clear_click_info();

   sci->frame =
     frame_num =
       num/TASK_MODULO;
   wid =
     task =
       num % TASK_MODULO;

   xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   aa = gdk_keyval_name(keyval);

   g_message ("Got keyboard event f:%d w:%d k:%d s:%s"
	      , frame_num, wid, keyval, aa ? aa : "?" );

   switch(keyval) {

    case GDK_KEY_Right:
      /* move one ray to the right */
    case GDK_KEY_Left:
      /* move one ray to the left */
      sci->which_widget_button = (keyval == GDK_KEY_Left)
	? EX_SCROLL_LEFT : EX_SCROLL_RIGHT;
      sxm_process_click (sci);
      break;

    case GDK_KEY_Up:
      /* shift up one screen */
    case GDK_KEY_Down:
      /* shift down one screen */

      widget = xmd->data_widget[EXAM_LAYOUT];
      height = gtk_widget_get_height(widget) -xmd->label_height;

      adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widget));
      value = gtk_adjustment_get_value(adj);
      value += (keyval == GDK_KEY_Up) ? -height : height;

      if (value < 1)
	{ value = 1; }
      if (value +height >= gtk_adjustment_get_upper(adj))
	{ value = gtk_adjustment_get_upper(adj) -height; }

      gtk_adjustment_set_value (adj, value);

      break;

   };
   return TRUE;
}

/* c---------------------------------------------------------------------- */

void sii_exam_click_in_data (guint frame_num)
{
   struct solo_click_info *sci = clear_click_info();
   ExamData *xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   WW_PTR wwptr = solo_return_wwptr (frame_num);

   if (!xmd || !xmd->frame_active)
     { return; }
   sci->frame = frame_num;
   xmd->clicked_range_km = wwptr->clicked_range;
   sxm_click_in_data(sci);
}

/* c---------------------------------------------------------------------- */

void sii_exam_menu_cb ( GtkWidget *w, gpointer   data )
{
   guint num = GPOINTER_TO_UINT (data);
   guint frame_num, task, wid, frame_wid, cancel_wid, active, height;
   gint nn, jj, value, cells;
   gchar str[128], *sptrs[16];
   const gchar *aa, *bb, *line;
   ExamData *xmd;
   GtkWidget *widget, *check_item, *rmi, *layout;
   GtkAdjustment *adj;
   gfloat ff, gg;
   struct solo_click_info *sci = clear_click_info();
   WW_PTR wwptr;
   gint x, y;
   struct solo_edit_stuff *seds, *return_sed_stuff();



   frame_num = num/TASK_MODULO;
   xmd = (ExamData *)frame_configs[frame_num]->exam_data;
   wwptr = solo_return_wwptr (frame_num);
   wid = task = num % TASK_MODULO;

   widget =
     check_item =
       rmi = xmd->data_widget[task];

   sci->frame = frame_num;
   sci->which_widget_button = xmd->equiv_solo_state[task];

			/* c...code */

   switch (task) {

    case EXAM_CANCEL:
    case EXAM_CLOSE:
      widget = sii_get_widget_ptr (frame_num, FRAME_EXAM_MENU);
      if( widget ) {
	gtk_widget_set_visible (widget, FALSE);
      }
      widget = sii_get_widget_ptr (frame_num, FRAME_EXAM_DISPLAY);
      if( widget ) {
	gtk_widget_set_visible (widget, FALSE);
      }
      xmd->frame_active = FALSE;
      break;

    case EXAM_DATA:
    case EXAM_RAYS:
    case EXAM_METADATA:
    case EXAM_EDT_HIST:
      g_return_if_fail (GTK_IS_CHECK_BUTTON(rmi));

      active = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_item));
# ifdef obsolete
      g_message ( "Exam Task: %d  State: %d  Active: %d"
		 , task, xmd->toggle[task], active );
# endif
      xmd->toggle[task] = active;

      if(active) {
	 xmd->display_state = task;
	 sxm_process_click(sci);
      }
      break;

    case EXAM_REFRESH:
     sxm_refresh_list (sci);
     se_update_exam_widgets (frame_num);
      break;

    case EXAM_DELETE:
    case EXAM_NEG_FOLD:
    case EXAM_POS_FOLD:
    case EXAM_ZAP_GND_SPD:
    case EXAM_DELETE_RAY:
    case EXAM_NEG_FOLD_RAY:
    case EXAM_POS_FOLD_RAY:
    case EXAM_NEG_FOLD_GT:
    case EXAM_POS_FOLD_GT:
    case EXAM_REPLACE:
      g_return_if_fail (GTK_IS_CHECK_BUTTON(rmi));

      active = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_item));
# ifdef obsolete
      g_message ( "Exam Task: %d  State: %d  Active: %d"
		 , task, xmd->toggle[task], active );
# endif
      xmd->toggle[task] = active;

      if(active) {
	 xmd->operation_state = task;
	 sxm_process_click(sci);
      }
      break;

    case EXAM_LST_RNGAZ:
    case EXAM_LST_CELLRAY:
      g_return_if_fail (GTK_IS_CHECK_BUTTON(rmi));

      active = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_item));
# ifdef obsolete
      g_message ( "Exam Task: %d  State: %d  Active: %d"
		 , task, xmd->toggle[task], active );
# endif
      xmd->toggle[task] = active;

      if( active) {
	 xmd->label_state = task;
	 sxm_refresh_list (sci);
      }

      break;

    case EXAM_REPLOT_THIS:
    case EXAM_REPLOT_LINKS:
    case EXAM_REPLOT_ALL:
      sii_plot_data (frame_num, xmd->equiv_solo_state[task]);
      break;

    case EXAM_ELECTRIC:
      xmd->toggle[task] = gtk_check_button_get_active(GTK_CHECK_BUTTON(rmi));
      break;

    case EXAM_RAY:
    case EXAM_RAY_COUNT:
    case EXAM_FORMAT:
    case EXAM_FIELDS_LIST:
      sxm_refresh_list (sci);
      break;

    case EXAM_RANGE:
    case EXAM_CELL:

      layout = xmd->data_widget[EXAM_LAYOUT];
      height = gtk_widget_get_height(layout);
      adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(layout));

      widget = xmd->data_widget[wid];
      aa = gtk_editable_get_text( GTK_EDITABLE (widget));
      if (wid == EXAM_RANGE) {
	 ff = -32768;
	 jj = sscanf (aa, "%f", &ff);
	 nn = (gint)((ff - xmd->r0_km)/xmd->gs_km +.5);
      }
      else {
	 nn = -1;
	 jj = sscanf (aa, "%d", &nn);
      }
      if (nn < 0)
	{ nn = 0; }
      cells = height/xmd->label_height;
      if (nn +cells >= xmd->max_cells)
	{ nn = xmd->max_cells -cells; }

      value = (nn +2) * xmd->label_height +1;

      sprintf (str, "%d", nn);
      gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[EXAM_CELL]), str);

      ff = xmd->r0_km + nn * xmd->gs_km;
      sprintf (str, "%.2f", ff);
      gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[EXAM_RANGE]), str);

      gtk_adjustment_set_value (adj, value);

      break;

    case EXAM_UNDO:
    case EXAM_CLEAR_CHANGES:
      sxm_process_click(sci);
      break;

    case EXAM_APPLY_CHANGES:
      sxm_process_click(sci);

      if (task == EXAM_APPLY_CHANGES && xmd->toggle[EXAM_ELECTRIC]) {
	 sii_plot_data (frame_num, xmd->equiv_solo_state[EXAM_REPLOT_LINKS]);
      }
      break;

   case EXAM_NYQVEL:
     aa = gtk_editable_get_text (GTK_EDITABLE (xmd->data_widget[task]));
     if (!strlen (aa))
       { ff = 0; }
     else {
       sii_str_values (aa, 1, &ff, &gg);
       if (ff < 0)
	 { ff = 0; }
     }
     if (!ff) {
       g_string_assign (xmd->orig_txt[task], "0");
     }
     else {
       g_string_printf (xmd->orig_txt[task], "%.3f", ff);
     }
     seds = return_sed_stuff();
     seds->nyquist_velocity = ff;
     gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[task])
			      , xmd->orig_txt[task]->str);
     break;

   case EXAM_LOG_DIR:
     aa = gtk_editable_get_text (GTK_EDITABLE (xmd->data_widget[task]));
     g_string_assign (xmd->orig_txt[task], aa);
     sxm_set_log_dir (aa);
     check_item = xmdata->data_widget[EXAM_LOG_ACTIVE];
     gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), TRUE);
     break;

   case EXAM_LOG_ACTIVE:
      xmd->toggle[task] = gtk_check_button_get_active(GTK_CHECK_BUTTON(rmi));
      sxm_toggle_log_dir ((xmd->toggle[task]) ? 1 : 0);
     break;

   case EXAM_LOG_CLOSE:
     sxm_close_log_stream();
     break;

   case EXAM_LOG_FLUSH:
     sxm_flush_log_stream();
     break;

   case EXAM_OVERVIEW:
     nn = sizeof (hlp_exam_overview)/sizeof (char *);
     sii_glom_strings (hlp_exam_overview, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case EXAM_HLP_OPRS:
     nn = sizeof (hlp_exam_operations)/sizeof (char *);
     sii_glom_strings (hlp_exam_operations, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case EXAM_HLP_OPTIONS:
     nn = sizeof (hlp_exam_options)/sizeof (char *);
     sii_glom_strings (hlp_exam_options, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case EXAM_HLP_NYQVEL:
     nn = sizeof (hlp_exam_nyqvel)/sizeof (char *);
     sii_glom_strings (hlp_exam_nyqvel, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case EXAM_HLP_LOG_DIR:
     nn = sizeof (hlp_exam_logdir)/sizeof (char *);
     sii_glom_strings (hlp_exam_logdir, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   };
}

/* c---------------------------------------------------------------------- */

void sii_exam_entry_paste_cb ( GtkWidget *w, gpointer   data )
{
   GtkWidget *widget;
   guint num = GPOINTER_TO_UINT (data);
   guint frame_num, wid;
   ExamData *xmd;

   frame_num = num/TASK_MODULO;
   wid = num % TASK_MODULO;
   xmd = (ExamData *)frame_configs[frame_num]->exam_data;

			/* clear out the entry befor pasting */
   gtk_editable_set_text (GTK_EDITABLE (xmd->data_widget[wid]), "");
}

/* c---------------------------------------------------------------------- */

guint exam_list_cb(GtkWidget *label, gpointer data )
{
   GtkWidget *button = GTK_WIDGET (data);
   guint frame_num, exam_index;

   frame_num = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button),
			  "frame_num" ));
   exam_index = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button),
			  "exam_index" ));

   g_message( "layout widget button fn: %d  exam_index: %d"
	     , frame_num, exam_index );

   return FALSE;
}

/* c---------------------------------------------------------------------- */

guint exam_text_event_cb(GtkWidget *text, GdkEvent *event
			      , gpointer data )
{
  guint frame_num, wid;
  gint nn, mark, start, end;
  gchar *aa, *bb, *line;
  ExamData *xmd;
  gboolean ok;

  frame_num = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT(text), "frame_num" ));

  wid  = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT(text), "widget_id" ));

  xmd = (ExamData *)frame_configs[frame_num]->exam_data;

  aa = (gchar *)data;

  mark = 0;
}

/* c---------------------------------------------------------------------- */

void sii_exam_entry_info
( GtkWidget *w, guint wid, guint frame_num, guint num_values )
{
   xmdata->data_widget[wid] = w;
   xmdata->entry_flag[wid] = TRUE;
   xmdata->num_values[wid] = num_values;
}

/* c---------------------------------------------------------------------- */

void sii_exam_widget( guint frame_num )
{
  GtkWidget *label;

  GtkWidget *window;
  GtkWidget *scrolledwindow;
  GtkWidget *edit_menubar;
  GtkWidget *hbox0;
  GtkWidget *hbox1;
  GtkWidget *hbox2;
  GtkWidget *hbox3;
  GtkWidget *hbbox;

  GtkWidget *vbox00;
  GtkWidget *vbox0;
  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *vbox3;

  GtkWidget *text;
  GtkWidget *entry;
  GtkWidget *event;
  GtkWidget *table;
  GtkWidget *hscrollbar;
  GtkWidget *vscrollbar;

  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *layout;

  GtkWidget *button;

  gint event_flags = 0; /* GTK4: event masks replaced by controllers */
  gchar str[256], *aa, *bb, *ee, ch, A = 'A';
  const gchar *cc, *tt = "00/00/2000 00:00:00 HHHHHHHH 000.0 HHH HHHHHHHH";
  gint length, width, height, mm, nn, mark, jj, kk, len;
  gint x, y, row;
  GString *gstr = g_string_new ("");
  guint widget_id = FRAME_EXAM_MENU, arg_id;

			/* c...code */

  sxm_update_examine_data (frame_num, 0);

  if( !frame_configs[frame_num]->exam_data ) {
     xmdata =
       frame_configs[frame_num]->exam_data = g_malloc0 (sizeof( ExamData ));
     xmdata->max_possible_cells = xmdata->max_cells = 2064;
     xmdata->max_chars_per_line = 256;
     xmdata->label_items = (GtkWidget **)g_malloc0
       (xmdata->max_possible_cells * sizeof (GtkWidget *));
  }
  xmdata = (ExamData *)frame_configs[frame_num]->exam_data;
  xmdata->frame_active = TRUE;

  window = gtk_window_new();
  sii_set_widget_ptr ( frame_num, widget_id, window );

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num*TASK_MODULO+widget_id));

  bb = g_strdup_printf ("Frame %d  Examine Menu", frame_num+1 );

  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );


  /* --- Create a new vertical box for storing widgets --- */
  vbox0 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW(window), vbox0);

  sii_exam_menubar2 (window, &menubar, frame_num);
  gtk_box_append (GTK_BOX(vbox0), menubar);



  table = gtk_grid_new ();
  gtk_box_append (GTK_BOX(vbox0), table);
  row = -1;

  row++;
  label = gtk_label_new ( " Fields " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  jj = 7; kk = jj+1;
  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "DZ VE RHOHV PHIDP");
  sii_exam_entry_info( entry, EXAM_FIELDS_LIST, frame_num, 2 );
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 3, 1);

  row++;
  label = gtk_label_new ( " Format " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "6.1f");
  sii_exam_entry_info( entry, EXAM_FORMAT, frame_num, 2 );
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);

  label = gtk_label_new ( " Ray " );
  gtk_grid_attach (GTK_GRID (table), label, 2, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "0");
  sii_exam_entry_info( entry, EXAM_RAY, frame_num, 2 );
  gtk_grid_attach (GTK_GRID (table), entry, 3, row, 1, 1);

  row++;
  label = gtk_label_new ( " Changes " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "0");
  gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE );
  sii_exam_entry_info( entry, EXAM_CHANGE_COUNT, frame_num, 2 );
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);

  label = gtk_label_new ( " Rays " );
  gtk_grid_attach (GTK_GRID (table), label, 2, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "5");
  sii_exam_entry_info( entry, EXAM_RAY_COUNT, frame_num, 2 );
  gtk_grid_attach (GTK_GRID (table), entry, 3, row, 1, 1);

  row++;
  sprintf (str, "%.3f", xmdata->r0_km );
  label = gtk_label_new ( " Range " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "0");
  sii_exam_entry_info( entry, EXAM_RANGE, frame_num, 2 );
  xmdata->precision[EXAM_RANGE] = 3;
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);

  label = gtk_label_new ( " Cell " );
  gtk_grid_attach (GTK_GRID (table), label, 2, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (entry), "0");
  sii_exam_entry_info( entry, EXAM_CELL, frame_num, 2 );
  gtk_grid_attach (GTK_GRID (table), entry, 3, row, 1, 1);

  row++;
  label = gtk_label_new ( " Nyq Vel " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

  widget_id = EXAM_NYQVEL;
  entry = gtk_entry_new ();
  sii_exam_entry_info( entry, widget_id, frame_num, 1 );
  xmdata->precision[widget_id] = 2;
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  label = gtk_label_new ( " 0 Implies the default Nyq Vel " );
  gtk_grid_attach (GTK_GRID (table), label, 2, row, 2, 1);

  row++;
  label = gtk_label_new ( " Log Dir " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

  widget_id = EXAM_LOG_DIR;
  entry = gtk_entry_new ();
  xmdata->log_dir = g_string_new ("./");
  gtk_editable_set_text (GTK_EDITABLE (entry), xmdata->log_dir->str);
  sii_exam_entry_info( entry, widget_id, frame_num, 2 );
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 3, 1);




  hbox0 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append (GTK_BOX(vbox0), hbox0);

  widget_id = EXAM_CLEAR_CHANGES;
  xmdata->equiv_solo_state[widget_id] = EX_CLEAR_CHANGES;
  button = gtk_button_new_with_label ("Clear Edits");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num * TASK_MODULO + widget_id;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_exam_menu_cb)
		      , (gpointer)nn
		      );

  widget_id = EXAM_UNDO;
  xmdata->equiv_solo_state[widget_id] = EX_UNDO;
  button = gtk_button_new_with_label ("Undo");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num * TASK_MODULO + widget_id;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_exam_menu_cb)
		      , (gpointer)nn
		      );

  widget_id = EXAM_APPLY_CHANGES;
  xmdata->equiv_solo_state[widget_id] = EX_COMMIT;
  button = gtk_button_new_with_label ("Apply Edits");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num * TASK_MODULO + widget_id;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_exam_menu_cb)
		      , (gpointer)nn
		      );

  widget_id = EXAM_REFRESH;
  button = gtk_button_new_with_label ("Refresh");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num * TASK_MODULO + widget_id;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_exam_menu_cb)
		      , (gpointer)nn
		      );



  for (jj=0; jj < EXAM_MAX_WIDGETS; jj++ ) {

     if (xmdata->entry_flag[jj] && xmdata->data_widget[jj]) {
	nn = frame_num * TASK_MODULO + jj;
	g_signal_connect (G_OBJECT(xmdata->data_widget[jj])
			    ,"activate"
			    , G_CALLBACK (sii_exam_menu_cb)
			    , (gpointer)nn );
	/* paste_clipboard signal removed - not available in GTK4 */
     }
  }
  se_update_exam_widgets (frame_num);


  /* --- Make everything visible --- */
  gtk_widget_set_visible (window, TRUE);
  mark = 0;


  widget_id = FRAME_EXAM_DISPLAY;
  window = gtk_window_new();

  sii_set_widget_ptr ( frame_num, widget_id, window );

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num*TASK_MODULO+widget_id));

  nn = frame_num * TASK_MODULO + widget_id;
  g_signal_connect (G_OBJECT(window),"key_press_event",
		      G_CALLBACK (sii_exam_keyboard_event)
		      , (gpointer)nn);

  bb = g_strdup_printf ("Frame %d  Examine Display Widget", frame_num+1 );
  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );

  vbox00 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW(window), vbox00);

  /* labels for data only
   */
  xmdata->label_bar0 = label = gtk_label_new ( "" );
  gtk_box_append (GTK_BOX(vbox00), label);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  xmdata->label_bar1 = label = gtk_label_new ( "" );
  gtk_box_append (GTK_BOX(vbox00), label);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);

  /* set keyboard events for this window to signal
   * arrows used for scrolling
   */
  vbox0 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append (GTK_BOX(vbox00), vbox0);

  scrolledwindow = gtk_scrolled_window_new ();
  xmdata->data_widget[EXAM_SCROLLED_WIN] = scrolledwindow;
  gtk_widget_set_size_request (scrolledwindow, 800, 440);
  gtk_box_append (GTK_BOX (vbox0), scrolledwindow);

  nn = xmdata->max_possible_cells;
  /* Use a reasonable default for label metrics since GdkFont is gone in GTK4.
   * These should be computed from a PangoFontDescription if precise metrics are needed.
   */
  xmdata->label_height =
    height = 16; /* approximate line height for a medium fixed font */
  xmdata->label_char_width =
    width = 8;   /* approximate character width for a medium fixed font */
  len = xmdata->max_chars_per_line -1;
  str[len] = '\0';
  gtk_widget_set_size_request (scrolledwindow, 120*width, 30*height);

  layout = gtk_fixed_new();
  xmdata->data_widget[EXAM_LAYOUT] = layout;
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrolledwindow), layout);
  gtk_widget_set_size_request(layout, len*width, nn*height);

  /* We set step sizes here since GtkLayout does not set
   * them itself.
   */
  {
    GtkAdjustment *hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(layout));
    GtkAdjustment *vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(layout));
    gtk_adjustment_set_step_increment(hadj, height);
    gtk_adjustment_set_step_increment(vadj, width);
  }

  for (mm=0; mm < nn; mm++ ) {
     ch = A + (mm % 26);
     sprintf (str, "%3d.", mm);
     memset (str+strlen(str), ch, len-strlen(str));

     /* GTK4: GtkEventBox removed; any widget can receive events */
     label = gtk_label_new ( "" );
     event = label;  /* event and label are same widget in GTK4 */
     xmdata->label_items[mm] = label;
     gtk_label_set_justify ((GtkLabel *)label, GTK_JUSTIFY_LEFT );
     gtk_label_set_xalign (GTK_LABEL (label), 0.0);

     g_signal_connect (G_OBJECT(event),"button_press_event",
			 G_CALLBACK (sii_exam_list_event)
			 , (gpointer)label);

     gtk_fixed_put(GTK_FIXED(layout), event,
		   0, mm*height);

     g_object_set_data (G_OBJECT(label),
			  "frame_num",
			  (gpointer)frame_num);
     g_object_set_data (G_OBJECT(label),
			  "exam_index",
			  (gpointer)mm);
  }
  se_refresh_examine_list(frame_num, NULL);

  gtk_widget_set_visible (window, TRUE);
  mark = 0;
}

/* c---------------------------------------------------------------------- */

/* GTK4: GtkItemFactory removed. Menu built dynamically using sii_submenu helpers. */

/* c---------------------------------------------------------------------- */

void sii_exam_menubar2( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num)
{
   gint jj, nn;
   guint radio_num = 0, widget_id;
   GtkWidget *check_item, *mbar, *menuitem, *submenu, *next_item;

   radio_group = NULL;

   *menubar = mbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

   submenu = sii_submenu ( "Display", mbar );

   widget_id = EXAM_CLOSE;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Close", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );

   /* add a seperator */
   menuitem = sii_submenu_item ( NULL, submenu, 0, NULL, frame_num );

   radio_group = NULL;
   radio_num = 0;

   widget_id = EXAM_DATA;
   xmdata->equiv_solo_state[widget_id] = EX_RADAR_DATA;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Cell Values", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(menuitem), TRUE);

   widget_id = EXAM_RAYS;
   xmdata->equiv_solo_state[widget_id] = EX_BEAM_INVENTORY;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Ray Info", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(menuitem), FALSE);

   widget_id = EXAM_METADATA;
   xmdata->equiv_solo_state[widget_id] = EX_DESCRIPTORS;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Metadata", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(menuitem), FALSE);

   widget_id = EXAM_EDT_HIST;
   xmdata->equiv_solo_state[widget_id] = EX_EDIT_HIST;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Edit Hist", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(menuitem), FALSE);


   submenu = sii_submenu ( "Edit", mbar );
   radio_group = NULL;
   radio_num = 0;

   widget_id = EXAM_DELETE;
   xmdata->equiv_solo_state[widget_id] = EX_DELETE;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Delete", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_NEG_FOLD;
   xmdata->equiv_solo_state[widget_id] = EX_MINUS_FOLD;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "- Fold", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_POS_FOLD;
   xmdata->equiv_solo_state[widget_id] = EX_PLUS_FOLD;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "+ Fold", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_DELETE_RAY;
   xmdata->equiv_solo_state[widget_id] = EX_RAY_IGNORE;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Delete Ray", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_NEG_FOLD_RAY;
   xmdata->equiv_solo_state[widget_id] = EX_RAY_MINUS_FOLD;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "- Fold Ray", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_POS_FOLD_RAY;
   xmdata->equiv_solo_state[widget_id] = EX_RAY_PLUS_FOLD;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "+ Fold Ray", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_NEG_FOLD_GT;
   xmdata->equiv_solo_state[widget_id] = EX_GT_MINUS_FOLD;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "- Fold Ray >", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_POS_FOLD_GT;
   xmdata->equiv_solo_state[widget_id] = EX_GT_PLUS_FOLD;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "+ Fold Ray >", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_ZAP_GND_SPD;
   xmdata->equiv_solo_state[widget_id] = EX_REMOVE_AIR_MOTION;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Zap Gnd Spd", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   submenu = sii_submenu ( "Options", mbar );
   radio_num = 0;

   widget_id = EXAM_LST_RNGAZ;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Az,Rng Labels", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);

   widget_id = EXAM_LST_CELLRAY;
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Cell,Ray Labels", submenu, widget_id
			      , (GCallback)sii_exam_menu_cb, frame_num, ++radio_num
			       , &radio_group);


   /* add a seperator */
   menuitem = sii_submenu_item ( NULL, submenu, 0, NULL, frame_num );

   widget_id = EXAM_LOG_ACTIVE;
   radio_num = 0;		/* implies a check menu item */
   xmdata->data_widget[widget_id] = menuitem =
     sii_toggle_submenu_item ( "Logging Active", submenu, widget_id
			       , (GCallback)sii_exam_menu_cb, frame_num, radio_num
			       , &radio_group);

   widget_id = EXAM_LOG_CLOSE;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Close log file", submenu, widget_id
			, (GCallback)sii_exam_menu_cb, frame_num );

   widget_id = EXAM_LOG_FLUSH;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Flush to log file", submenu, widget_id
			, (GCallback)sii_exam_menu_cb, frame_num );



   submenu = sii_submenu ( "Replot", mbar );

   widget_id = EXAM_REPLOT_THIS;
   xmdata->equiv_solo_state[widget_id] = REPLOT_THIS_FRAME;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Replot This", submenu, widget_id
			, (GCallback)sii_exam_menu_cb, frame_num );

   widget_id = EXAM_REPLOT_LINKS;
   xmdata->equiv_solo_state[widget_id] = REPLOT_LOCK_STEP;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Replot Links", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );

   widget_id = EXAM_REPLOT_ALL;
   xmdata->equiv_solo_state[widget_id] = REPLOT_ALL;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Replot All", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );

   submenu = sii_submenu ( "Help", mbar );

   widget_id = EXAM_OVERVIEW;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Overview", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );

   widget_id = EXAM_HLP_OPRS;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Edit", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );

   widget_id = EXAM_HLP_OPTIONS;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Options", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );

   widget_id = EXAM_HLP_NYQVEL;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Nyq Vel", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );

   widget_id = EXAM_HLP_LOG_DIR;
   xmdata->data_widget[widget_id] = menuitem =
     sii_submenu_item ( "Log Dir", submenu, widget_id
		       , (GCallback)sii_exam_menu_cb, frame_num );


   widget_id = EXAM_CANCEL;
   menuitem = gtk_button_new_with_label("Cancel");
   gtk_box_append(GTK_BOX(mbar), menuitem);
   nn = frame_num * TASK_MODULO + widget_id;
   g_signal_connect(menuitem, "clicked",
		    G_CALLBACK(sii_exam_menu_cb), GUINT_TO_POINTER(nn));
   g_object_set_data(G_OBJECT(menuitem),
		     "widget_id", GUINT_TO_POINTER(widget_id));
   g_object_set_data(G_OBJECT(menuitem),
		     "frame_num", GUINT_TO_POINTER(frame_num));


  check_item = xmdata->data_widget[EXAM_LOG_ACTIVE];
  gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), FALSE);

  check_item = xmdata->data_widget[EXAM_DATA];
  gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), TRUE);
  xmdata->display_state = EXAM_DATA;

  check_item = xmdata->data_widget[EXAM_DELETE];
  gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), TRUE);
  xmdata->operation_state = EXAM_DELETE;

  check_item = xmdata->data_widget[EXAM_LST_RNGAZ];
  gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), TRUE);
  xmdata->label_state = EXAM_LST_RNGAZ;

}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */
