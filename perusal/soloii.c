/* 	$Id$	 */

# include "soloii.h"
# include "sii_utils.h"
# include "help_sii.h"
# include <stdlib.h>
# include <date_stamp.h>

int solo_busy(void);  /* sp_basics.c */

GtkWidget *main_window = NULL;
GtkWidget *main_vbox = NULL;
GtkWidget *main_event_box;
GtkWidget *menubar = NULL;

GtkWidget *main_table = NULL;

GtkAllocation *sii_frame_alloc[maxFrames];

GHashTable *colors_hash;

GSList *gen_gslist = NULL;	/* generic GSList */

GSList *color_names = NULL;
GSList *Xcolor_names = NULL;
GSList *color_table_names = NULL;
GSList *color_palette_names = NULL;

GTree *colors_tree = NULL;
GTree *ascii_color_tables = NULL;
GTree *color_tables = NULL;
GTree *color_palettes_tree = NULL;


guint sii_config_count = 0;
gfloat sii_config_w_zoom = 1.0;
gfloat sii_config_h_zoom = 1.0;
sii_config_cell **config_cells = NULL;

guint sii_frame_count = 0;
SiiFrameConfig *frame_configs[2*maxFrames +1];
GdkRGBA *frame_border_colors[maxFrames];

guint sii_table_widget_width = DEFAULT_WIDTH;
guint sii_table_widget_height = DEFAULT_WIDTH;

guint previous_F_keyed_frame = 0;

gint cursor_frame = 0;
/* 0 => cursor not in any frame
 * otherwise frame_num = cursor_frame-1;
 */

guint click_count = 0;
guint clicks_in_que = 0;

const guint click_que_size = 123;
SiiLinkedList *sii_click_que = NULL;

guint sii_seq_num = 0;

SiiFont *small_fxd_font = NULL;
SiiFont *med_fxd_font = NULL;
SiiFont *big_fxd_font = NULL;

SiiFont *small_pro_font = NULL;
SiiFont *med_pro_font = NULL;
SiiFont *big_pro_font = NULL;

gchar *title_tmplt = "00 00/00/2000 00:00:00 HHHHHHHH 000.0 HHH HHHHHHHH";

guint screen_width_mm = 0;
guint screen_height_mm = 0;
guint screen_width_px = 0;
guint screen_height_px = 0;

gboolean time_series_mode = FALSE;

GString *gs_complaints = NULL;

gboolean sii_electric_centering = FALSE;

gboolean nexrad_mode_set = FALSE;

gboolean uniform_frame_shape = TRUE;

gboolean batch_mode = FALSE;

GString *gs_image_dir = NULL;

gfloat batch_threshold_value = -32768;

GString *batch_threshold_field = NULL;

gboolean sii_center_crosshairs = FALSE;

GString *gen_gs = NULL;

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

enum {
  SII_MAIN_ZERO,

  SII_MAIN_DIR,
  SII_MAIN_IMAGE_DIR,
  SII_MAIN_CONFIG_DIR,
  SII_MAIN_IGNORE_SWPIFS,
  SII_MAIN_CONFIG_FILE,
  SII_MAIN_CONFIG_COMMENT,
  SII_MAIN_SAVE_CONFIG,
  SII_MAIN_LIST_CONFIGS,
  SII_MAIN_CANCEL,
  SII_MAIN_CANCEL_IMAGE_GEN,
  SII_MAIN_CTR_TOGGLE,
  SII_MAIN_XHAIRS_TOGGLE,
  SII_MAIN_PNG,
  SII_MAIN_FILESEL_OK,
  SII_MAIN_SWPFI_FILESEL,
  SII_MAIN_CONFIG_FILESEL,
  SII_MAIN_COLORS_FILESEL,

  SII_MAIN_EVENT_ENTER,
  SII_MAIN_EVENT_LEAVE,

  SII_MAIN_HLP_ZERO,
  SII_MAIN_HLP_BASIC,
  SII_MAIN_HLP_FILE,
  SII_MAIN_HLP_ZOOM,
  SII_MAIN_HLP_CTR,
  SII_MAIN_HLP_CONFIG,
  SII_MAIN_HLP_SHORTCUTS,
  SII_MAIN_HLP_COMPARE,
  SII_MAIN_HLP_ABOUT,
  SII_MAIN_HLP_LAST_ENUM,

  SII_MAIN_LAST_ENUM,
};

static SiiFont *edit_font;
static GdkRGBA *edit_fore;
static GdkRGBA *edit_back;
static GtkWidget *main_config_text = NULL;
static GtkWidget *main_config_window = NULL;
static GtkWidget *main_image_gen_window = NULL;
static GString *gs_config_file_names = NULL;
static GString *gs_initial_dir_name = NULL;
static GString *gs_scratch = NULL;

GtkWidget * nexrad_check_item;
GtkWidget * anglr_fill_item;
static int ignore_swpfi_info = 0;

/* c---------------------------------------------------------------------- */

static void print_hello( GtkWidget *w, gpointer   data );
void show_solo_init();
void show_solo_image_gen();
void sii_initialization_widget();
void sii_image_gen_widget();
void show_all_click_data_widgets (gboolean really);
int sii_default_startup (const gchar *swpfi_dir);
void sii_init_frame_configs();
void sii_reset_config_cells();
void soloiv_build_main_menu(GtkWidget *window, GtkWidget *vbox);
void sii_set_default_colors ();
void custom_config_cb( GtkWidget *w, gpointer data );
void zoom_config_cb( GtkWidget *w, gpointer data );
void zoom_data_cb( GtkWidget *w, gpointer data );
void center_frame_cb( GtkWidget *w, gpointer   data );
void config_cb( GtkWidget *w, gpointer data );
void shape_cb( GtkWidget *w, gpointer data );
void test_widget( GtkWidget *w, gpointer data );
void image_cb( GtkWidget *w, gpointer data );
static void sii_menu_cb( gpointer data );

gboolean sii_frame_key_pressed_cb(GtkEventControllerKey *controller,
                                  guint keyval, guint keycode,
                                  GdkModifierType state, gpointer data);
const GString *sii_return_config_files (const char *dir);
gchar *sii_get_swpfi_dir (gchar *dir);
void sp_change_cursor (int normal);
void solo_save_window_info (const char *dir, const char *a_comment);
int solo_absorb_window_info (const char *dir, const char *fname, int ignore_swpfi);
gchar *sii_nab_line_from_text (const gchar *txt, guint position );
void sii_png_image_prep (char *dir);
gboolean sii_batch (gpointer argu);
/* c---------------------------------------------------------------------- */
/* c...events */
/* c---------------------------------------------------------------------- */

/* Obligatory basic callback */

static void print_hello( GtkWidget *w, gpointer   data )
{
  g_message ("Hello, Callback! data: %d\n", GPOINTER_TO_UINT (data) );
}
/* c---------------------------------------------------------------------- */

static void sii_menu_cb( gpointer data )
{
  guint id = GPOINTER_TO_UINT (data), mark;
  int nn;

  switch (id) {

  case SII_MAIN_CTR_TOGGLE:
    sii_electric_centering = !sii_electric_centering;
    break;

  case SII_MAIN_XHAIRS_TOGGLE:
    sii_center_crosshairs = !sii_center_crosshairs;
    break;

  case SII_MAIN_HLP_BASIC:
    nn = sizeof (hlp_sii_basics)/sizeof (char *);
    sii_glom_strings (hlp_sii_basics, nn, gen_gs);
    sii_message (gen_gs->str);
    break;

  case SII_MAIN_HLP_FILE:
    nn = sizeof (hlp_sii_file)/sizeof (char *);
    sii_glom_strings (hlp_sii_file, nn, gen_gs);
    sii_message (gen_gs->str);
    break;

  case SII_MAIN_HLP_ZOOM:
    nn = sizeof (hlp_sii_zoom)/sizeof (char *);
    sii_glom_strings (hlp_sii_zoom, nn, gen_gs);
    sii_message (gen_gs->str);
    break;

  case SII_MAIN_HLP_CTR:
    nn = sizeof (hlp_sii_ctr)/sizeof (char *);
    sii_glom_strings (hlp_sii_ctr, nn, gen_gs);
    sii_message (gen_gs->str);
    break;

  case SII_MAIN_HLP_CONFIG:
    nn = sizeof (hlp_sii_config)/sizeof (char *);
    sii_glom_strings (hlp_sii_config, nn, gen_gs);
    sii_message (gen_gs->str);
    break;

  case SII_MAIN_HLP_SHORTCUTS:
    nn = sizeof (hlp_sii_shortcuts)/sizeof (char *);
    sii_glom_strings (hlp_sii_shortcuts, nn, gen_gs);
    sii_message (gen_gs->str);
    break;

  case SII_MAIN_HLP_COMPARE:
    nn = sizeof (hlp_sii_comparisons)/sizeof (char *);
    sii_glom_strings (hlp_sii_comparisons, nn, gen_gs);
    sii_message (gen_gs->str);
    break;

  case SII_MAIN_HLP_ABOUT:
    nn = sizeof (hlp_sii_about)/sizeof (char *);
    sii_glom_strings (hlp_sii_about, nn, gen_gs);
    g_string_append (gen_gs, sii_date_stamp);
    sii_message (gen_gs->str);
    break;
  };

}
/* c---------------------------------------------------------------------- */

static void sii_main_enter_cb(GtkEventControllerMotion *controller,
                               double x, double y, gpointer data)
{
  guint cid;
  cid = (solo_busy()) ? 0 : 1;
  sp_change_cursor (cid);
}

/* c---------------------------------------------------------------------- */

void show_solo_init (GtkWidget *w, gpointer data)
{
   GtkWidget *text = main_config_text;
   GtkTextBuffer *buffer;

   if (!main_config_window) {
      sii_initialization_widget();
      return;
   }

   buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
   gtk_text_buffer_set_text(buffer, gs_config_file_names->str, -1);

   gtk_widget_set_visible (main_config_window, TRUE);
}

/* c---------------------------------------------------------------------- */

void show_solo_image_gen (GtkWidget *w, gpointer data)
{
   if (!main_image_gen_window) {
      sii_image_gen_widget();
      return;
   }
   gtk_widget_set_visible (main_image_gen_window, TRUE);
}

/* c---------------------------------------------------------------------- */

int sii_initialize_cb (GtkWidget *w, gpointer data)
{
   guint task = GPOINTER_TO_UINT (data);
   GtkWidget *entry;
   GtkWidget *button;
   gint nn;
   gboolean active;
   const gchar *aa, *bb;
   const gchar *cc;
   const GString *cgs;
   GtkTextBuffer *buffer;


   switch (task) {

    case SII_MAIN_IGNORE_SWPIFS:
      button = (GtkWidget *)g_object_get_data
	(G_OBJECT(main_config_window), "ignore");
      ignore_swpfi_info =
	gtk_check_button_get_active (GTK_CHECK_BUTTON (button));
      break;


    case SII_MAIN_CONFIG_COMMENT:
    case SII_MAIN_SAVE_CONFIG:
      entry = (GtkWidget *)g_object_get_data
	(G_OBJECT(main_config_window), "config_dir");
      aa = gtk_editable_get_text( GTK_EDITABLE (entry));
      entry = (GtkWidget *)g_object_get_data
	(G_OBJECT(main_config_window), "comment");
      bb = gtk_editable_get_text( GTK_EDITABLE (entry));
      solo_save_window_info (aa, bb);

    case SII_MAIN_LIST_CONFIGS:
    case SII_MAIN_CONFIG_DIR:
      entry = (GtkWidget *)g_object_get_data
	(G_OBJECT(main_config_window), "config_dir");
      aa = gtk_editable_get_text( GTK_EDITABLE (entry));
      cgs = sii_return_config_files (aa);

      buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(main_config_text));
      gtk_text_buffer_set_text(buffer, cgs->str, -1);
      g_string_assign (gs_config_file_names, cgs->str);
      break;

    case SII_MAIN_SWPFI_FILESEL:
    case SII_MAIN_CONFIG_FILESEL:
      /* TODO: Implement GtkFileDialog for GTK4 */
      break;

    case SII_MAIN_DIR:
      if (sii_frame_count > 0) {
	 sii_message (
"Use Sweepfiles Widget to change directories"
		      );
	 return 0;
      }

      entry = (GtkWidget *)g_object_get_data
	(G_OBJECT(main_config_window), "swpfi_dir");

      aa = gtk_editable_get_text( GTK_EDITABLE (entry));
      nn = sii_default_startup (aa);
      if (nn < 1 ) {
	g_string_printf (gs_complaints
		      , "\nNo sweepfiles in %s\n", aa);
	sii_message (gs_complaints->str);
	break;
      }
      config_cb (0, (gpointer)22 ); /* 2x2 */

    case SII_MAIN_CANCEL:
      if (main_config_window)
	{ gtk_widget_set_visible (main_config_window, FALSE); }
      break;

    case SII_MAIN_CANCEL_IMAGE_GEN:
      if (main_image_gen_window)
	{ gtk_widget_set_visible (main_image_gen_window, FALSE); }
      break;

    case SII_MAIN_IMAGE_DIR:
    case SII_MAIN_PNG:
      entry = (GtkWidget *)g_object_get_data
	(G_OBJECT(main_image_gen_window), "image_dir");

      aa = gtk_editable_get_text( GTK_EDITABLE (entry));
      if (!gs_image_dir)
	{ gs_image_dir = g_string_new (aa); }
      else
	{ g_string_assign (gs_image_dir, aa); }

      if (task == SII_MAIN_IMAGE_DIR)
	{ break; }

      sii_png_image_prep (gs_image_dir->str);
      break;
   };
   return 0;
}

/* c---------------------------------------------------------------------- */

void sii_main_text_click_cb (GtkGestureClick *gesture, int n_press,
                             double x, double y, gpointer data)
{
  GtkWidget *text = gtk_event_controller_get_widget(
    GTK_EVENT_CONTROLLER(gesture));
  GtkTextBuffer *buffer;
  GtkTextIter iter;
  gint nn, ii;
  gchar *line;
  const gchar *aa, *bb = gs_config_file_names->str;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
  /* Get character offset at click position */
  gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(text), &iter, (gint)x, (gint)y);
  nn = gtk_text_iter_get_offset(&iter);

  line = sii_nab_line_from_text (bb, nn);
  if (!line)
    { return; }
  g_strstrip (line);

  GtkWidget *entry = (GtkWidget *)g_object_get_data
    (G_OBJECT(main_config_window), "config_dir");
  aa = gtk_editable_get_text( GTK_EDITABLE (entry));

  ii = solo_absorb_window_info
    (aa, line, (sii_frame_count)? ignore_swpfi_info : 0);
}

/* c---------------------------------------------------------------------- */

void click_data_cb (GtkWidget *text, gpointer data )
{
  gboolean really = GPOINTER_TO_UINT (data) == 1;
  int mark;

  show_all_click_data_widgets (really);
}

/* c---------------------------------------------------------------------- */

static void soloiv_activate(GtkApplication *app, gpointer user_data)
{
  GdkDisplay *display;
  GdkMonitor *monitor;
  GdkRectangle geometry;
  gint jj, kk, ll, mm, nn;
  gchar *aa, *bb, *cc;
  gchar swi_dir[256], fname[256], str[256];
  GtkEventController *controller;

  colors_hash = g_hash_table_new (g_str_hash, g_str_equal);
  colors_tree = g_tree_new ((GCompareFunc)strcmp);
  ascii_color_tables = g_tree_new ((GCompareFunc)strcmp);
  color_tables = g_tree_new ((GCompareFunc)strcmp);
  gs_complaints = g_string_new ("\n");
  gen_gs = g_string_new ("");
  gs_config_file_names = g_string_new ("");

  /* GTK4: gtk_init() is called by GtkApplication; no args needed */
  /* gdk_rgb_init() and gtk_set_locale() are no longer needed */

  /* Get screen dimensions via GdkMonitor */
  display = gdk_display_get_default();
  monitor = g_list_model_get_item(gdk_display_get_monitors(display), 0);
  if (monitor) {
    gdk_monitor_get_geometry(monitor, &geometry);
    screen_width_px = geometry.width;
    screen_height_px = geometry.height;
    screen_width_mm = gdk_monitor_get_width_mm(monitor);
    screen_height_mm = gdk_monitor_get_height_mm(monitor);
    g_object_unref(monitor);
  } else {
    screen_width_px = 1920;
    screen_height_px = 1080;
    screen_width_mm = 500;
    screen_height_mm = 300;
  }

  /* Load fonts using Pango instead of gdk_font_load */
  small_fxd_font = sii_font_new("Monospace", 100, TRUE);
  med_fxd_font = sii_font_new("Monospace", 120, TRUE);
  big_fxd_font = sii_font_new("Monospace", 140, TRUE);

  small_pro_font = sii_font_new("Sans", 100, FALSE);
  med_pro_font = sii_font_new("Sans", 120, FALSE);
  big_pro_font = sii_font_new("Sans", 140, FALSE);

  /* GTK4: no need for set_default_visual/colormap/min_colors */

  sii_set_default_colors ();
  sii_init_frame_configs();
  sii_reset_config_cells();


  main_window = gtk_application_window_new (app);

  gtk_window_set_title (GTK_WINDOW(main_window), "SoloIV");
  gtk_window_set_resizable (GTK_WINDOW(main_window), TRUE);

  /* Key press event via GtkEventControllerKey */
  controller = gtk_event_controller_key_new();
  g_signal_connect(controller, "key-pressed",
                   G_CALLBACK(sii_frame_key_pressed_cb), GUINT_TO_POINTER(77));
  gtk_widget_add_controller(main_window, controller);


  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  /* Key press for vbox */
  controller = gtk_event_controller_key_new();
  g_signal_connect(controller, "key-pressed",
                   G_CALLBACK(sii_frame_key_pressed_cb), GUINT_TO_POINTER(78));
  gtk_widget_add_controller(main_vbox, controller);

  gtk_window_set_child (GTK_WINDOW (main_window), main_vbox);

  /* Build main menu */
  soloiv_build_main_menu(main_window, main_vbox);

  main_event_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (main_vbox), main_event_box);
  gtk_widget_set_vexpand(main_event_box, TRUE);
  gtk_widget_set_hexpand(main_event_box, TRUE);

  /* Enter/leave events via GtkEventControllerMotion */
  {
    GtkEventController *motion_controller = gtk_event_controller_motion_new();
    g_signal_connect(motion_controller, "enter",
                     G_CALLBACK(sii_main_enter_cb), NULL);
    gtk_widget_add_controller(main_event_box, motion_controller);
  }


  gs_initial_dir_name = g_string_new ("./");
  batch_mode = (getenv ("BATCH_MODE")) ? TRUE : FALSE;


  if ((aa = getenv ("WINDOW_DUMP_DIR"))) {
    if (strlen (aa))
      { gs_image_dir = g_string_new (aa); }
  }

  if ((aa = getenv ("swi")) || (aa = getenv ("SOLO_WINDOW_INFO"))) {
    if (strlen (aa)) {
      bb = strrchr (aa, '/');
      if (!bb)
	{ strcpy (swi_dir, "./"); }
      else
	{ for (cc = swi_dir; aa <= bb; *cc++ = *aa++); *cc = '\0'; }
      jj = solo_absorb_window_info (swi_dir, aa, 0);

      if (batch_mode && jj < 0 ) {
	g_message
	  ("Unable to start BATCH_MOD soloiv from config file");
	exit (1);
      }
    }
    else
      { aa = NULL; }
  }

  if (!aa && (aa = sii_get_swpfi_dir (NULL))) {
    sii_default_startup (aa);
    config_cb (0, (gpointer)22 ); /* 2x2 */
    g_string_assign (gs_initial_dir_name, aa);
  }
  else if (!aa) {
     g_string_truncate (gen_gs, 0);
     g_string_append
       (gen_gs, "The environment variable DORADE_DIR has not been\n");
     g_string_append
       (gen_gs, "set to point to a directory containing sweepfiles or the\n");
     g_string_append
       (gen_gs, "local directory does not contain any sweepfiles.\n");
    show_solo_init (main_window,(gpointer)main_window);
  }

  gtk_widget_set_visible (main_window, TRUE);

  g_idle_add (sii_batch, 0);	/* routine to control batch mode */

  /* GTK4: no gtk_main() needed - GtkApplication handles the event loop */
}

/* c---------------------------------------------------------------------- */

int main( int argc,
          char *argv[] )	/* c...main */
{
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.lrose.soloiv", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(soloiv_activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}

/* c---------------------------------------------------------------------- */

void
sii_param_colors_filesel (const gchar *str, GtkWidget *fs);

/* c---------------------------------------------------------------------- */

int sii_return_colors_filesel_wid() { return (SII_MAIN_COLORS_FILESEL); }

/* c---------------------------------------------------------------------- */

/* GTK4: File selection dialog using GtkFileDialog (async) */
/* TODO: Implement proper file chooser for GTK4 */

/* c---------------------------------------------------------------------- */

void sii_initialization_widget()
{
  GtkWidget *window;

  GtkWidget *vbox0;
  GtkWidget *hbox;
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *text;
  GtkWidget *grid;
  GtkWidget *scrolled_window;
  GtkTextBuffer *buffer;

  gchar str[128], *aa;
  gint mark, row;
  const GString *cgs;
				/* c...code */
  edit_font = med_pro_font;
  edit_fore = (GdkRGBA *)g_hash_table_lookup ( colors_hash, (gpointer)"black");
  edit_back = (GdkRGBA *)g_hash_table_lookup ( colors_hash, (gpointer)"white");

  main_config_window =
    window = gtk_window_new();

  g_signal_connect (window, "destroy",
		    G_CALLBACK(gtk_window_destroy),
		    NULL);
  gtk_window_set_title (GTK_WINDOW (window), "SoloIV Initialization");

  vbox0 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child (GTK_WINDOW(window), vbox0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append (GTK_BOX(vbox0), hbox);

  button = gtk_button_new_with_label (" Save Config ");
  gtk_box_append (GTK_BOX (hbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_SAVE_CONFIG));

  button = gtk_button_new_with_label (" List Configs ");
  gtk_box_append (GTK_BOX (hbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_LIST_CONFIGS));

  button = gtk_button_new_with_label (" Cancel ");
  gtk_box_append (GTK_BOX (hbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_CANCEL));

  g_string_truncate (gen_gs, 0);
  g_string_append (gen_gs,
"For default startup, type the sweepfile directory and hit <enter>\n");
  g_string_append (gen_gs,
"or enter the directory with your configuration files and hit <enter>\n");
  g_string_append (gen_gs,
"and then click on the desired file name.\n");
  label = gtk_label_new (gen_gs->str);

  gtk_label_set_xalign (GTK_LABEL(label), 0.0);
  gtk_box_append (GTK_BOX(vbox0), label);


  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
  gtk_box_append (GTK_BOX(vbox0), grid);
  row = 0;

  label = gtk_label_new ( " Swpfi Dir " );
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 5, 1);
  gtk_editable_set_text (GTK_EDITABLE (entry), gs_initial_dir_name->str);
  g_signal_connect (entry, "activate",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_DIR));
  g_object_set_data (G_OBJECT(window), "swpfi_dir", (gpointer)entry);

  row++;
  label = gtk_label_new ( " " );
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  button = gtk_button_new_with_label (" Swpfi File Select ");
  gtk_grid_attach (GTK_GRID (grid), button, 3, row, 2, 1);
  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_SWPFI_FILESEL));

  row++;
  label = gtk_label_new ( " Config Dir " );
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 5, 1);
  gtk_editable_set_text (GTK_EDITABLE (entry), gs_initial_dir_name->str);
  g_signal_connect (entry, "activate",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_CONFIG_DIR));
  g_object_set_data (G_OBJECT(window), "config_dir", (gpointer)entry);

  row++;
  label = gtk_label_new ( " Comment " );
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  entry = gtk_entry_new ();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 5, 1);
  gtk_editable_set_text (GTK_EDITABLE (entry), "_no_comment");
  g_signal_connect (entry, "activate",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_CONFIG_COMMENT));
  g_object_set_data (G_OBJECT(window), "comment", (gpointer)entry);

  row++;
  button = gtk_check_button_new_with_label (" Ignore Swpfi Info ");
  gtk_grid_attach (GTK_GRID (grid), button, 1, row, 2, 1);
  g_signal_connect (button, "toggled",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_IGNORE_SWPIFS));
  gtk_check_button_set_active (GTK_CHECK_BUTTON (button), FALSE);
  ignore_swpfi_info = 0;
  g_object_set_data (G_OBJECT(window), "ignore", (gpointer)button);

  button = gtk_button_new_with_label (" Config File Select ");
  gtk_grid_attach (GTK_GRID (grid), button, 3, row, 2, 1);
  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_CONFIG_FILESEL));

  /* Text view in scrolled window */
  scrolled_window = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (scrolled_window, 500, 200);
  gtk_widget_set_vexpand(scrolled_window, TRUE);
  gtk_box_append (GTK_BOX (vbox0), scrolled_window);

  main_config_text =
    text = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);

  /* Click handler for text view */
  {
    GtkGesture *click = gtk_gesture_click_new();
    g_signal_connect(click, "pressed",
                     G_CALLBACK(sii_main_text_click_cb),
                     (gpointer)gs_config_file_names);
    gtk_widget_add_controller(text, GTK_EVENT_CONTROLLER(click));
  }

  g_object_set_data (G_OBJECT(text),
		     "file_names_gs",
		     (gpointer)gs_config_file_names);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), text);

  cgs = sii_return_config_files (gs_initial_dir_name->str);
  g_string_assign (gs_config_file_names, cgs->str);
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
  gtk_text_buffer_set_text(buffer, gs_config_file_names->str, -1);

  /* --- Make everything visible --- */

  gtk_widget_set_visible (window, TRUE);
  mark = 0;
}

/* c---------------------------------------------------------------------- */

void sii_image_gen_widget()
{
  GtkWidget *window;
  GtkWidget *vbox0;
  GtkWidget *hbox;
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *grid;

  gchar str[128], *aa;
  gint mark, row;
				/* c...code */

  main_image_gen_window =
    window = gtk_window_new();

  g_signal_connect (window, "destroy",
		    G_CALLBACK(gtk_window_destroy),
		    NULL);
  gtk_window_set_title (GTK_WINDOW (window), "SoloIV PNG Images");

  vbox0 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child (GTK_WINDOW(window), vbox0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_append (GTK_BOX(vbox0), hbox);

  button = gtk_button_new_with_label (" Make PNG Image ");
  gtk_box_append (GTK_BOX (hbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_PNG));

  button = gtk_button_new_with_label (" Cancel ");
  gtk_box_append (GTK_BOX (hbox), button);
  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_CANCEL_IMAGE_GEN));

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
  gtk_grid_set_row_spacing(GTK_GRID(grid), 3);
  gtk_box_append (GTK_BOX(vbox0), grid);
  row = 0;

  label = gtk_label_new ( " Image Dir " );
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);

  entry = gtk_entry_new ();
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, row, 4, 1);

  aa = (gs_image_dir) ? gs_image_dir->str : gs_initial_dir_name->str;
  gs_image_dir = g_string_new (aa);
  gtk_editable_set_text (GTK_EDITABLE (entry), gs_image_dir->str);
  g_signal_connect (entry, "activate",
		    G_CALLBACK(sii_initialize_cb),
		    GUINT_TO_POINTER(SII_MAIN_IMAGE_DIR));

  g_object_set_data (G_OBJECT(window), "image_dir", (gpointer)entry);

  /* --- Make everything visible --- */

  gtk_widget_set_visible (window, TRUE);
  mark = 0;
}

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */
/* GTK4 Menu System using GMenu + GAction */

static void on_menu_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  const gchar *name = g_action_get_name(G_ACTION(action));

  if (g_str_equal(name, "quit")) {
    g_application_quit(G_APPLICATION(
      gtk_window_get_application(GTK_WINDOW(main_window))));
  }
  else if (g_str_equal(name, "set_source_dir")) {
    show_solo_init(main_window, NULL);
  }
  else if (g_str_equal(name, "config_files")) {
    show_solo_init(main_window, NULL);
  }
  else if (g_str_equal(name, "set_image_dir")) {
    show_solo_image_gen(main_window, NULL);
  }
  else if (g_str_equal(name, "hide_data")) {
    click_data_cb(NULL, GUINT_TO_POINTER(0));
  }
  else if (g_str_equal(name, "show_data")) {
    click_data_cb(NULL, GUINT_TO_POINTER(1));
  }
}

static void on_zoom_data_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gint32 val = g_variant_get_int32(parameter);
  zoom_data_cb(NULL, GUINT_TO_POINTER(val));
}

static void on_zoom_config_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gint32 val = g_variant_get_int32(parameter);
  zoom_config_cb(NULL, GUINT_TO_POINTER(val));
}

static void on_center_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gint32 val = g_variant_get_int32(parameter);
  center_frame_cb(NULL, GUINT_TO_POINTER(val));
}

static void on_config_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gint32 val = g_variant_get_int32(parameter);
  config_cb(NULL, GUINT_TO_POINTER(val));
}

static void on_custom_config_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gint32 val = g_variant_get_int32(parameter);
  custom_config_cb(NULL, GUINT_TO_POINTER(val));
}

static void on_shape_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  gint32 val = g_variant_get_int32(parameter);
  shape_cb(NULL, GUINT_TO_POINTER(val));
}

static void on_toggle_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  GVariant *state = g_action_get_state(G_ACTION(action));
  gboolean active = !g_variant_get_boolean(state);
  g_simple_action_set_state(action, g_variant_new_boolean(active));
  g_variant_unref(state);

  guint id = GPOINTER_TO_UINT(data);
  sii_menu_cb(GUINT_TO_POINTER(id));
}

static void on_help_action(GSimpleAction *action, GVariant *parameter, gpointer data)
{
  guint id = GPOINTER_TO_UINT(data);
  sii_menu_cb(GUINT_TO_POINTER(id));
}

