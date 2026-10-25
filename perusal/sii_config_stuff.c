/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include <sii_enums.h>

static gchar *def_param_names[] =
{ "DZ", "VE", "NCP", "SW", "DM", "ZDR", "LDR", "PHIDP", "RHOHV", "DX",
    "KDP", "PD", "DCZ", "DZ", "VE", "NCP", "SW", "", "", "", "", };

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

void sii_check_for_illegal_shrink(void)
{
  SiiFrameConfig *sfc = frame_configs[0];
  sii_table_parameters *stp;
  gint jj, mm;
  gdouble d;

  for (jj=0; jj < sii_return_frame_count(); jj++) {
    if (frame_configs[jj]->reconfig_count)
      { return; }
    /* don't do anything in the middle of a reconfig */
  }
  stp = &sfc->tbl_parms;
  mm = stp->right_attach - stp->left_attach;
  d = (gdouble)sfc->width/mm;
  if (d < sii_table_widget_width)
    { sii_new_frames (); }
}

/* c---------------------------------------------------------------------- */

void sii_check_def_widget_sizes(void)
{
  int nn;

  if (sii_table_widget_width < DEFAULT_WIDTH)
    { sii_table_widget_width = DEFAULT_WIDTH; }

  if (sii_table_widget_height < DEFAULT_HEIGHT)
    { sii_table_widget_height = DEFAULT_HEIGHT; }

  nn = (sii_table_widget_width -1)/4 +1;
  sii_table_widget_width = nn * 4;

  nn = (sii_table_widget_height -1)/4 +1;
  sii_table_widget_height = nn * 4;
}

/* c---------------------------------------------------------------------- */

void sii_init_frame_configs(void)
{
  int fn, jj, kk;
  GString *gstr = NULL;
  SiiLinkedList *sll;

  if (gstr == NULL)
    { gstr = g_string_new (""); }

  for( fn = 0; fn < maxFrames; fn++ ) {
    frame_configs[fn] = (SiiFrameConfig *)
      g_malloc0( sizeof( SiiFrameConfig ));
    frame_configs[fn]->frame_num = fn;
    frame_configs[fn]->data_widget_text = g_string_new ("");

    sii_initialize_parameter (fn, def_param_names[fn]);
    sii_initialize_view (fn);

    g_string_append_printf (gstr, "%s\n", def_param_names[fn]);

  }
  for( fn = 0; fn < maxFrames; fn++ ) {
     sii_set_param_names_list (fn, gstr, maxFrames);
  }
  g_string_free (gstr, TRUE);	/* free string struct and char data */
}

/* c---------------------------------------------------------------------- */

void sii_reset_config_cells(void)
{
  int cn;

  if( !config_cells ) {		/* allocate them */

    config_cells = (sii_config_cell **)
      g_malloc0( maxConfigCells * sizeof( sii_config_cell *));

    for( cn = 0; cn < maxConfigCells; cn++ ) {
      config_cells[cn] = (sii_config_cell *)
	g_malloc0( sizeof( sii_config_cell ));
      config_cells[cn]->cell_num = cn;
      config_cells[cn]->row = cn/maxConfigRows;
      config_cells[cn]->col = cn % maxConfigCols;
    }
  }

  for( cn = 0; cn < maxConfigCells; cn++ ) {
    config_cells[cn]->frame_num = 0;
    config_cells[cn]->processed = no;
  }
}

/* c---------------------------------------------------------------------- */

/*
 * Drag-resize.
 *
 * The frames expand to fill the window, so dragging its edge changes the
 * allocation of the frame table. Each drawing area's "resize" handler only
 * records the table's new size here and (re)starts one debounce timeout;
 * when the allocation has settled sii_apply_table_allocation() divides it
 * across the layout and replots each frame once. Nothing is plotted or
 * rebuilt from the resize or draw callbacks themselves.
 *
 * After sii_new_frames() the new frames have not been plotted yet; their
 * first allocation settles from an idle instead of the timeout.
 */
# define TABLE_SETTLE_MS 150

static guint table_settle_source = 0;
static gint pending_table_width = 0;
static gint pending_table_height = 0;
static gint applied_table_width = 0;
static gint applied_table_height = 0;
static gboolean frames_need_plot = FALSE;

/* c---------------------------------------------------------------------- */

void sii_apply_table_allocation(int width, int height)
{
  SiiFrameConfig *sfc, *sfc0 = frame_configs[0];
  sii_table_parameters *stp;
  guint fn, mm, nn, ncols, nrows, cell_w, cell_h;

  if (width <= 0 || height <= 0 || !sii_frame_count || !sfc0)
    { return; }

  applied_table_width = width;
  applied_table_height = height;

  ncols = (sfc0->ncols) ? sfc0->ncols : 1;
  nrows = (sfc0->nrows) ? sfc0->nrows : 1;

  /* cell size: a multiple of 4, no smaller than the default minimum */
  cell_w = ((guint)width / ncols / 4) * 4;
  cell_h = ((guint)height / nrows / 4) * 4;
  if (cell_w < DEFAULT_WIDTH)
    { cell_w = DEFAULT_WIDTH; }
  if (cell_h < DEFAULT_HEIGHT)
    { cell_h = DEFAULT_HEIGHT; }

  if (!frames_need_plot && cell_w == sii_table_widget_width &&
      cell_h == sii_table_widget_height)
    { return; }

  frames_need_plot = FALSE;
  sii_table_widget_width = cell_w;
  sii_table_widget_height = cell_h;

  /* Resize every frame in place before plotting any of them. The view's
   * center and magnification are untouched, so each frame shows more or
   * less area at the same scale. */
  for (fn = 0; fn < sii_frame_count; fn++) {
    sfc = frame_configs[fn];
    stp = &sfc->tbl_parms;
    mm = stp->right_attach - stp->left_attach;
    nn = stp->bottom_attach - stp->top_attach;

    sfc->width = sfc->data_width = mm * cell_w;
    sfc->height = sfc->data_height = nn * cell_h;
    sfc->config_sync_num = sii_inc_master_seq_num ();
    sii_check_image_size (fn);
    sfc->colorize_count = 0;
    sii_update_frame_info (fn);
  }

  for (fn = 0; fn < sii_frame_count; fn++) {
    sii_plot_data2 (fn, REPLOT_THIS_FRAME);
    if (frame_configs[fn]->frame)
      { gtk_widget_queue_draw (frame_configs[fn]->frame); }
  }
}

/* c---------------------------------------------------------------------- */

gboolean sii_table_settle_cb(gpointer data)
{
  table_settle_source = 0;

  if (solo_busy ()) {		/* a plot is under way; try again later */
    table_settle_source =
      g_timeout_add (TABLE_SETTLE_MS, sii_table_settle_cb, NULL);
    return G_SOURCE_REMOVE;
  }
  sii_apply_table_allocation (pending_table_width, pending_table_height);
  return G_SOURCE_REMOVE;
}

/* c---------------------------------------------------------------------- */

void sii_schedule_table_allocation(int width, int height)
{
  if (width <= 0 || height <= 0)
    { return; }

  pending_table_width = width;
  pending_table_height = height;

  if (table_settle_source) {
    g_source_remove (table_settle_source);
    table_settle_source = 0;
  }
  /* Back at the size already applied (or re-allocated at the same size,
   * e.g. after the window's default size was set to fit): nothing to do. */
  if (!frames_need_plot && width == applied_table_width &&
      height == applied_table_height)
    { return; }

  if (frames_need_plot)
    { table_settle_source = g_idle_add (sii_table_settle_cb, NULL); }
  else
    { table_settle_source =
	g_timeout_add (TABLE_SETTLE_MS, sii_table_settle_cb, NULL); }
}

/* c---------------------------------------------------------------------- */

/* Ask for a window size that fits a width x height frame table. GTK4 on a
 * mapped, resizable window honors a new default size (it resizes the
 * window, growing or shrinking it), which is how the Zoom, Shape and
 * layout menus resize the window. */

static void sii_fit_window_to_table (gint width, gint height)
{
  GtkWidget *child;
  gint min, nat, chrome = 0;

  if (!main_window || !main_vbox)
    { return; }

  /* everything stacked above/below the frame table (the menubar) */
  for (child = gtk_widget_get_first_child (main_vbox); child;
       child = gtk_widget_get_next_sibling (child)) {
    if (child == main_event_box || !gtk_widget_get_visible (child))
      { continue; }
    gtk_widget_measure (child, GTK_ORIENTATION_VERTICAL, -1,
			&min, &nat, NULL, NULL);
    chrome += nat;
  }
  if (gtk_window_is_maximized (GTK_WINDOW (main_window)))
    { gtk_window_unmaximize (GTK_WINDOW (main_window)); }
  gtk_window_set_default_size (GTK_WINDOW (main_window),
			       width, height + chrome);
}

/* c---------------------------------------------------------------------- */

void sii_new_frames(void)
{
   int fn, jj, kk, mm, nn, width, height;
   gboolean nonhomogeneous = FALSE, homogeneous = TRUE;
   SiiFrameConfig *sfc, *sfc0 = frame_configs[0];
   sii_table_parameters *stp;
   GtkWidget *frame;
   gdouble wzoom, hzoom;
   gpointer gptr;
   gchar str[256];

   GtkEventController *controller;
   GtkGesture *gesture;


   /* c...code sii_new_frames */


   if( main_table ) {		/* this is a gtk grid type container
				 */
      gtk_widget_unparent( main_table );
   }
   main_table = gtk_grid_new ();
   gtk_grid_set_row_homogeneous (GTK_GRID (main_table), homogeneous);
   gtk_grid_set_column_homogeneous (GTK_GRID (main_table), homogeneous);
   gtk_box_append (GTK_BOX (main_event_box), main_table);


  for (fn=0; fn < sii_frame_count; fn++ ) {

    sfc = frame_configs[fn];
    sfc->local_reconfig = TRUE;
    sfc->drag_resize_count = 0;
    ++sfc->new_frame_count;

    sfc->frame = gtk_drawing_area_new ();
    frame = sfc->frame;
    stp = &sfc->tbl_parms;
    mm = stp->right_attach - stp->left_attach;
    nn = stp->bottom_attach - stp->top_attach;

    width = mm * sii_table_widget_width;
    height = nn * sii_table_widget_height;
    snprintf (str, sizeof(str), "new_frame %d  %dx%d", fn, width, height );
    sii_append_debug_stuff (str);

    sfc->width = sfc->data_width = width;
    sfc->height = sfc->data_height = height;
    sii_check_image_size (fn);

    /* Only a minimum size: the frames expand to fill the window, and the
     * settled allocation sets their real size (sii_apply_table_allocation). */
    gtk_widget_set_size_request (frame, mm * DEFAULT_WIDTH,
				 nn * DEFAULT_HEIGHT);
    gtk_widget_set_hexpand (frame, TRUE);
    gtk_widget_set_vexpand (frame, TRUE);

    /* --- Draw function --- */
    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (frame),
				    sii_frame_draw_func, GINT_TO_POINTER(fn), NULL);

    /* --- Allocation changes: record and debounce, never plot here --- */
    g_signal_connect (frame, "resize",
		      G_CALLBACK (sii_frame_resize_cb), GINT_TO_POINTER(fn));

    /* --- GtkGestureClick for button press --- */
    gesture = gtk_gesture_click_new ();
    g_signal_connect (gesture, "pressed",
		      G_CALLBACK (sii_mouse_click_cb), GINT_TO_POINTER(fn));
    gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), 0); /* all buttons */
    gtk_widget_add_controller (frame, GTK_EVENT_CONTROLLER (gesture));

    /* --- GtkEventControllerKey for key press --- */
    controller = gtk_event_controller_key_new ();
    g_signal_connect (controller, "key-pressed",
		      G_CALLBACK (sii_frame_key_pressed_cb), GINT_TO_POINTER(fn));
    gtk_widget_add_controller (frame, controller);

    /* --- GtkEventControllerMotion for enter/leave --- */
    controller = gtk_event_controller_motion_new ();
    g_signal_connect (controller, "enter",
		      G_CALLBACK (sii_enter_cb), GINT_TO_POINTER(fn));
    g_signal_connect (controller, "leave",
		      G_CALLBACK (sii_leave_cb), GINT_TO_POINTER(fn));
    gtk_widget_add_controller (frame, controller);

    /* --- GtkEventControllerFocus for focus in/out --- */
    controller = gtk_event_controller_focus_new ();
    g_signal_connect (controller, "enter",
		      G_CALLBACK (sii_focus_in_cb), GINT_TO_POINTER(fn));
    g_signal_connect (controller, "leave",
		      G_CALLBACK (sii_focus_out_cb), GINT_TO_POINTER(fn));
    gtk_widget_add_controller (frame, controller);

    gtk_grid_attach (GTK_GRID (main_table), frame
		      , stp->left_attach
		      , stp->top_attach
		      , stp->right_attach - stp->left_attach
		      , stp->bottom_attach - stp->top_attach
		      );
    gtk_widget_set_visible (frame, TRUE);

  }
   gtk_widget_set_visible (main_table, TRUE);

   /* The new frames are plotted once their first allocation settles. */
   frames_need_plot = TRUE;
   applied_table_width = applied_table_height = 0;
   if (table_settle_source) {
      g_source_remove (table_settle_source);
      table_settle_source = 0;
   }
   sii_fit_window_to_table (MAX (sfc0->ncols, 1) * sii_table_widget_width,
			    MAX (sfc0->nrows, 1) * sii_table_widget_height);
   gtk_widget_set_visible (main_window, TRUE);
}

/* c---------------------------------------------------------------------- */

gboolean sii_set_config(void)
{
  int cll, cll2, jj, kk, ll, mm, nn, mark, frame, row, col;
  int row2, col2, ncols = 1, nrows = 1;
  int frame_count = 0;
  sii_config_cell * scc, *scc2;
  SiiFrameConfig *sfc;
  sii_table_parameters tbl_parms[maxFrames], *stp;

				/* start in the upper left hand corner */
				/* moving accross the columns in each row */

  for( cll = 0; cll < maxConfigCells; cll++ ) {
    scc = config_cells[cll];

    if( !scc->frame_num )
      { continue; }
    if(  scc->processed )
      { continue; }
				/* at next cell to process */
    stp = &tbl_parms[frame_count++];
    scc->processed = yes;
    frame = scc->frame_num;	/* 1-12 */
    row = row2 = cll/maxConfigRows;
    col = col2 = cll % maxConfigCols;

    stp->left_attach = col;
    stp->right_attach = col +1;
    stp->top_attach = row;
    stp->bottom_attach = row +1;
    if( stp->bottom_attach > nrows )
      { nrows = stp->bottom_attach; }
    if( stp->right_attach > ncols )
      { ncols = stp->right_attach; }

				/* see if this number appears again */
    for( jj = cll+1; jj < maxConfigCells; jj++ ) {
      if( config_cells[jj]->frame_num != frame || config_cells[jj]->processed )
	{ continue; }

      if( jj/maxConfigRows > row2 )
	{ row2 = jj/maxConfigRows; }

      if( jj % maxConfigCols > col2 )
	{ col2 = jj % maxConfigCols; }
    }

    if( row2 > row || col2 > col ) {

      for( ; row <= row2; row++ ) {
	if( row +1 > stp->bottom_attach ) {
	   stp->bottom_attach = row+1;
	   if( stp->bottom_attach > nrows )
	     { nrows = stp->bottom_attach; }
	}

	for( jj = col; jj <= col2; jj++ ) {
	  cll2 = row * maxConfigRows + jj;
	  config_cells[cll2]->processed = yes;

	  if( jj +1 > stp->right_attach ) {
	     stp->right_attach = jj+1;
	     if( stp->right_attach > ncols )
	       { ncols = stp->right_attach; }
	  }
	}
      }
      mark = 0;
    } /* row2 > row || col2 > col */
  } /* for( cll = 0; cll < maxConfigCells; */

  if( frame_count > 0 ) {
     for( jj = 0; jj < frame_count; jj++ ) {

	mm = tbl_parms[jj].right_attach - tbl_parms[jj].left_attach;
	nn = tbl_parms[jj].bottom_attach - tbl_parms[jj].top_attach;

	frame_configs[jj]->tbl_parms = tbl_parms[jj];
	frame_configs[jj]->ncols = ncols;
	frame_configs[jj]->nrows = nrows;
	frame_configs[jj]->local_reconfig = TRUE;
	frame_configs[jj]->reconfig_count = 0;

	/* Fix this! */
	frame_configs[jj]->lock_step = jj;
     }
     sii_frame_count = frame_count;
     return( yes );
  }

  return( no );
}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */
