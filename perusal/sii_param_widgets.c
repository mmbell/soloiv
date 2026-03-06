/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include "color_tables.h"
# include "sii_utils.h"
# include <solo_window_structs.h>
# include "help_param.h"
# include <math.h>
# include <stdio.h>


static gchar *param_names = NULL;
static gchar *param_palette_names = NULL;
static gchar *param_color_table_names = NULL;
static gchar *param_color_names = NULL;

static GString *gs_color_table_names = NULL;
static GString *gs_color_names = NULL;

static SiiLinkedList *palette_stack=NULL;
static SiiLinkedList *solo_palette_stack=NULL;

static SiiFont *edit_font;
static GdkRGBA *edit_fore;
static GdkRGBA *edit_back;


# define CB_ROUND .000001
# define COMMA ","

 /* feature colors */

enum {
  FEATURE_ZERO,
  FEATURE_RNG_AZ_GRID,
  FEATURE_TIC_MARKS,
  FEATURE_BACKGROUND,
  FEATURE_ANNOTATION,
  FEATURE_EXCEEDED,
  FEATURE_MISSING,
  FEATURE_EMPHASIS,
  FEATURE_BND,
  FEATURE_OVERLAY1,
  FEATURE_OVERLAY2,
  FEATURE_OVERLAY3,
  FEATURE_OVERLAY4,
  FEATURE_OVERLAY5,
  FEATURE_LAST_ENUM,
};

# define MAX_FEATURE_COLORS FEATURE_LAST_ENUM

 /* Parameter Menu Actions */

enum {
   PARAM_ZERO,
   PARAM_PALETTE_LIST,
   PARAM_CLR_TBL_LIST,
   PARAM_MPORT_CLR_TBL,
   PARAM_COLORS_LIST,
   PARAM_ELECTRIC,
   PARAM_HILIGHT,
   PARAM_LABEL,
   PARAM_COLOR_TEST,
   PARAM_NAMES_TEXT,
   PARAM_PALETTES_TEXT,
   PARAM_CLR_TBL_TEXT,
   PARAM_CLR_TBL_PATH,
   PARAM_COLORS_TEXT,
   PARAM_COLORS_FILESEL,

   PARAM_GNRK_ENTRY,
   PARAM_GNRK_ENTRY_CANCEL,

   PARAM_REPLOT_THIS,
   PARAM_REPLOT_LINKS,
   PARAM_REPLOT_ALL,

   PARAM_OK,
   PARAM_CANCEL,
   PARAM_CLOSE,
   PARAM_LINKS,

   PARAM_PALETTE_CANCEL,
   PARAM_CLR_TBL_CANCEL,
   PARAM_COLORS_CANCEL,

   PARAM_NAME,
   PARAM_MINMAX,
   PARAM_CTRINC,
   PARAM_GRID_CLR,
   PARAM_PALETTE,

   PARAM_BND_CLR,
   PARAM_XCEED_CLR,
   PARAM_MSSNG_CLR,
   PARAM_ANNOT_CLR,
   PARAM_BACKG_CLR,
   PARAM_EMPH_MINMAX,
   PARAM_EMPH_CLR,
   PARAM_CLR_TBL_NAME,

   PARAM_CB_BOTTOM,		/* color bar */
   PARAM_CB_LEFT,
   PARAM_CB_RIGHT,
   PARAM_CB_TOP,
   PARAM_CB_SYMBOLS,
   PARAM_CB_ALL_SYMBOLS,

   PARAM_BC_GRID,		/* broadcast */
   PARAM_BC_BND,
   PARAM_BC_CB,
   PARAM_BC_BACKG,
   PARAM_BC_XCEED,
   PARAM_BC_ANNOT,
   PARAM_BC_MSSNG,
   PARAM_BC_EMPH,

   PARAM_OVERVIEW,
   PARAM_HLP_FILE,
   PARAM_HLP_OPTIONS,
   PARAM_HLP_LINKS,
   PARAM_HLP_PALETTES,
   PARAM_HLP_MINMAX,
   PARAM_HLP_EMPHASIS,
   PARAM_HLP_,

   PARAM_LAST_ENUM,

}ParamWidgetIds;

# define PARAM_MAX_WIDGETS PARAM_LAST_ENUM
# define MAX_COLOR_TABLE_SIZE 128
# define MAX_BAR_CLRS 64

enum color_bar_orientation{
  CB_BOTTOM      = 1 << 0,
  CB_LEFT        = 1 << 1,
  CB_RIGHT       = 1 << 2,
  CB_TOP         = 1 << 3,
  CB_SYMBOLS     = 1 << 4,
  CB_ALL_SYMBOLS = 1 << 5,
};

/* c---------------------------------------------------------------------- */

typedef struct {

   GString *p_name;
   GString *usual_parms;

   gfloat minmax[2];
   gfloat ctrinc[2];
   gfloat emphasis_zone[2];

   GString *grid_color;
   GString *missing_data_color;
   GString *exceeded_color;
   GString *annotation_color;
   GString *background_color;
   GString *emphasis_color;
   GString *boundary_color;
 
   GString *units;
   guint num_colors;
   GString *color_table_name;

   GdkRGBA *feature_color[MAX_FEATURE_COLORS];
   GdkRGBA *data_color_table[MAX_COLOR_TABLE_SIZE];

   guchar feature_rgbs[MAX_FEATURE_COLORS][3];
   guchar color_table_rgbs[MAX_COLOR_TABLE_SIZE][3];

} SiiPalette;

/* c---------------------------------------------------------------------- */

typedef struct {
  gchar name[16];
  SiiPalette *pal;
} ParamFieldInfo;

/* c---------------------------------------------------------------------- */

typedef struct {
   gint change_count;

   GtkWidget *data_widget[PARAM_MAX_WIDGETS];

   gboolean change_flag[PARAM_MAX_WIDGETS];
   gboolean entry_flag[PARAM_MAX_WIDGETS];

   guint num_values[PARAM_MAX_WIDGETS];
   gdouble orig_values[PARAM_MAX_WIDGETS][2];
   gdouble values[PARAM_MAX_WIDGETS][2];
   guint precision[PARAM_MAX_WIDGETS];

   GString *orig_txt[PARAM_MAX_WIDGETS];
   GString *txt[PARAM_MAX_WIDGETS];

   gboolean toggle[PARAM_MAX_WIDGETS];
   gboolean  electric_params;
   guint cb_loc;
   guint cb_labels_state;
   guint num_colors;

   SiiPalette *pal;
   SiiPalette *orig_pal;
   LinksInfo *param_links;
   GString *param_names_list;

   int orientation;
   GString *cb_labels;
   GString *cb_symbols;

   guint field_toggle_count;
   SiiLinkedList *fields_list;
   SiiLinkedList *toggle_field;

} ParamData;


static ParamData *pdata;
static GSList *radio_group = NULL;
static gboolean its_changed = FALSE;


/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

GtkWidget *
sii_filesel (gint which_but, gchar * dirroot);

void 
param_entry_widget( guint frame_num, const gchar *prompt, const gchar * dirroot);

static void
param_list_widget( guint frame_num, guint widget_id
		  , guint menu_wid);
guint
param_text_event_cb(GtkWidget *text, gpointer data);

const gchar *
set_cb_labeling_info (guint frame_num, gdouble *relative_locs);

void
set_color_bar (guint frame_num, guint font_height);

static void
show_param_generic_list_widget (guint frame_num
				      , guint menu_wid );
void
show_param_widget (GtkWidget *text, gpointer data );

GdkRGBA *
sii_annotation_color (guint frame_num, gint exposed);

GdkRGBA *
sii_background_color (guint frame_num, gint exposed);

GdkRGBA *
sii_boundary_color (guint frame_num, gint exposed);

gchar *
sii_color_tables_lister ( gpointer name, gpointer data );

void
sii_colorize_image (guint frame_num);

gchar *
sii_colors_lister ( gpointer name, gpointer data );

void
sii_ctr_inc_from_min_max (guint ncolors, gfloat *ctr, gfloat *inc,
			  gfloat *min, gfloat *max );
gchar *
sii_default_palettes_list();

void
sii_do_annotation (guint frame_num, gint exposed, gboolean blow_up, cairo_t *cr);

void
sii_double_colorize_image (guint frame_num);

void
sii_get_clip_rectangle (guint frame_num, Rectangle *clip
			, gboolean blow_up);
gint
sii_get_ctr_and_inc (guint frame_num, float *ctr, float *inc);

gint
sii_get_emph_min_max (guint frame_num, float *zmin, float *zmax);

GdkRGBA *
sii_grid_color (guint frame_num, gint exposed);

void
sii_initialize_parameter (guint frame_num, gchar *name);

SiiPalette *
sii_malloc_palette();

void
sii_min_max_from_ctr_inc (guint ncolors, gfloat *ctr, gfloat *inc,
			  gfloat *min, gfloat *max );
SiiLinkedList *
sii_new_palette_for_param (const gchar *p_name, const gchar *name);

gchar *
sii_new_palettes_list ();

gchar *
sii_new_color_tables_list ();

gchar *
sii_new_colors_list ();

SiiLinkedList *
sii_palette_for_param (const gchar *name);

void
sii_palette_prepend_usual_param ( SiiPalette *pal, const gchar *name);

void 
sii_palette_remove_usual_param ( SiiPalette *palx, const gchar *name);

SiiLinkedList *
sii_palette_seek (const gchar *p_name);

void
sii_param_dup_entries (guint frame_num);

void
sii_param_dup_opal (struct solo_palette *opal);

void
sii_param_dup_orig (guint frame_num);

void
sii_param_dup_pal (gpointer sii_pal, gpointer old_pal);

void
sii_param_check_changes (guint frame_num);

void
sii_param_entry_info( GtkWidget *w, guint wid, guint frame_num
		     , guint num_values );
void
sii_param_entry_paste_cb ( GtkWidget *w, gpointer   data );

gchar *
sii_param_fix_color (gchar *old_color);

void
sii_param_menu_cb ( GtkWidget *w, gpointer   data );

void
sii_param_menubar2( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num);

const gchar *
sii_param_palette_name (guint frame_num);

void 
sii_param_process_changes (guint frame_num);

void
sii_param_reset_changes (guint frame_num);

void
sii_param_set_plot_field (int frame_num, char *fname);

void
sii_param_toggle_field (guint frame_num);

void
sii_param_widget( guint frame_num );

void
sii_redisplay_list (guint frame_num, guint widget_id);

void
sii_reset_image (guint frame_num);

void
sii_set_entries_from_palette (guint frame_num, const gchar *name, 
        const gchar *p_name);

SiiPalette *
sii_set_palette (const gchar *name);

gboolean
sii_set_param_info (guint frame_num);

gchar *
sii_set_param_names_list(guint frame_num, GString *gs_list, guint count);

void
solo_get_palette(char *name, int frame_num);

int
solo_hardware_color_table(gint frame_num);

int
solo_palette_color_numbers(int frame_num);

SiiLinkedList *
sii_solo_palette_stack ();

void
sii_update_param_widget (guint frame_num);


/* external routines */

struct solo_perusal_info *
solo_return_winfo_ptr();
void solo_sort_slm_entries ();

/* c...mark */

/* c---------------------------------------------------------------------- */

void param_set_cb_loc (int frame_num, int loc)
{
  ParamData *pd = frame_configs[frame_num]->param_data;
  GtkWidget *check_item, *widget;

  switch (loc) {
  case -1:
    pd->cb_loc = PARAM_CB_LEFT;
    break;
  case 1:
    pd->cb_loc = PARAM_CB_RIGHT;
    break;
  default:
    pd->cb_loc = PARAM_CB_BOTTOM;
    break;
  };
  widget = sii_get_widget_ptr (frame_num, FRAME_PARAMETERS);
  if (widget) {
    check_item = pd->data_widget[pd->cb_loc];
    gtk_check_button_set_active (GTK_CHECK_BUTTON (check_item), TRUE );
  }
}

/* c---------------------------------------------------------------------- */

void 
sii_param_colors_filesel (const gchar *str, GtkWidget *fs);
int sii_initialize_cb (GtkWidget *w, gpointer data);

void 
sii_param_colors_filesel (const gchar *str, GtkWidget *fs )
{
   gint frame_num =  GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (fs)
							    , "frame_num"));
   sii_param_absorb_ctbl (frame_num, str);
}
/* c---------------------------------------------------------------------- */

void param_entry_widget( guint frame_num, const gchar *prompt, const gchar * dirroot)
{
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  GtkWidget *label;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *entry;
  guint wid, nn;
  gint x, y;
  gchar *bb;
  static gchar str[256];

  wid = FRAME_PARAM_ENTRY;
  window = gtk_window_new ();
  sii_set_widget_ptr (frame_num, wid, window);
  


  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);
  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num*TASK_MODULO+wid));
  bb = g_strdup_printf ("Frame %d  %s", frame_num+1, "Entry Widget" );
  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_window_set_child (GTK_WINDOW (window), vbox);

  label = gtk_label_new ( prompt );
  gtk_box_append (GTK_BOX (vbox), label);

  entry = gtk_entry_new ();
  pd->data_widget[PARAM_GNRK_ENTRY] = entry;
  gtk_box_append (GTK_BOX (vbox), entry);
  nn = frame_num * TASK_MODULO + PARAM_CLR_TBL_PATH;
  g_signal_connect (G_OBJECT (entry)
		      ,"activate"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn );

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append (GTK_BOX (vbox), hbox);

  button = gtk_button_new_with_label ("OK");
  gtk_box_append (GTK_BOX (hbox), button);
  nn = frame_num*TASK_MODULO + PARAM_CLR_TBL_PATH;
  g_signal_connect (G_OBJECT (button)
		      ,"clicked"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn
		      );

  if (dirroot) {
     button = gtk_button_new_with_label ("Colors FileSelect");
     gtk_box_append (GTK_BOX (hbox), button);
     nn = frame_num*TASK_MODULO + PARAM_COLORS_FILESEL;
     g_signal_connect (G_OBJECT (button)
		      ,"clicked"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn
		      );
     slash_path (str, dirroot);
     g_object_set_data (G_OBJECT (button)
			  , "dirroot", (gpointer)str);
   }				      

  button = gtk_button_new_with_label ("Cancel");
  gtk_box_append (GTK_BOX (hbox), button);
  nn = frame_num*TASK_MODULO + PARAM_GNRK_ENTRY_CANCEL;
  g_signal_connect (G_OBJECT (button)
		      ,"clicked"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn
		      );

  gtk_widget_set_visible (window, TRUE);
}

/* c---------------------------------------------------------------------- */

void sii_param_add_ctbl (const char **at, int nn)
{
  char *buf, str[256], *sptrs[32], *aa, *bb;
  int nt;

  strcpy (str, at[0]);
  nt = dd_tokens (str, sptrs);	/* the name should be the second token */
  sii_glom_strings (at, nn, gen_gs);

  buf = (char *)g_malloc0 (gen_gs->len+ 1);
  strncpy (buf, gen_gs->str, gen_gs->len);
  buf[gen_gs->len] = '\0';
  put_ascii_color_table (sptrs[1], buf);
  sii_new_color_tables_list ();
}

/* c---------------------------------------------------------------------- */

gboolean sii_param_absorb_ctbl (guint frame_num, const gchar *filename)
{
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  gchar str[256], *sptrs[32], mess[256], *buf, *aa, *bb;
  const gchar *name;
  gchar *bptrs[128];
  FILE *stream;
  size_t lenx, len0, len2;
  gint ii, jj, nn, nt;

  if(!(stream = fopen(filename, "r"))) {
    sprintf(mess, "Unable to open color table file %s\n", filename);
    sii_message(mess);
    return FALSE;
  }

  lenx = fseek(stream, 0L, (int)2); /* at end of file */
  len0 = ftell(stream);	/* how big is the file */
  rewind(stream);

  if (name = strrchr (filename, '/'))
    { name++; }
  else
    { name = filename; }
  strcpy (str, "colortable ");
  strcat (str, name);
  strcat (str, "\n");

  buf = (char *)g_malloc0 (len0+1);
  lenx = fread (buf, sizeof(char), len0, stream);
  nt = dd_tokenz (buf, bptrs, "\n");
        
  g_string_printf (gen_gs, "colortable %s\n", name);

  for (jj=0; jj < nt; jj++) {
     if (aa = strstr (bptrs[jj], "!"))
       { *aa = '\0'; }

     if (strlen (bptrs[jj]) < 2)
       { continue; }
     if (strstr( bptrs[jj], "colortable"))
       { continue; }

     g_string_append_printf (gen_gs, "%s\n", bptrs[jj]);
  }
  g_free (buf);
  buf = (char *)g_malloc0 (gen_gs->len +1);
  memcpy (buf, gen_gs->str, gen_gs->len); 
  buf[gen_gs->len] = '\0';
  put_ascii_color_table (name, buf);
  sii_new_color_tables_list ();

  gtk_editable_set_text
    (GTK_EDITABLE (pd->data_widget[PARAM_CLR_TBL_NAME]), name);
  sii_redisplay_list (frame_num, PARAM_CLR_TBL_TEXT);
}

/* c---------------------------------------------------------------------- */

int sii_param_dump_ctbls (FILE *stream)
{
  GSList *gsl = color_table_names;
  static GString *gs = NULL;
  gchar mess[256], *aa, *bb;
  struct gen_dsc {
    char name_struct[4];
    long sizeof_struct;
  };
  struct gen_dsc gd;
  size_t nn, len;
  
  strncpy (gd.name_struct, "SCTB", 4);

  for(; gsl; gsl = gsl->next) {
    aa = (gchar *)gsl->data;
    bb = (gchar *)g_tree_lookup (ascii_color_tables, (gpointer)aa);
    len = strlen (bb);
    gd.sizeof_struct = 8 + len;
    if((nn = fwrite(&gd, sizeof(char), sizeof(gd), stream))
	   < sizeof(gd))
      {
	sprintf(mess, "Problem writing color table: %s\n", aa);
	solo_message(mess);
	return 0;
      }
    if((nn = fwrite((void *)bb, sizeof(char), len, stream))
	   < len)
      {
	sprintf(mess, "Problem writing color table: %s\n", aa);
	solo_message(mess);
	return 0;
      }
    
  }
  return 1;
}

/* c---------------------------------------------------------------------- */

static void param_list_widget( guint frame_num, guint widget_id
			     , guint menu_wid)
{
  GtkWidget *label;

  GtkWidget *window;
  GtkWidget *vbox;

  GtkWidget *text;

  GtkWidget *button;

  guint xpadding = 0;
  guint ypadding = 0;
  guint padding = 0, row = 0;

  char *aa;
  gchar str[256], str2[256], *bb, *ee, *name;
  const gchar *cc;
  gint length, width=0, height=0, max_width = 0, jj, nn;
  gfloat upper;
  gint x, y;
  guint cancel_wid, text_wid = 0;
				/* c...mark code starts here */

  bb = str;
  strcpy (bb, "  2 and 5     " );
  ee = g_strstrip (bb);

  switch (menu_wid) {

   case PARAM_PALETTE_LIST:
     if (!param_palette_names)
       { sii_new_palettes_list (); }
     aa = param_palette_names;
     width = height = 360;
     name = "Color Palettes Widget";
     cancel_wid = PARAM_PALETTE_CANCEL;
     text_wid = PARAM_PALETTES_TEXT;
     break;

   case PARAM_CLR_TBL_LIST:
     if (!param_color_table_names)
       { sii_new_color_tables_list (); }
     aa = param_color_table_names;
     width = height = 220;
     name = "Color Tables Widget";
     text_wid = PARAM_CLR_TBL_TEXT;
     cancel_wid = PARAM_CLR_TBL_CANCEL;
     break;

   case PARAM_COLORS_LIST:
     if (!param_color_names)
       { sii_new_colors_list (); }
     aa = param_color_names;
     height = 512;
     name = "Color Names Widget";
     text_wid = PARAM_COLORS_TEXT;
     cancel_wid = PARAM_COLORS_CANCEL;
     break;
  }
  length = strlen( aa );

  window = gtk_window_new ();

  sii_set_widget_ptr ( frame_num, widget_id, window );
  

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);
  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num*TASK_MODULO+widget_id));

  /* --- Title and border --- */
  bb = g_strdup_printf ("Frame %d  %s", frame_num+1, name );
  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );
  /* GTK4: use margins instead of border_width */

  /* --- Create a new vertical box for storing widgets --- */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW (window), vbox);

  button = gtk_button_new_with_label ("Cancel");
  gtk_box_append (GTK_BOX (vbox), button );

  nn = frame_num*TASK_MODULO + cancel_wid;
  g_signal_connect (G_OBJECT (button)
		      ,"clicked"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn
		      );

  text = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);
  if (text_wid)
    { pdata->data_widget[text_wid] = text; }

  {
    GtkWidget *sw = gtk_scrolled_window_new ();
    gtk_widget_set_size_request (sw, width, height);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (sw), text);
    gtk_box_append (GTK_BOX (vbox), sw);
  }

  /* GTK4: click handling via GtkGestureClick */
  {
    GtkGesture *gesture = gtk_gesture_click_new ();
    g_object_set_data (G_OBJECT (text), "click_data", (gpointer)aa);
    g_signal_connect (gesture, "pressed", G_CALLBACK (param_text_event_cb), text);
    gtk_widget_add_controller (text, GTK_EVENT_CONTROLLER (gesture));
  }

  g_object_set_data (G_OBJECT (text),
		       "frame_num", (gpointer)frame_num);

  g_object_set_data (G_OBJECT (text),
		       "widget_id", (gpointer)text_wid);

  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
    gtk_text_buffer_set_text (buffer, aa, length);
  }


  {
    GtkWidget *child = gtk_widget_get_first_child (vbox);
    while (child) {
      sii_set_widget_frame_num (child, (gpointer)frame_num);
      child = gtk_widget_get_next_sibling (child);
    }
  }

  /* --- Make everything visible --- */
  gtk_widget_set_visible (window, TRUE);

}

/* c---------------------------------------------------------------------- */

guint param_text_event_cb(GtkWidget *text, gpointer data)
{
  GtkWidget* draw_widget;
  guint position = 0; /* GTK4: cursor position from GtkTextView */
  guint frame_num, wid;
  gint jj, kk, nn, nt, mark, start, end;
  const gchar *aa, *bb = "", *p_name, *name;
  gchar *line;
  ParamData *pd;
  gboolean ok;
  gchar str[256], *sptrs[32];

  GdkRGBA *test_color;

  nn = PARAM_CLR_TBL_TEXT;

  frame_num = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT (text), "frame_num" ));

  wid  = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT (text), "widget_id" ));

  pd = (ParamData *)frame_configs[frame_num]->param_data;

  {
    GtkTextBuffer *tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    GtkTextIter iter;
    GtkTextMark *mark = gtk_text_buffer_get_insert(tbuf);
    gtk_text_buffer_get_iter_at_mark(tbuf, &iter, mark);
    nn = gtk_text_iter_get_offset(&iter);
  }

  switch (wid) {
  case PARAM_NAMES_TEXT:
    aa = pd->param_names_list->str;
    bb = "PARAM_NAMES_TEXT";
    break;
  case PARAM_PALETTES_TEXT:
    aa = param_palette_names;
    bb = "PARAM_PALETTES_TEXT";
    break;
  case PARAM_COLORS_TEXT:
    aa = param_color_names;
    bb = "PARAM_COLORS_TEXT";
    break;
  case PARAM_CLR_TBL_TEXT:
    aa = param_color_table_names;
    bb = "PARAM_CLR_TBL_TEXT";
    break;
  };

  line = sii_nab_line_from_text (aa, nn);

  if( line ) {
# ifdef obsolete
     g_message ("Frame:%d Parameter_text: %s", frame_num, line);
# endif

     if( wid == PARAM_NAMES_TEXT ) {
	g_strstrip (line);
	gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[PARAM_NAME]), line);
	if (pd->toggle[PARAM_ELECTRIC]) {
	  if (sii_set_param_info (frame_num))
	    { sii_plot_data (frame_num, REPLOT_THIS_FRAME); }
	}
	else {
	  sii_param_process_changes (frame_num);
	}
	sii_redisplay_list (frame_num, PARAM_PALETTES_TEXT);
     }
     else if ( wid == PARAM_PALETTES_TEXT ) {
	strcpy (str, line);
	nt = dd_tokens (str, sptrs);
	p_name = sptrs [0];
	gtk_editable_set_text
	    (GTK_EDITABLE (pd->data_widget[PARAM_PALETTE]), p_name);
	name = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[PARAM_NAME]));
	sii_new_palette_for_param (p_name, name);
	sii_param_process_changes (frame_num);
	sii_redisplay_list (frame_num, PARAM_PALETTES_TEXT);
     }
     else if ( wid == PARAM_CLR_TBL_TEXT ) {
	gtk_editable_set_text
	    (GTK_EDITABLE (pd->data_widget[PARAM_CLR_TBL_NAME]), line);
     }
     else if ( wid == PARAM_COLORS_TEXT &&
	      (ok = sii_nab_region_from_text (aa, nn, &start, &end )))
       {
	  {
	    GtkTextBuffer *tbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	    GtkTextIter start_iter, end_iter;
	    gtk_text_buffer_get_iter_at_offset(tbuf, &start_iter, start);
	    gtk_text_buffer_get_iter_at_offset(tbuf, &end_iter, end);
	    gtk_text_buffer_select_range(tbuf, &start_iter, &end_iter);

	    GdkClipboard *clipboard = gdk_display_get_clipboard(gdk_display_get_default());
	    GtkTextIter sel_start, sel_end;
	    if (gtk_text_buffer_get_selection_bounds(tbuf, &sel_start, &sel_end)) {
	      char *sel_text = gtk_text_buffer_get_text(tbuf, &sel_start, &sel_end, FALSE);
	      gdk_clipboard_set_text(clipboard, sel_text);
	      g_free(sel_text);
	    }
	  }

# ifndef obsolete
	  for (kk=0,jj=start; jj < end; str[kk++] = aa[jj++]); str[kk] = '\0';
	  test_color = (GdkRGBA *)g_hash_table_lookup
	    (colors_hash, (gpointer)str);

	  /* GTK4: queue a redraw; actual drawing happens in draw callback */
	  draw_widget = pd->data_widget[PARAM_COLOR_TEST];
	  if (draw_widget) {
	    g_object_set_data (G_OBJECT (draw_widget), "test_color", test_color);
	    gtk_widget_queue_draw (draw_widget);
	  }
# endif
       }
     else {
     }
  }
  else {
     g_message ("Frame:%d Parameter_text: Bad Pointer from %s"
		, frame_num, bb );
  }
  mark = 0;
}

/* c---------------------------------------------------------------------- */

const gchar *set_cb_labeling_info (guint frame_num, gdouble *relative_locs)
{
  gdouble min, max, ctr, inc, log10diff, dval, dloc, dinc, d;
  gint jj, kk, nc, nn, nlabs, nnlabs;
  ParamData *pd = frame_configs[frame_num]->param_data;
  gchar *fmt, *name, str[512], *sptrs[64];
  WW_PTR wwptr = solo_return_wwptr(frame_num);
  const gchar **syms=NULL;

  /* Take the log10 of the increment to get decimal places in the label
   * n = (gint)((f < 0) ? f-.5 : f+.5);
   */
  nc = pd->pal->num_colors;
  ctr = pd->pal->ctrinc[0];
  inc = pd->pal->ctrinc[1];
  name = pd->orig_txt[PARAM_NAME]->str;
  name = wwptr->parameter->parameter_name;

  if (pd->toggle[PARAM_CB_SYMBOLS] || wwptr->color_bar_symbols) {
    if (strstr ("RR_DSD,RNX,RZD,RKD,", name )) {
      syms = log_rr_symbols;
      nnlabs = sizeof (log_rr_symbols)/sizeof (char *);
    }
    else if (strstr ("PD,WPD,HYDID,", name )) {
      syms = pid_labelz;
      nnlabs = sizeof (pid_labelz)/sizeof (char *);
      syms = pid_symbols;
      nnlabs = sizeof (pid_symbols)/sizeof (char *);
    }
  }

  nlabs = (nc < 7) ? nc : 7;
  switch (nc) {
  case 13:
  case 12:
  case 11:
  case 10:
  case 9:
    nlabs = 9;
    break;
  };

  d = nc*inc;
  log10diff = log10 (d);

  if (log10diff > 1.3)		/* 20 */
    { fmt = "%.0f "; }
  else if (log10diff > -.15)	/* .7 */
    { fmt = "%.1f "; }
  else 
    { fmt = "%.2f "; }

  nn = (gint)((double)nc/(nlabs +1) + .4);
  dinc = (double)nn/nc;
  dval = ctr - (nlabs/2) * nn * inc;
  dloc = .5 - (nlabs/2) * dinc;

  if (syms) {
    nlabs = nnlabs;
    nn = 1;
    dinc = 1.0/nc;
    dloc = .5 * dinc;
  }

  if (!pd->cb_labels)
    { pd->cb_labels = g_string_new (""); }
  else
    { g_string_truncate (pd->cb_labels, 0); }

  for (jj = 0; jj < nlabs; jj++, dval += nn*inc, dloc += dinc) {
    d = (dval < 0) ? dval-.0001 : dval+.0001;
    if (syms) {
      g_string_append_printf (pd->cb_labels, "%s ", syms[jj]);
    }
    else {
      g_string_append_printf (pd->cb_labels, fmt, d);
    }
    relative_locs[jj] = dloc;
  }
  return (const gchar *)pd->cb_labels->str;
}

/* c---------------------------------------------------------------------- */

void set_color_bar (guint frame_num, guint font_height)
{
  SiiFrameConfig *sfc = frame_configs[frame_num];
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  double f_inc, f_min, f_max, f_val, ctr, inc, half;
  guchar *aa, *bb, *cc, *ee;
  glong *lp;
  WW_PTR wwptr = solo_return_wwptr(frame_num);
  gint x, y, jj, kk, ll, nn, nc, len, width, offs, stride;
  
  if (!pd || !pd->pal)
    { return; }

  len = (pd->cb_loc == PARAM_CB_LEFT || pd->cb_loc == PARAM_CB_RIGHT)
    ? sfc->height : sfc->width;

  if (len > sfc->cb_pattern_len) {
    if (sfc->cb_pattern)
      { g_free (sfc->cb_pattern); }
    sfc->cb_pattern = (guchar *)g_malloc0 (len);
    sfc->cb_pattern_len = len;
  }
  ctr = pd->pal->ctrinc[0];
  half = .5 * pd->pal->num_colors * pd->pal->ctrinc[1];
  len -= 4;

  f_val = f_min = ctr - half;
  f_max = ctr + half;
  f_inc = 2 * half/len;
  aa = sfc->cb_pattern;

  for(jj=0; jj++ < len; f_val += f_inc) {
    kk = DD_SCALE(f_val, wwptr->parameter_scale, wwptr->parameter_bias);
    if(kk < -K32) kk = -K32;
    if(kk >= K32) kk = K32-1;
    *aa++ = *(wwptr->data_color_lut+kk);
  }

  aa = sfc->cb_pattern;
  bb = (guchar *)frame_configs[frame_num]->image->data;

  font_height = sfc->font_height;

  if (pd->cb_loc == PARAM_CB_LEFT || pd->cb_loc == PARAM_CB_RIGHT) {
    /* color bar increases from bottom to top */
    cc = bb + (sfc->height -2) * sfc->width;

    cc += (pd->cb_loc == PARAM_CB_LEFT) ? 1 : sfc->width -font_height -1;
# ifdef obsolete
    cc -= 2 * sfc->width;
# endif
    stride = sfc->width +font_height;

    for (jj=0; jj < len; jj++, y--, aa++, cc -= stride) {
      ee = cc + font_height;
      for (; cc < ee ; *cc++ = *aa);
    }
  }
  else {
    width = sfc->width;
    y = sfc->height -2;
    bb += y * width +2;
    x = 0;
    for (jj = 0; jj < font_height; jj++, bb -= width) {
      memcpy (bb, aa, len);
    }
  }
 }

/* c---------------------------------------------------------------------- */

static void show_param_generic_list_widget (guint frame_num, guint menu_wid )
{
  gint x, y;
  guint widget_id;
  GtkWidget *widget;

  switch (menu_wid) {
    case PARAM_PALETTE_LIST:
     widget_id = FRAME_PALETTE_LIST;
     break;

    case PARAM_CLR_TBL_LIST:
     widget_id = FRAME_COLR_TBL_LIST;
     break;

    case PARAM_COLORS_LIST:
     widget_id = FRAME_COLORS_LIST;
     break;
  };

  widget = sii_get_widget_ptr (frame_num, widget_id);

  if( !widget )
    { param_list_widget ( frame_num, widget_id, menu_wid); }
  else {
    if (menu_wid == PARAM_CLR_TBL_LIST) {
      sii_redisplay_list (frame_num, PARAM_CLR_TBL_TEXT);
    }
     
     gtk_widget_set_visible (widget, TRUE);
  }
}

/* c---------------------------------------------------------------------- */

void show_param_widget (GtkWidget *text, gpointer data )
{
  GtkWidget *widget;
  guint frame_num = GPOINTER_TO_UINT (data);
  gint x, y;

  widget = sii_get_widget_ptr (frame_num, FRAME_PARAMETERS);

  if( !widget )
    { sii_param_widget( frame_num ); }
  else {
     
     sii_update_param_widget (frame_num);
     gtk_widget_set_visible (widget, TRUE);
  }
}

/* c---------------------------------------------------------------------- */

GdkRGBA *sii_annotation_color (guint frame_num, gint exposed)
{
  GdkRGBA *gcolor;
  ParamData *pd = frame_configs[frame_num]->param_data;
  SiiPalette *pal;

  if (!pd || !pd->pal)
    { return NULL; }
  pal = (exposed && pd->orig_pal) ? pd->orig_pal : pd->pal;
  gcolor = pal->feature_color[FEATURE_ANNOTATION];
  return gcolor;
}

/* c---------------------------------------------------------------------- */

GdkRGBA *sii_background_color (guint frame_num, gint exposed)
{
  GdkRGBA *gcolor;
  ParamData *pd = frame_configs[frame_num]->param_data;
  SiiPalette *pal;

  if (!pd || !pd->pal)
    { return NULL; }
  pal = (exposed && pd->orig_pal) ? pd->orig_pal : pd->pal;
  gcolor = pal->feature_color[FEATURE_BACKGROUND];
  return gcolor;
}

/* c---------------------------------------------------------------------- */

GdkRGBA *sii_boundary_color (guint frame_num, gint exposed)
{
  GdkRGBA *gcolor;
  ParamData *pd = frame_configs[frame_num]->param_data;
  SiiPalette *pal;

  if (!pd || !pd->pal)
    { return NULL; }
  pal = (exposed && pd->orig_pal) ? pd->orig_pal : pd->pal;
# ifdef notyet
  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)"purple1");
# else
  gcolor = pal->feature_color[FEATURE_BND];
# endif
  return gcolor;
}

/* c---------------------------------------------------------------------- */

gchar *sii_color_tables_lister ( gpointer name, gpointer data )
{
   g_string_append_printf (gs_color_table_names, "%s\n", (gchar*)name );

   return( FALSE );		/* to continue a tree traverse */
}

/* c---------------------------------------------------------------------- */

void sii_colorize_image (guint frame_num)
{
  SiiFrameConfig *sfc = frame_configs[frame_num];
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  guchar *aa, *bb, *cc, **ct_rgbs, *rgbs;
  gint jj, nn, ndx;

  if (!pd || !pd->pal || !pd->orig_pal || !sfc->image)
    { return; }

  sfc->colorize_count++;
  nn = sfc->width * sfc->height;
  aa = bb = (guchar *)sfc->image->data;
  bb += sfc->height * sfc->width; /* 3 byte color data space follows the image */
  rgbs = pd->pal->color_table_rgbs[0]; /* table of 3 byte colors */


  for (jj=0; jj < nn; jj++, aa++) {
    cc = rgbs + (*aa) * 3;
    *bb++ = *cc++;
    *bb++ = *cc++;
    *bb++ = *cc++;
  }
}

/* c---------------------------------------------------------------------- */

gchar *sii_colors_lister ( gpointer name, gpointer data )
{
   g_string_append_printf (gs_color_names, "%s\n", (gchar*)name );

   return( FALSE );		/* to continue a tree traverse */
}

/* c---------------------------------------------------------------------- */

void sii_ctr_inc_from_min_max (guint ncolors, gfloat *ctr, gfloat *inc,
			       gfloat *min, gfloat *max )
{
   gfloat tmp;

   if ( *min > *max ) {
      tmp = *min; *min = *max; *max = tmp;
   }
   *inc = (*max - *min)/ncolors + CB_ROUND;
   *ctr = *min + *inc * ncolors * .5;
   *ctr += (*ctr < 0) ? -CB_ROUND : CB_ROUND;
}

/* c---------------------------------------------------------------------- */

gchar *sii_default_palettes_list()
{
   guint size=0, jj, nn;
   gchar *aa, *bb, *name;
   SiiPalette * pal;
   SiiLinkedList *item;


# ifdef notyet

   pal = sii_malloc_palette();
   pal->pname = g_string_new ("");
   pal->usual_parms = g_string_new ();
   pal->ctrinc[0] = ; pal->ctrinc[1] = ;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);
# endif
   
   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_ahav");
   pal->usual_parms = g_string_new ("AH,AV,");
   pal->ctrinc[0] = 0; pal->ctrinc[1] = 22;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_chcv");
   pal->usual_parms = g_string_new ("CH,CV,");
   pal->ctrinc[0] = .5; pal->ctrinc[1] = .1;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_pdesc");
   pal->usual_parms = g_string_new ("PD,WPD,HYDID,");
   pal->ctrinc[0] = 9.; pal->ctrinc[1] = 1.;
   g_string_assign (pal->color_table_name, "pd17");
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_raccum");
   pal->usual_parms = g_string_new ("KAC,ZAC,DAC,HAC,NAC,GAC,");
   pal->num_colors = 10;
   pal->ctrinc[0] = 50.; pal->ctrinc[1] = 10.;
   g_string_truncate (pal->color_table_name, 0);
   g_string_append (pal->color_table_name, "bluebrown10");
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_rho");
   pal->usual_parms = g_string_new ("RHOHV,RHO,RH,RX,");
   pal->ctrinc[0] = .5; pal->ctrinc[1] = .1;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_kdp");
   pal->usual_parms = g_string_new ("KDP,CKDP,NKDP,MKDP,DKD_DSD,");
   pal->ctrinc[0] = .7; pal->ctrinc[1] = .12;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_nt");
   pal->usual_parms = g_string_new ("NT_DSD,");
   pal->ctrinc[0] = 2.; pal->ctrinc[1] = .25;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_res");
   pal->usual_parms = g_string_new ("RES_DSD,");
   pal->ctrinc[0] = 5.; pal->ctrinc[1] = .6;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_d0");
   pal->usual_parms = g_string_new ("D0_DSD,");
   pal->ctrinc[0] = 2.; pal->ctrinc[1] = .25;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_lam");
   pal->usual_parms = g_string_new ("LAM_DSD,");
   pal->ctrinc[0] = 5.; pal->ctrinc[1] = .6;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_lwd");
   pal->usual_parms = g_string_new ("LWC_DSD,");
   pal->ctrinc[0] = .8; pal->ctrinc[1] = .1;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_mu");
   pal->usual_parms = g_string_new ("MU_DSD,");
   pal->ctrinc[0] = 5.; pal->ctrinc[1] = .6;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_n0");
   pal->usual_parms = g_string_new ("N0_DSD,");
   pal->ctrinc[0] = 4.; pal->ctrinc[1] = .5;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_phi");
   pal->usual_parms = g_string_new ("PHIDP,PHI,PH,DP,NPHI,CPHI");
   pal->ctrinc[0] = 70; pal->ctrinc[1] = 10;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_zdr");
   pal->usual_parms = g_string_new ("ZDR,ZD,DR,UZDR,");
   pal->ctrinc[0] = 4; pal->ctrinc[1] = .7;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_ldr");
   pal->usual_parms = g_string_new ("LDR,TLDR,ULDR,LVDR,LH,LV");
   pal->ctrinc[0] = -6; pal->ctrinc[1] = 4;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_dBm");
# ifdef notyet
   pal->usual_parms = g_string_new ("DM,LM,XM,XL,DL,DX,XH,XV,CH,CV");
# else
   pal->usual_parms = g_string_new ("DM,LM,XM,XL,DL,DX");
# endif
   pal->ctrinc[0] = -80.; pal->ctrinc[1] = 5;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_dBz");
   pal->usual_parms = g_string_new ("DBZ,DZ,XZ,DB,Z,UDBZ,CDZ,DCZ,");
   pal->ctrinc[0] = 15; pal->ctrinc[1] = 5;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_spectral");
   pal->usual_parms = g_string_new ("SR,SW,S1,S2");
   pal->ctrinc[0] = 8; pal->ctrinc[1] = 1;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_ncp");
   pal->usual_parms = g_string_new ("NCP,NC,");
   pal->ctrinc[0] = .5; pal->ctrinc[1] = .1;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

# ifdef notyet
   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_fore");
   pal->usual_parms = g_string_new ("VR,");
   pal->ctrinc[0] = -40.; pal->ctrinc[1] = 7;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_aft");
   pal->usual_parms = g_string_new ("VR,");
   pal->ctrinc[0] = 40.; pal->ctrinc[1] = 7;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);
# endif

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_vel");
   pal->usual_parms = g_string_new ("VR,VF,VG,VH,VN,VE,VU,VT,V1,V2,VELOCITY,");
   pal->ctrinc[0] = 0; pal->ctrinc[1] = 3;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_rrate");
   pal->usual_parms = g_string_new ("RR_DSD,RNX,RZD,RKD,");
   pal->ctrinc[0] = 0; pal->ctrinc[1] = .4;
   pal->color_table_name = g_string_new ("rrate11");
   pal->num_colors = 11;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_niq");
   pal->usual_parms = g_string_new ("NIQ,");
   pal->ctrinc[0] = -60; pal->ctrinc[1] = 7;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   pal = sii_malloc_palette();
   pal->p_name = g_string_new ("p_aiq");
   pal->usual_parms = g_string_new ("AIQ,");
   pal->ctrinc[0] = 0; pal->ctrinc[1] = 22;
   item = sii_ll_malloc_item ();
   item->data = (gpointer)pal;
   sii_ll_push (&palette_stack, item);

   return NULL;
}

/* c---------------------------------------------------------------------- */

void sii_do_annotation (guint frame_num, gint exposed, gboolean blow_up, cairo_t *cr)
{
  SiiFrameConfig *sfc = frame_configs[frame_num];
  SiiFrameConfig *sfc0 = frame_configs[0];
  SiiFont *font;
  GdkRGBA *gcolor;
  GtkWidget *frame;
  ParamData *pd = frame_configs[frame_num]->param_data;
  SiiPalette *pal;
  gchar *aa, annot[16], str[256], *sptrs[32];
  const gchar *cc;
  WW_PTR wwptr = solo_return_wwptr(frame_num);
  gint x, y, lbearing, rbearing, width, height, ascent, descent, font_height;
  gint frame_width, frame_height, label_height;
  gint b_width, mx_width = 0, jj, kk, nl, widths[64], cb_height, xb, yb;
  gint cb_thickness;
  gdouble cb_locs[64];

  
  if (!pd || !pd->pal)
    { return; }

  if (blow_up) {
    font = sfc0->big_font;
    cb_thickness = sfc->font_height *2;
    font_height = sfc0->big_font_height;
    frame  = sfc0->blow_up_frame;
    frame_width = sfc0->big_width;
    frame_height = sfc0->big_height;
  }
  else {
    font = sfc->font;
    cb_thickness = 
      font_height = sfc->font_height;
    frame  = sfc->frame;
    frame_width = sfc->width;
    frame_height = sfc->height;
  }

  pal = (exposed && pd->orig_pal) ? pd->orig_pal : pd->pal;
  b_width = sii_font_string_width (font, " ");
  

  /* color bar labels */
  cc = set_cb_labeling_info (frame_num, cb_locs);
  strcpy (str, cc);
  nl = dd_tokens (str, sptrs);
  label_height = font->ascent + font->descent;
  
  for (mx_width=0,jj=0; jj < nl; jj++) {	/* set width of longest annotation */
    aa = sptrs[jj];
    *(widths+jj) = sii_font_string_width (font, aa);
    if (widths[jj] > mx_width)
      { mx_width = widths[jj]; }
  }

  if (pd->cb_loc == PARAM_CB_LEFT || pd->cb_loc == PARAM_CB_RIGHT) {
    if (blow_up) {
      sfc0->big_max_lbl_wdt = mx_width + 2*b_width;
    }
    else {
      sfc->max_lbl_wdt = mx_width + 2*b_width;
    }
    cb_height = frame_height;

    for (kk=nl-1,jj=0; jj < nl; jj++,kk--) {
      x = (pd->cb_loc == PARAM_CB_LEFT)
	? cb_thickness + b_width + mx_width -widths[jj]
	: frame_width -cb_thickness -b_width -widths[jj];

      y = frame_height -cb_height * cb_locs[jj]
	- font_height/2 -2;
      gcolor = pal->feature_color[FEATURE_BACKGROUND];
      sii_cairo_set_color (cr, gcolor);
      if (pd->toggle[PARAM_HILIGHT]) {
	cairo_rectangle (cr, x-1, y+3, widths[jj]+1, font_height-3 ); cairo_fill (cr);
      }
      y += label_height +1;
      gcolor = pal->feature_color[FEATURE_ANNOTATION];
      sii_cairo_set_color (cr, gcolor);
      aa = sptrs[jj];
      sii_cairo_draw_text (cr, font, x, y, aa, strlen (aa));
    }
  }
  else {
    
    /* clear space for color bar labels */
    gcolor = pal->feature_color[FEATURE_BACKGROUND];
    sii_cairo_set_color (cr, gcolor);

    y = frame_height -cb_thickness -4;
    yb = frame_height -cb_thickness -font_height +1;
    
    for (jj=0; jj < nl; jj++) {
      x = (gint)(frame_width * cb_locs[jj]);
      aa = sptrs[jj];
      x -= widths[jj]/2;

      if (pd->toggle[PARAM_HILIGHT]) {
	gcolor = pal->feature_color[FEATURE_BACKGROUND];
	sii_cairo_set_color (cr, gcolor);
	cairo_rectangle (cr, x-1, yb, widths[jj]+2, font_height-2 ); cairo_fill (cr);
      }
      gcolor = pal->feature_color[FEATURE_ANNOTATION];
      sii_cairo_set_color (cr, gcolor);
      sii_cairo_draw_text (cr, font, x, y, aa, strlen (aa));
    }
  }

  gcolor = pal->feature_color[FEATURE_BACKGROUND];
  sii_cairo_set_color (cr, gcolor);
  
  aa = wwptr->top_line;
  width = sii_font_string_width (font, aa);
  x = (frame_width -width)/2;
  
  /* clear space for plot title */
  if (pd->toggle[PARAM_HILIGHT]) {
    cairo_rectangle (cr, x, 0, width+1, font_height ); cairo_fill (cr);
  }
  y = font_height -2;
  gcolor = pal->feature_color[FEATURE_ANNOTATION];
  sii_cairo_set_color (cr, gcolor);
  
  sii_cairo_draw_text (cr, font, x, y, aa, strlen (aa));

  b_width = sii_font_string_width (font, " ");
}

/* c---------------------------------------------------------------------- */

void sii_double_colorize_image (guint frame_num)
{
  SiiFrameConfig *sfc = frame_configs[frame_num];
  SiiFrameConfig *sfc0 = frame_configs[0];
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  guchar *aa, *bb, *cc, *ee, **ct_rgbs, *rgbs;
  gint jj, kk, mm, nn, stride, ndx;

  mm = sfc->width;
  nn = sfc->height;
  stride = sfc->width * 3 * 2;

  aa = (guchar *)sfc->image->data;
  ee = bb = frame_configs[0]->big_image->data;
  rgbs = pd->pal->color_table_rgbs[0]; /* table of 3 byte colors */


  for (kk=0; kk < nn; kk++, bb += stride) {
     ee = bb;
     for (jj=0; jj < mm; jj++, aa++) {
	cc = rgbs + (*aa) * 3;
	*bb++ = *cc;
	*bb++ = *(cc+1);
	*bb++ = *(cc+2);
	*bb++ = *cc;
	*bb++ = *(cc+1);
	*bb++ = *(cc+2);
     }
     memcpy (bb, ee, stride);
  }

}

/* c---------------------------------------------------------------------- */

void sii_get_clip_rectangle (guint frame_num, Rectangle *clip
			     , gboolean blow_up)
{
  SiiFrameConfig *sfc = frame_configs[frame_num];
  SiiFrameConfig *sfc0 = frame_configs[0];
  ParamData *pd = frame_configs[frame_num]->param_data;
  guint mlw;
  gint font_height, max_lbl_wdt, frame_width, frame_height, cb_thickness;

  if (blow_up) {
    max_lbl_wdt = sfc0->big_max_lbl_wdt;
    font_height = sfc0->big_font_height;  
    frame_width = sfc0->big_width;
    frame_height = sfc0->big_height;
    cb_thickness = 2 * sfc->font_height;
  }
  else {
    max_lbl_wdt = sfc->max_lbl_wdt;
    font_height = sfc->font_height;  
    frame_width = sfc->width;
    frame_height = sfc->height;
    cb_thickness = sfc->font_height;
  }

  if (pd->cb_loc == PARAM_CB_LEFT || pd->cb_loc == PARAM_CB_RIGHT) {
    mlw = max_lbl_wdt;

    clip->x = (pd->cb_loc == PARAM_CB_LEFT)
      ? cb_thickness + mlw +1
      : 0;
    clip->y = font_height +2;
    clip->width = frame_width -cb_thickness - mlw -2;
    clip->height = frame_height -font_height -2;
  }
  else {
    clip->x = 0;
    clip->width = frame_width;
    clip->y = font_height +2;
    clip->height = frame_height - 2*font_height -cb_thickness -4;
  }
}

/* c---------------------------------------------------------------------- */

gint sii_get_ctr_and_inc (guint frame_num, float *ctr, float *inc)
{
  WW_PTR wwptr;
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;

  *ctr = pd->pal->ctrinc[0];
  *inc = pd->pal->ctrinc[1];
  return pd->pal->num_colors;
}

/* c---------------------------------------------------------------------- */

gint sii_get_emph_min_max (guint frame_num, float *zmin, float *zmax)
{
  WW_PTR wwptr;
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;

  *zmin = pd->pal->emphasis_zone[0];
  *zmax = pd->pal->emphasis_zone[1];
  return pd->pal->num_colors;
}

/* c---------------------------------------------------------------------- */

GdkRGBA *sii_grid_color (guint frame_num, gint exposed)
{
  GdkRGBA *gcolor;
  ParamData *pd = frame_configs[frame_num]->param_data;
  SiiPalette *pal;

  if (!pd || !pd->pal)
    { return NULL; }
  pal = (exposed && pd->orig_pal) ? pd->orig_pal : pd->pal;
  gcolor = pal->feature_color[FEATURE_RNG_AZ_GRID];
  return gcolor;
}

/* c---------------------------------------------------------------------- */

void sii_initialize_parameter (guint frame_num, gchar *name)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   SiiLinkedList *item;
   GString gs;
   SiiPalette *pal;
   guint size, jj;
   gchar *aa;

   if (!pd) {
      pd = (ParamData *)g_malloc0 (sizeof(ParamData));
      frame_configs[frame_num]->param_data = (gpointer)pd;

      frame_configs[frame_num]->link_set[LI_PARAM] = 
	pd->param_links = sii_new_links_info
	  ( "Parameter Links", frame_num, FRAME_PARAM_LINKS, TRUE );
      pd->cb_loc = PARAM_CB_BOTTOM;
      pd->toggle[PARAM_HILIGHT] = TRUE;
   }
   if (!param_palette_names)
     { sii_new_palettes_list (); }

   pd->pal = pal = sii_set_palette (name);

   for (jj=0; jj < PARAM_MAX_WIDGETS; jj++) {
      pd->precision[jj] = 3;
      pd->orig_txt[jj] = g_string_new ("");
      pd->txt[jj] = g_string_new ("");
   }
   /*
    * Set all the original values and copies from the palette
    *
    */
   g_string_assign (pd->orig_txt[PARAM_NAME], name);

   g_string_assign (pd->orig_txt[PARAM_GRID_CLR], pal->grid_color->str);

   g_string_assign (pd->orig_txt[PARAM_CLR_TBL_NAME], pal->color_table_name->str);

   g_string_assign (pd->orig_txt[PARAM_PALETTE], pal->p_name->str);

   g_string_assign (pd->orig_txt[PARAM_XCEED_CLR], pal->exceeded_color->str);

   g_string_assign (pd->orig_txt[PARAM_MSSNG_CLR], pal->missing_data_color->str);

   g_string_assign (pd->orig_txt[PARAM_BND_CLR], pal->boundary_color->str);

   g_string_assign (pd->orig_txt[PARAM_ANNOT_CLR], pal->annotation_color->str);

   g_string_assign (pd->orig_txt[PARAM_BACKG_CLR], pal->background_color->str);

   g_string_assign (pd->orig_txt[PARAM_EMPH_CLR], pal->emphasis_color->str);


   size = 2 * sizeof (gfloat);

   g_memmove (pd->orig_values[PARAM_CTRINC], pal->ctrinc, size);
   aa = sii_set_string_from_vals (pd->orig_txt[PARAM_CTRINC], 2
				  , pal->ctrinc[0], pal->ctrinc[1], 3);

   sii_min_max_from_ctr_inc (pd->pal->num_colors, &pal->ctrinc[0], &pal->ctrinc[1],
			     &pal->minmax[0], &pal->minmax[1]);
   
   g_memmove (pd->orig_values[PARAM_MINMAX], pal->minmax, size);
   aa = sii_set_string_from_vals (pd->orig_txt[PARAM_MINMAX], 2
				  , pal->minmax[0], pal->minmax[1], 3);

   g_memmove (pd->orig_values[PARAM_EMPH_MINMAX], pal->emphasis_zone, size);
   aa = sii_set_string_from_vals (pd->orig_txt[PARAM_EMPH_MINMAX], 2
				  , pal->emphasis_zone[0], pal->emphasis_zone[1], 3);


   sii_param_dup_orig (frame_num);

   /* set up circular que of toggle info */

   pd->fields_list = sii_init_circ_que (maxFrames);
   pd->field_toggle_count = 2;

}
/* c---------------------------------------------------------------------- */

SiiPalette *sii_malloc_palette()
{
   SiiPalette * pal;

   pal = (SiiPalette*)g_malloc0 (sizeof (SiiPalette));
   pal->ctrinc[0] = 0; pal->ctrinc[1] = 5;
   pal->minmax[0] = pal->ctrinc[0] - .5 * pal->num_colors * pal->ctrinc[1];
   pal->minmax[1] = pal->ctrinc[0] + pal->num_colors * pal->ctrinc[1];
   pal->num_colors = 17;
   pal->color_table_name = g_string_new ("carbone17");
   pal->grid_color = g_string_new ("gray90");
   pal->missing_data_color = g_string_new ("darkslateblue");
   pal->exceeded_color = g_string_new ("gray70");
   pal->annotation_color = g_string_new ("gray90");
   pal->background_color = g_string_new ("midnightblue");
   pal->emphasis_color = g_string_new ("hotpink");
   pal->boundary_color = g_string_new ("orangered");
   return pal;
}

/* c---------------------------------------------------------------------- */

void sii_min_max_from_ctr_inc (guint ncolors, gfloat *ctr, gfloat *inc,
			       gfloat *min, gfloat *max )
{
   gfloat tmp;

   *min = *ctr - *inc * ncolors * .5;
   *max = *min + *inc * ncolors;

   *min += (*min < 0) ? -CB_ROUND : CB_ROUND;
   *max += (*max < 0) ? -CB_ROUND : CB_ROUND;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_new_palette_for_param (const gchar *p_name, const gchar *name)
{
   SiiPalette *pal = NULL;
   SiiLinkedList *item;
   gchar str[512], *sptrs[64];
   int nt, nn;
   
   /* see if this palette already exists
    * and if so return it with the name added
    * to the usualparams
    */

   if (item = sii_palette_seek (p_name)) {			/* found */
     sii_ll_remove (&palette_stack, item);
   }
   else {
      pal = sii_malloc_palette();
      pal->p_name = g_string_new (p_name);
      pal->usual_parms = g_string_new ("");
      item = sii_ll_malloc_item ();
      item->data = (gpointer)pal;
   }
   pal = (SiiPalette *)item->data;
   sii_palette_prepend_usual_param (pal, name);
   sii_palette_remove_usual_param (pal, name);
   sii_ll_push (&palette_stack, item);
   sii_new_palettes_list ();
   return item;
}

/* c---------------------------------------------------------------------- */

gchar *sii_new_color_tables_list ()
{ 
   SiiPalette * pal;
   GSList *list;
   guint size=0, jj, nn;
   gchar *aa, *bb, *name, *usual_names;;

   if (!gs_color_table_names) {
      gs_color_table_names = g_string_new ("");
   }
   g_string_truncate (gs_color_table_names, 0);

   g_tree_traverse ( ascii_color_tables
		   , (gpointer)sii_color_tables_lister, G_IN_ORDER, 0 );

   param_color_table_names = gs_color_table_names->str;
   return param_color_table_names;
}

/* c---------------------------------------------------------------------- */

gchar *sii_new_colors_list ()
{ 
   SiiPalette * pal;
   GSList *list;
   guint size=0, jj, nn;
   gchar *aa, *bb, *name, *usual_names;

   if (!gs_color_names) {
      gs_color_names = g_string_new ("");
   }
   g_string_truncate (gs_color_names, 0);

   g_tree_traverse ( colors_tree
		   , (gpointer)sii_colors_lister, G_IN_ORDER, 0 );

   param_color_names = gs_color_names->str;
   return param_color_names;
}

/* c---------------------------------------------------------------------- */

gchar *sii_new_palettes_list ()
{ 
   static GString *gs_list = NULL;
   SiiPalette * pal;
   GSList *list;
   guint size=0, jj, nn;
   gchar *aa, *bb, *name, *usual_names;
   SiiLinkedList *item = palette_stack;

   if (!gs_list) {
      gs_list = g_string_new ("");
      sii_default_palettes_list();
   }
   g_string_truncate (gs_list, 0);

   for (item = palette_stack; item;  item = item->next) {
      pal = (SiiPalette *)item->data;
      g_string_append_printf (gs_list, "%s  (%s)\n", pal->p_name->str
			 , pal->usual_parms->str );
   }
   param_palette_names = gs_list->str;

   return param_palette_names;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_palette_for_param (const gchar *name)
{
   SiiPalette *pal = NULL;
   SiiLinkedList *item = palette_stack;
   gchar str[512], *sptrs[64];
   int nt, nn;
   
   for(; item; item = item->next) {
      pal = (SiiPalette *)item->data;
      strcpy (str, pal->usual_parms->str);
      nt = dd_tokenz (str, sptrs, " ,");
      nn = sii_str_seek (sptrs, nt, name);
      if (nn >= 0)
	{ return item; }
   }
   return NULL;
}

/* c---------------------------------------------------------------------- */

void sii_palette_prepend_usual_param (SiiPalette *pal, const gchar *name)
{
   /*
    * Or shift the name to the front
    */
   gchar str[512], *sptrs[64];
   int jj, nt, nn;
   
   if (!pal)
     { return; }

   strcpy (str, pal->usual_parms->str);
   nt = dd_tokenz (str, sptrs, COMMA);
   nn = sii_str_seek (sptrs, nt, name);

   g_string_truncate (pal->usual_parms, 0);
   g_string_append (pal->usual_parms, name);
   g_string_append (pal->usual_parms, COMMA);

   for (jj=0; jj < nt; jj++) {
      if( nn >= 0 && jj == nn)
	{ continue; }		/* ignore if already there */
      g_string_append (pal->usual_parms, sptrs[jj]);
      g_string_append (pal->usual_parms, COMMA);
   }
}

/* c---------------------------------------------------------------------- */

void sii_palette_remove_usual_param ( SiiPalette *palx, const gchar *name)
{
   /*
    * remove the name from all other palettes except this one
    */
   SiiPalette *pal = NULL;
   SiiLinkedList *item = palette_stack;
   gchar str[512], *sptrs[64];
   int jj, nt, nn;
   
   if (!palx)
     { return; }

   for(; item; item = item->next) {

      pal = (SiiPalette *)item->data;
      if (pal == palx)
	{ continue;}
      strcpy (str, pal->usual_parms->str);
      nt = dd_tokenz (str, sptrs, COMMA);
      nn = sii_str_seek (sptrs, nt, name);

      if (nn < 0 || nt < 1)
	{ continue; }

      g_string_truncate (pal->usual_parms, 0);
      if (nt == 1) {
	 g_string_append (pal->usual_parms, COMMA);
	 continue;
      }
      for (jj=0; jj < nt; jj++) {
	 if( nn >= 0 && jj == nn)
	   { continue; }	/* ignore if already there */
	 g_string_append (pal->usual_parms, sptrs[jj]);
	 g_string_append (pal->usual_parms, COMMA);
      }
   }
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_palette_seek (const gchar *p_name)
{
   SiiPalette *pal = NULL;
   SiiLinkedList *item = palette_stack;
   gchar str[512], *sptrs[64];
   int nt, nn;
   
   for(; item; item = item->next) {
      pal = (SiiPalette *)item->data;
      if (strcmp (pal->p_name->str, p_name) == 0 )
	{ return item; }
   }
   return NULL;
}

/* c---------------------------------------------------------------------- */

void sii_param_dup_entries (guint frame_num)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   guint size, jj, entry_flag;
   const gchar *aa;
   size = sizeof (*pd->values[jj]);

   for (jj=0; jj < PARAM_MAX_WIDGETS; jj++) {
      if (!pd->entry_flag[jj])
	{ continue; }

      aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[jj]));
      g_string_assign (pd->txt[jj], aa);
   }
}


/* c---------------------------------------------------------------------- */

void sii_param_dup_opal (struct solo_palette *opal)
{
  SiiPalette *pal;
  SiiLinkedList *item;
  gchar *aa, *bb, str[256];

  item = sii_palette_seek (opal->palette_name);
  if (item) {
    sii_ll_remove (&palette_stack, item);
  }
  else {
    pal = sii_malloc_palette();
    pal->p_name = g_string_new (opal->palette_name);
    pal->usual_parms = g_string_new ("");
    item = sii_ll_malloc_item ();
    item->data = (gpointer)pal;
  }
  pal = (SiiPalette *)item->data;
  sii_ll_push (&palette_stack, item);
  sii_new_palettes_list ();


  g_string_assign (pal->usual_parms, opal->usual_parms);
  pal->ctrinc[0] = opal->center;
  pal->ctrinc[1] = opal->increment;
  pal->num_colors = opal->num_colors;
  pal->emphasis_zone[0] = opal->emphasis_zone_lower;
  pal->emphasis_zone[1] = opal->emphasis_zone_upper;

  aa = opal->color_table_name;
  bb = (gchar *)g_tree_lookup( ascii_color_tables, (gpointer)aa);

  if (!bb) {
    strcpy (str, aa);
    if (bb = strstr (str, ".colors")) {
      *bb = '\0';
      aa = str;
      bb = (gchar *)g_tree_lookup( ascii_color_tables, (gpointer)aa);
      if (!bb)
      { aa = "carbone17"; pal->num_colors = 17; }
    }
    else
      { aa = "carbone17"; pal->num_colors = 17; }
  }

  g_string_assign (pal->color_table_name, aa);

  g_string_assign (pal->grid_color
		   , sii_param_fix_color (opal->grid_color));
  g_string_assign (pal->missing_data_color
		   , sii_param_fix_color (opal->missing_data_color));
  g_string_assign (pal->exceeded_color
		   , sii_param_fix_color (opal->exceeded_color));
  g_string_assign (pal->annotation_color
		   , sii_param_fix_color (opal->annotation_color));
  g_string_assign (pal->background_color
		   , sii_param_fix_color (opal->background_color));
  g_string_assign (pal->emphasis_color
		   , sii_param_fix_color (opal->emphasis_color));
  g_string_assign (pal->boundary_color
		   , sii_param_fix_color (opal->bnd_color));
}

/* c---------------------------------------------------------------------- */

void sii_param_dup_orig (guint frame_num)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   guint size, jj, entry_flag;
   size = sizeof (*pd->values[jj]);

   for (jj=0; jj < PARAM_MAX_WIDGETS; jj++) {

      entry_flag = pd->entry_flag[jj];
      if (!entry_flag)
	{ continue; }
				/*
      g_message ("jj:%d  ef:%d", jj, entry_flag);
				 */
      switch (entry_flag) {

       case ENTRY_TXT_ONLY:
	 break;

       default:
	 g_string_assign (pd->txt[jj], pd->orig_txt[jj]->str);
	 g_memmove (pd->values[jj], pd->orig_values[jj], sizeof (*pd->values[jj]));
	 break;
      };
   }
}

/* c---------------------------------------------------------------------- */

void sii_param_dup_pal (gpointer sii_pal, gpointer old_pal)
{
  struct solo_palette *opal;
  SiiPalette *pal;

  opal = (struct solo_palette *)old_pal;
  pal = (SiiPalette *)sii_pal;

  strcpy (opal->palette_name, pal->p_name->str);
  strcpy (opal->usual_parms, pal->usual_parms->str);
  opal->center = pal->ctrinc[0];
  opal->increment = pal->ctrinc[1];
  opal->num_colors = pal->num_colors;
  opal->emphasis_zone_lower = pal->emphasis_zone[0];
  opal->emphasis_zone_upper = pal->emphasis_zone[1];  
  strcpy (opal->color_table_name, pal->color_table_name->str);
  strcpy (opal->grid_color, pal->grid_color->str);
  strcpy (opal->missing_data_color, pal->missing_data_color->str);
  strcpy (opal->exceeded_color, pal->exceeded_color->str);
  strcpy (opal->annotation_color, pal->annotation_color->str);
  strcpy (opal->background_color, pal->background_color->str);
  strcpy (opal->emphasis_color, pal->emphasis_color->str);
  strcpy (opal->bnd_color, pal->boundary_color->str);

}

/* c---------------------------------------------------------------------- */

void sii_param_check_changes (guint frame_num)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   const gchar *aa;
   guint jj;

   for (jj=0; jj < PARAM_MAX_WIDGETS; pd->change_flag[jj++] = FALSE);
   pd->change_count = 0;
   
   for (jj=0; jj < PARAM_MAX_WIDGETS; jj++) {
     if (!pd->entry_flag[jj])
       { continue; }
     aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[jj]));
     aa = pd->txt[jj]->str;

     if (strcmp (pd->orig_txt[jj]->str, aa) != 0) {
       pd->change_flag[jj] = TRUE;
       pd->change_count++;
     }
   }
}

/* c---------------------------------------------------------------------- */

void sii_param_entry_info
(GtkWidget *w, guint wid, guint frame_num, guint entry_flag)
{
   pdata->data_widget[wid] = w;
   pdata->entry_flag[wid] = entry_flag;
}

/* c---------------------------------------------------------------------- */

void sii_param_entry_paste_cb ( GtkWidget *w, gpointer   data )
{
   GtkWidget *widget;
   guint num = GPOINTER_TO_UINT (data);
   guint frame_num, wid;
   ParamData *pdata;
   
   frame_num = num/TASK_MODULO;
   wid = num % TASK_MODULO;
   pdata = (ParamData *)frame_configs[frame_num]->param_data;

				/* clear out the entry befor pasting */
   gtk_editable_set_text (GTK_EDITABLE (pdata->data_widget[wid]), "");

}

/* c---------------------------------------------------------------------- */

gchar *sii_param_fix_color (gchar *old_color)
{
  static gchar str[32], *aa;

  if(!(aa = strstr (old_color, "grey")))
    { return old_color; }
  strcpy (str, old_color);
  strncpy (str, "gray", 4);
  return str;
}
/* c---------------------------------------------------------------------- */

