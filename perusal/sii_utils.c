/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"
# include "sii_utils.h"
# include "sii_overlays.h"
# include <stdio.h>
static GString *dbgs = NULL;
static gint nlines = 0;

/* c---------------------------------------------------------------------- */

void sii_dump_debug_stuff ()
{
   if (dbgs && dbgs->len > 0) {
      printf ("%s", dbgs->str);
      g_string_truncate (dbgs, 0);
      g_string_append (dbgs, "\n");
      nlines = 0;
   }
}

/* c---------------------------------------------------------------------- */

void sii_append_debug_stuff (gchar *gch)
{
   gchar *aa, *bb;
   gint len, mm, nn, max=777, slack=111;

   if (!dbgs)
     { dbgs = g_string_new (""); }

   if (gch && strlen (gch) > 0) {
      mm = dbgs->len;
      g_string_append (dbgs, gch);
      g_string_append (dbgs, "\n");
      nn = dbgs->len;

      for (aa = dbgs->str; aa && mm < nn; mm++) {
	 if (aa[mm] == '\n')
	   { ++nlines; }
      }

      if (nlines > max+slack) {
	 while (nlines > max) {
	    nn = dbgs->len;
	    for (aa=dbgs->str,mm=0; mm < nn; mm++) {
	       if (aa[mm] == '\n') {
		  mm++;
		  break;
	       }
	    }
	    if (mm < dbgs->len) {
	       g_string_erase (dbgs, 0, mm);
	       --nlines;
	    }
	 }
      }
   }
}

/* c------------------------------------------------------------------------ */

void sii_glom_strings (const gchar **cptrs, int nn, GString *gs)
{
   g_string_truncate (gs, 0);
   for (; nn--; cptrs++) {
      g_string_append (gs, *cptrs);
      if (nn)
	{ g_string_append (gs, "\n"); }
   }
}

/* c------------------------------------------------------------------------ */

int sii_absorb_zeb_map (gchar *path_name, siiOverlay *overlay)
{
  /* routine to read in a zeb map overlay */

    int ii, jj, kk, nn, np, nt, mark, count = 0;
    char *aa, *bb, str[256], str2[256], *sptrs[64];
    FILE *stream;
    gboolean new = TRUE;
    siiPolyline *top, *next, *prev=NULL;


    if(!(stream = fopen(path_name, "r"))) {
	printf("Unable to open %s", path_name);
	return(-1);
    }
    top = next = overlay->plines;

    /* read in the new strings
     */
    for(nn=0;; nn++) {
	if(!(aa = fgets(str, (int)128, stream))) {
	    break;
	}
	if ((nt = dd_tokens (str, sptrs)) < 2)
	  { continue; }
	if (new) {
	  np = atoi (sptrs[0])/2;
	  if (!next) {
	    next = g_malloc0 (sizeof (siiPolyline));
	    if (!top)
	      { top = overlay->plines = next; }
	    next->lat = (gdouble *)g_malloc0 (np * sizeof (gdouble));
	    next->lon = (gdouble *)g_malloc0 (np * sizeof (gdouble));
	    next->x = (int *)g_malloc0 (np * sizeof (int));
	    next->y = (int *)g_malloc0 (np * sizeof (int));
	  }
	  next->urc.lat = atof (sptrs[1]);
	  next->urc.lon = atof (sptrs[3]);
	  next->llc.lat = atof (sptrs[2]);
	  next->llc.lon = atof (sptrs[4]);

	  next->num_points = np;
	  if (np > overlay->max_pline_length)
	    { overlay->max_pline_length = np; }

	  overlay->num_plines++;
	  if (prev)
	    { prev->next = next; }
	  prev = next;
	  new = FALSE;
	  jj = 0;
	  continue;
	}
	for (ii=0; ii < nt; ii +=2, jj++, np--) {
	  next->lat[jj] = atof (sptrs[ii]);
	  next->lon[jj] = atof (sptrs[ii+1]);
	  count++;
	}
	if (np == 0)
	  { new = TRUE; next = next->next; }
    }
    fclose(stream);
    return(count);
}

/* c---------------------------------------------------------------------- */

guint sii_inc_master_seq_num ()
{
   return ++sii_seq_num;
}

/* c---------------------------------------------------------------------- */

guint sii_get_master_seq_num ()
{
   return sii_seq_num;
}

/* c---------------------------------------------------------------------- */

void sii_zap_quick_message (GtkWidget *w, gpointer data )
{
   gtk_window_destroy (GTK_WINDOW (data));
}


/* c---------------------------------------------------------------------- */