void soloiv_build_main_menu(GtkWidget *window, GtkWidget *vbox)
{
  GMenu *menubar_model;
  GMenu *file_menu, *zoom_menu, *center_menu, *config_menu, *help_menu;
  GMenu *zoom_data_section, *zoom_config_section;
  GSimpleActionGroup *actions;
  GtkWidget *menu_bar;

  actions = g_simple_action_group_new();

  /* File actions */
  GSimpleAction *action;

  action = g_simple_action_new("quit", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(on_menu_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  action = g_simple_action_new("set_source_dir", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(on_menu_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  action = g_simple_action_new("config_files", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(on_menu_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  action = g_simple_action_new("set_image_dir", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(on_menu_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  action = g_simple_action_new("hide_data", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(on_menu_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  action = g_simple_action_new("show_data", NULL);
  g_signal_connect(action, "activate", G_CALLBACK(on_menu_action), NULL);
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  /* Zoom data actions */
  {
    static const struct { const char *name; int val; } zoom_data_items[] = {
      {"zoom_data_default", 0}, {"zoom_data_p200", 200}, {"zoom_data_p100", 100},
      {"zoom_data_p50", 50}, {"zoom_data_p25", 25}, {"zoom_data_p10", 10},
      {"zoom_data_m10", 10010}, {"zoom_data_m25", 10025},
      {"zoom_data_m50", 10050}, {"zoom_data_m80", 10080},
    };
    for (int i = 0; i < (int)(sizeof(zoom_data_items)/sizeof(zoom_data_items[0])); i++) {
      action = g_simple_action_new(zoom_data_items[i].name, G_VARIANT_TYPE_INT32);
      g_signal_connect(action, "activate", G_CALLBACK(on_zoom_data_action), NULL);
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
    }
  }

  /* Zoom config actions */
  {
    static const struct { const char *name; int val; } zoom_cfg_items[] = {
      {"zoom_cfg_p50", 50}, {"zoom_cfg_p25", 25},
      {"zoom_cfg_p17", 10}, {"zoom_cfg_p06", 6},
      {"zoom_cfg_m05", 10005}, {"zoom_cfg_m11", 10011},
    };
    for (int i = 0; i < (int)(sizeof(zoom_cfg_items)/sizeof(zoom_cfg_items[0])); i++) {
      action = g_simple_action_new(zoom_cfg_items[i].name, G_VARIANT_TYPE_INT32);
      g_signal_connect(action, "activate", G_CALLBACK(on_zoom_config_action), NULL);
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
    }
  }

  /* Center actions */
  {
    static const struct { const char *name; int val; } center_items[] = {
      {"center_click1", 1}, {"center_click2", 2}, {"center_click4", 4},
      {"center_radar", 0},
    };
    for (int i = 0; i < (int)(sizeof(center_items)/sizeof(center_items[0])); i++) {
      action = g_simple_action_new(center_items[i].name, G_VARIANT_TYPE_INT32);
      g_signal_connect(action, "activate", G_CALLBACK(on_center_action), NULL);
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
    }
  }

  /* Toggle actions */
  action = g_simple_action_new_stateful("electric_ctr", NULL, g_variant_new_boolean(TRUE));
  g_signal_connect(action, "activate", G_CALLBACK(on_toggle_action),
                   GUINT_TO_POINTER(SII_MAIN_CTR_TOGGLE));
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  action = g_simple_action_new_stateful("ctr_crosshairs", NULL, g_variant_new_boolean(FALSE));
  g_signal_connect(action, "activate", G_CALLBACK(on_toggle_action),
                   GUINT_TO_POINTER(SII_MAIN_XHAIRS_TOGGLE));
  g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));

  /* Config actions */
  {
    static const struct { const char *name; int val; } cfg_items[] = {
      {"cfg_11", 11}, {"cfg_12", 12}, {"cfg_13", 13}, {"cfg_14", 14},
      {"cfg_21", 21}, {"cfg_22", 22}, {"cfg_23", 23}, {"cfg_24", 24},
      {"cfg_31", 31}, {"cfg_32", 32}, {"cfg_33", 33}, {"cfg_34", 34},
      {"cfg_41", 41}, {"cfg_42", 42}, {"cfg_43", 43},
    };
    for (int i = 0; i < (int)(sizeof(cfg_items)/sizeof(cfg_items[0])); i++) {
      action = g_simple_action_new(cfg_items[i].name, G_VARIANT_TYPE_INT32);
      g_signal_connect(action, "activate", G_CALLBACK(on_config_action), NULL);
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
    }
  }

  /* Shape actions */
  {
    static const struct { const char *name; int val; } shape_items[] = {
      {"shape_square", 11}, {"shape_slide", 43}, {"shape_vslide", 34},
    };
    for (int i = 0; i < (int)(sizeof(shape_items)/sizeof(shape_items[0])); i++) {
      action = g_simple_action_new(shape_items[i].name, G_VARIANT_TYPE_INT32);
      g_signal_connect(action, "activate", G_CALLBACK(on_shape_action), NULL);
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
    }
  }

  /* Custom config actions */
  {
    static const struct { const char *name; int val; } ccfg_items[] = {
      {"ccfg_default", 2022}, {"ccfg_1b2vs", 1012}, {"ccfg_1b2hs", 2012},
      {"ccfg_1b3s", 1013}, {"ccfg_1b5s", 1015}, {"ccfg_1b7s", 1017},
      {"ccfg_2b4vs", 1024}, {"ccfg_2b4hs", 2024},
    };
    for (int i = 0; i < (int)(sizeof(ccfg_items)/sizeof(ccfg_items[0])); i++) {
      action = g_simple_action_new(ccfg_items[i].name, G_VARIANT_TYPE_INT32);
      g_signal_connect(action, "activate", G_CALLBACK(on_custom_config_action), NULL);
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
    }
  }

  /* Help actions */
  {
    static const struct { const char *name; guint id; } help_items[] = {
      {"help_basics", SII_MAIN_HLP_BASIC}, {"help_files", SII_MAIN_HLP_FILE},
      {"help_zooming", SII_MAIN_HLP_ZOOM}, {"help_centering", SII_MAIN_HLP_CTR},
      {"help_configuring", SII_MAIN_HLP_CONFIG},
      {"help_shortcuts", SII_MAIN_HLP_SHORTCUTS},
      {"help_comparisons", SII_MAIN_HLP_COMPARE},
      {"help_about", SII_MAIN_HLP_ABOUT},
    };
    for (int i = 0; i < (int)(sizeof(help_items)/sizeof(help_items[0])); i++) {
      action = g_simple_action_new(help_items[i].name, NULL);
      g_signal_connect(action, "activate", G_CALLBACK(on_help_action),
                       GUINT_TO_POINTER(help_items[i].id));
      g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
    }
  }

  gtk_widget_insert_action_group(window, "menu", G_ACTION_GROUP(actions));

  /* Build the GMenu model */
  menubar_model = g_menu_new();

  /* File menu */
  file_menu = g_menu_new();
  g_menu_append(file_menu, "Set Source Dir", "menu.set_source_dir");
  g_menu_append(file_menu, "Config Files", "menu.config_files");
  g_menu_append(file_menu, "Set Image Dir", "menu.set_image_dir");
  g_menu_append(file_menu, "Hide Data Widgets", "menu.hide_data");
  g_menu_append(file_menu, "Show Data Widgets", "menu.show_data");
  g_menu_append(file_menu, "Exit", "menu.quit");
  g_menu_append_submenu(menubar_model, "File", G_MENU_MODEL(file_menu));

  /* Zoom menu */
  zoom_menu = g_menu_new();
  zoom_data_section = g_menu_new();
  g_menu_append(zoom_data_section, "Default", "menu.zoom_data_default(0)");
  g_menu_append(zoom_data_section, "+200%", "menu.zoom_data_p200(200)");
  g_menu_append(zoom_data_section, "+100%", "menu.zoom_data_p100(100)");
  g_menu_append(zoom_data_section, "+50%", "menu.zoom_data_p50(50)");
  g_menu_append(zoom_data_section, "+25%", "menu.zoom_data_p25(25)");
  g_menu_append(zoom_data_section, "+10%", "menu.zoom_data_p10(10)");
  g_menu_append(zoom_data_section, "-10%", "menu.zoom_data_m10(10010)");
  g_menu_append(zoom_data_section, "-25%", "menu.zoom_data_m25(10025)");
  g_menu_append(zoom_data_section, "-50%", "menu.zoom_data_m50(10050)");
  g_menu_append(zoom_data_section, "-80%", "menu.zoom_data_m80(10080)");
  g_menu_append_section(zoom_menu, "Data", G_MENU_MODEL(zoom_data_section));

  zoom_config_section = g_menu_new();
  g_menu_append(zoom_config_section, "+50%", "menu.zoom_cfg_p50(50)");
  g_menu_append(zoom_config_section, "+25%", "menu.zoom_cfg_p25(25)");
  g_menu_append(zoom_config_section, "+17%", "menu.zoom_cfg_p17(10)");
  g_menu_append(zoom_config_section, "+06%", "menu.zoom_cfg_p06(6)");
  g_menu_append(zoom_config_section, "-05%", "menu.zoom_cfg_m05(10005)");
  g_menu_append(zoom_config_section, "-11%", "menu.zoom_cfg_m11(10011)");
  g_menu_append_section(zoom_menu, "Config", G_MENU_MODEL(zoom_config_section));
  g_menu_append_submenu(menubar_model, "Zoom", G_MENU_MODEL(zoom_menu));

  /* Center menu */
  center_menu = g_menu_new();
  g_menu_append(center_menu, "Last click", "menu.center_click1(1)");
  g_menu_append(center_menu, "Last 2 clicks", "menu.center_click2(2)");
  g_menu_append(center_menu, "Last 4 clicks", "menu.center_click4(4)");
  g_menu_append(center_menu, "Radar", "menu.center_radar(0)");
  g_menu_append(center_menu, "Electric", "menu.electric_ctr");
  g_menu_append(center_menu, "Ctr CrossHairs", "menu.ctr_crosshairs");
  g_menu_append_submenu(menubar_model, "Center", G_MENU_MODEL(center_menu));

  /* Config menu */
  config_menu = g_menu_new();
  g_menu_append(config_menu, "Default Config", "menu.ccfg_default(2022)");
  {
    GMenu *grid_section = g_menu_new();
    g_menu_append(grid_section, "1x1", "menu.cfg_11(11)");
    g_menu_append(grid_section, "1x2", "menu.cfg_12(12)");
    g_menu_append(grid_section, "1x3", "menu.cfg_13(13)");
    g_menu_append(grid_section, "1x4", "menu.cfg_14(14)");
    g_menu_append(grid_section, "2x1", "menu.cfg_21(21)");
    g_menu_append(grid_section, "2x2", "menu.cfg_22(22)");
    g_menu_append(grid_section, "2x3", "menu.cfg_23(23)");
    g_menu_append(grid_section, "2x4", "menu.cfg_24(24)");
    g_menu_append(grid_section, "3x1", "menu.cfg_31(31)");
    g_menu_append(grid_section, "3x2", "menu.cfg_32(32)");
    g_menu_append(grid_section, "3x3", "menu.cfg_33(33)");
    g_menu_append(grid_section, "3x4", "menu.cfg_34(34)");
    g_menu_append(grid_section, "4x1", "menu.cfg_41(41)");
    g_menu_append(grid_section, "4x2", "menu.cfg_42(42)");
    g_menu_append(grid_section, "4x3", "menu.cfg_43(43)");
    g_menu_append_section(config_menu, NULL, G_MENU_MODEL(grid_section));
    g_object_unref(grid_section);
  }
  {
    GMenu *shape_section = g_menu_new();
    g_menu_append(shape_section, "Square Shaped", "menu.shape_square(11)");
    g_menu_append(shape_section, "Slide Shaped", "menu.shape_slide(43)");
    g_menu_append(shape_section, "VSlide Shaped", "menu.shape_vslide(34)");
    g_menu_append_section(config_menu, NULL, G_MENU_MODEL(shape_section));
    g_object_unref(shape_section);
  }
  {
    GMenu *custom_section = g_menu_new();
    g_menu_append(custom_section, "1 Big 2v Small", "menu.ccfg_1b2vs(1012)");
    g_menu_append(custom_section, "1 Big 2h Small", "menu.ccfg_1b2hs(2012)");
    g_menu_append(custom_section, "1 Big 3 Small", "menu.ccfg_1b3s(1013)");
    g_menu_append(custom_section, "1 Big 5 Small", "menu.ccfg_1b5s(1015)");
    g_menu_append(custom_section, "1 Big 7 Small", "menu.ccfg_1b7s(1017)");
    g_menu_append(custom_section, "2 Big 4v Small", "menu.ccfg_2b4vs(1024)");
    g_menu_append(custom_section, "2 Big 4h Small", "menu.ccfg_2b4hs(2024)");
    g_menu_append_section(config_menu, NULL, G_MENU_MODEL(custom_section));
    g_object_unref(custom_section);
  }
  g_menu_append_submenu(menubar_model, "Config", G_MENU_MODEL(config_menu));

  /* Help menu */
  help_menu = g_menu_new();
  g_menu_append(help_menu, "Basics", "menu.help_basics");
  g_menu_append(help_menu, "Files", "menu.help_files");
  g_menu_append(help_menu, "Zooming", "menu.help_zooming");
  g_menu_append(help_menu, "Centering", "menu.help_centering");
  g_menu_append(help_menu, "Configuring", "menu.help_configuring");
  g_menu_append(help_menu, "Shortcuts", "menu.help_shortcuts");
  g_menu_append(help_menu, "Comparisons", "menu.help_comparisons");
  g_menu_append(help_menu, "About", "menu.help_about");
  g_menu_append_submenu(menubar_model, "Help", G_MENU_MODEL(help_menu));

  /* Create the PopoverMenuBar widget */
  menubar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(menubar_model));
  gtk_box_prepend(GTK_BOX(vbox), menubar);

  /* Cleanup menu models */
  g_object_unref(menubar_model);
  g_object_unref(file_menu);
  g_object_unref(zoom_menu);
  g_object_unref(zoom_data_section);
  g_object_unref(zoom_config_section);
  g_object_unref(center_menu);
  g_object_unref(config_menu);
  g_object_unref(help_menu);
}

/* c------------------------------------------------------------------------ */
void sp_change_cursor (int normal) {
  /* GTK4: Cursor management via gtk_widget_set_cursor */
  const gchar *cursor_name;

  if (normal == 0) {
    cursor_name = "wait";
  } else if (normal == 1) {
    cursor_name = "crosshair";
  } else {
    cursor_name = "default";
  }

  gtk_widget_set_cursor_from_name(main_event_box, cursor_name);
}

/* c---------------------------------------------------------------------- */
/* c...end */