void sii_param_menu_cb ( GtkWidget *w, gpointer   data )
{
   guint num = GPOINTER_TO_UINT (data);
   guint nn, frame_num, task, wid, frame_wid, cancel_wid, active, taskx;
   const gchar *aa, *bb, *line, *p_name, *name;
   gfloat f1, f2, ftmp, ctr, inc, rnd = .000001;
   gint nt, ncolors = 17, jj, flag;
   ParamData *pd, *pdx;
   GtkWidget *widget, *check_item, *rmi;
   SiiLinkedList *item;   
   SiiPalette *pal;
   WW_PTR wwptr;
   GtkWidget *fs;
   gchar * dirroot;

				/* c...menu_cb */
   frame_num = num/TASK_MODULO;
   wid = task = num % TASK_MODULO;

   pd = (ParamData *)frame_configs[frame_num]->param_data;
   widget =
     check_item =
       rmi = pd->data_widget[task];



   switch (task) {
   case PARAM_REPLOT_THIS:
   case PARAM_REPLOT_LINKS:
   case PARAM_REPLOT_ALL:
     flag = REPLOT_THIS_FRAME;

     if (task == PARAM_REPLOT_LINKS)
       { flag = REPLOT_LOCK_STEP; }
     else if (task == PARAM_REPLOT_ALL)
       { flag = REPLOT_ALL; }

     sii_param_dup_entries (frame_num);
     if (sii_set_param_info (frame_num))
       { sii_plot_data (frame_num, flag); }
     break;
     
   case PARAM_PALETTE_LIST:
   case PARAM_COLORS_LIST:
     show_param_generic_list_widget (frame_num, task);
     break;

   case PARAM_CLR_TBL_LIST:
     show_param_generic_list_widget (frame_num, task);
     break;

   case PARAM_COLORS_FILESEL:
     wid = sii_return_colors_filesel_wid();
     dirroot = (gchar *)g_object_get_data (G_OBJECT (w), "dirroot");
     fs = sii_filesel (wid, dirroot);
     g_object_set_data (G_OBJECT (fs)
			  , "frame_num", (gpointer)frame_num);
     widget = sii_get_widget_ptr (frame_num, FRAME_PARAM_ENTRY);
     gtk_widget_set_visible (widget, FALSE);
     break;

   case PARAM_MPORT_CLR_TBL:
     widget = sii_get_widget_ptr (frame_num, FRAME_PARAM_ENTRY);
     if (!widget)
       {
	  if (!(aa = getenv ("COLORS_FILESEL")))
	    { aa = "/"; }
	 param_entry_widget(frame_num
 , "\n Type the full path name of the color table file and press return \n"
       , aa); }
     else
       { gtk_widget_set_visible (widget, TRUE); }
     break;

   case PARAM_CLR_TBL_PATH:
     aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[PARAM_GNRK_ENTRY]));
     sii_param_absorb_ctbl (frame_num, aa);

   case PARAM_GNRK_ENTRY_CANCEL:
     widget = sii_get_widget_ptr (frame_num, FRAME_PARAM_ENTRY);
     if (widget)
       { gtk_widget_set_visible (widget, FALSE); }
     break;

   case PARAM_PALETTE:
      p_name = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[wid]));
      name = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[PARAM_NAME]));
      sii_new_palette_for_param (p_name, name);
      sii_param_process_changes (frame_num);
      sii_redisplay_list (frame_num, PARAM_PALETTES_TEXT);
      break;

    case PARAM_NAME:
      nn = PARAM_NAME;
      aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[wid]));
      if( aa && strlen (aa))
	{ sii_param_dup_entries (frame_num); }
      else
	{ break; }

      sii_param_process_changes (frame_num);
      sii_redisplay_list (frame_num, PARAM_PALETTES_TEXT);
      break;

    case PARAM_MINMAX:
      aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[wid]));
      if( !sii_str_values ( aa, 2, &f1, &f2 )) {
	 gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[PARAM_MINMAX])
			     , pd->txt[PARAM_MINMAX]->str);
	 break;
      }
      sii_ctr_inc_from_min_max (pd->pal->num_colors, &ctr, &inc, &f1, &f2);

      aa = sii_set_string_from_vals (pd->txt[PARAM_MINMAX], 2, f1, f2, 3);
      gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[PARAM_MINMAX]), aa);
			  
      aa = sii_set_string_from_vals (pd->txt[PARAM_CTRINC], 2, ctr, inc, 3);
      gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[PARAM_CTRINC]), aa);

      break;

    case PARAM_CTRINC:
      aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[wid]));
      if( !sii_str_values ( aa, 2, &ctr, &inc ))
	{ break; }

      sii_min_max_from_ctr_inc (pd->pal->num_colors, &ctr, &inc, &f1, &f2);

      aa = sii_set_string_from_vals (pd->txt[PARAM_MINMAX], 2, f1, f2, 3);
      gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[PARAM_MINMAX]), aa);
			  
      aa = sii_set_string_from_vals (pd->txt[PARAM_CTRINC], 2, ctr, inc, 3);
      gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[PARAM_CTRINC]), aa);

      break;

    case PARAM_EMPH_MINMAX:
      aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[wid]));
      if( !sii_str_values ( aa, 2, &f1, &f2 ))
	{ break; }
      
      pd->values[task][0] = f1;
      pd->values[task][1] = f2;
      break;
      
    case PARAM_OK:
      sii_param_dup_entries (frame_num);
      sii_set_param_info (frame_num);
    case PARAM_CANCEL:
    case PARAM_CLOSE:
      widget = sii_get_widget_ptr (frame_num, FRAME_PARAMETERS);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      widget = sii_get_widget_ptr (frame_num, FRAME_PALETTE_LIST);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      widget = sii_get_widget_ptr (frame_num, FRAME_COLR_TBL_LIST);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      widget = sii_get_widget_ptr (frame_num, FRAME_COLORS_LIST);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      widget = sii_get_widget_ptr (frame_num, FRAME_PARAM_LINKS);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      break;
      
    case PARAM_LINKS:
      show_links_widget (pd->param_links); 
      break;

    case PARAM_PALETTE_CANCEL:
      widget = sii_get_widget_ptr (frame_num, FRAME_PALETTE_LIST);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      break;
      
    case PARAM_CLR_TBL_CANCEL:
      widget = sii_get_widget_ptr (frame_num, FRAME_COLR_TBL_LIST);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      break;
      
    case PARAM_COLORS_CANCEL:
      widget = sii_get_widget_ptr (frame_num, FRAME_COLORS_LIST);
      if( widget )
	{ gtk_widget_set_visible (widget, FALSE); }
      break;
      
    case PARAM_GRID_CLR:
    case PARAM_CLR_TBL_NAME:
    case PARAM_XCEED_CLR:
    case PARAM_MSSNG_CLR:
    case PARAM_ANNOT_CLR:
    case PARAM_BACKG_CLR:
    case PARAM_EMPH_CLR:
    case PARAM_BND_CLR:
      aa = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[wid]));
      if( aa && strlen (aa))
	{ g_string_assign (pd->txt[task], aa ); }
      break;

      /* Check menu item */
   case PARAM_ELECTRIC:
   case PARAM_HILIGHT:
   case PARAM_CB_SYMBOLS:
      pd->toggle[task] = gtk_check_button_get_active(GTK_CHECK_BUTTON(rmi));
      break;

   case PARAM_CB_BOTTOM:
   case PARAM_CB_LEFT:
   case PARAM_CB_RIGHT:
     g_return_if_fail(GTK_IS_CHECK_BUTTON(rmi));
     active = gtk_check_button_get_active(GTK_CHECK_BUTTON(check_item));
     if (active)
       { pd->cb_loc = task; }
     pd->toggle[task] = active;
     break;

   case PARAM_BC_GRID:
   case PARAM_BC_BND:
   case PARAM_BC_BACKG:
   case PARAM_BC_XCEED:
   case PARAM_BC_MSSNG:
   case PARAM_BC_EMPH:
     for(item = palette_stack; item; item = item->next) {
	pal = (SiiPalette *)item->data;
	if (pal == pd->pal)
	  { continue; }
	if (task == PARAM_BC_GRID)
	  {g_string_assign (pal->grid_color, pd->pal->grid_color->str);}
	else if (task == PARAM_BC_BND)
	  {g_string_assign (pal->boundary_color, pd->pal->boundary_color->str);}
	else if (task == PARAM_BC_BACKG)
	  {g_string_assign (pal->background_color, pd->pal->background_color->str);}
	else if (task == PARAM_BC_XCEED)
	  {g_string_assign (pal->exceeded_color, pd->pal->exceeded_color->str);}
	else if (task == PARAM_BC_MSSNG)
	  {g_string_assign (pal->missing_data_color, pd->pal->missing_data_color->str);}
	else if (task == PARAM_BC_EMPH)
	  {g_string_assign (pal->emphasis_color, pd->pal->emphasis_color->str);}


	g_string_assign (pal->grid_color, pd->pal->grid_color->str);
     }
     for (jj=0; jj < maxFrames; jj++) {
	solo_return_wwptr(jj)->parameter->changed = YES;
     }
     break;

   case PARAM_BC_ANNOT:
     for(item = palette_stack; item; item = item->next) {
	pal = (SiiPalette *)item->data;
	if (pal == pd->pal)
	  { continue; }
	g_string_assign (pal->annotation_color
			 , pd->pal->annotation_color->str);
     }
     for (jj=0; jj < maxFrames; jj++) {
	if (jj == frame_num)
	  { continue; }
	widget = sii_get_widget_ptr (jj, FRAME_PARAMETERS);
	pdx = (ParamData *)frame_configs[jj]->param_data;
	pdx->toggle[PARAM_HILIGHT] = active = 
	  pd->toggle[PARAM_HILIGHT];
	if( widget ) {
	   check_item = pd->data_widget[PARAM_HILIGHT];
	   gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), active);
	}
	solo_return_wwptr(jj)->parameter->changed = YES;
     }
     break;

   case PARAM_BC_CB:
     for(item = palette_stack; item; item = item->next) {
	pal = (SiiPalette *)item->data;
	if (pal == pd->pal)
	  { continue; }
     }
     for (jj=0; jj < maxFrames; jj++) {
	if (jj == frame_num)
	  { continue; }
	widget = sii_get_widget_ptr (jj, FRAME_PARAMETERS);
	pdx = (ParamData *)frame_configs[jj]->param_data;
	pdx->cb_loc = pd->cb_loc;

	if( widget ) {
	   check_item = pd->data_widget[pdx->cb_loc];
	   gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), TRUE);
	}
	pdx->toggle[PARAM_CB_SYMBOLS] = active = 
	  pd->toggle[PARAM_CB_SYMBOLS];
	if( widget ) {
	   check_item = pd->data_widget[PARAM_CB_SYMBOLS];
	   gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), active);
	}
	pdx->toggle[PARAM_HILIGHT] = active = 
	  pd->toggle[PARAM_HILIGHT];
	if( widget ) {
	   check_item = pd->data_widget[PARAM_HILIGHT];
	   gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), active);
	}
	solo_return_wwptr(jj)->parameter->changed = YES;
	sii_set_param_info (jj);
     }
     break;

   case PARAM_OVERVIEW:
     nn = sizeof (hlp_param_overview)/sizeof (char *);
     sii_glom_strings (hlp_param_overview, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case PARAM_HLP_FILE:
     nn = sizeof (hlp_param_file)/sizeof (char *);
     sii_glom_strings (hlp_param_file, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case PARAM_HLP_OPTIONS:
     nn = sizeof (hlp_param_options)/sizeof (char *);
     sii_glom_strings (hlp_param_options, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case PARAM_HLP_LINKS:
     nn = sizeof (hlp_param_links)/sizeof (char *);
     sii_glom_strings (hlp_param_links, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case PARAM_HLP_PALETTES:
     nn = sizeof (hlp_param_palettes)/sizeof (char *);
     sii_glom_strings (hlp_param_palettes, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   case PARAM_HLP_MINMAX:
     nn = sizeof (hlp_param_minmax)/sizeof (char *);
     sii_glom_strings (hlp_param_minmax, nn, gen_gs);
     sii_message (gen_gs->str);
     break;

   };
}

/* c---------------------------------------------------------------------- */

void sii_param_menubar2( GtkWidget  *window, GtkWidget **menubar
		       , guint frame_num)
{
   /* GTK4: Build menu using GMenu/GAction model */
   GMenu *menu_bar_model = g_menu_new ();
   GMenu *file_menu = g_menu_new ();
   GMenu *options_menu = g_menu_new ();
   GMenu *replot_menu = g_menu_new ();
   GMenu *links_menu = g_menu_new ();
   GMenu *help_menu = g_menu_new ();
   GSimpleActionGroup *actions = g_simple_action_group_new ();
   GSimpleAction *action;
   gchar action_name[64];
   guint nn;

   /* Helper macro for adding menu items with actions */
#define PARAM_ADD_ACTION(wid, label, menu) do { \
     nn = frame_num * TASK_MODULO + (wid); \
     g_snprintf (action_name, sizeof(action_name), "param-%d-%d", frame_num, (wid)); \
     action = g_simple_action_new (action_name, NULL); \
     g_object_set_data (G_OBJECT (action), "widget_id", GUINT_TO_POINTER(wid)); \
     g_object_set_data (G_OBJECT (action), "frame_num", GUINT_TO_POINTER(frame_num)); \
     g_signal_connect (action, "activate", G_CALLBACK (sii_param_menu_cb), GUINT_TO_POINTER(nn)); \
     g_action_map_add_action (G_ACTION_MAP (actions), G_ACTION (action)); \
     { gchar detailed[80]; g_snprintf (detailed, sizeof(detailed), "param.%s", action_name); \
       g_menu_append (menu, label, detailed); } \
     g_object_unref (action); \
   } while(0)

   /* File menu */
   PARAM_ADD_ACTION (PARAM_CLOSE, "Close", file_menu);
   PARAM_ADD_ACTION (PARAM_PALETTE_LIST, "List Palettes", file_menu);
   PARAM_ADD_ACTION (PARAM_CLR_TBL_LIST, "List Color Tables", file_menu);
   PARAM_ADD_ACTION (PARAM_COLORS_LIST, "List Colors", file_menu);
   PARAM_ADD_ACTION (PARAM_MPORT_CLR_TBL, "Import a Color Table", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_GRID, "Grid Color", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_BND, "Boundary Color", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_XCEED, "Exceeded Color", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_MSSNG, "Missing Data Color", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_ANNOT, "Annotation Color", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_BACKG, "Background Color", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_EMPH, "Emphasis Color", file_menu);
   PARAM_ADD_ACTION (PARAM_BC_CB, "Color Bar Settings", file_menu);

   /* Options menu - includes toggle items as check buttons in the widget */
   PARAM_ADD_ACTION (PARAM_CB_BOTTOM, "At Bottom", options_menu);
   PARAM_ADD_ACTION (PARAM_CB_LEFT, "At Left", options_menu);
   PARAM_ADD_ACTION (PARAM_CB_RIGHT, "At Right", options_menu);
   PARAM_ADD_ACTION (PARAM_CB_SYMBOLS, "Use Symbols", options_menu);
   PARAM_ADD_ACTION (PARAM_HILIGHT, "HiLight Labels", options_menu);
   PARAM_ADD_ACTION (PARAM_ELECTRIC, "Electric Params", options_menu);

   /* Replot menu */
   PARAM_ADD_ACTION (PARAM_REPLOT_LINKS, "Replot Links", replot_menu);
   PARAM_ADD_ACTION (PARAM_REPLOT_ALL, "Replot All", replot_menu);

   /* ParamLinks menu */
   PARAM_ADD_ACTION (PARAM_LINKS, "Set Links", links_menu);

   /* Help menu */
   PARAM_ADD_ACTION (PARAM_OVERVIEW, "Overview", help_menu);
   PARAM_ADD_ACTION (PARAM_HLP_FILE, "File", help_menu);
   PARAM_ADD_ACTION (PARAM_HLP_OPTIONS, "Options", help_menu);
   PARAM_ADD_ACTION (PARAM_HLP_LINKS, "ParamLinks", help_menu);
   PARAM_ADD_ACTION (PARAM_HLP_PALETTES, "Palettes", help_menu);
   PARAM_ADD_ACTION (PARAM_HLP_MINMAX, "Min & Max", help_menu);

#undef PARAM_ADD_ACTION

   g_menu_append_submenu (menu_bar_model, "File", G_MENU_MODEL (file_menu));
   g_menu_append_submenu (menu_bar_model, "Options", G_MENU_MODEL (options_menu));
   g_menu_append_submenu (menu_bar_model, "Replot", G_MENU_MODEL (replot_menu));
   g_menu_append_submenu (menu_bar_model, "ParamLinks", G_MENU_MODEL (links_menu));
   g_menu_append_submenu (menu_bar_model, "Help", G_MENU_MODEL (help_menu));

   gtk_widget_insert_action_group (window, "param", G_ACTION_GROUP (actions));
   *menubar = gtk_popover_menu_bar_new_from_model (G_MENU_MODEL (menu_bar_model));

   g_object_unref (file_menu);
   g_object_unref (options_menu);
   g_object_unref (replot_menu);
   g_object_unref (links_menu);
   g_object_unref (help_menu);
   g_object_unref (menu_bar_model);

   /* Toggle defaults are now handled by the action states */
   pdata->toggle[PARAM_ELECTRIC] = TRUE;
   pdata->toggle[PARAM_HILIGHT] = TRUE;
}

/* c---------------------------------------------------------------------- */

const gchar *sii_param_palette_name (guint frame_num)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   return pd->pal->p_name->str;
}

/* c---------------------------------------------------------------------- */

void sii_param_process_changes (guint frame_num)
{
   /*
      If you want to associate the current parameter name with a
      differen palette, type in the new palette name and press <enter>.

      Or if you want to define a new name and a new palette,
      type in the name but DO NOT press enter. Then type in
      the new palette name and press <enter>.
      
      Change the other parameters to suite your needs and click
      "Replot" or "OK" and the parameters will persist.
    */

   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   SiiPalette *pal, *pal0;
   gfloat f1, f2;
   guint jj, kk, wid;
   const gchar *aa, *bb, *p_name, *name;
   gboolean ok;


   sii_param_dup_entries (frame_num);
   sii_param_check_changes (frame_num);
   name = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[PARAM_NAME]));

   if(pd->change_flag[PARAM_PALETTE]) {
      p_name = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[PARAM_PALETTE]));
      sii_new_palette_for_param (p_name, name);
   }
   else if(pd->change_flag[PARAM_NAME]) {
     /* the name changed but the palette didn't */
     pd->pal = sii_set_palette (name);
   }
   
   pal = pd->pal;
   jj = PARAM_CTRINC;
   kk = PARAM_MINMAX;
   
   if(pd->change_flag[jj]) {
      /* calculate min max */
     ok = sii_str_values (pd->txt[jj]->str, 2, &f1, &f2);

     if (ok) {
       pal->ctrinc[0] = f1; pal->ctrinc[1] = f2;
       bb = sii_set_string_from_vals
	 (pd->orig_txt[jj], 2, pal->ctrinc[0], pal->ctrinc[1]
	  , pd->precision[jj]);
       gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[jj]), bb);
       sii_min_max_from_ctr_inc (pal->num_colors
				 , &pal->ctrinc[0], &pal->ctrinc[1]
				 , &pal->minmax[0], &pal->minmax[1]);
       bb = sii_set_string_from_vals
	 (pd->orig_txt[kk], 2, pal->minmax[0], pal->minmax[1]
	  , pd->precision[kk]);
       gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[kk]), bb);
     }
   }
   else if(pd->change_flag[kk]) {
     /* calculate ctr inc */
     ok = sii_str_values (pd->txt[jj]->str, 2, &f1, &f2);

     if (ok) {
       pal->minmax[0] = f1; pal->minmax[1] = f2;
       bb = sii_set_string_from_vals
	 (pd->orig_txt[kk], 2, pal->minmax[0], pal->minmax[1]
	  , pd->precision[kk]);
       gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[kk]), bb);
       sii_ctr_inc_from_min_max (pal->num_colors
				 , &pal->ctrinc[0], &pal->ctrinc[1]
				 , &pal->minmax[0], &pal->minmax[1]);
       bb = sii_set_string_from_vals
	   (pd->orig_txt[jj], 2, pal->ctrinc[0], pal->ctrinc[1]
	    , pd->precision[jj]);
       gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[jj]), bb);
     }
   }
   
   for (jj=0; jj < PARAM_MAX_WIDGETS; jj++) {
      if (!pd->entry_flag[jj] || !pd->change_flag[jj])
	{ continue; }

      its_changed = TRUE;
      aa = pd->txt[jj]->str;

      switch (jj) {
	 
       case PARAM_GRID_CLR:
	 g_string_assign (pal->grid_color, aa);
	 break;
	 
       case PARAM_CLR_TBL_NAME:
	 g_string_assign (pal->color_table_name, aa);
	 break;
	 
       case PARAM_BND_CLR:
	 g_string_assign (pal->boundary_color, aa);
	 break;
	 
       case PARAM_XCEED_CLR:
	 g_string_assign (pal->exceeded_color, aa);
	 break;
	 
       case PARAM_MSSNG_CLR:
	 g_string_assign (pal->missing_data_color, aa);
	 break;

       case PARAM_ANNOT_CLR:
	 g_string_assign (pal->annotation_color, aa);
	 break;

       case PARAM_BACKG_CLR:
	 g_string_assign (pal->background_color, aa);
	 break;

       case PARAM_EMPH_MINMAX:
	 sii_str_values (aa, 2, &pal->emphasis_zone[0]
			 , &pal->emphasis_zone[1]);
	 break;

       case PARAM_EMPH_CLR:
	 g_string_assign (pal->emphasis_color, aa);
	 break;
      };
   }
   name = gtk_editable_get_text( GTK_EDITABLE (pd->data_widget[PARAM_NAME]));
   sii_set_entries_from_palette (frame_num, name, NULL);
}

/* c---------------------------------------------------------------------- */

void sii_param_reset_changes (guint frame_num)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   guint jj;

   for (jj=0; jj < PARAM_MAX_WIDGETS; jj++) {
      pd->change_flag[jj] = FALSE;
   }
}

/* c---------------------------------------------------------------------- */

void sii_param_set_plot_field (int frame_num, char *fname)
{
  /* for toggling */

  ParamData *pd = frame_configs[frame_num]->param_data;
  SiiLinkedList *sll = pd->fields_list;
  ParamFieldInfo *pfi;
  char str[16];

  strcpy (str, fname);
  g_strstrip (str);

  if (!sll->data) {		/* first time */
    sll->data = (gpointer)g_malloc0 (sizeof (*pfi));
    pfi = (ParamFieldInfo *)sll->data;
    strcpy (pfi->name, str);
    return;
  }
  pfi = (ParamFieldInfo *)sll->data;
  if (strcmp (pfi->name, str) == 0) /* replotting the same field */
    { return;}

  pd->toggle_field = pd->fields_list;
  pd->fields_list = sll = sll->next;

  if (!sll->data) {
    sll->data = (gpointer)g_malloc0 (sizeof (*pfi));
  }
  pfi = (ParamFieldInfo *)sll->data;
  strcpy (pfi->name, str);
  return;
}

/* c---------------------------------------------------------------------- */

void sii_param_toggle_field (guint frame_num)
{
  ParamData *pd = frame_configs[frame_num]->param_data;
  SiiLinkedList *sll = pd->toggle_field;
  ParamFieldInfo *pfi, *pfi1, *pfi2;

  if (!sll)
    { return; }

  pfi = (ParamFieldInfo *)sll->data;
  gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[PARAM_NAME]), pfi->name);
  if (sii_set_param_info (frame_num))
    { sii_plot_data (frame_num, REPLOT_THIS_FRAME); }
  sii_redisplay_list (frame_num, PARAM_PALETTES_TEXT);

  pd->toggle_field = pd->fields_list->previous;
  pfi1 = (ParamFieldInfo *)pd->fields_list->data;
  pfi2 = (ParamFieldInfo *)pd->toggle_field->data;
}

/* c---------------------------------------------------------------------- */

void sii_param_widget( guint frame_num )
{
  GtkWidget *label;
  GtkWidget *widget;

  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hbox0;
  GtkWidget *hbox2;
  GtkWidget *hbbox;

  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *vbox3;

  GtkWidget *text;
  GtkWidget *table;
  GtkWidget *table2;

  GtkWidget *entry;
  GtkWidget *entrybar;
  GtkWidget *entrybar2;
  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;

  GtkWidget *button;

  guint xpadding = 0;
  guint ypadding = 0;
  guint padding = 0, row = 0;

  char *aa;
  gchar str[256], str2[256], *bb, *ee;
  const gchar *cc;
  gint length, width, max_width = 0, jj, nn;
  guint widget_id;
  gfloat upper;
  gint x, y;
  SiiPalette *pal;

				/* c...code starts here */

  pdata = (ParamData *)frame_configs[frame_num]->param_data;
  pal = pdata->pal;

  edit_font = med_pro_font;
  edit_fore = (GdkRGBA *)g_hash_table_lookup ( colors_hash, (gpointer)"black");
  edit_back = (GdkRGBA *)g_hash_table_lookup ( colors_hash, (gpointer)"white");


  window = gtk_window_new ();
  sii_set_widget_ptr ( frame_num, FRAME_PARAMETERS, window );
  

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);
  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num * TASK_MODULO + FRAME_PARAMETERS));

  /* --- Title and border --- */
  bb = g_strdup_printf ("Frame %d  Parameter and Colors Widget", frame_num+1 );
  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );
  /* GTK4: use margins instead of border_width */

  /* --- Create a new vertical box for storing widgets --- */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW (window), vbox);
				/*
  sii_param_menubar (window, &menubar, frame_num);
				 */
  sii_param_menubar2 (window, &menubar, frame_num);
  gtk_box_append (GTK_BOX (vbox), menubar);

  hbox0 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append (GTK_BOX (vbox), hbox0);

  widget = gtk_drawing_area_new ();
  gtk_box_append (GTK_BOX (hbox0), widget);
  pdata->data_widget[PARAM_COLOR_TEST] = widget;

  g_string_truncate (pdata->orig_txt[PARAM_LABEL], 0);
  g_string_append_printf (pdata->orig_txt[PARAM_LABEL], "   %d Colors   "
			, pal->num_colors);
  label = gtk_label_new ( pdata->orig_txt[PARAM_LABEL]->str );
  pdata->data_widget[PARAM_LABEL] = label;
  gtk_box_append (GTK_BOX (hbox0), label);
  gtk_widget_set_size_request(widget, 96, 0);


  hbbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append(GTK_BOX(hbox0), hbbox);

  button = gtk_button_new_with_label ("Replot");
  gtk_box_append (GTK_BOX (hbbox), button);
  nn = frame_num*TASK_MODULO + PARAM_REPLOT_THIS;
  g_signal_connect (G_OBJECT (button)
		      ,"clicked"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn
		      );

  button = gtk_button_new_with_label ("OK");
  gtk_box_append (GTK_BOX (hbbox), button);
  nn = frame_num*TASK_MODULO + PARAM_OK;
  g_signal_connect (G_OBJECT (button)
		      ,"clicked"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn
		      );

  button = gtk_button_new_with_label ("Cancel");
  gtk_box_append (GTK_BOX (hbbox), button);
  nn = frame_num*TASK_MODULO + PARAM_CANCEL;
  g_signal_connect (G_OBJECT (button)
		      ,"clicked"
		      , G_CALLBACK (sii_param_menu_cb)
		      , (gpointer)nn
		      );
  g_object_set_data (G_OBJECT (button)
		       , "widget_id", (gpointer)PARAM_CANCEL);
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append (GTK_BOX (vbox), hbox);
  /* GTK4: use margins instead of border_width */

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (hbox), vbox2);

  label = gtk_label_new ( "Parameters" );
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER );
  gtk_box_append (GTK_BOX (vbox2), label);

  table = gtk_grid_new ();
  gtk_box_append (GTK_BOX (vbox2), table);

  aa = pdata->param_names_list->str;

  widget_id = PARAM_NAMES_TEXT;
  text = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);
  pdata->data_widget[widget_id] = text;

  {
    GtkWidget *sw = gtk_scrolled_window_new ();
    gtk_widget_set_size_request (sw, 0, 288);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (sw), text);
    gtk_box_append (GTK_BOX (vbox2), sw);
  }

  /* GTK4: click handling via GtkGestureClick */
  {
    GtkGesture *gesture = gtk_gesture_click_new ();
    g_object_set_data (G_OBJECT (text), "click_data", (gpointer)aa);
    g_signal_connect (gesture, "pressed", G_CALLBACK (param_text_event_cb), text);
    gtk_widget_add_controller (text, GTK_EVENT_CONTROLLER (gesture));
  }

  g_object_set_data (G_OBJECT (text),
		       "frame_num",
		       (gpointer)frame_num);

  g_object_set_data (G_OBJECT (text),
		       "widget_id",
		       (gpointer)widget_id);

  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
    gtk_text_buffer_set_text (buffer, aa, -1);
  }


  table = gtk_grid_new ();
  gtk_box_append (GTK_BOX (hbox), table);
  row = -1;

  row++;
  label = gtk_label_new ( " Parameter Name " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_NAME]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_NAME, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Min & Max " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_MINMAX]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_MINMAX, frame_num, ENTRY_TWO_FLOATS );

  row++;
  label = gtk_label_new ( " Center & Increment " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_CTRINC]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_CTRINC, frame_num, ENTRY_TWO_FLOATS );

  row++;
  label = gtk_label_new ( " Color Palette " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_PALETTE]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_PALETTE, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Grid Color " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_GRID_CLR]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_GRID_CLR, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Color Table Name " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_CLR_TBL_NAME]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_CLR_TBL_NAME, frame_num, ENTRY_TXT_ONLY );


  row++;
  label = gtk_label_new ( " " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);

  row++;
  label = gtk_label_new ( " Boundary Color " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_BND_CLR]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_BND_CLR, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Exceeded Color " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_XCEED_CLR]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_XCEED_CLR, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Missing Data Color " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_MSSNG_CLR]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_MSSNG_CLR, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Annotation Color " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_ANNOT_CLR]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_ANNOT_CLR, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Background Color " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_BACKG_CLR]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_BACKG_CLR, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Emphasis Color " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_EMPH_CLR]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_EMPH_CLR, frame_num, ENTRY_TXT_ONLY );

  row++;
  label = gtk_label_new ( " Emphasis Min/Max " );
  gtk_grid_attach (GTK_GRID (table), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  aa = pdata->orig_txt[PARAM_EMPH_MINMAX]->str;
  gtk_editable_set_text (GTK_EDITABLE (entry), aa);
  gtk_grid_attach (GTK_GRID (table), entry, 1, row, 1, 1);
  sii_param_entry_info( entry, PARAM_EMPH_MINMAX, frame_num, ENTRY_TWO_FLOATS );


  {
    GtkWidget *child = gtk_widget_get_first_child (table);
    while (child) {
      sii_set_widget_frame_num (child, (gpointer)frame_num);
      child = gtk_widget_get_next_sibling (child);
    }
  }

  sii_update_param_widget (frame_num);

 /* --- Make everything visible --- */
  gtk_widget_set_visible (window, TRUE);

  for (jj=0; jj < PARAM_MAX_WIDGETS; jj++ ) {

     if (pdata->entry_flag[jj]) {
	nn = frame_num * TASK_MODULO + jj;
	g_signal_connect (G_OBJECT (pdata->data_widget[jj])
			    ,"activate"
			    , G_CALLBACK (sii_param_menu_cb)
			    , (gpointer)nn );
	/* paste_clipboard signal removed - not available in GTK4 */
     }
  }
}

/* c---------------------------------------------------------------------- */

void sii_redisplay_list (guint frame_num, guint widget_id)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   GtkWidget *text = pd->data_widget[widget_id];
   char *aa;

   if (!text)
     { return; }

   switch (widget_id) {

    case PARAM_PALETTES_TEXT:
      aa = param_palette_names;
      break;

    case PARAM_NAMES_TEXT:
      aa = param_names;
      break;

    case PARAM_CLR_TBL_TEXT:
      aa = param_color_table_names;
      break;

    default:
      return;
   };

   {
     GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
     gtk_text_buffer_set_text (buffer, aa, -1);
   }
   /* scroll to top */
   {
     GtkAdjustment *adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (text));
     if (adj) gtk_adjustment_set_value (adj, 0);
   }
}

/* c---------------------------------------------------------------------- */

void sii_reset_image (guint frame_num)
{
  SiiFrameConfig *sfc = frame_configs[frame_num];
  WW_PTR wwptr = solo_return_wwptr(frame_num);
  guchar *img;
  int jj;

  jj = wwptr->background_color_num;

  if (!(img = (guchar *)sfc->image->data))
    { return; }

  memset (img, jj, sfc->width * sfc->height);
}

/* c---------------------------------------------------------------------- */

void sii_set_entries_from_palette (guint frame_num, const gchar *name, 
        const gchar *p_name)
{
   SiiPalette *pal = NULL;
   SiiLinkedList *item = NULL;
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   gchar *aa, p_name_new[16];
   guint jj, wid;

   if( name && strlen (name)) {
      pal = sii_set_palette (name);
   }
   else if( p_name && strlen (p_name)) {
      item = sii_palette_seek (p_name);
      pal = (SiiPalette *)item->data;
   }

   if (!pal)
     { pal = pd->pal; }
   pd->pal = pal;

# ifdef notyet

   aa = pal->->str;
   wid = ;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);
# endif

   aa = pal->grid_color->str;
   wid = PARAM_GRID_CLR;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->color_table_name->str;
   wid = PARAM_CLR_TBL_NAME;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->p_name->str;
   wid = PARAM_PALETTE;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->boundary_color->str;
   wid = PARAM_BND_CLR;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->exceeded_color->str;
   wid = PARAM_XCEED_CLR;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->missing_data_color->str;
   wid = PARAM_MSSNG_CLR;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->annotation_color->str;
   wid = PARAM_ANNOT_CLR;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->background_color->str;
   wid = PARAM_BACKG_CLR;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);

   aa = pal->emphasis_color->str;
   wid = PARAM_EMPH_CLR;
   g_string_assign (pd->orig_txt[wid], aa);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), aa);


   wid = PARAM_CTRINC;
   g_memmove (pd->orig_values[wid], pal->ctrinc, sizeof (*pal->ctrinc));
   sii_set_string_from_vals
     (pd->orig_txt[wid], 2, pal->ctrinc[0], pal->ctrinc[1]
      , pd->precision[wid]);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid]), pd->orig_txt[wid]->str);

   wid = PARAM_MINMAX;
   sii_min_max_from_ctr_inc (pal->num_colors, &pal->ctrinc[0], &pal->ctrinc[1],
			     &pal->minmax[0], &pal->minmax[1]);
   g_memmove (pd->orig_values[wid], pal->minmax, sizeof(*pal->minmax));
   aa = sii_set_string_from_vals
     (pd->orig_txt[wid], 2, pal->minmax[0], pal->minmax[1]
      , pd->precision[wid]);

   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid])
		       , pd->orig_txt[wid]->str);

   wid = PARAM_EMPH_MINMAX;
   g_memmove (pd->orig_values[wid], pal->emphasis_zone
	      , sizeof (*pal->emphasis_zone));
   sii_set_string_from_vals
     (pd->orig_txt[wid], 2, pal->emphasis_zone[0], pal->emphasis_zone[1]
      , pd->precision[wid]);
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid])
		       , pd->orig_txt[wid]->str);
}

/* c---------------------------------------------------------------------- */

SiiPalette *sii_set_palette (const gchar *name)
{
   int nn, nt;
   SiiPalette *pal = NULL;
   SiiLinkedList *item;
   
   item = sii_palette_for_param (name);

   if (!item) {
      item = (SiiLinkedList *)g_malloc0 (sizeof (SiiLinkedList));
      pal = sii_malloc_palette();
      pal->p_name = g_string_new ("");
      item->data = (gpointer)pal;
      g_string_append_printf (pal->p_name, "p_%s", name);
      pal->usual_parms = g_string_new ("");
      g_string_append_printf (pal->usual_parms, "%s,", name);
   }
   else {
      sii_ll_remove (&palette_stack, item);
      pal = (SiiPalette *)item->data;
      sii_palette_prepend_usual_param (pal, name);
   }

   sii_ll_push (&palette_stack, item);
   sii_new_palettes_list ();
   return pal;
}

/* c---------------------------------------------------------------------- */

gboolean
sii_set_param_info (guint frame_num)
{
  GtkWidget *widget;
  LinksInfo *li;
  gfloat ff, gg;
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  struct parameter_widget_info pwi;
  char str[256], *sptrs[32];
  const gchar *aa, *bb, *cc;
  gint nt, nn, jj;
  WW_PTR wwptr = solo_return_wwptr(frame_num);

  memset (&pwi, 0, sizeof (pwi));

  pwi.frame_num = frame_num;
  pwi.changed = (its_changed) ? YES : NO;

  li = frame_configs[frame_num]->link_set[LI_PARAM];
  for (jj=0; jj < maxFrames; jj++) {
    pwi.linked_windows[jj] = (li->link_set[jj]) ? 1 : 0;
  }
  wwptr->color_bar_location = 0;
  if (pd->cb_loc == PARAM_CB_LEFT)
    {  wwptr->color_bar_location = -1; }
  else if (pd->cb_loc == PARAM_CB_RIGHT)
    {  wwptr->color_bar_location = 1; }
  
  wwptr->color_bar_symbols = (pd->toggle[PARAM_CB_SYMBOLS]) ? YES : NO;

  widget = sii_get_widget_ptr (frame_num, FRAME_PARAMETERS);

  if (!widget) {
     nn = solo_set_parameter_info (&pwi);
     return (nn) ? FALSE : TRUE;
  }

  sii_param_process_changes (frame_num);

  aa = gtk_editable_get_text (GTK_EDITABLE (pd->data_widget[PARAM_PALETTE]));
  strcpy (pwi.palette_name, aa);

  aa = gtk_editable_get_text (GTK_EDITABLE (pd->data_widget[PARAM_NAME]));
  strcpy (pwi.parameter_name, aa);

  aa = gtk_editable_get_text (GTK_EDITABLE (pd->data_widget[PARAM_CTRINC]));
  sii_str_values (aa, 2, &pd->pal->ctrinc[0], &pd->pal->ctrinc[1]);
  sii_min_max_from_ctr_inc (pd->pal->num_colors
			    , &pd->pal->ctrinc[0], &pd->pal->ctrinc[1]
			    , &pd->pal->minmax[0], &pd->pal->minmax[1]);

  aa = gtk_editable_get_text (GTK_EDITABLE (pd->data_widget[PARAM_EMPH_MINMAX]));
  sii_str_values (aa, 2, &ff, &gg);
  pwi.emphasis_zone_lower = ff;
  pwi.emphasis_zone_upper = gg;

# ifdef notyet
  li = frame_configs[frame_num]->link_set[LI_PARAM];
  for (jj=0; jj < maxFrames; jj++) {
    pwi.linked_windows[jj] = (li->link_set[jj]) ? 1 : 0;
  }
  wwptr->color_bar_location = 0;
  if (pd->cb_loc == PARAM_CB_LEFT)
    {  wwptr->color_bar_location = -1; }
  else if (pd->cb_loc == PARAM_CB_RIGHT)
    {  wwptr->color_bar_location = 1; }
  
  wwptr->color_bar_symbols = (pd->toggle[PARAM_CB_SYMBOLS]) ? YES : NO;
# endif
  nn = solo_set_parameter_info (&pwi);

  sii_update_param_widget (frame_num);
  its_changed = FALSE;

  return (nn) ? FALSE : TRUE;
  
}

/* c---------------------------------------------------------------------- */

gchar *sii_set_param_names_list
(guint frame_num, GString *gs_list, guint count)
{
   GString *gs_str;
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;

   if (!pd) {
      pd = (ParamData *)g_malloc0 (sizeof(ParamData));
      frame_configs[frame_num]->param_data = (gpointer)pd;
   }

   if (!pd->param_names_list) {
      pd->param_names_list = g_string_new ("");
   }
   gs_str = pd->param_names_list;
   g_string_truncate (gs_str, 0);

   if (gs_list)
     { g_string_append (gs_str, gs_list->str); }

   return gs_str->str;
}

/* c------------------------------------------------------------------------ */

void
solo_get_palette(char *name, int frame_num)
{
  SiiPalette *pal;
  WW_PTR wwptr;
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  SiiLinkedList *item;
  gchar p_name[16];

  pal = pd->pal;
  wwptr = solo_return_wwptr(frame_num);

  item = sii_palette_for_param (name);

  if (item) {
    strcpy (p_name, "p_");
    strcat (p_name, name);
    item = sii_new_palette_for_param (p_name, name);
  }

}

/* c------------------------------------------------------------------------ */

int solo_hardware_color_table(gint frame_num)
{
  int jj, kk, mm, nc=0, nn, ndx = 0, start, end, cmax;
  struct solo_perusal_info *spi, *solo_return_winfo_ptr();
  WW_PTR wwptr;
  struct solo_list_mgmt *lm;
  SiiPalette *pal;
  gchar mess[256], *aa, *bb, *cc, line[256], *table, *name, *p_name;
  GdkRGBA *gcolor, *gclist;
  gboolean ok = TRUE, ok2;
  float red, green, blue, rnum, rden, gnum, gden, bnum, bden;
  
  /* GdkColormap removed in GTK4 */
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  GtkWidget *widget;

# ifdef obsolete
  if (!frame_configs[frame_num]->color_map) {
    rgb_visual = gdk_rgb_get_visual ();
    frame_configs[frame_num]->color_map = gdk_colormap_new (rgb_visual, FALSE);
  }
# endif

  widget = sii_get_widget_ptr (frame_num, FRAME_PARAMETERS);
 
  wwptr = solo_return_wwptr(frame_num);

  if (!sii_get_widget_ptr (frame_num, FRAME_PARAMETERS)) {
     /* the palette might be wrong for this parameter */
     pd->pal = sii_set_palette (wwptr->parameter->parameter_name);
  }
  else if (strcmp (wwptr->parameter->parameter_name
		   , pd->orig_txt[PARAM_NAME]->str) == 0) {
     pd->pal = sii_set_palette (wwptr->parameter->parameter_name);
  }
  pal = pd->pal;
  mess[0] = '\0';
  lm = wwptr->list_colors;
  wwptr->num_colors_in_table = lm->num_entries = 0;
  spi = solo_return_winfo_ptr();
  
  table = (gchar *)g_tree_lookup( ascii_color_tables
				  , (gpointer)pal->color_table_name->str );
  if (!table) {
    sprintf (mess, "Color table %s cannot be found"
	     , pal->color_table_name->str);
    sii_message (mess);
    return 0;
  }
  cmax = strlen (table);
  gclist = (GdkRGBA *)g_malloc0 (256 * sizeof (GdkRGBA));
  
  for(; ndx < cmax;) {
    
    ok = sii_nab_region_from_text (table, ndx, &start, &end);
    
    if (!ok || end <= start)
      { break; }
    /*
     * advance to first non-whiteout character
     */
    strncpy (line, table+start, end-start);
    line[end-start] = '\0';
    ndx = end +1;
    bb = table+ndx;
    g_strstrip (line);
    
    if(!strlen(line) || strstr(line, "colortable") ||
       strstr(line, "endtable") || strstr(line, "!")) {
      continue;
    }
    
    if(aa=strstr(line, "xcolor")) { /* X color name */
      aa += strlen ("xcolor");
      g_strstrip (aa);
      solo_add_list_entry(lm, aa, strlen(aa));
    }
    else if(strstr(line, "/")) { /* funkier zeb color files */
      sscanf(line, "%f/%f %f/%f %f/%f"
	     , &rnum, &rden
	     , &gnum, &gden
	     , &bnum, &bden);
      kk = nc++;
      gcolor = gclist +kk;
      gcolor->red =   rnum/rden;
      gcolor->green = gnum/gden;
      gcolor->blue =  bnum/bden;
      gcolor->alpha = 1.0;

      if (!ok2)
	{ ok = NO; }
    }
    else {			/* assume rgb vals between 0 and 1 */
      sscanf(line, "%f %f %f", &red, &green, &blue);
      kk = nc++;
      gcolor = gclist +kk;
      gcolor->red =   red;
      gcolor->green = green;
      gcolor->blue =  blue;
      gcolor->alpha = 1.0;
      if (!ok2)
	{ ok = NO; }
    }		   
  }

  if(!nc) {
    lm = wwptr->list_colors;
    nc = lm->num_entries;
    for(kk=0; kk < nc; kk++) {

      aa = *(lm->list +kk);
      gcolor = 
	(GdkRGBA *)g_tree_lookup (colors_tree, (gpointer)aa);
      *(gclist +kk) = *gcolor;

      if (!gcolor)
	{ ok = NO; }
      /* pixel index no longer needed in GTK4 (GdkRGBA uses float RGBA) */

      if (!ok2)
	{ ok = NO; }
    }
  }

  if (!ok) {
    sprintf (mess, "Unable to allocate colors for %s"
	     , pal->color_table_name->str);
    sii_message (mess);
    return ok;
  }
  wwptr->num_colors_in_table = 
    pal->num_colors = nc;

  if (!pal->data_color_table[0]) {

    pal->data_color_table[0] = (GdkRGBA *)g_new
      (GdkRGBA, MAX_COLOR_TABLE_SIZE);
    for (kk=1; kk < MAX_COLOR_TABLE_SIZE; kk++)
      { pal->data_color_table[kk] = pal->data_color_table[0] +kk; }

    pal->feature_color[0] = g_new (GdkRGBA, MAX_FEATURE_COLORS);
    for (kk=0; kk < MAX_FEATURE_COLORS; kk++)
      { pal->feature_color[kk] = pal->feature_color[0] +kk; }
  }

  for (kk=0; kk < nc; kk++) {
    gcolor = gclist +kk;
    *pal->data_color_table[kk] = *gcolor;

    pal->color_table_rgbs[kk][0] = (guchar)(gcolor->red * 255.0 + 0.5);
    pal->color_table_rgbs[kk][1] = (guchar)(gcolor->green * 255.0 + 0.5);
    pal->color_table_rgbs[kk][2] = (guchar)(gcolor->blue * 255.0 + 0.5);

    (wwptr->xcolors+kk)->pixel = kk;

# ifdef notyet
# endif
  }

  g_free (gclist);

  if (!solo_palette_color_numbers(frame_num))
    { ok = NO; }

  if (ok)
    { pd->orig_pal = pal; }

  return ok;
				/*
Gdk-ERROR **: BadAccess (attempt to access private resource denied)
  serial 2508 error_code 10 request_code 89 minor_code 0
Gdk-ERROR **: BadAccess (attempt to access private resource denied)
  serial 2509 error_code 10 request_code 89 minor_code 0
				 */
}

/* c------------------------------------------------------------------------ */

int solo_palette_color_numbers(int frame_num)
{
  /* this routine assumes that all the color names have been checked
   */
  gint kk, ok=YES, nc, mm, nn, ct_ndx;
  gboolean ok2;
  WW_PTR wwptr, solo_return_wwptr();
  SiiPalette *pal;
  gchar mess[256];
  GdkRGBA *gcolor;
  /* GdkColormap removed in GTK4 */
  ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
  
  wwptr = solo_return_wwptr(frame_num);
  pal = pd->pal;
  nc = pd->pal->num_colors;
  /* colormap not needed in GTK4 */
  mess[0] = '\0';

  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->exceeded_color->str);
  if (!gcolor) {
    strcat (mess, pal->exceeded_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_EXCEEDED] = *gcolor;
# ifdef notyet
# endif
  }

  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->missing_data_color->str);
  if (!gcolor) {
    strcat (mess, pal->missing_data_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_MISSING] = *gcolor;
# ifdef notyet
# endif
  }
  
  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->background_color->str);
  if (!gcolor) {
    strcat (mess, pal->background_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_BACKGROUND] = *gcolor;
# ifdef notyet
# endif
  }

  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->emphasis_color->str);
  if (!gcolor) {
    strcat (mess, pal->emphasis_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_EMPHASIS] = *gcolor;
# ifdef notyet
# endif
  }

  /*
   * the following two colors are drawn directly
   * as opposed to being part of the rgb image
   */

  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->boundary_color->str);
  if (!gcolor) {
    strcat (mess, pal->boundary_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_BND] = *gcolor;
    wwptr->bnd_color_num = 0 /* GTK4: no pixel field */;
  }

  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->annotation_color->str);
  if (!gcolor) {
    strcat (mess, pal->annotation_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_ANNOTATION] = *gcolor;
    wwptr->annotation_color_num = 0 /* GTK4: no pixel field */;
  }

  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->boundary_color->str);
  if (!gcolor) {
    strcat (mess, pal->boundary_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_BND] = *gcolor;
  }

  gcolor = (GdkRGBA *)g_tree_lookup
    (colors_tree, (gpointer)pal->grid_color->str);
  if (!gcolor) {
    strcat (mess, pal->grid_color->str); strcat (mess, " ");
    ok = NO;
  }
  else {
    *pal->feature_color[FEATURE_RNG_AZ_GRID] = *gcolor;
    *pal->feature_color[FEATURE_TIC_MARKS] = *gcolor;
    wwptr->grid_color_num = 0 /* GTK4: no pixel field */;
  }

# ifdef notyet
  wwptr->bnd_color_num = 0 /* GTK4: no pixel field */;
  wwptr->bnd_alert_num = 0 /* GTK4: no pixel field */;
  wwptr->bnd_last_num = 0 /* GTK4: no pixel field */;
# endif


  ct_ndx = pal->num_colors -1;

  for (kk=0; kk < MAX_FEATURE_COLORS; kk++) {
    gcolor = pal->feature_color[kk];
    if (gcolor && (gcolor->red > 0 || gcolor->green > 0 || gcolor->blue > 0)) {
      /* color is set - convert from GdkRGBA (0.0-1.0) to 8-bit RGB */
      pal->feature_rgbs[kk][0] = (guchar)(gcolor->red * 255.0);
      pal->feature_rgbs[kk][1] = (guchar)(gcolor->green * 255.0);
      pal->feature_rgbs[kk][2] = (guchar)(gcolor->blue * 255.0);

      switch (kk) {
      case FEATURE_BACKGROUND:
	wwptr->background_color_num = ++ct_ndx;
	memcpy (pal->color_table_rgbs[ct_ndx], pal->feature_rgbs[kk]
		, sizeof (pal->feature_rgbs[kk]));
	break;
      case FEATURE_EXCEEDED:
	wwptr->exceeded_color_num = ++ct_ndx;
	memcpy (pal->color_table_rgbs[ct_ndx], pal->feature_rgbs[kk]
		, sizeof (pal->feature_rgbs[kk]));
	break;
      case FEATURE_MISSING:
	wwptr->missing_data_color_num = ++ct_ndx;
	memcpy (pal->color_table_rgbs[ct_ndx], pal->feature_rgbs[kk]
		, sizeof (pal->feature_rgbs[kk]));
	break;
      case FEATURE_EMPHASIS:
	wwptr->emphasis_color_num = ++ct_ndx;
	memcpy (pal->color_table_rgbs[ct_ndx], pal->feature_rgbs[kk]
		, sizeof (pal->feature_rgbs[kk]));
	break;
      };
    }
  }

  if (!ok) {
    sprintf (mess+strlen(mess), "not useful color name(s) in frame %d"
	     , frame_num);
    sii_message (mess);
  }
  return ok;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_solo_palette_stack ()
{
    struct solo_palette s_pal, *solo_malloc_old_palette();
    SiiPalette *pal;
    SiiLinkedList *item = palette_stack, *item2;
    SiiLinkedList *sps = solo_palette_stack;
    int mm = 0, nn = 0;

    /* count sii palettes */

    if (!solo_palette_stack) {
      sps = solo_palette_stack = sii_ll_malloc_item ();
      sps->data = (gpointer)solo_malloc_old_palette();
    }
    sps = solo_palette_stack;
    item = palette_stack;

    for (; item; item = item->next) {
      if (item == palette_stack) {
	sii_param_dup_pal (item->data, sps->data);
	continue;
      }
      if (!sps->next) {
	sps->next = sii_ll_malloc_item ();
	sps->next->data = (gpointer)solo_malloc_old_palette();
	sps->next->previous = sps;
      }
      sps = sps->next;
      sii_param_dup_pal (item->data, sps->data);
    }
    return solo_palette_stack;
}

/* c---------------------------------------------------------------------- */

void sii_update_param_widget (guint frame_num)
{
   ParamData *pd = (ParamData *)frame_configs[frame_num]->param_data;
   GtkWidget *text;
   LinksInfo *li;
   GtkWidget *entry, *check_item;
   guint wid, jj;
   char *solo_list_entry (struct solo_list_mgmt *slm, int jj);
   void solo_gen_parameter_list (guint frme);
   gchar *p_name, *name;
   WW_PTR wwptr = solo_return_wwptr(frame_num);
   struct solo_list_mgmt *slm;
   struct solo_perusal_info *spi = solo_return_winfo_ptr();
   GString *gs;
   GtkAdjustment *adj;
   gint value;


   pd->toggle[PARAM_CB_SYMBOLS] = (wwptr->color_bar_symbols) ? TRUE : FALSE;

   if (wwptr->color_bar_location) {
      pd->cb_loc = (wwptr->color_bar_location > 0) ?
	PARAM_CB_RIGHT : PARAM_CB_LEFT;
   }
   else
     { pd->cb_loc = PARAM_CB_BOTTOM; }

   if (!sii_get_widget_ptr (frame_num, FRAME_PARAMETERS))
     { return; }


   check_item = pd->data_widget[pd->cb_loc];
   gtk_check_button_set_active (GTK_CHECK_BUTTON (check_item), TRUE );

   check_item = pd->data_widget[PARAM_CB_SYMBOLS];
   gtk_check_button_set_active(GTK_CHECK_BUTTON(check_item), pd->toggle[PARAM_CB_SYMBOLS]);

   text = pd->data_widget[PARAM_NAMES_TEXT];
   name = wwptr->parameter->parameter_name;
   p_name = wwptr->parameter->palette_name;
   g_string_assign (pd->orig_txt[PARAM_NAME], name);
   g_string_assign (pd->orig_txt[PARAM_PALETTE], p_name);
   g_string_printf (pd->orig_txt[PARAM_LABEL], "   %d Colors   "
		     , pd->pal->num_colors);
   gtk_label_set_text(GTK_LABEL (pd->data_widget[PARAM_LABEL])
		  , pd->orig_txt[PARAM_LABEL]->str);

   

   wid = PARAM_NAME;
   gtk_editable_set_text (GTK_EDITABLE (pd->data_widget[wid])
		       , pd->orig_txt[wid]->str);
   sii_set_entries_from_palette (frame_num, name, p_name);

   li = frame_configs[frame_num]->link_set[LI_PARAM];
   for (jj=0; jj < maxFrames; jj++) {
     li->link_set[jj] = (wwptr->parameter->linked_windows[jj]) ? TRUE : FALSE;
   }

   solo_gen_parameter_list (frame_num);
   slm = spi->list_parameters;
   solo_sort_slm_entries (slm);
   gs = pd->param_names_list;
   g_string_truncate (gs, 0);

   for (jj=0; jj < slm->num_entries; jj++) {
     g_string_append (gs, solo_list_entry (slm, jj));
     g_string_append (gs, "\n");
   }
   adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (text));
   value = adj ? gtk_adjustment_get_value (adj) : 0;

   {
     GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
     gtk_text_buffer_set_text (buffer, pd->param_names_list->str, -1);
   }

   if (adj) gtk_adjustment_set_value (adj, value);
   sii_redisplay_list (frame_num, PARAM_PALETTES_TEXT);
}
/* c---------------------------------------------------------------------- */
/* c...mark */
/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

/* GTK4: GtkItemFactory removed. Menu is now built dynamically
 * in sii_param_menu_widget() using sii_submenu/sii_submenu_item helpers.
 * The menu structure is:
 *   File: Close, List Palettes, List Color Tables, Import Color Table, List Colors
 *   Options: rePlot Links, rePlot All, Electric Params (toggle)
 *   ParamLinks: Set Links
 *   Help: Overview, With ?
 */

/* c---------------------------------------------------------------------- */



