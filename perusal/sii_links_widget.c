/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include "sii_utils.h"

/* GTK4: replacement for gtk_container_foreach */
typedef void (*SiiWidgetCallback)(GtkWidget *widget, gpointer data);

static void
sii_widget_foreach(GtkWidget *parent, SiiWidgetCallback callback, gpointer data)
{
  GtkWidget *child = gtk_widget_get_first_child(parent);
  while (child) {
    GtkWidget *next = gtk_widget_get_next_sibling(child);
    callback(child, data);
    child = next;
  }
}

enum {
   LINKS_ZERO,
   LINKS_OK,
   LINKS_CANCEL,
   LINKS_SET_ALL,
   LINKS_CLEAR_ALL,
   LINKS_LAST_ENUM,
};

void sii_links_widget( guint frame_num, guint widget_id, LinksInfo *linkfo );
void sii_view_update_links (guint frame_num, int li_type);

/* c---------------------------------------------------------------------- */

void sii_set_links_from_solo_struct (int frame_num, int links_id,
				     long *linked_windows)
{
   LinksInfo *li = frame_configs[frame_num]->link_set[links_id];
   guint jj;

   for (jj=0; jj < maxFrames; jj++)
     { li->link_set[jj] = (linked_windows[jj]) ? TRUE : FALSE; }
}

/* c---------------------------------------------------------------------- */

void sii_links_set_foreach (GtkWidget *button, gpointer data )
{
   gint active = GPOINTER_TO_UINT (data);
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);
}

/* c---------------------------------------------------------------------- */

void  sii_links_set_from_struct (GtkWidget *button, gpointer data )
{
   LinksInfo *li = (LinksInfo *)data;
   gint button_num = GPOINTER_TO_UINT
     (g_object_get_data (G_OBJECT(button), "button_num"));

   gint active = li->link_set[button_num];
   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);
}

/* c---------------------------------------------------------------------- */

void  sii_links_get_foreach (GtkWidget *button, gpointer data )
{
   LinksInfo *li = (LinksInfo *)data;
   gint active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
   gint button_num = GPOINTER_TO_UINT
     (g_object_get_data (G_OBJECT(button), "button_num"));
   li->link_set[button_num] = active;
}

/* c---------------------------------------------------------------------- */

LinksInfo *sii_new_links_info ( gchar *name, guint frame_num, guint widget_id
			       , gboolean this_frame_only )
{
   LinksInfo *li = g_malloc0 (sizeof( LinksInfo ));
   int fn;

   li->name = g_strdup (name);
   li->frame_num = frame_num;
   li->widget_id = widget_id;
   li->num_links = (this_frame_only) ? 1 : maxFrames;

   if (this_frame_only) {
      li->link_set[frame_num] = TRUE;
   }
   else {
      for (fn=0; fn < maxFrames; fn++ ) {
	 li->link_set[fn] = TRUE;
      }
   }
   return li;
}

/* c---------------------------------------------------------------------- */

void show_links_widget (LinksInfo *linkfo)
{
  GtkWidget *widget;
  GtkWidget *table;

  guint frame_num = linkfo->frame_num;
  guint widget_id = linkfo->widget_id;

  GdkPoint *ptp;
  gint x, y;

  widget = sii_get_widget_ptr (frame_num, widget_id);

  if( !widget )
    { sii_links_widget( frame_num, widget_id, linkfo ); }
  else {
     ptp = &frame_configs[frame_num]->widget_origin[FRAME_MENU];
     x = ptp->x; y = ptp->y;
     sii_widget_foreach(linkfo->table,
			 sii_links_set_from_struct,
			 (gpointer)linkfo );
     /* gtk_widget_set_uposition removed for GTK4 */
     gtk_widget_set_visible (widget, TRUE);
  }
}

/* c---------------------------------------------------------------------- */

void sii_links_widget_cb ( GtkWidget *w, gpointer   data )
{
   GtkWidget *widget;
   GtkWidget *table;
   GtkWidget *button = GTK_WIDGET (data);
   LinksInfo *li = (LinksInfo *)g_object_get_data (G_OBJECT(button)
						     , "link_info");
   guint task = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button)
						       , "button_wid"));
   gint active, jj, li_type;

   switch (task) {

    case LINKS_OK:
      table = GTK_WIDGET
	(g_object_get_data (G_OBJECT(button), "table_widget"));
      sii_widget_foreach(table,
			 sii_links_get_foreach,
			 (gpointer)li );
      switch (li->widget_id) {
       case FRAME_SWPFI_LINKS:
       case FRAME_LOCKSTEP_LINKS:
	 sii_set_swpfi_info (li->frame_num, -1);
	 break;
       case FRAME_VIEW_LINKS:
       case FRAME_CTR_LINKS:
       case FRAME_LMRK_LINKS:

	 sii_update_linked_view_widgets (li->frame_num);
	 if (li->widget_id != FRAME_VIEW_LINKS) {
	   li_type = (li->widget_id == FRAME_CTR_LINKS) ? LI_CENTER : LI_LANDMARK;
	   for (jj=0; jj < maxFrames; jj++) {
	     if (jj != li->frame_num) {
	        sii_view_update_links(jj, li_type);
	     }
	   }
	 }
	 break;
       case FRAME_PARAM_LINKS:
	 sii_set_param_info (li->frame_num);
	 break;
      }

    case LINKS_CANCEL:
      if( widget = sii_get_widget_ptr (li->frame_num, li->widget_id))
	{ gtk_widget_set_visible (widget, FALSE); }
      break;

    case LINKS_CLEAR_ALL:
    case LINKS_SET_ALL:
      active = (task == LINKS_SET_ALL);
      table = GTK_WIDGET
	(g_object_get_data (G_OBJECT(button), "table_widget"));
      sii_widget_foreach(table,
			 sii_links_set_foreach,
			 (gpointer)active );
      break;

   };
}

/* c---------------------------------------------------------------------- */

void sii_links_cb ( GtkWidget *w, gpointer   data )
{
   GtkWidget *button = GTK_WIDGET (data);
   guint button_num = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT(button)
			     , "button_num"));
   LinksInfo *li = (LinksInfo *)g_object_get_data (G_OBJECT(button)
			     , "link_info");

}

/* c---------------------------------------------------------------------- */

void sii_links_widget( guint frame_num, guint widget_id, LinksInfo *linkfo )
{
  GtkWidget *window;
  GtkWidget *vbox0;
  GtkWidget *hbox0;
  GtkWidget *button;
  GtkWidget *table;

  guint xpadding = 0;
  guint ypadding = 0;
  GdkPoint *ptp;
  gint x, y, row, col, cols, jj, kk, nn, button_num, mark;
  gchar *bb, str[16];


  window = gtk_window_new();
  sii_set_widget_ptr ( frame_num, widget_id, window );
  ptp = &frame_configs[frame_num]->widget_origin[FRAME_MENU];
  x = ptp->x; y = ptp->y;
  /* gtk_widget_set_uposition removed for GTK4 */

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_widget_destroyed_cb),
		      &window);

  g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(sii_nullify_widget_cb),
		      (gpointer)(frame_num*TASK_MODULO+widget_id));

  /* --- Title and border --- */
  bb = g_strdup_printf ("Frame %d  %s", frame_num+1, linkfo->name );
  gtk_window_set_title (GTK_WINDOW (window), bb);
  g_free( bb );
  gtk_widget_set_margin_start (GTK_WIDGET (window), 0);
  gtk_widget_set_margin_end (GTK_WIDGET (window), 0);
  gtk_widget_set_margin_top (GTK_WIDGET (window), 0);
  gtk_widget_set_margin_bottom (GTK_WIDGET (window), 0);

  /* --- Create a new vertical box for storing widgets --- */
  vbox0 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child (GTK_WINDOW(window), vbox0);

  linkfo->table =
    table = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID(table), TRUE);
  gtk_grid_set_row_homogeneous (GTK_GRID(table), TRUE);
  gtk_box_append (GTK_BOX(vbox0), table);
  g_object_set_data (G_OBJECT(window)
		       , "table_widget", (gpointer)table);

  xpadding = ypadding = 2;
  button_num = 0;

  for (row=0; row < maxConfigRows-1; row++ ) {
     for (col=0; col < maxConfigCols; col++, button_num++ ) {

	sprintf (str, "F%d", button_num+1 );
	button = gtk_toggle_button_new_with_label (str);
	gtk_widget_set_size_request (button, 0, 40);
	gtk_widget_set_hexpand (button, TRUE);
	gtk_widget_set_vexpand (button, TRUE);
	gtk_widget_set_margin_start (button, xpadding);
	gtk_widget_set_margin_end (button, xpadding);
	gtk_widget_set_margin_top (button, ypadding);
	gtk_widget_set_margin_bottom (button, ypadding);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button)
				      , linkfo->link_set[button_num]);

	gtk_grid_attach (GTK_GRID (table), button
			  , col, row, 1, 1);

	g_signal_connect (G_OBJECT(button)
		      ,"toggled"
		      , G_CALLBACK (sii_links_cb)
		      , (gpointer)button
		      );
	g_object_set_data (G_OBJECT(button)
			     , "button_num", (gpointer)(button_num));
	g_object_set_data (G_OBJECT(button)
			     , "link_info", (gpointer)linkfo);
     }
  }


  hbox0 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append (GTK_BOX(vbox0), hbox0);


  button = gtk_button_new_with_label ("Clear All");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num*TASK_MODULO + LINKS_CLEAR_ALL;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_links_widget_cb)
		      , (gpointer)button
		      );
  g_object_set_data (G_OBJECT(button)
		       , "link_info", (gpointer)linkfo);
  g_object_set_data (G_OBJECT(button)
		       , "button_wid", (gpointer)LINKS_CLEAR_ALL);
  g_object_set_data (G_OBJECT(button)
		       , "table_widget", (gpointer)table);

  button = gtk_button_new_with_label ("Set All");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num*TASK_MODULO + LINKS_SET_ALL;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_links_widget_cb)
		      , (gpointer)button
		      );
  g_object_set_data (G_OBJECT(button)
		       , "link_info", (gpointer)linkfo);
  g_object_set_data (G_OBJECT(button)
		       , "button_wid", (gpointer)LINKS_SET_ALL);
  g_object_set_data (G_OBJECT(button)
		       , "table_widget", (gpointer)table);


  button = gtk_button_new_with_label ("OK");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num*TASK_MODULO + LINKS_OK;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_links_widget_cb)
		      , (gpointer)button
		      );
  g_object_set_data (G_OBJECT(button)
		       , "link_info", (gpointer)linkfo);
  g_object_set_data (G_OBJECT(button)
		       , "button_wid", (gpointer)LINKS_OK);
  g_object_set_data (G_OBJECT(button)
		       , "table_widget", (gpointer)table);

  button = gtk_button_new_with_label ("Cancel");
  gtk_widget_set_hexpand (button, TRUE);
  gtk_box_append (GTK_BOX (hbox0), button);
  nn = frame_num*TASK_MODULO + LINKS_CANCEL;
  g_signal_connect (G_OBJECT(button)
		      ,"clicked"
		      , G_CALLBACK (sii_links_widget_cb)
		      , (gpointer)button
		      );
  g_object_set_data (G_OBJECT(button)
		       , "link_info", (gpointer)linkfo);
  g_object_set_data (G_OBJECT(button)
		       , "button_wid", (gpointer)LINKS_CANCEL);


  /* --- Make everything visible --- */

  gtk_widget_set_visible (window, TRUE);
  mark = 0;

}

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */
