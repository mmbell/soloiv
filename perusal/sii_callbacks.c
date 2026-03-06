/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include "sii_enums.h"
# include <gdk/gdk.h>
# define config_debug

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

/* GTK4: expose_event -> GtkDrawingArea draw function */
void sii_frame_draw_func(GtkDrawingArea *area, cairo_t *cr,
			 int width, int height, gpointer data);
/* GTK4: configure_event -> resize signal handler */
void sii_frame_resize_cb(GtkWidget *frame, int width, int height,
			 gpointer data);
/* GTK4: focus events -> GtkEventControllerFocus */
void sii_focus_in_cb(GtkEventControllerFocus *controller, gpointer data);
void sii_focus_out_cb(GtkEventControllerFocus *controller, gpointer data);
/* GTK4: enter/leave -> GtkEventControllerMotion */
void sii_enter_cb(GtkEventControllerMotion *controller, double x, double y,
		  gpointer data);
void sii_leave_cb(GtkEventControllerMotion *controller, gpointer data);
void checkup_cb( GtkWidget *w, gpointer   data );

void sii_plot_data (guint frame_num, guint plot_function);

void solo_set_halt_flag();

void sii_set_geo_coords ( int frame_num, gdouble dx, gdouble dy);

void sii_check_def_widget_sizes ();

void sii_dump_debug_stuff();

guint sii_inc_master_seq_num ();
guint sii_get_master_seq_num ();

/* c---------------------------------------------------------------------- */

void center_frame_cb( GtkWidget *w, gpointer data )
{
   guint nn = GPOINTER_TO_UINT (data);

   if (nn < 1) {
     sii_default_center (-1);	/* do every frame */
     return;
   }
   sii_center_on_clicks (nn);
}

/* c---------------------------------------------------------------------- */

/* GTK4: blow up expose -> draw function for GtkDrawingArea */
void sii_blow_up_draw_func (GtkDrawingArea *area, cairo_t *cr,
			    int width, int height, gpointer data)
{
   GtkWidget *frame = GTK_WIDGET(area);
   guint frame_num = GPOINTER_TO_UINT (data); /* frame number */
   SiiFrameConfig *sfc, *sfc0;
   guint ix=0, iy=0, ncols, nrows, cn, mm, nn, jj;
   sii_table_parameters *stp;
   gboolean reconfigured, uncovered, totally_exposed;
   static gboolean new_frames = FALSE;
   GdkRectangle area_rect;
   gchar *aa, str[256], mess[256];

  frame_num = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT(frame), "frame_num" ));

   sfc = frame_configs[frame_num];
   sfc0 = frame_configs[0];
   ++sfc0->big_expose_count;

   /* GTK4: use alloc_width/alloc_height instead of widget->allocation */
   gint alloc_w = sfc0->alloc_width;
   gint alloc_h = sfc0->alloc_height;

   /* In GTK4 draw function, width/height are passed as parameters.
    * The "exposed area" is the full widget in draw functions. */
   area_rect.x = 0;
   area_rect.y = 0;
   area_rect.width = width;
   area_rect.height = height;

   reconfigured = alloc_w != (gint)sfc0->big_width ||
      alloc_h != (gint)sfc0->big_height;

   reconfigured = sfc0->big_reconfig_count > 0;

   uncovered = alloc_w != width ||
     alloc_h != height;

   totally_exposed = alloc_w == width &&
     alloc_h == height;


   *str = '\0';
   if (reconfigured) {
      strcat (str, "reconfig ");
   }
   if (uncovered) {
     strcat (str, "uncovered ");
   }
   if (totally_exposed) {
     strcat (str, "totally ");
   }
   if (!(reconfigured || uncovered || totally_exposed)) {
     strcat (str, "neither ");
   }

# ifdef config_debug
   sprintf
     (mess,"bup expose: %s frm:%d gtk:%dx%d sii:%dx%d exp:%dx:%d rc:%d xp:%d"
	      , str
	      , frame_num
	      , alloc_w
	      , alloc_h
	      , sfc0->big_width, sfc0->big_height
	      , width, height
	      , sfc0->big_reconfig_count
	      , sfc0->big_expose_count
	      );
   sii_append_debug_stuff (mess);
# endif
   sfc0->big_reconfig_count = 0;

			/*
			 */
   if (!reconfigured) {
   }
     sii_really_xfer_images (frame_num, &area_rect, TRUE);
}

/* c---------------------------------------------------------------------- */

/* GTK4: frame expose -> draw function for GtkDrawingArea */
void sii_frame_draw_func(GtkDrawingArea *area, cairo_t *cr,
			 int width, int height, gpointer data)
{
			/* c...expose */
   GtkWidget *frame = GTK_WIDGET(area);
   guint frame_num = GPOINTER_TO_UINT (data); /* frame number */
   SiiFrameConfig *sfc;
   guint ix=0, iy=0, ncols, nrows, cn, mm, nn, jj, ndx;
   gint mark;
   sii_table_parameters *stp;
   gboolean reconfigured, uncovered, totally_exposed, second_expose;
   static gboolean new_frames = FALSE;
   GdkRectangle area_rect;
   gchar mess[256];
   static GString *gs=NULL;


   if(!gs)
     { gs = g_string_new (""); }
   g_string_truncate (gs, 0);
   g_string_append (gs, "XPZ");

   sfc = frame_configs[frame_num];
   ++sfc->expose_count;

   /* GTK4: use alloc_width/alloc_height instead of widget->allocation */
   gint alloc_w = sfc->alloc_width;
   gint alloc_h = sfc->alloc_height;

   /* In GTK4 draw function, width/height are the widget dimensions.
    * The "exposed area" is the full widget. */
   area_rect.x = 0;
   area_rect.y = 0;
   area_rect.width = width;
   area_rect.height = height;

   reconfigured = sfc->colorize_count == 0;
   if (reconfigured)
     { g_string_append (gs, " REC"); }

   uncovered = alloc_w != width ||
     alloc_h != height;
   if (uncovered)
     { g_string_append (gs, " UNC"); }

   totally_exposed = alloc_w == width &&
     alloc_h == height;
   if (totally_exposed)
     { g_string_append (gs, " TOT"); }

   sfc->most_recent_expose = area_rect;

   g_string_append_printf
     (gs, " frm:%d frm:%dx%d  gtk:%dx%d  ec:%d cc:%d nf:%d lc:%d"
      , frame_num
      , sfc->width, sfc->height
      , alloc_w
      , alloc_h
      , sfc->expose_count
      , sfc->reconfig_count
      , sfc->new_frame_count
      , sfc->local_reconfig
      );
# ifdef obsolete
     g_string_append_printf
       (gs, "\n  x:%d y:%d %dx%d"
	, area_rect.x, area_rect.y
	, area_rect.width, area_rect.height
	);
# endif
   second_expose = sfc->config_sync_num == sfc->expose_sync_num;
  sfc->expose_sync_num = sfc->config_sync_num;

   if (reconfigured) {
     mark = 0;
   }
   else if (uncovered) {
     sii_xfer_images (frame_num, &area_rect);
   }
   else if (!sfc->reconfig_count && totally_exposed) {
      sii_xfer_images (frame_num, &area_rect);
   }

   /* the reconfig_count is reset once it's been plotted by sii_displayq() */

   while (sfc->reconfig_count) {

      if (!sfc->local_reconfig && (sfc->reconfig_flag & FrameDragResize)) {
	 /*
	  * This requires no reconfiguration other than increasing
	  * the image size if the frame is bigger
	  *
	  */
	 if (!second_expose) {
	    ndx = sfc->cfg_que_ndx;
	    stp = &sfc->tbl_parms;
	    mm = stp->right_attach - stp->left_attach;
	    nn = stp->bottom_attach - stp->top_attach;
	    sfc->width = sfc->cfg_width[ndx];
	    sfc->height = sfc->cfg_height[ndx];
	    sii_check_image_size (frame_num);
	    if (mm == 1) {
	       sii_table_widget_width = sfc->width;
	    }
	    if (nn == 1) {
	       sii_table_widget_height = sfc->height;
	    }
	    sii_check_def_widget_sizes ();

	    sfc->sync_num = sfc->config_sync_num;
	    g_string_append (gs, " X0");
	    sii_plot_data2 (frame_num, REPLOT_THIS_FRAME);
	 }
	 else
	   { g_string_append (gs, " X1"); }
	 break;
      }

     if (sfc->local_reconfig && sfc->expose_count == sfc->reconfig_count)
       {
	 /* reconfig_count == 1 for first time through
	  * other local reconfigs emit two exposes
	  * one for the new drawable and one for the config
	  */
	 sfc->width = alloc_w;
	 sfc->height = alloc_h;
	 sfc->data_width = alloc_w;
	 sfc->data_height = alloc_h;
	 sfc->local_reconfig = FALSE;
	 g_string_append (gs, " X2");
	 sfc->sync_num = sfc->config_sync_num;
	 sii_plot_data (frame_num, REPLOT_THIS_FRAME);
	 break;
       }

     if (sfc->expose_count == 1)
       {
	 /* We've rebuilt the tables and this is the last expose,
	  * now plot the data
	  */
	 g_string_append (gs, " X3");
	 sfc->sync_num = sfc->config_sync_num;
	 sii_plot_data (frame_num, REPLOT_THIS_FRAME);
	 break;
       }

# ifdef config_debug
     g_string_append (gs, " X4");
# endif
     break;
   }
# ifdef config_debug
   /*
   printf ("%s\n", gs->str);
    */
   sii_append_debug_stuff (gs->str);
# endif

}

/* c---------------------------------------------------------------------- */

/* GTK4: blow up configure -> resize signal */
void sii_blow_up_resize_cb(GtkWidget *frame, int width, int height,
			   gpointer data)
{
			/* c...config */
  guint frame_num = GPOINTER_TO_UINT (data), mark;
  gchar mess[256];
  gdouble d;

  frame_num = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT(frame), "frame_num" ));

  ++frame_configs[0]->big_reconfig_count;
  frame_configs[0]->big_expose_count = 0;
  frame_configs[0]->big_colorize_count = 0;

# ifdef config_debug
  sprintf (mess,"bup configure frm %d %dx%d"
	     , frame_num
	     , width
	     , height
	     );
	 sii_append_debug_stuff (mess);
# endif
}

/* c---------------------------------------------------------------------- */

/* GTK4: frame configure -> resize signal */
void sii_frame_resize_cb(GtkWidget *frame, int width, int height,
			 gpointer data)
{
			/* c...config */
  guint frame_num = GPOINTER_TO_UINT (data), mark, ndx;
  gchar *bb, mess[256];
  gdouble d;
  SiiFrameConfig *sfc;

  strcpy (mess, " ");
  sfc = frame_configs[frame_num];
  sfc->config_sync_num = sii_inc_master_seq_num ();
  ++sfc->reconfig_count;
  sfc->colorize_count = 0;

  ndx = (sfc->cfg_que_ndx +1) % CFG_QUE_SIZE;
  sfc->cfg_que_ndx = ndx;
  /* GTK4: width/height passed as parameters to resize callback */
  sfc->cfg_width[ndx] = width;
  sfc->cfg_height[ndx] = height;
  /* Also update alloc_width/alloc_height */
  sfc->alloc_width = width;
  sfc->alloc_height = height;
  if (!sfc->local_reconfig) {
     sfc->reconfig_flag |= FrameDragResize;
     ++sfc->drag_resize_count;
  }

# ifdef config_debug
  sprintf (mess, "CFG frm:%d %dx%d gtk:%dx%d  ec:%d cc:%d nf:%d"
	     , frame_num
	     , sfc->width
	     , sfc->height
	     , width
	     , height
	     , sfc->expose_count
	     , sfc->reconfig_count
	     , sfc->new_frame_count
	     );
  sii_append_debug_stuff (mess);
  /*
  printf ("%s\n", mess);
   */
# endif
  sfc->expose_count = 0;
}

/* c---------------------------------------------------------------------- */

/* GTK4: enter_notify_event -> GtkEventControllerMotion enter callback */
void sii_enter_cb(GtkEventControllerMotion *controller, double x, double y,
		  gpointer data)
{
  guint frame_num = GPOINTER_TO_UINT (data), mark;
  cursor_frame = frame_num +1;
			/*
  g_message ("frame %d enter", frame_num );
			 */
}

/* c---------------------------------------------------------------------- */

/* GTK4: leave_notify_event -> GtkEventControllerMotion leave callback */
void sii_leave_cb(GtkEventControllerMotion *controller, gpointer data)
{
  guint frame_num = GPOINTER_TO_UINT (data), mark;
  cursor_frame = 0;
# ifdef obsolete
  g_message ("frame %d leave", frame_num );
# endif
}

/* c---------------------------------------------------------------------- */

/* GTK4: button_press_event -> GtkGestureClick callback */
void sii_blow_up_mouse_click_cb (GtkGestureClick *gesture, int n_press,
				 double x, double y, gpointer data)
{
  GtkWidget *frame = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER(gesture));
  guint frame_num = GPOINTER_TO_UINT (data), mark, width, height;
  gint jj;
  gboolean the_same;
  gint xx, yy;
  SiiPoint pt, *prev_pt;
  gchar str[16], *aa, mess[256];
  gboolean double_click, triple_click;
  gchar *clicks = "";
  gdouble dx, dy, d_xx, d_yy, d_zz;
  SiiFrameConfig *sfc;
  SiiFrameConfig *sfc0;
  guint button;


  frame_num = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT(frame), "frame_num" ));

  /* GTK4: x, y are passed directly as doubles */
  xx = (gint)x;
  yy = (gint)y;

  /* GTK4: get which button from the gesture */
  button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE(gesture));

# ifdef config_debug
  sprintf (mess, "xx:%d yy:%d button:%d", xx, yy, button);
  sii_append_debug_stuff (mess);
# endif

  xx /= 2;
  yy /= 2;
  sfc = frame_configs[frame_num];
  sfc0 = frame_configs[0];

  d_xx = sfc->ulc_radar.xx;
  d_yy = sfc->ulc_radar.yy;
  sfc->click_loc.x = xx;
  sfc->click_loc.y = yy;
  width = sfc0->data_width;
  height = sfc0->data_height;



  double_click = (n_press == 2);
  triple_click = (n_press == 3);

  /* set offsets from the center of the frame
   */
  dx = xx -.5 * width;
  dy = (height -1 -yy) -.5 * height;

  *str = '\0';
  pt.button_mask = 0;

  if( double_click || button == 1 || button == 2) {
    if (double_click || button == 2)
      { pt.button_mask |= GDK_BUTTON2_MASK; }
    else
      { pt.button_mask |= GDK_BUTTON1_MASK; }

    /* send to editor which should ignore it if
     * not shown or the suspend boundary definition
     * flag is set ("No BND" button)
       void sii_editor_data_click (SiiPoint *point);
     */
    strcat( str, (button == 1) ? "B1 ": "B2 ");

    pt.frame_num = frame_num;
    pt.x = xx;
    pt.y = yy;
    pt.dtime = pt.lat = pt.lon = pt.alt = 0;
    the_same = 0;
    if (clicks_in_que) {
      prev_pt = (SiiPoint *)sii_click_que->data;
      the_same = (memcmp (prev_pt, &pt, sizeof (SiiPoint)) == 0);
    }
    if (!the_same)
      { insert_clicks_que (&pt); }

    /* set the geographical coordinates of the click
     */
    sii_set_geo_coords ((int)frame_num, dx, dy);
  }

  if( button == 3 ) {
    strcat( str, "B3 " );
    pt.button_mask |= GDK_BUTTON3_MASK;
  }

   if( button == 3 ) {
      jj = FRAME_MENU;
      show_frame_menu ( frame, data );
   }

}

/* c---------------------------------------------------------------------- */

/* GTK4: button_press_event -> GtkGestureClick callback */
void sii_mouse_click_cb (GtkGestureClick *gesture, int n_press,
			 double x, double y, gpointer data)
{
  GtkWidget *frame = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER(gesture));
  guint frame_num = GPOINTER_TO_UINT (data), mark, width, height;
  gint jj;
  gboolean the_same;
  gint xx, yy;
  SiiPoint pt, *prev_pt;
  gchar str[16], *aa;
  gboolean double_click, triple_click;
  gchar *clicks = "";
  gdouble dx, dy, d_xx, d_yy;
  guint button;

  /* GTK4: x, y are passed directly as doubles */
  xx = (gint)x;
  yy = (gint)y;

  /* GTK4: get which button from the gesture */
  button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE(gesture));

  d_xx = frame_configs[frame_num]->ulc_radar.xx;
  d_yy = frame_configs[frame_num]->ulc_radar.yy;
  frame_configs[frame_num]->click_loc.x = xx;
  frame_configs[frame_num]->click_loc.y = yy;
  width = frame_configs[frame_num]->data_width;
  height = frame_configs[frame_num]->data_height;

  /* set offsets from the center of the frame
   */
  dx = xx -.5 * width;
  dy = (height -1 -yy) -.5 * height;
  g_string_printf (gs_complaints,
"click:%dx%d(%d,%d)(%.4f,%.4f)ulcR:(%.4f,%.4f)"
		    , width, height, xx, yy, dx, dy, d_xx, d_yy
		    );

  double_click = (n_press == 2);
  if (double_click)
    { clicks = "double"; }
  triple_click = (n_press == 3);
  if (triple_click)
    { clicks = "triple"; }

  *str = '\0';
  pt.button_mask = 0;

  if( double_click || button == 1 || button == 2) {
    if (double_click || button == 2)
      { pt.button_mask |= GDK_BUTTON2_MASK; }
    else
      { pt.button_mask |= GDK_BUTTON1_MASK; }

    /* send to editor which should ignore it if
     * not shown or the suspend boundary definition
     * flag is set ("No BND" button)
       void sii_editor_data_click (SiiPoint *point);
     */
    strcat( str, (button == 1) ? "B1 ": "B2 ");

    pt.frame_num = frame_num;
    pt.x = xx;
    pt.y = yy;
    pt.dtime = pt.lat = pt.lon = pt.alt = 0;
    the_same = 0;
    if (clicks_in_que) {
      prev_pt = (SiiPoint *)sii_click_que->data;
      the_same = (memcmp (prev_pt, &pt, sizeof (SiiPoint)) == 0);
# ifdef notyet
      g_message ("Prev x:%d  y:%d %d", prev_pt->x, prev_pt->y, the_same);
# endif
    }
    if (!the_same)
      { insert_clicks_que (&pt); }

    /* set the geographical coordinates of the click
     */
    sii_set_geo_coords ((int)frame_num, dx, dy);
  }

  if( button == 3 ) {
    strcat( str, "B3 " );
    pt.button_mask |= GDK_BUTTON3_MASK;
  }

# ifdef notyet
  g_message ("frame %d clicked in at %d %d %d %s %d %x %s"
	     , frame_num
	     , xx, yy, button, clicks, the_same, pt.button_mask
	     , (strlen(str)) ? str : "?");
# endif
   if( button == 3 ) {
      jj = FRAME_MENU;
      show_frame_menu ( frame, data );
   }


}

/* c---------------------------------------------------------------------- */

/* GTK4: key_press_event -> GtkEventControllerKey callback */
gboolean sii_blow_up_key_pressed_cb(GtkEventControllerKey *controller,
				    guint keyval, guint keycode,
				    GdkModifierType state, gpointer data)
{
  GtkWidget *frame = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER(controller));
  gchar *aa, *bb, *mod = "nomod";
  guint frame_num = GPOINTER_TO_UINT (data), mark, jj;
  guint F_frame_id = 0;
  gboolean modifier_only = FALSE;
  gboolean modified = FALSE;
  gboolean plot_command = FALSE;

  frame_num = GPOINTER_TO_UINT
    (g_object_get_data (G_OBJECT(frame), "frame_num" ));

  modified = (state & GDK_CONTROL_MASK || state & GDK_SHIFT_MASK);

  switch( keyval ) {

  case GDK_KEY_T:
  case GDK_KEY_t:
    sii_param_toggle_field (frame_num);
    break;

  case GDK_KEY_D:
  case GDK_KEY_d:
    sii_edit_zap_last_bnd_point();
    break;

  case GDK_KEY_Control_L:
  case GDK_KEY_Control_R:
  case GDK_KEY_Shift_L:
  case GDK_KEY_Shift_R:
    /* Lone modifier key. Ignore */
    modifier_only = TRUE;
    break;

  case GDK_KEY_F1:
  case GDK_KEY_F2:
  case GDK_KEY_F3:
  case GDK_KEY_F4:
  case GDK_KEY_F5:
  case GDK_KEY_F6:
  case GDK_KEY_F7:
  case GDK_KEY_F8:
  case GDK_KEY_F9:
  case GDK_KEY_F10:
  case GDK_KEY_F11:
  case GDK_KEY_F12:
    /* Function buttons */
    F_frame_id = keyval - GDK_KEY_F1 +1;
# ifdef notyet
    g_message( "bup Caught F key:%d frm:%d", F_frame_id, frame_num );
# endif
    break;

  case 268828432:		/* SunF36 when you punch F11 on Sun Solaris */
    g_message ("bup Caught funky SunF36 for F11");
    F_frame_id = 11;
    break;

  case 268828433:		/* SunF37 when you punch F12 on Sun Solaris */
    g_message ("bup Caught funky SunF37 for F12");
    F_frame_id = 12;
    break;
  };

  if (F_frame_id > 0)
    { sii_blow_up (F_frame_id-1); }

  return (frame_num > 2 * maxFrames) ? FALSE : TRUE;
}

/* c---------------------------------------------------------------------- */

/* GTK4: key_press_event -> GtkEventControllerKey callback */
gboolean sii_frame_key_pressed_cb(GtkEventControllerKey *controller,
				  guint keyval, guint keycode,
				  GdkModifierType state, gpointer data)
{
 /* Middle mouse button centers the lockstep on the clicked point
  * There will also be a option to set the limits of the plot
  * (center and magnification) by using the last N points clicked.
  *
  */
   gchar mess[256];

# ifdef just_for_info
  /* GDK_SHIFT_MASK | GDK_CONTROL_MASK | GDK_MOD1_MASK */
  case GDK_KEY_BackSpace:
  case GDK_KEY_Tab:
  jj = gdk_keyval_from_name ( "Escape" );
  jj = gdk_keyval_from_name ( "leftarrow" );
  jj = gdk_keyval_from_name ( "uparrow" );
  jj = gdk_keyval_from_name ( "rightarrow" );
  jj = gdk_keyval_from_name ( "downarrow" );
  jj = gdk_keyval_from_name ( "Return" );
  jj = gdk_keyval_from_name ( "F1" );
  jj = gdk_keyval_from_name ( "Delete" );
  jj = gdk_keyval_from_name ( "BackSpace" );
# endif


  gchar *aa, *bb, *mod = "nomod";
  guint frame_num = GPOINTER_TO_UINT (data), mark, jj;
  guint F_frame_id = 0;
  gboolean modifier_only = FALSE;
  gboolean modified = FALSE;
  gboolean plot_command = FALSE;


  modified = (state & GDK_CONTROL_MASK || state & GDK_SHIFT_MASK);

  if( state & GDK_CONTROL_MASK )
    { mod = "Control"; }
  if( state & GDK_SHIFT_MASK )
    { mod = "Shift"; }

  switch( keyval ) {

  case GDK_KEY_T:
  case GDK_KEY_t:
    sii_param_toggle_field (cursor_frame-1);
    break;

  case GDK_KEY_D:
  case GDK_KEY_d:
    sii_edit_zap_last_bnd_point();
    break;

  case GDK_KEY_J:
  case GDK_KEY_j:
    sii_dump_debug_stuff();
    break;

  case GDK_KEY_Control_L:
  case GDK_KEY_Control_R:
  case GDK_KEY_Shift_L:
  case GDK_KEY_Shift_R:
    /* Lone modifier key. Ignore */
    modifier_only = TRUE;
    break;

  case GDK_KEY_F1:
  case GDK_KEY_F2:
  case GDK_KEY_F3:
  case GDK_KEY_F4:
  case GDK_KEY_F5:
  case GDK_KEY_F6:
  case GDK_KEY_F7:
  case GDK_KEY_F8:
  case GDK_KEY_F9:
  case GDK_KEY_F10:
  case GDK_KEY_F11:
  case GDK_KEY_F12:
    /* Function buttons */
    F_frame_id = keyval - GDK_KEY_F1 +1;

# ifdef notyet
    g_message( "Caught F key:%d ", F_frame_id );
# endif
    break;

  case 268828432:		/* SunF36 when you punch F11 on Sun Solaris */
    g_message ("Caught funky SunF36 for F11");
    F_frame_id = 11;
    break;

  case 268828433:		/* SunF37 when you punch F12 on Sun Solaris */
    g_message ("Caught funky SunF37 for F12");
    F_frame_id = 12;
    break;

  case GDK_KEY_Escape:		/* Stop display or editing  */
    g_message( "Caught Escape " );
    solo_set_halt_flag ();
    break;

  case GDK_KEY_Delete:		/* delete the last boundary point */
# ifdef notyet
    g_message( "Caught Delete " );
# endif
      break;

  case GDK_KEY_Right:
    /* if the cursor is currently residing in a frame, then
     * plot the next sweep for the set of frames (lockstep)
     * of which this frame is a member
     */
    if( state & ( GDK_SHIFT_MASK | GDK_CONTROL_MASK )) {
      /* fast forward mode (escape interrupts this mode) */
      g_message( "Caught Control/Shift Right " );

      if (cursor_frame) {
	sii_plot_data (cursor_frame-1, FORWARD_FOREVER);
      }
    }
    else {
      if (cursor_frame) {
	sii_plot_data (cursor_frame-1, FORWARD_ONCE);
      }
# ifdef notyet
      g_message( "Caught Right " );
# endif
    }
    break;

  case GDK_KEY_Left:
    /* Same as Right only the other direction */
    if( state & ( GDK_SHIFT_MASK | GDK_CONTROL_MASK )) {
      g_message( "Caught Control/Shift Left " );

      if (cursor_frame) {
	sii_plot_data (cursor_frame-1, BACKWARDS_FOREVER);
      }
    }
    else {
      if (cursor_frame) {
	sii_plot_data (cursor_frame-1, BACKWARDS_ONCE);
      }
# ifdef notyet
      g_message( "Caught Left " );
# endif
    }
    break;

  case GDK_KEY_Up:
    /* Next sweep at same fixed angle */
      break;

  case GDK_KEY_Down:
    /* Previous sweep at same fixed angle */
    break;


  case GDK_KEY_Return:
      g_message( "Caught Return " );
      if (cursor_frame) {
	sii_plot_data (cursor_frame-1, REPLOT_LOCK_STEP);
      }
      else
	{ sii_plot_data (cursor_frame-1, REPLOT_ALL); }
    break;


  default:
    break;
  };


  if (F_frame_id > 0)		/* Blowup or pop down a big image */
    { sii_blow_up (F_frame_id-1); }


  aa = gdk_keyval_name( keyval );
  if (!aa)
    { aa = "unk"; }

# ifdef config_debug
  sprintf (mess,"data %d frame: %d keyed k:%d s:%d aa:%s mod:%s"
	   , frame_num, cursor_frame
	   , keyval, state, aa, mod);
  sii_append_debug_stuff (mess);
# endif

  return (frame_num > 2 * maxFrames) ? FALSE : TRUE;
}

/* c---------------------------------------------------------------------- */

void custom_config_cb( GtkWidget *w, gpointer   data )
{
  guint cfg = GPOINTER_TO_UINT (data);
  guint ncols = cfg/10;
  guint nrows = cfg % 10;
  g_message ("Hello, Custom Config World! cols: %d  rows: %d\n"
	     , ncols, nrows );
# define ONE_BIG_TWOV_SMALL  1012
# define ONE_BIG_TWO_SMALL   2012
# define ONE_BIG_THREE_SMALL 1013
# define ONE_BIG_FIVE_SMALL  1015
# define ONE_BIG_SEVEN_SMALL 1017
# define TWO_BIG_FOURV_SMALL 1024
# define TWO_BIG_FOUR_SMALL  2024
# define FOUR_512x512        4022
# define FOUR_DEFAULT        2022

  sii_reset_config_cells();


  if( cfg == FOUR_512x512 ) {
    config_cells[0]->frame_num = 1;
    config_cells[1]->frame_num = 2;
    config_cells[4]->frame_num = 3;
    config_cells[5]->frame_num = 4;
    sii_table_widget_width = 512;
    sii_table_widget_height = 512;

  }
  else if( cfg == FOUR_DEFAULT ) {
    config_cells[0]->frame_num = 1;
    config_cells[1]->frame_num = 2;
    config_cells[4]->frame_num = 3;
    config_cells[5]->frame_num = 4;
    sii_table_widget_width = DEFAULT_WIDTH;
    sii_table_widget_height = DEFAULT_WIDTH;

  }
  else if( cfg == ONE_BIG_TWOV_SMALL ) {
    config_cells[0]->frame_num = 1;
    config_cells[1]->frame_num = 1;
    config_cells[4]->frame_num = 1;
    config_cells[2]->frame_num = 2;
    config_cells[6]->frame_num = 3;
  }
  else if( cfg == ONE_BIG_TWO_SMALL ) {
    config_cells[0]->frame_num = 1;
    config_cells[1]->frame_num = 1;
    config_cells[4]->frame_num = 1;
    config_cells[8]->frame_num = 2;
    config_cells[9]->frame_num = 3;
  }
  else if( cfg == ONE_BIG_THREE_SMALL ) {
    config_cells[0]->frame_num = 1;
    config_cells[2]->frame_num = 1;
    config_cells[8]->frame_num = 1;
    config_cells[3]->frame_num = 2;
    config_cells[7]->frame_num = 3;
    config_cells[11]->frame_num = 4;
  }
  else if( cfg == ONE_BIG_FIVE_SMALL ) {
    config_cells[0]->frame_num = 1;
    config_cells[1]->frame_num = 1;
    config_cells[4]->frame_num = 1;
    config_cells[2]->frame_num = 2;
    config_cells[6]->frame_num = 3;
    config_cells[8]->frame_num = 4;
    config_cells[9]->frame_num = 5;
    config_cells[10]->frame_num = 6;
  }
  else if( cfg == ONE_BIG_SEVEN_SMALL ) {
    config_cells[0]->frame_num = 1;
    config_cells[2]->frame_num = 1;
    config_cells[8]->frame_num = 1;
    config_cells[3]->frame_num = 2;
    config_cells[7]->frame_num = 3;
    config_cells[11]->frame_num = 4;
    config_cells[12]->frame_num = 5;
    config_cells[13]->frame_num = 6;
    config_cells[14]->frame_num = 7;
    config_cells[15]->frame_num = 8;
  }
  else if( cfg == TWO_BIG_FOURV_SMALL ) {
    config_cells[0]->frame_num = 1;
    config_cells[1]->frame_num = 1;
    config_cells[4]->frame_num = 1;

    config_cells[2]->frame_num = 2;
    config_cells[3]->frame_num = 2;
    config_cells[6]->frame_num = 3;
    config_cells[7]->frame_num = 3;

    config_cells[8]->frame_num = 4;
    config_cells[9]->frame_num = 4;
    config_cells[12]->frame_num = 4;

    config_cells[10]->frame_num = 5;
    config_cells[11]->frame_num = 5;
    config_cells[14]->frame_num = 6;
    config_cells[15]->frame_num = 6;
  }
  else if( cfg == TWO_BIG_FOUR_SMALL ) {
    config_cells[0]->frame_num = 1;
    config_cells[1]->frame_num = 1;
    config_cells[4]->frame_num = 1;
    config_cells[2]->frame_num = 2;
    config_cells[3]->frame_num = 2;
    config_cells[6]->frame_num = 2;

    config_cells[8]->frame_num = 3;
    config_cells[9]->frame_num = 4;
    config_cells[10]->frame_num = 5;
    config_cells[11]->frame_num = 6;
  }
  else {
    printf( "Bogus custom config #: %d\n", cfg );
    return;
  }

  sii_set_config();
  sii_new_frames ();
# ifdef notyet
# endif
}

/* c---------------------------------------------------------------------- */

void zoom_data_cb( GtkWidget *w, gpointer data )
{
   guint pct = GPOINTER_TO_UINT (data);
   gfloat factor = 0;


   if( pct > 10000 ) {
     pct %= 10000;
     factor = 1. - pct*.01;
   }
   else if (pct != 0)
     { factor = 1. + pct * .01; }

   sii_blanket_zoom_change (factor);
}

/* c---------------------------------------------------------------------- */

void zoom_config_cb( GtkWidget *w, gpointer   data )
{
   guint pct = GPOINTER_TO_UINT (data), mm, nn;
   gdouble factor, zoom;
   gint jj;


   if (!pct) {
      return;
   }
   else if( pct > 10000 ) {
     pct %= 10000;
     if( pct == 5 )
       { factor = 100./106; }
     if( pct == 11 )
       { factor = 100./112; }
     else if( pct == 33 )
       { factor = 100./150; }
     else
       { factor = 1. - pct*.01; }
   }
   else
     { factor = 1. + pct * .01; }

   for (jj=0; jj < sii_return_frame_count(); jj++) {
      /*
	frame_configs[jj]->local_reconfig = TRUE;
	frame_configs[jj]->reconfig_count = 0;
       */
   }
   sii_table_widget_width = (guint)(factor * sii_table_widget_width +.5 );
   sii_table_widget_height =(guint)(factor * sii_table_widget_height +.5 );

   sii_check_def_widget_sizes ();
   sii_new_frames ();
}

/* c---------------------------------------------------------------------- */

void shape_cb( GtkWidget *w, gpointer   data )
{
   guint cfg = GPOINTER_TO_UINT (data), jj;
   gdouble factor;

# ifdef obsolete
   if( cfg == 11 ) {
     sii_table_widget_width = (guint)(DEFAULT_WIDTH * sii_config_w_zoom);
     sii_table_widget_height = (guint)(DEFAULT_WIDTH * sii_config_h_zoom);
   }
   else if(cfg == 43 ) {
     sii_table_widget_width = (guint)(DEFAULT_WIDTH * sii_config_w_zoom);
     sii_table_widget_height =
       (guint)(DEFAULT_SLIDE_HEIGHT * sii_config_h_zoom);
   }
   else if(cfg == 34 ) {
     sii_table_widget_width = (guint)(DEFAULT_WIDTH * sii_config_w_zoom);
     sii_table_widget_height =
       (guint)(DEFAULT_VSLIDE_HEIGHT * sii_config_h_zoom);
   }
# else
   factor = (gdouble)sii_table_widget_width/DEFAULT_WIDTH;

   if( cfg == 11 ) {
     sii_table_widget_height = sii_table_widget_width;
   }
   else if(cfg == 43 ) {
     sii_table_widget_height =
       (guint)(.75 * sii_table_widget_width);
   }
   else if(cfg == 34 ) {
     sii_table_widget_height =
       (guint)((gdouble)4./3. * sii_table_widget_width);
   }
# endif
   for (jj=0; jj < sii_return_frame_count(); jj++) {
      /*
	frame_configs[jj]->local_reconfig = TRUE;
	frame_configs[jj]->reconfig_count = 0;
       */
   }
   sii_check_def_widget_sizes ();
   sii_new_frames ();
}
/* c---------------------------------------------------------------------- */

void config_cb( GtkWidget *w, gpointer   data )
{
   guint cfg = GPOINTER_TO_UINT (data);
   guint ncols = cfg/10;
   guint nrows = cfg % 10;
   guint ii, jj, kk, frame_num = 0;
   gboolean result;

  if( cfg == 99 ) {		/* the config cells have been set already */
     result = sii_set_config();
     return;
  }

  sii_reset_config_cells();

  for( jj = 0; jj < nrows; jj++ ) {
    for( ii = 0; ii < ncols; ii++ ) {
      kk = ii + jj * maxConfigCols;
      config_cells[kk]->frame_num = ++frame_num; /* tag the cell with the frame no. */
    }
  }
  result = sii_set_config();
  sii_new_frames ();
}

/* c---------------------------------------------------------------------- */
guint test_event_cb(GtkWidget *label, gpointer data )
{
  guint position = GPOINTER_TO_UINT (data);
  g_message( "layout widget line: %d"
	    , position
	    );
  return FALSE;
}

/* c---------------------------------------------------------------------- */

void test_widget( GtkWidget *w, gpointer   data )
{
  GtkWidget *label;

  GtkWidget *window;
  GtkWidget *scrolledwindow;
  GtkWidget *vbox;
  GtkWidget *edit_menubar;
  GtkWidget *hbox;

  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *vbox3;

  GtkWidget *text;
  GtkWidget *hscrollbar;
  GtkWidget *vscrollbar;

  GtkWidget *menubar;
  GtkWidget *menu;
  GtkWidget *menuitem;

  GtkWidget *button;


  gchar str[256], *aa, *bb, *ee;
  guint frame_num = GPOINTER_TO_UINT (data);
  const gchar *cc, *tt = "00/00/2000 00:00:00 HHHHHHHH 000.0 HHH HHHHHHHH";
  gint length, width, height, mm, nn, mark;
  gfloat upper;

  window = gtk_window_new();
      g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK(sii_widget_destroyed_cb),
			  &window);

  /* --- Set the window title and size --- */
  gtk_widget_set_size_request (window, 320, 600);
  gtk_window_set_title (GTK_WINDOW (window), "Test Widget");

  /* --- Create a new vertical box for storing widgets --- */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW(window), vbox);

  scrolledwindow = gtk_scrolled_window_new ();
  gtk_box_append (GTK_BOX (vbox), scrolledwindow);

  nn = 500;
  height = 18;

  /* GTK4: GtkLayout replaced by a GtkFixed inside a scrolled window,
   * or use a GtkBox with buttons. Simplified here. */
  GtkWidget *list_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrolledwindow), list_box);

  aa = "00/00/2000 00:00:00.000 HHHHHHHH";
  bb = " HHH V0000";

  for (mm=0; mm < nn; mm++ ) {
     sprintf (str, "%s%6.1f%s               ", aa, (gfloat)mm, bb );
     button = gtk_button_new_with_label (str);
     gtk_box_append (GTK_BOX (list_box), button);

     g_signal_connect (G_OBJECT(button)
			 ,"clicked"
			 , G_CALLBACK (test_event_cb)
			 , (gpointer)button);

     g_object_set_data (G_OBJECT(button),
			  "frame_num",
			  (gpointer)frame_num);
     g_object_set_data (G_OBJECT(button),
			  "swpfi_index",
			  GUINT_TO_POINTER(mm));
  }

  /* --- Make everything visible --- */

  gtk_widget_set_visible (window, TRUE);
  mark = 0;
}

/* c---------------------------------------------------------------------- */

void dialog_a_cb( GtkWidget *w, gpointer   data )
{
  guint itype = GPOINTER_TO_UINT (data);
  GtkWidget *dialog, *label, *button, *button2;

# ifdef obsolete
  dialog = gtk_dialog_new_with_buttons
    ( "Test Dialog",
      main_window,
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_STOCK_BUTTON_OK,
      GTK_RESPONSE_NONE,
      NULL );
  /* GTK4: GtkDialog->vbox no longer accessible directly */
# endif
  /* GTK4: GtkDialog is deprecated in GTK4; use GtkWindow with custom layout.
   * Keeping a simplified version here. */
  dialog = gtk_window_new ();
  gtk_window_set_title (GTK_WINDOW(dialog), "Test Dialog");

  GtkWidget *content_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_window_set_child (GTK_WINDOW(dialog), content_box);

  label = gtk_label_new ( "This is the label" );
  gtk_widget_set_margin_start (label, 10);
  gtk_widget_set_margin_end (label, 10);
  gtk_widget_set_margin_top (label, 10);
  gtk_widget_set_margin_bottom (label, 10);
  gtk_box_append (GTK_BOX (content_box), label);

  GtkWidget *action_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append (GTK_BOX (content_box), action_box);

  button = gtk_button_new_with_label ("Cancel");
  gtk_box_append (GTK_BOX (action_box), button);
  gtk_widget_grab_focus (button);

  g_signal_connect_swapped ( G_OBJECT (button), "clicked",
			      G_CALLBACK (gtk_window_destroy),
			      dialog);
  button2 = gtk_button_new_with_label ("Finish");
  gtk_box_append (GTK_BOX (action_box), button2);

  gtk_widget_set_visible (dialog, TRUE);
}


/* c---------------------------------------------------------------------- */

void checkup_cb( GtkWidget *w, gpointer   data )
{
  guint itype = GPOINTER_TO_UINT (data);
  g_message ("Not yet... Just checking");
}

/* c---------------------------------------------------------------------- */

void image_cb( GtkWidget *w, gpointer   data )
{
  guint itype = GPOINTER_TO_UINT (data);
  gchar *aa;

  switch (itype ) {
  case 0:
    aa = "dir";
    break;
  case MAIN_PNG:
    aa = "png";
    break;
  case MAIN_GIF:
    aa = "gif";
    break;
  case MAIN_PRNT:
    aa = "Print";
    break;
  default:
    break;
  }
  g_message ("Hello, Image World! %s", aa );
}

/* c---------------------------------------------------------------------- */

/* GTK4: focus_in_event -> GtkEventControllerFocus */
void sii_focus_in_cb(GtkEventControllerFocus *controller, gpointer data)
{
  GtkWidget *frame = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER(controller));
  guint frame_num = GPOINTER_TO_UINT (data), mark;
# ifdef obsolete
  g_message ("frame %d focus in\n", frame_num );
# endif
}

/* c---------------------------------------------------------------------- */

/* GTK4: focus_out_event -> GtkEventControllerFocus */
void sii_focus_out_cb(GtkEventControllerFocus *controller, gpointer data)
{
  guint frame_num = GPOINTER_TO_UINT (data), mark;
# ifdef obsolete
  g_message ("frame %d focus out\n", frame_num );
# endif
}

/* c---------------------------------------------------------------------- */

/* GTK4 stub for sii_filesel - file selection dialog.
 * TODO: Replace with GtkFileDialog when implementing file operations.
 */
GtkWidget *sii_filesel(gint which_but, gchar *dirroot)
{
  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Select File");
  g_message("sii_filesel: file selection not yet implemented for GTK4");
  return dialog;
}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */
