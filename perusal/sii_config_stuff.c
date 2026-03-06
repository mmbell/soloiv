/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"

static gchar *def_param_names[] =
{ "DZ", "VE", "NCP", "SW", "DM", "ZDR", "LDR", "PHIDP", "RHOHV", "DX",
    "KDP", "PD", "DCZ", "DZ", "VE", "NCP", "SW", "", "", "", "", };

/* c---------------------------------------------------------------------- */

void sii_initialize_parameter (guint frame_num, gchar *name);
void sii_initialize_view (guint frame_num);
void sii_frame_draw_func(GtkDrawingArea *area, cairo_t *cr,
			 int width, int height, gpointer data);
void sii_mouse_click_cb(GtkGestureClick *gesture, int n_press,
			double x, double y, gpointer data);
void sii_focus_in_cb(GtkEventControllerFocus *controller,
		     gpointer data);
void sii_focus_out_cb(GtkEventControllerFocus *controller,
		      gpointer data);
void sii_enter_cb(GtkEventControllerMotion *controller,
		  double x, double y, gpointer data);
void sii_leave_cb(GtkEventControllerMotion *controller,
		  gpointer data);
gboolean sii_frame_key_pressed_cb(GtkEventControllerKey *controller,
				  guint keyval, guint keycode,
				  GdkModifierType state, gpointer data);
void sii_new_frames ();

void sii_check_def_widget_sizes ();

SiiLinkedList *sii_init_linked_list (guint length);

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */

void sii_check_for_illegal_shrink ()
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

void sii_check_def_widget_sizes ()
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

void sii_init_frame_configs()
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

void sii_reset_config_cells()
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

void sii_new_frames ()
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
    sprintf (str, "new_frame %d  %dx%d", fn, width, height );
    sii_append_debug_stuff (str);

    sfc->width = sfc->data_width = width;
    sfc->height = sfc->data_height = height;
    sii_check_image_size (fn);

    gtk_widget_set_size_request (frame, width, height);

    /* --- Draw function --- */
    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (frame),
				    sii_frame_draw_func, GINT_TO_POINTER(fn), NULL);

    /* --- GtkGestureClick for button press --- */
    gesture = gtk_gesture_click_new ();
    g_signal_connect (gesture, "pressed",
		      G_CALLBACK (sii_mouse_click_cb), GINT_TO_POINTER(fn));
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

  }
   gtk_widget_set_visible (main_window, TRUE);
   nn = 0;
}

/* c---------------------------------------------------------------------- */

gboolean sii_set_config()
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