void sii_message (const gchar *message )
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *label;

  if (batch_mode)
    { return; }

  window = gtk_window_new();
  gtk_window_set_title (GTK_WINDOW (window), "SoloIV Message");

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(vbox, 10);
  gtk_widget_set_margin_end(vbox, 10);
  gtk_widget_set_margin_top(vbox, 10);
  gtk_widget_set_margin_bottom(vbox, 10);
  gtk_window_set_child (GTK_WINDOW(window), vbox);

  label = gtk_label_new (message);
  gtk_label_set_xalign (GTK_LABEL(label), 0.0);
  gtk_box_append (GTK_BOX (vbox), label);

  button = gtk_button_new_with_label (" Dismiss ");
  gtk_box_append (GTK_BOX (vbox), button);

  g_signal_connect (button, "clicked",
		    G_CALLBACK(sii_zap_quick_message),
		    window);

  gtk_widget_set_visible (window, TRUE);
}

/* c---------------------------------------------------------------------- */

int sii_str_seek ( char **sptrs, int count, const char *sought )
{
   int ii = 0;

   if (!sptrs || count < 1 || !sought || strlen (sought) < 1)
     { return -1; }

   for(; ii < count; ii++) {
      if (sptrs[ii] && strcmp (sptrs[ii], sought) == 0)
	{ return ii; }
   }
   return -1;
}

/* c---------------------------------------------------------------------- */

char *sii_glom_tokens ( char *str, char **sptrs, int count, char *joint )
{
   /* tokens with a NULL pointer or zero length will be omitted
    */
   int ii = 0;

   if (!str || !sptrs || count < 1 )
     { return NULL; }

   str[0] = '\0';

   for(; ii < count; ii++ ) {
      if (sptrs[ii] && strlen (sptrs[ii])) {
	 strcat (str, sptrs[ii]);
	 if (joint && strlen (joint))
	   { strcat (str, joint); }
      }
   }
   return str;
}

/* c---------------------------------------------------------------------- */

gchar *sii_set_gtktext_data (GSList *list)
{
   return NULL;
}

/* c---------------------------------------------------------------------- */

gchar *sii_generic_gslist_insert ( gpointer name, gpointer data )
{
   gen_gslist = g_slist_insert_sorted
     ( gen_gslist, name, (GCompareFunc)strcmp );

   return( FALSE );		/* to continue a tree traverse */
}

/* c---------------------------------------------------------------------- */

void sii_set_widget_frame_num ( GtkWidget *w, gpointer data )
{
   guint frame_num = GPOINTER_TO_UINT (data);
   g_object_set_data (G_OBJECT(w),
		      "frame_num",
		      (gpointer)frame_num);
}

/* c---------------------------------------------------------------------- */

gboolean sii_str_values ( const gchar *line, guint nvals, gfloat *f1
			 , gfloat *f2 )
{
   gchar str[128], *sptrs[16];
   gint nt;

   strcpy (str, line);
   nt = dd_tokenz (str, sptrs, " \t\n," );
   if( nt < nvals )
     { return FALSE; }

   nt = sscanf ( sptrs[0], "%f", f1 );
   if( nt != 1 )
     { return FALSE; }

   if( nvals == 1 )
     { return TRUE; }

   nt = sscanf ( sptrs[1], "%f", f2 );
   if( nt != 1 )
     { return FALSE; }

   return TRUE;
}

/* c---------------------------------------------------------------------- */

void sii_nullify_widget_cb (GtkWidget *widget, gpointer data)
{
  guint num = GPOINTER_TO_UINT (data);
  guint frame_num, task, window_id;

  frame_num = num/TASK_MODULO;
  window_id = num % TASK_MODULO;

  frame_configs[frame_num]->toplevel_windows[window_id] = NULL;
}

/* c---------------------------------------------------------------------- */

GtkWidget *sii_get_widget_ptr ( guint frame_num, gint window_id )
{
   return frame_configs[frame_num]->toplevel_windows[window_id];
}

/* c---------------------------------------------------------------------- */

void sii_set_widget_ptr ( guint frame_num, gint window_id, GtkWidget *widget )
{
   frame_configs[frame_num]->toplevel_windows[window_id] = widget;
}

/* c---------------------------------------------------------------------- */

gchar *sii_nab_line_from_text (const gchar *txt, guint position )
{
   static gchar line[256];
   gint mm, nn = position;

   if( !txt || strlen( txt ) <= position || position < 0 )
      { return NULL; }

   for (; txt[nn] && txt[nn] != '\n'; nn++ ); /* find end of line */
   for (mm = nn; mm > 0 && txt[mm-1] != '\n'; mm-- ); /* find BOL */

   line[0] = '\0';
   strncpy (line, &txt[mm], nn-mm );
   line[nn-mm] = '\0';
   return line;
}

/* c---------------------------------------------------------------------- */

