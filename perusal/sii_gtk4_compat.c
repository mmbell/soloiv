/* sii_gtk4_compat.c - GTK4 compatibility helpers for soloiv
 *
 * Provides font management, color allocation, and drawing utilities
 * that bridge the gap between the old GDK/GTK1 API and GTK4+Cairo+Pango.
 */

#include "soloii.h"

/* c---------------------------------------------------------------------- */
/* Font management - replaces GdkFont with PangoFontDescription */
/* c---------------------------------------------------------------------- */

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

  /* Get metrics for ascent/descent calculation */
  context = pango_font_map_create_context(
    pango_cairo_font_map_get_default());
  lang = pango_language_get_default();
  f->metrics = pango_context_get_metrics(context, f->desc, lang);

  f->ascent = pango_font_metrics_get_ascent(f->metrics) / PANGO_SCALE;
  f->descent = pango_font_metrics_get_descent(f->metrics) / PANGO_SCALE;

  g_object_unref(context);

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
  PangoContext *context;
  PangoLayout *layout;
  gint width, height;

  context = pango_font_map_create_context(
    pango_cairo_font_map_get_default());
  layout = pango_layout_new(context);
  pango_layout_set_font_description(layout, font->desc);
  pango_layout_set_text(layout, text, -1);
  pango_layout_get_pixel_size(layout, &width, &height);

  g_object_unref(layout);
  g_object_unref(context);

  return width;
}

/* c---------------------------------------------------------------------- */
/* Cairo drawing helpers - replaces gdk_draw_* functions */
/* c---------------------------------------------------------------------- */

void sii_cairo_draw_text(cairo_t *cr, SiiFont *font, double x, double y,
                         const gchar *text, int len)
{
  PangoLayout *layout;
  gchar *str;

  if (!text || len == 0) return;

  layout = pango_cairo_create_layout(cr);
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

  g_object_unref(layout);
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
