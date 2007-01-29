/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2003-2005 Mickael Graf (korbinus at xfce.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the 
       Free Software Foundation
       51 Franklin Street, 5th Floor
       Boston, MA 02110-1301 USA

 */

#include <stdio.h>
#include <stdlib.h>
#define _XOPEN_SOURCE /* glibc2 needs this */
#include <time.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libxfce4util/libxfce4util.h>


/**************************************
 *  Functions for drawing interfaces  *
 **************************************/


struct tm orage_i18_date_to_tm_date(const char *i18_date)
{
    const char *date_format;
    char *ret;
    struct tm tm_date = {0,0,0,0,0,0,0,0,0};

    date_format = _("%m/%d/%Y");
    ret = (char *)strptime(i18_date, date_format, &tm_date);
    if (ret == NULL)
        g_error("Orage: orage_i18_date_to_tm_date wrong format (%s)"
                , i18_date);
    else if (strlen(ret))
        g_error("Orage: orage_i18_date_to_tm_date too long format (%s)"
                , i18_date);
    return(tm_date);
}

char *orage_tm_date_to_i18_date(struct tm tm_date)
{
    const char *date_format;
    static char i18_date[32];
    /*
    struct tm d = {0,0,0,0,0,0,0,0,0};
    */

    date_format = _("%m/%d/%Y");
    /*
    d.tm_mday = day;
    d.tm_mon = month - 1;
    d.tm_year = year - 1900;
    */

    if (strftime(i18_date, 32, date_format, &tm_date))
        return(i18_date);
    else {
        g_error("Orage: orage_tm_date_to_i18_date too long string in strftime");
        return(NULL);
    }
}

GtkWidget *orage_toolbar_append_button(GtkWidget *toolbar
    , const gchar *stock_id, GtkTooltips *tooltips, const char *tooltip_text
    , gint pos)
{
    GtkWidget *button;

    button = (GtkWidget *)gtk_tool_button_new_from_stock(stock_id);
    gtk_tool_item_set_tooltip(GTK_TOOL_ITEM(button), tooltips
            , (const gchar *) tooltip_text, NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), pos);
    return button;
}

GtkWidget *orage_toolbar_append_separator(GtkWidget *toolbar, gint pos)
{
    GtkWidget *separator;

    separator = (GtkWidget *)gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(separator), pos);

    return separator;
}

GtkWidget *orage_table_new(guint rows, guint border)
{
    GtkWidget *table;

    table = gtk_table_new(rows, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(table), border);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    return table;
}

void orage_table_add_row(GtkWidget *table, GtkWidget *label
        , GtkWidget *input, guint row
        , GtkAttachOptions input_x_option
        , GtkAttachOptions input_y_option)
{
    if (label) {
        gtk_table_attach(GTK_TABLE (table), label, 0, 1, row, row+1
                , (GTK_FILL), (0), 0, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    }

    if (input) {
        gtk_table_attach(GTK_TABLE(table), input, 1, 2, row, row+1,
                  input_x_option, input_y_option, 0, 0);
    }
}

GtkWidget *orage_menu_new(const gchar *menu_header_title
    , GtkWidget *menu_bar)
{
    GtkWidget *menu_header, *menu;

    menu_header = gtk_menu_item_new_with_mnemonic(menu_header_title);
    gtk_container_add(GTK_CONTAINER(menu_bar), menu_header);

    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_header), menu);

    return menu;
}

GtkWidget *orage_image_menu_item_new_from_stock(const gchar *stock_id
    , GtkWidget *menu, GtkAccelGroup *ag)
{
    GtkWidget *menu_item;

    menu_item = gtk_image_menu_item_new_from_stock(stock_id, ag);
    gtk_container_add(GTK_CONTAINER(menu), menu_item);
    return menu_item;
}

GtkWidget *orage_separator_menu_item_new(GtkWidget *menu)
{
    GtkWidget *menu_item;

    menu_item = gtk_separator_menu_item_new();
    gtk_container_add(GTK_CONTAINER(menu), menu_item);
    return menu_item;
}

GtkWidget *orage_menu_item_new_with_mnemonic(const gchar *label
    , GtkWidget *menu)
{
    GtkWidget *menu_item;

    menu_item = gtk_menu_item_new_with_mnemonic(label);
    gtk_container_add(GTK_CONTAINER(menu), menu_item);
    return menu_item;
}

struct tm *orage_localtime()
{
    time_t tt;

    tt = time(NULL);
    return(localtime(&tt));
}

void orage_select_date(GtkCalendar *cal
    , guint year, guint month, guint day)
{
    gtk_calendar_select_day(cal, 0); /* needed to avoid illegal day/month */
    gtk_calendar_select_month(cal, month, year);
    gtk_calendar_select_day(cal, day);
}

void orage_select_today(GtkCalendar *cal)
{
    struct tm *t;

    t = orage_localtime();
    orage_select_date(cal, t->tm_year+1900, t->tm_mon, t->tm_mday);
}
