/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

/*
 * sii_png_image: Export the current radar display to a PNG file.
 *
 * Primary method: Compose all visible frame images into a single
 * cairo image surface and write to PNG via cairo_surface_write_to_png().
 *
 * Fallback: Use platform-specific screenshot tools (screencapture on macOS,
 * gnome-screenshot on Linux).
 */

/* Helper: render a single frame's RGB image data onto a cairo surface */
static int
sii_render_frame_to_surface (cairo_t *cr, int frame_num,
			     int dest_x, int dest_y)
{
  SiiFrameConfig *sfc = frame_configs[frame_num];
  guchar *img;
  int width, height, stride, x, y;
  cairo_surface_t *frame_surface;
  guchar *surf_data;
  int surf_stride;

  if (!sfc || !sfc->image || !sfc->image->data)
    return 0;

  width = sfc->width;
  height = sfc->height;
  if (width < 1 || height < 1)
    return 0;

  /* The RGB data starts after the indexed color data (width*height bytes) */
  img = (guchar *)sfc->image->data;
  img += width * height;
  stride = width * 3;

  frame_surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
					      width, height);
  if (cairo_surface_status (frame_surface) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy (frame_surface);
    return 0;
  }

  surf_data = cairo_image_surface_get_data (frame_surface);
  surf_stride = cairo_image_surface_get_stride (frame_surface);

  cairo_surface_flush (frame_surface);

  for (y = 0; y < height; y++) {
    const guchar *src_row = img + y * stride;
    guchar *dst_row = surf_data + y * surf_stride;
    for (x = 0; x < width; x++) {
      /* Cairo RGB24: stored as B, G, R, 0 in each 4-byte pixel */
      dst_row[x * 4 + 0] = src_row[x * 3 + 2]; /* blue */
      dst_row[x * 4 + 1] = src_row[x * 3 + 1]; /* green */
      dst_row[x * 4 + 2] = src_row[x * 3 + 0]; /* red */
      dst_row[x * 4 + 3] = 0xFF;
    }
  }

  cairo_surface_mark_dirty (frame_surface);

  cairo_save (cr);
  cairo_set_source_surface (cr, frame_surface, dest_x, dest_y);
  cairo_paint (cr);
  cairo_restore (cr);

  cairo_surface_destroy (frame_surface);
  return 1;
}

int sii_png_image (char *fname)
{
  static int err_count = 0;
  char mess[256], png_fname[512];
  SiiFrameConfig *sfc0 = frame_configs[0];
  guint fc, ncols, nrows, fw, fh, total_w, total_h;
  guint row, col, fn;
  cairo_surface_t *composite;
  cairo_t *cr;
  cairo_status_t status;
  int rendered = 0;

  sprintf (png_fname, "%s.png", fname);

  /* Try to compose all visible frames into a single PNG */
  fc = sii_frame_count;
  if (fc < 1 || !sfc0)
    goto fallback;

  ncols = sfc0->ncols;
  nrows = sfc0->nrows;
  fw = sii_table_widget_width;
  fh = sii_table_widget_height;

  if (ncols < 1 || nrows < 1 || fw < 1 || fh < 1)
    goto fallback;

  total_w = ncols * fw;
  total_h = nrows * fh;

  composite = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
					  total_w, total_h);
  if (cairo_surface_status (composite) != CAIRO_STATUS_SUCCESS) {
    cairo_surface_destroy (composite);
    goto fallback;
  }

  cr = cairo_create (composite);

  /* Fill background with black */
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_paint (cr);

  /* Render each frame into its grid position */
  fn = 0;
  for (row = 0; row < nrows && fn < fc; row++) {
    for (col = 0; col < ncols && fn < fc; col++, fn++) {
      int dest_x = col * fw;
      int dest_y = row * fh;
      if (sii_render_frame_to_surface (cr, fn, dest_x, dest_y))
	rendered++;
    }
  }

  cairo_destroy (cr);

  if (rendered > 0) {
    status = cairo_surface_write_to_png (composite, png_fname);
    cairo_surface_destroy (composite);

    if (status == CAIRO_STATUS_SUCCESS) {
      err_count = 0;
      return 0;
    }

    /* cairo write failed - report and try fallback */
    if (++err_count == 1) {
      sprintf (mess, "Cairo PNG write failed for %s: %s",
	       png_fname, cairo_status_to_string (status));
      sii_message (mess);
    }
    goto fallback;
  }

  cairo_surface_destroy (composite);

fallback:
  /* Platform-specific screenshot fallback */
  {
    char command[512];
    int ii;

#ifdef __APPLE__
    /* macOS: screencapture -w captures the front window */
    sprintf (command, "screencapture -w \"%s\" 2>/dev/null", png_fname);
#else
    /* Linux: try gnome-screenshot, then import (ImageMagick) */
    sprintf (command,
	     "gnome-screenshot -w -f \"%s\" 2>/dev/null || "
	     "import -window root \"%s\" 2>/dev/null",
	     png_fname, png_fname);
#endif

    ii = system (command);
    if (ii && ++err_count == 1) {
      sprintf (mess,
	       "Unable to write %s - no image data and screenshot tool failed",
	       png_fname);
      sii_message (mess);
    }
    else if (!ii) {
      err_count = 0;
    }
    return ii;
  }
}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */
