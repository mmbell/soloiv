/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include "sii_utils.h"
# include <solo_window_structs.h>

gchar *sample_click_text = "caca";

enum {
   FRAME_MENU_ZERO,
   FRAME_MENU_CANCEL,
   FRAME_MENU_REPLOT,
   FRAME_MENU_SWEEPFILES,
   FRAME_MENU_SWPFI_LIST,
   FRAME_MENU_PARAMETERS,
   FRAME_MENU_VIEW,
   FRAME_MENU_CENTERING,
   FRAME_MENU_LOCKSTEPS,
   FRAME_MENU_EDITOR,
   FRAME_MENU_EXAMINE,
   FRAME_MENU_CLICK_DATA,
   FRAME_MENU_LAST_ENUM,
};

/* c---------------------------------------------------------------------- */

void frame_menu_widget ( guint frame_num );

void frame_click_data_widget ( guint frame_num );

void show_click_data_widget (guint frame_num);

void show_edit_widget (GtkWidget *text, gpointer data );

void show_exam_widget (GtkWidget *text, gpointer data );

void show_frame_menu (GtkWidget *text, gpointer data );

void show_param_widget (GtkWidget *text, gpointer data );

void show_swpfi_widget (GtkWidget *text, gpointer data );

void show_view_widget (GtkWidget *text, gpointer data );

/* c---------------------------------------------------------------------- */

static SiiFont *edit_font;
static GdkRGBA *edit_fore;
static GdkRGBA *edit_back;

static gchar *click_tmplt = "W0000.00km.-000.00 -000.00 -000.00W";

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

void sii_set_click_data_text (guint frame_num, GString *gs)
{
  GtkWidget *widget, *text;
  widget = sii_get_widget_ptr ( frame_num, FRAME_CLICK_DATA );

  g_string_assign (frame_configs[frame_num]->data_widget_text, gs->str);

  if (!widget)
    { return; }

  text = (GtkWidget *)g_object_get_data (G_OBJECT(widget), "text_widget");

  if (text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
    gtk_text_buffer_set_text (buffer, gs->str, gs->len);
  }
}

/* c---------------------------------------------------------------------- */

void frame_menu_cb (GtkWidget *frame, gpointer data )
{
  GtkWidget *widget;
  guint num = GPOINTER_TO_UINT (data);
  guint frame_num, task, window_id;

  frame_num = num/TASK_MODULO;
  window_id = task = num % TASK_MODULO;

  switch (task) {

  case FRAME_MENU_REPLOT:
    if (sii_set_param_info (frame_num))
      { sii_plot_data (frame_num, REPLOT_THIS_FRAME); }
    break;

  case FRAME_MENU_CANCEL:
    widget = sii_get_widget_ptr ( frame_num, FRAME_MENU );
    if( widget )
    { gtk_widget_set_visible (widget, FALSE); }
    break;

  case FRAME_MENU_SWEEPFILES:
    widget = sii_get_widget_ptr ( frame_num, FRAME_MENU );
    gtk_widget_set_visible (widget, FALSE);
    show_swpfi_widget (frame, (gpointer)frame_num );
    break;

  case FRAME_MENU_PARAMETERS:
    widget = sii_get_widget_ptr ( frame_num, FRAME_MENU );
    gtk_widget_set_visible (widget, FALSE);
    show_param_widget (frame, (gpointer)frame_num );
    break;

  case FRAME_MENU_VIEW:
    widget = sii_get_widget_ptr ( frame_num, FRAME_MENU );
    gtk_widget_set_visible (widget, FALSE);
    show_view_widget (frame, (gpointer)frame_num );
    break;

  case FRAME_MENU_LOCKSTEPS:
    break;

  case FRAME_MENU_EDITOR:
    widget = sii_get_widget_ptr ( frame_num, FRAME_MENU );
    gtk_widget_set_visible (widget, FALSE);
    show_edit_widget (frame, (gpointer)frame_num );
    break;

  case FRAME_MENU_EXAMINE:
    widget = sii_get_widget_ptr ( frame_num, FRAME_MENU );
    gtk_widget_set_visible (widget, FALSE);
    show_exam_widget (frame, (gpointer)frame_num );
    break;

  case FRAME_MENU_CLICK_DATA:
    /* Show the clicked data widget for this frame
     */
    frame_configs[frame_num]->show_data_widget =
      !frame_configs[frame_num]->show_data_widget;

    widget = sii_get_widget_ptr (frame_num, FRAME_CLICK_DATA);

    if (frame_configs[frame_num]->show_data_widget) {
      show_click_data_widget (frame_num);
    }
    else {
      if (widget) {
	/* GTK4: gdk_window_get_origin not available; position tracking removed */
	gtk_widget_set_visible (widget, FALSE);
      }
    }
    widget = sii_get_widget_ptr (frame_num, FRAME_MENU);
    gtk_widget_set_visible (widget, FALSE);
    break;
  };
}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

void show_frame_menu (GtkWidget *text, gpointer data )
{
  guint frame_num = GPOINTER_TO_UINT (data);
  GtkWidget *widget;

  widget = sii_get_widget_ptr (frame_num, FRAME_MENU);

  if( !widget )
    { frame_menu_widget (frame_num); }
  else {
     /* GTK4: gtk_widget_set_uposition removed; use gtk_window_present */
     gtk_window_present (GTK_WINDOW (widget));
     gtk_widget_set_visible (widget, TRUE);
  }
}

/* c---------------------------------------------------------------------- */
void show_all_click_data_widgets (gboolean really)
{
  GtkWidget *widget, *frame;
  guint frame_num;

  for (frame_num=0; frame_num < sii_frame_count; frame_num++) {

    if (really) {
      if (frame_configs[frame_num]->show_data_widget) {
	continue;		/* already shown */
      }
      else {
	show_click_data_widget (frame_num);
      }
    }
    else {			/* hide it! */
      widget = sii_get_widget_ptr (frame_num, FRAME_CLICK_DATA);

      if (widget) {
	/* GTK4: gdk_window_get_origin not available; position tracking removed */
	gtk_widget_set_visible (widget, FALSE);
      }
      frame_configs[frame_num]->show_data_widget = FALSE;
    }
  }
}

/* c---------------------------------------------------------------------- */

void show_click_data_widget (guint frame_num)
{
  GtkWidget *widget;

  widget = sii_get_widget_ptr ( frame_num, FRAME_CLICK_DATA );
  frame_configs[frame_num]->show_data_widget = TRUE;

  if( !widget )
    { frame_click_data_widget (frame_num); }
  else {
     /* GTK4: gtk_widget_set_uposition removed; use gtk_window_present */
     gtk_window_present (GTK_WINDOW (widget));
     gtk_widget_set_visible (widget, TRUE);
  }
}

/* c---------------------------------------------------------------------- */

void frame_click_data_widget ( guint frame_num )
{
  GtkWidget *widget;
  GtkWidget *label;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *table;
  GtkWidget *text;

  guint nn, height, nlines = 11;
  gchar *aa, *bb, str[128];
  gint width;


  edit_font = med_fxd_font;
  edit_fore = (GdkRGBA *)g_hash_table_lookup ( colors_hash, (gpointer)"black");
  edit_back = (GdkRGBA *)g_hash_table_lookup ( colors_hash, (gpointer)"white");

  width = sii_font_string_width (edit_font, click_tmplt);
  height = edit_font->ascent + edit_font->descent + 1;

  window = gtk_window_new();
  sii_set_widget_ptr ( frame_num, FRAME_CLICK_DATA, window );

  widget = sii_get_widget_ptr (frame_num, FRAME_MENU);
  if (widget) {
    /* GTK4: position tracking via widget_origin may need rethinking */
  }
  else {
    /* GTK4: gdk_window_get_origin not available; position tracking removed */
  }
  /* GTK4: gtk_widget_set_uposition removed */

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);
  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num * TASK_MODULO + FRAME_MENU));

  /* --- Title and border --- */
  sprintf (str, "Frame %d  Data", frame_num+1 );
  gtk_window_set_title (GTK_WINDOW (window), str);
  /* GTK4: gtk_container_border_width removed; use margins on child */

  /* --- Create a new vertical box for storing widgets --- */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW(window), vbox);

  text = gtk_text_view_new ();
  gtk_widget_set_size_request (text, width, nlines * height);
  gtk_box_append (GTK_BOX (vbox), text);

  aa = frame_configs[frame_num]->data_widget_text->str;
  if (!frame_configs[frame_num]->data_widget_text->len)
    { aa = sample_click_text; }

  g_object_set_data (G_OBJECT(window),
		       "text_widget",
		       (gpointer)text);

  {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text));
    gtk_text_buffer_set_text (buffer, aa, -1);
  }

  gtk_text_view_set_editable (GTK_TEXT_VIEW (text), FALSE);

  /* --- Make everything visible --- */
  gtk_widget_set_visible (window, TRUE);
}

/* c---------------------------------------------------------------------- */

void frame_menu_widget ( guint frame_num )
{
  GtkWidget *label;
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;

  guint nn;
  gchar *aa, *bb;

  window = gtk_window_new();
  sii_set_widget_ptr ( frame_num, FRAME_MENU, window );
  /* GTK4: gtk_widget_set_uposition removed; position tracking not supported */

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);
  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num * TASK_MODULO + FRAME_MENU));

  /* --- Title and border --- */
  bb = g_strdup_printf ("Frame %d", frame_num+1 );
  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );
  /* GTK4: gtk_container_border_width removed; use margins on child */

  /* --- Create a new vertical box for storing widgets --- */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW(window), vbox);

  button = gtk_button_new_with_label ("Cancel");
  gtk_box_append (GTK_BOX (vbox), button);

  nn = frame_num*TASK_MODULO + FRAME_MENU_CANCEL;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );
# ifdef notyet
  button = gtk_button_new_with_label ("Replot");
  gtk_box_append (GTK_BOX (vbox), button);
  nn = frame_num*TASK_MODULO + FRAME_MENU_REPLOT;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );
# endif
  button = gtk_button_new_with_label ("Sweepfiles");
  gtk_box_append (GTK_BOX (vbox), button);

  nn = frame_num*TASK_MODULO + FRAME_MENU_SWEEPFILES;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );

  button = gtk_button_new_with_label (" Parameters + Colors ");
  gtk_box_append (GTK_BOX (vbox), button);

  nn = frame_num*TASK_MODULO + FRAME_MENU_PARAMETERS;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );

  button = gtk_button_new_with_label ("View");
  gtk_box_append (GTK_BOX (vbox), button);

  nn = frame_num*TASK_MODULO + FRAME_MENU_VIEW;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );
# ifdef obsolete
  button = gtk_button_new_with_label ("Centering");
  gtk_box_append (GTK_BOX (vbox), button);
  button = gtk_button_new_with_label ("Lock Steps");
  gtk_box_append (GTK_BOX (vbox), button);
# endif

  button = gtk_button_new_with_label ("Editor");
  gtk_box_append (GTK_BOX (vbox), button);

  nn = frame_num*TASK_MODULO + FRAME_MENU_EDITOR;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );

  button = gtk_button_new_with_label ("Examine");
  gtk_box_append (GTK_BOX (vbox), button);

  nn = frame_num*TASK_MODULO + FRAME_MENU_EXAMINE;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );

  button = gtk_button_new_with_label ("Data Widget");
  gtk_box_append (GTK_BOX (vbox), button);

  nn = frame_num*TASK_MODULO + FRAME_MENU_CLICK_DATA;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (frame_menu_cb)
		      , (gpointer)nn
		      );

  /* --- Make everything visible --- */
  gtk_widget_set_visible (window, TRUE);

}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */
