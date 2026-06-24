/* sii_gtk4_compat.c - GTK4 compatibility helpers for soloiv
 *
 * Provides font management, color allocation, and drawing utilities
 * that bridge the gap between the old GDK/GTK1 API and GTK4+Cairo+Pango.
 */

#include "soloii.h"

/* c---------------------------------------------------------------------- */
/* Font management - replaces GdkFont with PangoFontDescription */
/* c---------------------------------------------------------------------- */

/* A single long-lived PangoContext shared by all text measurement.
 *
 * Previously sii_font_string_width() and sii_cairo_draw_text() created (and
 * destroyed) a fresh PangoContext + PangoLayout on every call -- many times
 * per frame draw. Each fresh context re-resolves the font family through the
 * platform backend; on macOS that drove Pango's CoreText font-descriptor
 * cache hard and was the source of a rare crash (objc_msgSend on a freed
 * CoreText object inside the draw path, GH #3). Reusing one context + one
 * cached layout per role keeps the resolved font warm and removes the
 * per-call churn. The GUI draws on the main thread only, so a process-wide
 * cache is safe. */
static PangoContext *sii_shared_context(void)
{
  static PangoContext *ctx = NULL;
  if (!ctx)
    ctx = pango_font_map_create_context(pango_cairo_font_map_get_default());
  return ctx;
}

SiiFont *sii_font_new(const char *family, int size_points, gboolean monospace)
{
  SiiFont *f = g_malloc0(sizeof(SiiFont));
  PangoContext *context;
  PangoLanguage *lang;

  f->desc = pango_font_description_new();

  if (monospace) {
    pango_font_description_set_family(f->desc, family ? family : "Monospace");
  } else {
    pango_font_description_set_family(f->desc, family ? family : "Sans");
  }

  /* size_points is in tenths of a point (e.g., 100 = 10pt, 120 = 12pt) */
  pango_font_description_set_size(f->desc,
    (size_points * PANGO_SCALE) / 10);

  /* Get metrics for ascent/descent calculation (shared context, not a
   * throwaway one). */
  context = sii_shared_context();
  lang = pango_language_get_default();
  f->metrics = pango_context_get_metrics(context, f->desc, lang);

  f->ascent = pango_font_metrics_get_ascent(f->metrics) / PANGO_SCALE;
  f->descent = pango_font_metrics_get_descent(f->metrics) / PANGO_SCALE;

  return f;
}

void sii_font_free(SiiFont *font)
{
  if (!font) return;
  if (font->desc) pango_font_description_free(font->desc);
  if (font->metrics) pango_font_metrics_unref(font->metrics);
  g_free(font);
}

gint sii_font_string_width(SiiFont *font, const gchar *text)
{
  static PangoLayout *layout = NULL;
  gint width, height;

  if (!layout)
    layout = pango_layout_new(sii_shared_context());
  pango_layout_set_font_description(layout, font->desc);
  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_size(layout, &width, &height);

  return width;
}

/* c---------------------------------------------------------------------- */
/* Cairo drawing helpers - replaces gdk_draw_* functions */
/* c---------------------------------------------------------------------- */

void sii_cairo_draw_text(cairo_t *cr, SiiFont *font, double x, double y,
                         const gchar *text, int len)
{
  static PangoLayout *layout = NULL;
  gchar *str;

  if (!text || len == 0) return;

  /* Reuse one layout, re-binding it to the current cairo context each call
   * (rather than creating + destroying a layout, and re-resolving the font,
   * on every draw). See sii_shared_context() above for the rationale. */
  if (!layout)
    layout = pango_cairo_create_layout(cr);
  else
    pango_cairo_update_layout(cr, layout);
  pango_layout_set_font_description(layout, font->desc);

  if (len < 0) {
    pango_layout_set_text(layout, text, -1);
  } else {
    str = g_strndup(text, len);
    pango_layout_set_text(layout, str, -1);
    g_free(str);
  }

  /* Position text: y is baseline in old API, but Pango draws from top-left */
  cairo_move_to(cr, x, y - font->ascent);
  pango_cairo_show_layout(cr, layout);
}

void sii_cairo_set_color(cairo_t *cr, GdkRGBA *color)
{
  if (!color) return;
  cairo_set_source_rgba(cr, color->red, color->green, color->blue, color->alpha);
}

/* c---------------------------------------------------------------------- */
/* Color management - replaces GdkColor + gdk_color_alloc */
/* c---------------------------------------------------------------------- */

/* GTK4 replacement for gtk_widget_destroyed().
 * Sets the widget pointer to NULL when the widget is destroyed.
 * Usage: g_signal_connect(widget, "destroy",
 *            G_CALLBACK(sii_widget_destroyed_cb), &widget_ptr);
 */
void sii_widget_destroyed_cb(GtkWidget *widget, GtkWidget **widget_pointer)
{
  if (widget_pointer)
    *widget_pointer = NULL;
}

GdkRGBA *sii_color_new(gfloat red, gfloat green, gfloat blue)
{
  GdkRGBA *c = g_malloc(sizeof(GdkRGBA));
  c->red = red;
  c->green = green;
  c->blue = blue;
  c->alpha = 1.0;
  return c;
}
