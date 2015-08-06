/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * html.c
 *
 * Copyright (C) 1999, 2000 Rasca, Berlin
 * EMail: thron@gmx.de
 * Copyright (c) 2001-2013 Andreas J. Guelzow
 * EMail: aguelzow@pyrshep.ca
 * Copyright 2013 Morten Welinder <terra@gnone.org>
 *
 * Contributors :
 *   Almer. S. Tigelaar <almer1@dds.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gnumeric.h>
#include <goffice/goffice.h>
#include "workbook-view.h"
#include "workbook.h"
#include "sheet-style.h"
#include "style.h"
#include "style-color.h"
#include "html.h"
#include "cell.h"
#include "sheet.h"
#include "sheet-merge.h"
#include "value.h"
#include "font.h"
#include "cellspan.h"
#include "style-border.h"
#include <rendered-value.h>
#include "style.h"
#include "hlink.h"

#include <gsf/gsf-output.h>
#include <string.h>


/*
 * html_version_t:
 *
 * version selector
 *
 */
typedef enum {
	HTML40    = 0,
	HTML32    = 1,
	HTML40F   = 2,
	XHTML     = 3
} html_version_t;

/*
 * html_print_encoded:
 *
 * @output: the stream
 * @str: the string
 *
 * print the string to output encoded all special chars
 *
 */
static void
html_print_encoded (GsfOutput *output, char const *str)
{
    gunichar c;

	if (str == NULL)
		return;
	for (; *str != '\0' ; str = g_utf8_next_char (str)) {
		switch (*str) {
			case '<':
				gsf_output_puts (output, "&lt;");
				break;
			case '>':
				gsf_output_puts (output, "&gt;");
				break;
			case '&':
				gsf_output_puts (output, "&amp;");
				break;
			case '\"':
				gsf_output_puts (output, "&quot;");
				break;
			case '\n':
				gsf_output_puts (output, "<br />\n");
				break;
			case '\r':
				gsf_output_puts (output, "<br />\r");
				if( *(str+1) == '\n' ) {
					gsf_output_puts (output, "\n");
					str++;
				}
				break;
			default:
				c = g_utf8_get_char (str);
                gsf_output_puts (output, g_ucs4_to_utf8(&c, 1, NULL, NULL, NULL));
				break;
		}
	}
}

static void
html_print_encoded_no_br (GsfOutput *output, char const *str)
{
    gunichar c;

    if (str == NULL)
        return;
    for (; *str != '\0' ; str = g_utf8_next_char (str)) {
        switch (*str) {
            case '<':
                gsf_output_puts (output, "&lt;");
                break;
            case '>':
                gsf_output_puts (output, "&gt;");
                break;
            case '&':
                gsf_output_puts (output, "&amp;");
                break;
            case '\"':
                gsf_output_puts (output, "&quot;");
                break;
            default:
                c = g_utf8_get_char (str);
                gsf_output_puts (output, g_ucs4_to_utf8(&c, 1, NULL, NULL, NULL));
                break;
        }
    }
}

static void
html_get_text_color (GnmCell *cell, GnmStyle const *style, guint *r, guint *g, guint *b)
{
	GOColor fore = gnm_cell_get_render_color (cell);

	if (fore == 0)
		*r = *g = *b = 0;
	else {
		*r = GO_COLOR_UINT_R (fore);
		*g = GO_COLOR_UINT_G (fore);
		*b = GO_COLOR_UINT_B (fore);
	}
}


/*****************************************************************************/


static void
cb_html_add_chars (GsfOutput *output, char const *text, int len)
{
	char * str;

	g_return_if_fail (len > 0);

	str = g_strndup (text, len);
	html_print_encoded (output, str);
	g_free (str);
}

static char const *
cb_html_attrs_as_string (GsfOutput *output, PangoAttribute *a, html_version_t version)
{
/* 	PangoColor const *c; */
	char const *closure = NULL;

	switch (a->klass->type) {
	case PANGO_ATTR_FAMILY :
		break; /* ignored */
	case PANGO_ATTR_SIZE :
		break; /* ignored */
	case PANGO_ATTR_RISE:
		if (((PangoAttrInt *)a)->value > 5) {
			gsf_output_puts (output, "<sup>");
			closure = "</sup>";
		} else if (((PangoAttrInt *)a)->value < -5) {
			gsf_output_puts (output, "<sub>");
			closure = "</sub>";
		}
		break;
	case PANGO_ATTR_STYLE :
		if (((PangoAttrInt *)a)->value == PANGO_STYLE_ITALIC) {
			gsf_output_puts (output, "<i>");
			closure = "</i>";
		}
		break;
	case PANGO_ATTR_WEIGHT :
		if (((PangoAttrInt *)a)->value > 600){
			gsf_output_puts (output, "<b>");
			closure = "</b>";
		}
		break;
	case PANGO_ATTR_STRIKETHROUGH :
		if (((PangoAttrInt *)a)->value == 1) {
			if (version == HTML32) {
				gsf_output_puts (output, "<strike>");
				closure = "</strike>";
			} else {
				gsf_output_puts
					(output,
					 "<span style=\"text-decoration: "
					 "line-through;\">");
				closure = "</span>";
			}
		}
		break;
	case PANGO_ATTR_UNDERLINE :
		if ((version != HTML40) &&
		    (((PangoAttrInt *)a)->value != PANGO_UNDERLINE_NONE)) {
			gsf_output_puts (output, "<u>");
			closure = "</u>";
		}
		break;
	case PANGO_ATTR_FOREGROUND :
/* 		c = &((PangoAttrColor *)a)->color; */
/* 		g_string_append_printf (accum, "[color=%02xx%02xx%02x", */
/* 			((c->red & 0xff00) >> 8), */
/* 			((c->green & 0xff00) >> 8), */
/* 			((c->blue & 0xff00) >> 8)); */
		break;/* ignored */
	default :
		if (a->klass->type ==
		    go_pango_attr_subscript_get_attr_type ()) {
			if (((GOPangoAttrSubscript *)a)->val) {
				gsf_output_puts (output, "<sub>");
				closure = "</sub>";
			}
		} else if (a->klass->type ==
			   go_pango_attr_superscript_get_attr_type ()) {
			if (((GOPangoAttrSuperscript *)a)->val) {
				gsf_output_puts (output, "<sup>");
				closure = "</sup>";
			}
		}
		break; /* ignored */
	}

	return closure;
}

static void
html_new_markup (GsfOutput *output, const PangoAttrList *markup, char const *text,
		 html_version_t version)
{
	int handled = 0;
	PangoAttrIterator * iter;
	int from, to;
	int len = strlen (text);
	GString *closure = g_string_new ("");

	iter = pango_attr_list_get_iterator ((PangoAttrList *) markup);

	do {
		GSList *list, *l;

		g_string_erase (closure, 0, -1);
		pango_attr_iterator_range (iter, &from, &to);
		from = (from > len) ? len : from; /* Since "from" can be really big! */
		to = (to > len) ? len : to;       /* Since "to" can be really big!   */
		if (from > handled)
			cb_html_add_chars (output, text + handled, from - handled);
		list = pango_attr_iterator_get_attrs (iter);
		for (l = list; l != NULL; l = l->next) {
			char const *result = cb_html_attrs_as_string (output, l->data, version);
			if (result != NULL)
				g_string_prepend (closure, result);
		}
		g_slist_free (list);
		if (to > from)
			cb_html_add_chars (output, text + from, to - from);
		gsf_output_puts (output, closure->str);
		handled = to;
	} while (pango_attr_iterator_next (iter));

	g_string_free (closure, TRUE);
	pango_attr_iterator_destroy (iter);

	return;
}


/*****************************************************************************/

static void
html_write_cell_content (GsfOutput *output, GnmCell *cell, GnmStyle const *style, html_version_t version)
{
	guint r = 0;
	guint g = 0;
	guint b = 0;
	char *rendered_string;
	gboolean hidden = gnm_style_get_contents_hidden (style);
	GnmHLink* hlink = gnm_style_get_hlink (style);
	const gchar* hlink_target = NULL;

	if (hlink && IS_GNM_HLINK_URL (hlink)) {
		hlink_target = gnm_hlink_get_target (hlink);
	}

	if (version == HTML32 && hidden)
		gsf_output_puts (output, "<!-- 'HIDDEN DATA' -->");
	else {
		if (style != NULL) {
			if (gnm_style_get_font_italic (style))
				gsf_output_puts (output, "<i>");
			if (gnm_style_get_font_bold (style))
				gsf_output_puts (output, "<b>");
			if (gnm_style_get_font_uline (style) != UNDERLINE_NONE)
				gsf_output_puts (output, "<u>");
			if (font_is_monospaced (style))
				gsf_output_puts (output, "<tt>");
			if (gnm_style_get_font_strike (style)) {
				if (version == HTML32)
					gsf_output_puts (output, "<strike>");
				else
					gsf_output_puts (output,
							 "<span style=\"text-decoration: line-through;\">");
			}
			switch (gnm_style_get_font_script (style)) {
			case GO_FONT_SCRIPT_SUB:
				gsf_output_puts (output, "<sub>");
				break;
			case GO_FONT_SCRIPT_SUPER:
				gsf_output_puts (output, "<sup>");
				break;
			default:
				break;
			}
		}

		if (hlink_target)
			gsf_output_printf (output, "<a href=\"%s\">", hlink_target);

		if (cell != NULL) {
			const PangoAttrList * markup = NULL;

			if (style != NULL && version != HTML40) {
				html_get_text_color (cell, style, &r, &g, &b);
				if (r > 0 || g > 0 || b > 0)
					gsf_output_printf (output, "<font color=\"#%02X%02X%02X\">", r, g, b);
			}

			if ((cell->value->type == VALUE_STRING)
			    && (VALUE_FMT (cell->value) != NULL)
			    && go_format_is_markup (VALUE_FMT (cell->value)))
				markup = go_format_get_markup (VALUE_FMT (cell->value));

			if (markup != NULL) {
				GString *str = g_string_new ("");
				value_get_as_gstring (cell->value, str, NULL);
				html_new_markup (output, markup, str->str, version);
				g_string_free (str, TRUE);
			} else {
				rendered_string = gnm_cell_get_rendered_text (cell);
				html_print_encoded (output, rendered_string);
				g_free (rendered_string);
			}
		}

		if (r > 0 || g > 0 || b > 0)
			gsf_output_puts (output, "</font>");
		if (hlink_target)
			gsf_output_puts (output, "</a>");
		if (style != NULL) {
			if (gnm_style_get_font_strike (style)) {
				if (version == HTML32)
					gsf_output_puts (output, "</strike>");
				else
					gsf_output_puts (output, "</span>");
			}
			switch (gnm_style_get_font_script (style)) {
			case GO_FONT_SCRIPT_SUB:
				gsf_output_puts (output, "</sub>");
				break;
			case GO_FONT_SCRIPT_SUPER:
				gsf_output_puts (output, "</sup>");
				break;
			default:
				break;
			}
			if (font_is_monospaced (style))
				gsf_output_puts (output, "</tt>");
			if (gnm_style_get_font_uline (style) != UNDERLINE_NONE)
				gsf_output_puts (output, "</u>");
			if (gnm_style_get_font_bold (style))
				gsf_output_puts (output, "</b>");
			if (gnm_style_get_font_italic (style))
				gsf_output_puts (output, "</i>");
		}
	}
}

static void
write_cell (GsfOutput *output, Sheet *sheet, gint row, gint col, html_version_t version, gboolean is_merge)
{
	GnmCell *cell;
	GnmStyle const *style;

	style = sheet_style_get (sheet, col, row);
	cell = sheet_cell_get (sheet, col, row);
	gsf_output_printf (output, ">");
	html_write_cell_content (output, cell, style, version);
	gsf_output_puts (output, "</td>\n");
}


/*
 * write_row:
 *
 * @output: the stream
 * @sheet: the gnumeric sheet
 * @row: the row number
 *
 * Set up a TD node for each cell in the given row, witht eh  appropriate
 * colspan and rowspan.
 * Call write_cell for each cell.
 */
static void
write_row (GsfOutput *output, Sheet *sheet, gint row, GnmRange *range, html_version_t version)
{
	char const *text = NULL;
    GnmCell *cell;


    gint col;
    ColRowInfo const *ri = sheet_row_get_info (sheet, row);
    if (ri->needs_respan)
        row_calc_spans ((ColRowInfo *) ri, row, sheet);

    for (col = range->start.col; col <= range->end.col; col++) {
        GnmRange const *merge_range;
        GnmCellPos pos;
        pos.col = col;
        pos.row = row;


        /* is this covered by a merge */
        merge_range = gnm_sheet_merge_contains_pos  (sheet, &pos);
        if (merge_range != NULL) {
            cell = sheet_cell_get (sheet, merge_range->start.col, merge_range->start.row);
            if (cell != NULL && cell->value) {
                text = value_peek_string (cell->value);
                gsf_output_puts (output, "<td raw=\"");
                html_print_encoded_no_br (output, text);
                gsf_output_puts (output, "\" ");

                write_cell (output, sheet, merge_range->start.row, merge_range->start.col, version, FALSE);
                continue;
            }

            gsf_output_puts (output, "<td ");
            write_cell (output, sheet, merge_range->start.row, merge_range->start.col, version, FALSE);
            continue;
        }


        cell = sheet_cell_get (sheet, col, row);
        if (cell != NULL && cell->value) {
            text = value_peek_string (cell->value);
            gsf_output_puts (output, "<td raw=\"");
            html_print_encoded_no_br (output, text);
            gsf_output_puts (output, "\" ");

            write_cell (output, sheet, row, col, version, FALSE);
            continue;
        }

        gsf_output_puts (output, "<td ");
		write_cell (output, sheet, row, col, version, FALSE);
	}
}

/*
 * write_sheet:
 *
 * @output: the stream
 * @sheet: the gnumeric sheet
 *
 * set up a table and call write_row for each row
 */
static void
write_sheet (GsfOutput *output, Sheet *sheet,
	     html_version_t version, GOFileSaveScope save_scope)
{
	GnmRange total_range;
	gint row;

	gsf_output_puts (output, "<table>\n");

	total_range = sheet_get_extent (sheet, TRUE, TRUE);
	for (row = total_range.start.row; row <=  total_range.end.row; row++) {
		gsf_output_puts (output, "<tr>\n");
		write_row (output, sheet, row, &total_range, version);
		gsf_output_puts (output, "</tr>\n");
	}
	gsf_output_puts (output, "</table>\n");
}

/*
 * html_file_save:
 *
 * write the html file (version of html according to version argument)
 */
static void
html_file_save (GOFileSaver const *fs, GOIOContext *io_context,
		WorkbookView const *wb_view, GsfOutput *output, html_version_t version)
{
	GSList *sheets, *ptr;
	Workbook *wb = wb_view_get_workbook (wb_view);
	GOFileSaveScope save_scope;

	g_return_if_fail (fs != NULL);
	g_return_if_fail (wb != NULL);
	g_return_if_fail (output != NULL);

	gsf_output_puts (output,
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"\t\t\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
"<html>\n"
"<head>\n\t<title>Tables 123</title>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
"<meta name=\"generator\" content=\"Gnumeric " GNM_VERSION_FULL  " via " G_PLUGIN_FOR_HTML "\" />\n"
"</head>\n<body>\n");

	sheets = workbook_sheets (wb);
	save_scope = go_file_saver_get_save_scope (fs);
	for (ptr = sheets ; ptr != NULL ; ptr = ptr->next) {
		write_sheet (output, (Sheet *) ptr->data, version, save_scope);
	}
	g_slist_free (sheets);
	if (version == HTML32 || version == HTML40 || version == XHTML)
		gsf_output_puts (output, "</body>\n</html>\n");
}

void
pwhtml_file_save (GOFileSaver const *fs, GOIOContext *io_context,
		 WorkbookView const *wb_view, GsfOutput *output)
{
	html_file_save (fs, io_context, wb_view, output, XHTML);
}

