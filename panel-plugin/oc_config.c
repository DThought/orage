/* vim: set expandtab ts=4 sw=4: */
/*
 *
 *  Copyright © 2006 Juha Kautto <juha@xfce.org>
 *
 *  it under the terms of the GNU Library General Public License as published 
 *  by the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  Authors:
 *      Juha Kautto <juha@xfce.org>
 *      Based on Xfce panel plugin clock and date-time plugin
 */

#include <config.h>
#include <sys/stat.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkevents.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "orageclock.h"


/* -------------------------------------------------------------------- *
 *                        Configuration Dialog                          *
 * -------------------------------------------------------------------- */

static void oc_show_frame_toggled(GtkToggleButton *cb, Clock *clock)
{
    clock->show_frame = gtk_toggle_button_get_active(cb);
    oc_show_frame_set(clock);
}

static void oc_set_fg_toggled(GtkToggleButton *cb, Clock *clock)
{
    clock->fg_set = gtk_toggle_button_get_active(cb);
    oc_fg_set(clock);
}

static void oc_fg_color_changed(GtkWidget *widget, Clock *clock)
{
    gtk_color_button_get_color((GtkColorButton *)widget, &clock->fg);
    oc_fg_set(clock);
}

static void oc_set_bg_toggled(GtkToggleButton *cb, Clock *clock)
{
    clock->bg_set = gtk_toggle_button_get_active(cb);
    oc_bg_set(clock);
}

static void oc_bg_color_changed(GtkWidget *widget, Clock *clock)
{
    gtk_color_button_get_color((GtkColorButton *)widget, &clock->bg);
    oc_bg_set(clock);
}

static void oc_timezone_changed(GtkWidget *widget, GdkEventKey *key
        , Clock *clock)
{
    /* is it better to change only with GDK_Tab GDK_Return GDK_KP_Enter ? */
    g_string_assign(clock->timezone, gtk_entry_get_text(GTK_ENTRY(widget)));
    oc_timezone_set(clock);
}

static void oc_timezone_selected(GtkWidget *widget, Clock *clock)
{
    GtkWidget *dialog;
    gchar *filename = NULL;
    gchar *clockname = NULL;

    dialog = gtk_file_chooser_dialog_new("Select timezone", NULL
            , GTK_FILE_CHOOSER_ACTION_OPEN
            , GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL
            , GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
/* let's try to start on few standard positions */
    if (gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog)
            , "/usr/local/etc/zoneinfo/GMT") == FALSE)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog)
                , "/usr/share/zoneinfo/GMT");
    if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(clock->tz_entry), filename);
        g_string_assign(clock->timezone, filename);
        oc_timezone_set(clock);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void oc_show1(GtkToggleButton *cb, Clock *clock)
{
    clock->line[0].show = gtk_toggle_button_get_active(cb);
    oc_show_line_set(clock, 0);
}

static void oc_show2(GtkToggleButton *cb, Clock *clock)
{
    clock->line[1].show = gtk_toggle_button_get_active(cb);
    oc_show_line_set(clock, 1);
}

static void oc_show3(GtkToggleButton *cb, Clock *clock)
{
    clock->line[2].show = gtk_toggle_button_get_active(cb);
    oc_show_line_set(clock, 2);
}

static gboolean oc_line_changed(GtkWidget *entry, GdkEventKey *key
        , GString *data)
{
    g_string_assign(data, gtk_entry_get_text(GTK_ENTRY(entry)));

    return(FALSE);
}

static void oc_line_font_changed1(GtkWidget *widget, Clock *clock)
{
    g_string_assign(clock->line[0].font
            , gtk_font_button_get_font_name((GtkFontButton *)widget));
    oc_line_font_set(clock, 0);
}

static void oc_line_font_changed2(GtkWidget *widget, Clock *clock)
{
    g_string_assign(clock->line[1].font
            , gtk_font_button_get_font_name((GtkFontButton *)widget));
    oc_line_font_set(clock, 1);
}

static void oc_line_font_changed3(GtkWidget *widget, Clock *clock)
{
    g_string_assign(clock->line[2].font
            , gtk_font_button_get_font_name((GtkFontButton *)widget));
    oc_line_font_set(clock, 2);
}

static void oc_properties_appearance(GtkWidget *dlg, Clock *clock)
{
    GtkWidget *frame, *bin, *vbox, *cb, *hbox, *color;
    GdkColor def_fg, def_bg;
    GtkStyle *def_style;

    frame = xfce_create_framebox(_("Appearance"), &bin);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, FALSE, FALSE, 0);
    
    vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_add(GTK_CONTAINER(bin), vbox);

    cb = gtk_check_button_new_with_mnemonic(_("Show _frame"));
    gtk_box_pack_start(GTK_BOX(vbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), clock->show_frame);
    g_signal_connect(cb, "toggled", G_CALLBACK(oc_show_frame_toggled), clock);
    
    def_style = gtk_widget_get_default_style();
    def_fg = def_style->fg[GTK_STATE_NORMAL];
    def_bg = def_style->bg[GTK_STATE_NORMAL];

    /* foreground color */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic(_("set foreground _color:"));
    gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), clock->fg_set);
    g_signal_connect(cb, "toggled", G_CALLBACK(oc_set_fg_toggled), clock);

    if (!clock->fg_set) {
        clock->fg = def_fg;
    }
    color = gtk_color_button_new_with_color(&clock->fg);
    gtk_box_pack_start(GTK_BOX(hbox), color, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(color), "color-set"
            , G_CALLBACK(oc_fg_color_changed), clock);

    /* background color */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic(_("set _background color:"));
    gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), clock->bg_set);
    g_signal_connect(cb, "toggled", G_CALLBACK(oc_set_bg_toggled), clock);

    if (!clock->bg_set) {
        clock->bg = def_bg;
    }
    color = gtk_color_button_new_with_color(&clock->bg);
    gtk_box_pack_start(GTK_BOX(hbox), color, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(color), "color-set"
            , G_CALLBACK(oc_bg_color_changed), clock);
}

static void oc_properties_options(GtkWidget *dlg, Clock *clock)
{
    GtkWidget *frame, *bin, *vbox, *hbox, *cb, *label, *entry, *font, *button
            , *image;
    GtkStyle *def_style;
    gchar *def_font;

    frame = xfce_create_framebox(_("Clock Options"), &bin);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 6);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), frame, FALSE, FALSE, 0);

    vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_add(GTK_CONTAINER(bin), vbox);

    /* timezone */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new(_("set timezone to:"));
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    clock->tz_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(clock->tz_entry), clock->timezone->str);
    gtk_box_pack_start(GTK_BOX(hbox), clock->tz_entry, FALSE, FALSE, 5);
    g_signal_connect(clock->tz_entry, "key-release-event"
            , G_CALLBACK(oc_timezone_changed), clock);
    gtk_tooltips_set_tip(clock->tips, GTK_WIDGET(clock->tz_entry),
            _("Set any valid timezone (=TZ) value or pick one from the list.")
            , NULL);


    button = gtk_button_new_from_stock(GTK_STOCK_OPEN);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(button), "clicked"
            , G_CALLBACK(oc_timezone_selected), clock);

    /* line 1 */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic(_("show line _1:"));
    gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), clock->line[0].show);
    g_signal_connect(cb, "toggled", G_CALLBACK(oc_show1), clock);

    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), clock->line[0].data->str); 
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);
    g_signal_connect(entry, "key-release-event", G_CALLBACK(oc_line_changed)
            , clock->line[0].data);
    gtk_tooltips_set_tip(clock->tips, GTK_WIDGET(entry),
            ("Enter any valid strftime function parameter.")
            , NULL);
    
    def_style = gtk_widget_get_default_style();
    def_font = pango_font_description_to_string(def_style->font_desc);
    if (clock->line[0].font->len) {
        font = gtk_font_button_new_with_font(clock->line[0].font->str);
    }
    else {
        font = gtk_font_button_new_with_font(def_font);
    }
    gtk_box_pack_start(GTK_BOX(hbox), font, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(font), "font-set"
            , G_CALLBACK(oc_line_font_changed1), clock);

    /* line 2 */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic(_("show line _2:"));
    gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), clock->line[1].show);
    g_signal_connect(cb, "toggled", G_CALLBACK(oc_show2), clock);

    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), clock->line[1].data->str); 
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);
    g_signal_connect(entry, "key-release-event", G_CALLBACK(oc_line_changed)
            , clock->line[1].data);
    
    if (clock->line[1].font->len) {
        font = gtk_font_button_new_with_font(clock->line[1].font->str);
    }
    else {
        font = gtk_font_button_new_with_font(def_font);
    }
    gtk_box_pack_start(GTK_BOX(hbox), font, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(font), "font-set"
            , G_CALLBACK(oc_line_font_changed2), clock);
    
    /* line 3 */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    cb = gtk_check_button_new_with_mnemonic(_("show line _3:"));
    gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cb), clock->line[2].show);
    g_signal_connect(cb, "toggled", G_CALLBACK(oc_show3), clock);
    
    entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), clock->line[2].data->str); 
    gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 5);
    g_signal_connect(entry, "key-release-event", G_CALLBACK(oc_line_changed)
            , clock->line[2].data);
    
    if (clock->line[2].font->len) {
        font = gtk_font_button_new_with_font(clock->line[2].font->str);
    }
    else {
        font = gtk_font_button_new_with_font(def_font);
    }
    gtk_box_pack_start(GTK_BOX(hbox), font, FALSE, FALSE, 0);
    g_signal_connect(G_OBJECT(font), "font-set"
            , G_CALLBACK(oc_line_font_changed3), clock);

    /* hints */
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 6);

    image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DND);
    gtk_misc_set_alignment(GTK_MISC(image), 0.5f, 0.0f);
    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);

    label = gtk_label_new(_("This program uses strftime function to get time.\nUse any valid code to get time in the format you prefer.\nSome common codes are:\n\t%A = weekday\t\t\t%B = month\n\t%c = date & time\t\t%R = hour & minute\n\t%V = week number\t\t%Z = timezone in use\n\t%H = hours \t\t\t\t%M = minute\n\t%X = local time\t\t\t%x = local date"));
    gtk_misc_set_alignment(GTK_MISC(label), 0.0f, 0.0f);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
}

static void oc_dialog_response(GtkWidget *dlg, int reponse, Clock *clock)
{
    g_object_set_data(G_OBJECT(clock->plugin), "dialog", NULL);
    gtk_widget_destroy(dlg);
    xfce_panel_plugin_unblock_menu(clock->plugin);
    oc_write_rc_file(clock->plugin, clock);
}

void oc_properties_dialog(XfcePanelPlugin *plugin, Clock *clock)
{
    GtkWidget *dlg, *header;

    xfce_panel_plugin_block_menu(plugin);
    
    dlg = gtk_dialog_new_with_buttons(_("Properties"), 
            GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
            GTK_DIALOG_DESTROY_WITH_PARENT |
            GTK_DIALOG_NO_SEPARATOR,
            GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
            NULL);
    
    g_object_set_data(G_OBJECT(plugin), "dialog", dlg);

    gtk_window_set_position(GTK_WINDOW(dlg), GTK_WIN_POS_CENTER);
    
    g_signal_connect(dlg, "response", G_CALLBACK(oc_dialog_response), clock);

    gtk_container_set_border_width(GTK_CONTAINER(dlg), 2);
    
    header = xfce_create_header(NULL, _("Orage clock"));
    gtk_widget_set_size_request(GTK_BIN(header)->child, 200, 32);
    gtk_container_set_border_width(GTK_CONTAINER(header), 6);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), header, FALSE, TRUE, 0);
    
    oc_properties_appearance(dlg, clock);

    oc_properties_options(dlg, clock);

    gtk_widget_show_all(dlg);
}