gboolean sii_nab_region_from_text (const gchar *txt, guint position
				   , gint *start, gint *end)
{
   static gchar line[256];
   gint mm, nn = position;

   if( !txt || strlen( txt ) <= position || position < 0 )
      { return FALSE; }

   for (; txt[nn] && txt[nn] != '\n'; nn++ ); /* find end of line */
   for (mm = nn; mm > 0 && txt[mm-1] != '\n'; mm-- ); /* find BOL */

   *start = mm; *end = nn;
   return TRUE;
}

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_ll_queue (SiiLinkedList **list, SiiLinkedList *link)
{
   SiiLinkedList *this=link, *previous, *next;

   if (!link)
     { return link; }

   if (!(*list))  {
      *list = link;
      link->previous = link->next = link;
   }
   else {			/* add to end of que */
      link->previous = (*list)->previous; /* (*list)->previous is of queue */
      link->next = NULL;
      (*list)->previous->next = link;
      (*list)->previous = link;
   }
   return link;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_ll_dequeue (SiiLinkedList **list)
{
  SiiLinkedList *this;

  if (!(*list))  {
    return NULL;
  }
  /* remove top item  */
  this = *list;
  if (!(*list)->next)		/* last item in queue */
    { *list = NULL; }
  else {
    (*list)->next->previous = (*list)->previous;
    *list = (*list)->next;
  }
  this->previous = this->next = NULL;
  return this;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_ll_push (SiiLinkedList **list, SiiLinkedList *link)
{
   SiiLinkedList *this=link, *previous, *next;

   if (!link)
     { return link; }

   if (!(*list))  {
      *list = link;
      link->previous = link->next = NULL;
   }
   else {
      link->previous = NULL;
      link->next = *list;
      (*list)->previous = link;
      *list = link;
   }
   return link;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_ll_remove (SiiLinkedList **list, SiiLinkedList *link)
{
   if (!link)
     { return NULL; }

   if (link == *list ) {		/* top of list */
      *list = link->next;
      if (link->next)
	{ link->next->previous = NULL; }
   }
   else if (!link->next) {	/* bottom of list */
      link->previous->next = NULL;
   }
   else {
      link->previous->next = link->next;
      link->next->previous = link->previous;
   }
   return link;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_ll_pop (SiiLinkedList **list )
{
   SiiLinkedList *this=sii_ll_remove (list, *list);
   return this;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_init_linked_list (guint length)
{
  SiiLinkedList *this, *first=NULL, *previous = NULL;
  guint cn = 0;

  for( ; cn < length; cn++ ) {
    this = (SiiLinkedList *)g_malloc0 (sizeof (SiiLinkedList));

    if( previous ) {
      previous->next = this;
      this->previous = previous;
    }
    else {
      first = this;
    }
    previous = this;
  }
  return first;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_ll_malloc_item ()
{
   SiiLinkedList *item = (SiiLinkedList *)g_malloc0 (sizeof (SiiLinkedList));;
   return item;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *sii_init_circ_que (guint length)
{
  SiiLinkedList *this, *first=NULL, *previous = NULL;

  first = sii_init_linked_list (length);
  this = previous = first;

  for(; this; previous = this, this = this->next ); /* loop to end */

  if (previous) {
     first->previous = previous;
     previous->next = first;
  }
  return first;
}

/* c---------------------------------------------------------------------- */

SiiLinkedList *insert_clicks_que ( SiiPoint *click )
{
  SiiPoint *click2, *click1, *click0;

  if( !sii_click_que ) {
    sii_click_que = sii_init_circ_que (click_que_size);
  }
  sii_click_que = sii_click_que->next;

  if( !sii_click_que->data ) {
    sii_click_que->data = g_memdup2 ((gpointer)click, sizeof (*click));
  }
  else {
    memmove( sii_click_que->data, (gpointer)click, sizeof (*click));
 }
  click2 = (SiiPoint *)sii_click_que->data;
  click1 = (SiiPoint *)sii_click_que->previous->data;
  click0 = (SiiPoint *)sii_click_que->previous->previous->data;

  if( clicks_in_que < click_que_size )
    { clicks_in_que++; }

  return sii_click_que;
}

/* c------------------------------------------------------------------------ */
/* c---------------------------------------------------------------------- */

gchar *
sii_item_delims ()
{
  return " \t\n,";
}

/* c---------------------------------------------------------------------- */

gchar *sii_set_string_from_vals (GString *gs, guint nvals,
				 gfloat f1, gfloat f2,
				 guint precision)
{
   gchar fmt[64], prec[16];

   g_string_truncate (gs, 0);
   sprintf (prec, ".%df", precision);
   strcpy (fmt, "%");
   strcat (fmt, prec);
   if (nvals > 1) {
      strcat (fmt, ", %");
      strcat (fmt, prec);
      g_string_append_printf (gs, fmt, f1, f2);
   }
   else {
      g_string_append_printf (gs, fmt, f1);
   }
   return gs->str;
}

/* c---------------------------------------------------------------------- */

void sii_bad_entry_message ( const gchar *ee, guint items )
{
   gchar str[256];

   str[0] = '\0';
   if (ee && strlen (ee))
     { strcat( str, ee ); }
   sprintf (str+strlen(str)
	    , "\n Entry could not be interpreted. Requires %d item(s)"
	    , items );
   sii_message (str);
}

/* c---------------------------------------------------------------------- */
/* GTK4 menu helper functions */

/* Create a popover menu from a GMenu model and add it to a menu button */
GtkWidget *sii_create_menu_button(const gchar *label_text, GMenuModel *menu_model)
{
  GtkWidget *button = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(button), label_text);
  gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(button), menu_model);
  return button;
}

/* c---------------------------------------------------------------------- */
/* GTK4 replacements for old GtkMenu-based menu helpers.
 * Uses a GtkBox of GtkButtons to approximate old-style menubars.
 * The callback receives frame_num * TASK_MODULO + widget_id as user_data.
 */

GtkWidget *sii_submenu (gchar *name, GtkWidget *mbar)
{
  /* In GTK4 there is no GtkMenu. We approximate submenus with a
   * GtkPopover containing a vertical box of buttons.
   * The mbar is a GtkBox acting as a menu bar.
   */
  GtkWidget *popover_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER(popover), popover_box);

  GtkWidget *menu_button = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(menu_button), name);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu_button), popover);
  gtk_box_append(GTK_BOX(mbar), menu_button);

  return popover_box;
}

/* c---------------------------------------------------------------------- */

static void
sii_submenu_item_activated(GtkButton *button, gpointer user_data)
{
  /* Close the popover before invoking the real callback */
  GtkWidget *popover = g_object_get_data(G_OBJECT(button), "parent_popover");
  if (popover)
    gtk_popover_popdown(GTK_POPOVER(popover));

  GCallback real_cb = (GCallback)g_object_get_data(G_OBJECT(button), "real_callback");
  if (real_cb) {
    void (*fn)(GtkWidget*, gpointer) = (void(*)(GtkWidget*, gpointer))real_cb;
    fn(GTK_WIDGET(button), user_data);
  }
}

GtkWidget *
sii_submenu_item (gchar *name, GtkWidget *submenu, guint widget_id,
                  GCallback sigfun, guint frame_num)
{
  GtkWidget *item;
  guint nn = frame_num * TASK_MODULO + widget_id;

  if (!name) {
    /* separator */
    item = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  } else {
    item = gtk_button_new_with_label(name);
    gtk_button_set_has_frame(GTK_BUTTON(item), FALSE);
    if (sigfun) {
      g_object_set_data(G_OBJECT(item), "real_callback", (gpointer)sigfun);
      /* Find the parent popover by walking up from the box */
      GtkWidget *popover = gtk_widget_get_parent(submenu);
      if (popover && GTK_IS_POPOVER(popover))
        g_object_set_data(G_OBJECT(item), "parent_popover", popover);

      g_signal_connect(item, "clicked",
                       G_CALLBACK(sii_submenu_item_activated),
                       GUINT_TO_POINTER(nn));
    }
    g_object_set_data(G_OBJECT(item), "widget_id",
                      GUINT_TO_POINTER(widget_id));
    g_object_set_data(G_OBJECT(item), "frame_num",
                      GUINT_TO_POINTER(frame_num));
  }
  gtk_box_append(GTK_BOX(submenu), item);
  return item;
}

/* c---------------------------------------------------------------------- */

GtkWidget *
sii_toggle_submenu_item (gchar *name, GtkWidget *submenu, guint widget_id,
                         GCallback sigfun, guint frame_num,
                         guint radio_item, GSList **radio_group_ptr)
{
  GtkWidget *item;
  guint nn = frame_num * TASK_MODULO + widget_id;

  item = gtk_check_button_new_with_label(name);
  if (sigfun) {
    g_signal_connect(item, "toggled",
                     G_CALLBACK(sigfun), GUINT_TO_POINTER(nn));
  }
  g_object_set_data(G_OBJECT(item), "widget_id",
                    GUINT_TO_POINTER(widget_id));
  g_object_set_data(G_OBJECT(item), "frame_num",
                    GUINT_TO_POINTER(frame_num));

  if (radio_item && radio_group_ptr) {
    if (*radio_group_ptr) {
      GtkCheckButton *group_btn = GTK_CHECK_BUTTON((*radio_group_ptr)->data);
      gtk_check_button_set_group(GTK_CHECK_BUTTON(item), group_btn);
    }
    *radio_group_ptr = g_slist_prepend(*radio_group_ptr, item);
  }

  gtk_box_append(GTK_BOX(submenu), item);
  return item;
}

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */
