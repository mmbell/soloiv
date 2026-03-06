/* 	$Id$	 */

# include "soloii.h"
# include "sii_externals.h"

/* GTK4: X11-specific window capture is no longer portable.
 * Use a toolkit-agnostic approach for screenshots.
 * For now, use a simple placeholder that could be implemented
 * with platform-specific APIs later.
 */

/* c---------------------------------------------------------------------- */
/* c---------------------------------------------------------------------- */

int sii_png_image (char *fname)
{
  static int err_count = 0;
  char mess[256], command[512];

  /* GTK4/Wayland: Cannot use xwd for screen capture.
   * For now, attempt to use gnome-screenshot or similar tools.
   * A proper implementation would use GdkPaintable/cairo to render
   * the widget tree to a surface and save as PNG.
   */

  /* TODO: Implement proper widget-to-PNG export using Cairo:
   * 1. Create a cairo_surface_t (image surface)
   * 2. Snapshot the widget tree
   * 3. Save surface as PNG
   */

  sprintf(command, "gnome-screenshot -w -f %s.png 2>/dev/null || "
          "screencapture -w %s.png 2>/dev/null", fname, fname);

  int ii = system(command);
  if (ii && ++err_count == 1) {
    sprintf(mess, "Unable to write %s.png - screenshot tool not available", fname);
    sii_message(mess);
  }
  else {
    err_count = 0;
  }
  return ii;
}

/* c---------------------------------------------------------------------- */

/* c---------------------------------------------------------------------- */
