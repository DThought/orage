/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2008 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2004-2005 Mickael Graf (korbinus at xfce.org)
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "orage-i18n.h"
#include "functions.h"
#include "mainbox.h"
#include "about-xfcalendar.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "interface.h"
#include "parameters.h"
#include "tray_icon.h"
#include "day-view.h"

/*
#define ORAGE_DEBUG 1
*/


gboolean orage_mark_appointments()
{
#undef P_N
#define P_N "orage_mark_appointments: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (!xfical_file_open(TRUE))
        return(FALSE);
    xfical_mark_calendar(GTK_CALENDAR(g_par.xfcal->mCalendar));
    xfical_file_close(TRUE);
    return(TRUE);
}

static void mFile_newApp_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
#undef P_N
#define P_N "mFile_newApp_activate_cb: "
    struct tm *t;
    char cur_date[9];

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    t = orage_localtime();
    g_snprintf(cur_date, 9, "%04d%02d%02d", t->tm_year+1900
            , t->tm_mon+1, t->tm_mday);
    create_appt_win("NEW", cur_date);  
}

static void mFile_interface_activate_cb(GtkMenuItem *menuitem
        , gpointer user_data)
{
#undef P_N
#define P_N "mFile_interface_activate_cb: "
    CalWin *cal = (CalWin *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    orage_external_interface(cal);
}

static void mFile_close_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
#undef P_N
#define P_N "mFile_close_activate_cb: "
    CalWin *cal = (CalWin *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    gtk_widget_hide(cal->mWindow);
}

static void mFile_quit_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
#undef P_N
#define P_N "mFile_quit_activate_cb: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    gtk_main_quit();
}

static void mEdit_preferences_activate_cb(GtkMenuItem *menuitem
        , gpointer user_data)
{
#undef P_N
#define P_N "mEdit_preferences_activate_cb: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    show_parameters();
}

static void mView_ViewSelectedDate_activate_cb(GtkMenuItem *menuitem
        , gpointer user_data)
{
#undef P_N
#define P_N "mView_ViewSelectedDate_activate_cb: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    create_el_win(NULL);
}

static void mView_ViewSelectedWeek_activate_cb(GtkMenuItem *menuitem
        , gpointer user_data)
{
#undef P_N
#define P_N "mView_ViewSelectedWeek_activate_cb: "
    CalWin *cal = (CalWin *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    create_day_win(orage_cal_to_i18_date(GTK_CALENDAR(cal->mCalendar)));
}

static void mView_selectToday_activate_cb(GtkMenuItem *menuitem
        , gpointer user_data)
{
#undef P_N
#define P_N "mView_selectToday_activate_cb: "
    CalWin *cal = (CalWin *)user_data;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    orage_select_today(GTK_CALENDAR(cal->mCalendar));
}

static void mHelp_help_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
#undef P_N
#define P_N "mHelp_help_activate_cb: "
    gchar *helpdoc;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    helpdoc = g_strconcat("xfbrowser4 ", PACKAGE_DATA_DIR
           , G_DIR_SEPARATOR_S, "orage"
           , G_DIR_SEPARATOR_S, "doc"
           , G_DIR_SEPARATOR_S, "C"
           , G_DIR_SEPARATOR_S, "orage.html", NULL);
    orage_exec(helpdoc, NULL, NULL);
}

static void mHelp_about_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
#undef P_N
#define P_N "mHelp_about_activate_cb: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    create_wAbout((GtkWidget *)menuitem, user_data);
}

static void mCalendar_day_selected_double_click_cb(GtkCalendar *calendar
        , gpointer user_data)
{
#undef P_N
#define P_N "mCalendar_day_selected_double_click_cb: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (g_par.show_days)
        create_day_win(orage_cal_to_i18_date(calendar));
    else
        create_el_win(NULL);
}

void orage_show_events_for_selected_date(void)
{
    /* rebuild the info for the selected date */
    if (!xfical_file_open(TRUE))
        return;
    build_mainbox_info();
    xfical_file_close(TRUE);   
}

static void mCalendar_day_selected_cb(GtkCalendar *calendar
        , gpointer user_data)
{
#undef P_N
#define P_N "mCalendar_day_selected_cb: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    orage_show_events_for_selected_date();
}

static gboolean upd_calendar(GtkCalendar *calendar)
{
#undef P_N
#define P_N "upd_calendar: "
    static guint year=-1, month=-1;
    guint year_n, month_n, day_n;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /* we only need to do this if it is really a new month. We may get
     * many of these while calender is changing months and it is enough
     * to show only the last one, which is visible */
    gtk_calendar_get_date(calendar, &year_n, &month_n, &day_n);
    if (month != month_n || year != year_n) { /* need really do it */
        orage_mark_appointments();
        year = year_n;
        month = month_n;
    }
    return(FALSE); /* we do this only once */
}

void mCalendar_month_changed_cb(GtkCalendar *calendar, gpointer user_data)
{
#undef P_N
#define P_N "mCalendar_month_changed_cb: "
#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /* orage_mark_appointments is rather heavy (=slow), so doing
     * it here is not a good idea. We can't keep up with the autorepeat
     * speed if we do the whole thing here. bug 2080 proofs it. so let's
     * throw it to background and do it later */
    g_timeout_add(500, (GtkFunction)upd_calendar, calendar);
}

static void build_menu(void)
{
#undef P_N
#define P_N "build_menu: "
    GtkWidget *menu_separator;
    CalWin *cal = g_par.xfcal;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    cal->mMenubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(cal->mVbox), cal->mMenubar, FALSE, FALSE, 0);

    /* File menu */
    cal->mFile_menu = orage_menu_new(_("_File"), cal->mMenubar);

    cal->mFile_newApp = orage_image_menu_item_new_from_stock("gtk-new"
            , cal->mFile_menu, cal->mAccel_group);

    menu_separator = orage_separator_menu_item_new(cal->mFile_menu);

    cal->mFile_interface = 
            orage_menu_item_new_with_mnemonic(_("_Exchange data")
                    , cal->mFile_menu);

    menu_separator = orage_separator_menu_item_new(cal->mFile_menu);

    cal->mFile_close = orage_image_menu_item_new_from_stock("gtk-close"
            , cal->mFile_menu, cal->mAccel_group);
    cal->mFile_quit = orage_image_menu_item_new_from_stock("gtk-quit"
            , cal->mFile_menu, cal->mAccel_group);

    /* Edit menu */
    cal->mEdit_menu = orage_menu_new(_("_Edit"), cal->mMenubar);

    cal->mEdit_preferences = 
            orage_image_menu_item_new_from_stock("gtk-preferences"
                    , cal->mEdit_menu, cal->mAccel_group);

    /* View menu */
    cal->mView_menu = orage_menu_new(_("_View"), cal->mMenubar);

    cal->mView_ViewSelectedDate = 
            orage_menu_item_new_with_mnemonic(_("View selected _date")
                    , cal->mView_menu);
    cal->mView_ViewSelectedWeek = 
            orage_menu_item_new_with_mnemonic(_("View selected _week")
                    , cal->mView_menu);

    menu_separator = orage_separator_menu_item_new(cal->mView_menu);

    cal->mView_selectToday = 
            orage_menu_item_new_with_mnemonic(_("Select _Today")
                    , cal->mView_menu);

    /* Help menu */
    cal->mHelp_menu = orage_menu_new(_("_Help"), cal->mMenubar);
    cal->mHelp_help = orage_image_menu_item_new_from_stock("gtk-help"
            , cal->mHelp_menu, cal->mAccel_group);
    cal->mHelp_about = orage_image_menu_item_new_from_stock("gtk-about"
            , cal->mHelp_menu, cal->mAccel_group);

    gtk_widget_show_all(g_par.xfcal->mMenubar);

    /* Signals */
    g_signal_connect((gpointer) cal->mFile_newApp, "activate"
            , G_CALLBACK(mFile_newApp_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mFile_interface, "activate"
            , G_CALLBACK(mFile_interface_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mFile_close, "activate"
            , G_CALLBACK(mFile_close_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mFile_quit, "activate"
            , G_CALLBACK(mFile_quit_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mEdit_preferences, "activate"
            , G_CALLBACK(mEdit_preferences_activate_cb), NULL);
    g_signal_connect((gpointer) cal->mView_ViewSelectedDate, "activate"
            , G_CALLBACK(mView_ViewSelectedDate_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mView_ViewSelectedWeek, "activate"
            , G_CALLBACK(mView_ViewSelectedWeek_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mView_selectToday, "activate"
            , G_CALLBACK(mView_selectToday_activate_cb),(gpointer) cal);
    g_signal_connect((gpointer) cal->mHelp_help, "activate"
            , G_CALLBACK(mHelp_help_activate_cb), NULL);
    g_signal_connect((gpointer) cal->mHelp_about, "activate"
            , G_CALLBACK(mHelp_about_activate_cb),(gpointer) cal);
}

static void todo_clicked(GtkWidget *widget
        , GdkEventButton *event, gpointer *user_data)
{
#undef P_N
#define P_N "todo_clicked: "
    gchar *uid;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (event->type==GDK_2BUTTON_PRESS) {
        uid = g_object_get_data(G_OBJECT(widget), "UID");
        create_appt_win("UPDATE", uid);
    }
}

static void add_info_row(xfical_appt *appt, GtkBox *parentBox, gboolean todo)
{
#undef P_N
#define P_N "add_info_row: "
    GtkWidget *ev, *label;
    CalWin *cal = g_par.xfcal;
    gchar *tip, *tmp;
    struct tm *t;
    char      *l_time, *s_time, *e_time, *c_time, *na;
    gint  len;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    /***** add data into the vbox *****/
    ev = gtk_event_box_new();
    tmp = g_strdup_printf("  %s", appt->title);
    label = gtk_label_new(tmp);
    g_free(tmp);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 5, 0);
    gtk_container_add(GTK_CONTAINER(ev), label);
    gtk_box_pack_start(parentBox, ev, FALSE, FALSE, 0);
    g_object_set_data_full(G_OBJECT(ev), "UID", g_strdup(appt->uid), g_free);
    g_signal_connect((gpointer)ev, "button-press-event"
            , G_CALLBACK(todo_clicked), cal);

    /***** set color *****/
    if (todo) {
        t = orage_localtime();
        l_time = orage_tm_time_to_icaltime(t);
        if (appt->starttimecur[8] == 'T') /* date+time */
            len = 15;
        else /* date only */
            len = 8;
        if (strncmp(appt->endtimecur,  l_time, len) < 0) /* gone */
            gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &cal->mRed);
        else if (strncmp(appt->starttimecur,  l_time, len) <= 0
             &&  strncmp(appt->endtimecur,  l_time, len) >= 0)
            gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &cal->mBlue);
    }

    /***** set hint *****/
    s_time = g_strdup(orage_icaltime_to_i18_time(appt->starttimecur));
    e_time = g_strdup(orage_icaltime_to_i18_time(appt->endtimecur));
    if (todo) {
        na = _("Never");
        c_time = g_strdup(appt->completed
                ? orage_icaltime_to_i18_time(appt->completedtime) : na);
        tip = g_strdup_printf(_("Title: %s\n Start:\t%s\n Due:\t%s\n Done:\t%s\nNote:\n%s")
                , appt->title, s_time, e_time, c_time, appt->note);
        g_free(c_time);
    }
    else { /* it is event */
        tip = g_strdup_printf(_("Title: %s\n Start:\t%s\n End:\t%s\n Note:\n%s")
                , appt->title, s_time, e_time, appt->note);
    }
    gtk_tooltips_set_tip(cal->Tooltips, ev, tip, NULL);
    g_free(s_time);
    g_free(e_time);
    g_free(tip);
}

static void insert_rows(GList **todo_list, char *a_day, xfical_type ical_type
        , gchar *file_type)
{
#undef P_N
#define P_N "insert_rows: "
    xfical_appt *appt;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    for (appt = xfical_appt_get_next_on_day(a_day, TRUE, 0
                , ical_type , file_type);
         appt;
         appt = xfical_appt_get_next_on_day(a_day, FALSE, 0
                , ical_type , file_type)) {
        *todo_list = g_list_prepend(*todo_list, appt);
    }
}

static gint list_order(gconstpointer a, gconstpointer b)
{
#undef P_N
#define P_N "list_order: "
    xfical_appt *appt1, *appt2;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    appt1 = (xfical_appt *)a;
    appt2 = (xfical_appt *)b;

    return(strcmp(appt1->starttimecur, appt2->starttimecur));
}

static void info_process(gpointer a, gpointer pbox)
{
#undef P_N
#define P_N "info_process: "
    xfical_appt *appt = (xfical_appt *)a;;
    GtkBox *box = GTK_BOX(pbox);
    CalWin *cal = g_par.xfcal;
    gboolean todo;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    if (pbox == cal->mTodo_rows_vbox)
        todo = TRUE;
    else
        todo = FALSE;
    add_info_row(appt, box, todo);
    xfical_appt_free(appt);
}

void create_mainbox_todo_info(void)
{
#undef P_N
#define P_N "create_mainbox_todo_info: "
    CalWin *cal = g_par.xfcal;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    cal->mTodo_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cal->mVbox), cal->mTodo_vbox, FALSE, FALSE, 0);
    cal->mTodo_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(cal->mTodo_label), _("<b>To do:</b>"));
    gtk_box_pack_start(GTK_BOX(cal->mTodo_vbox), cal->mTodo_label
            , FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(cal->mTodo_label), 0, 0.5);
    cal->mTodo_scrolledWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cal->mTodo_scrolledWin)
            , GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(
            cal->mTodo_scrolledWin), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(cal->mTodo_vbox), cal->mTodo_scrolledWin
            , TRUE, TRUE, 0);
    cal->mTodo_rows_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(
            GTK_SCROLLED_WINDOW(cal->mTodo_scrolledWin), cal->mTodo_rows_vbox);
}

void create_mainbox_event_info(void)
{
#undef P_N
#define P_N "create_mainbox_event_info: "
    CalWin *cal = g_par.xfcal;
    gchar *tmp;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    cal->mEvent_vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cal->mVbox), cal->mEvent_vbox, FALSE, FALSE, 0);
    cal->mEvent_label = gtk_label_new(NULL);
    tmp = g_strdup_printf(_("<b>Events for %s:</b>")
            , orage_cal_to_i18_date(GTK_CALENDAR(cal->mCalendar)));
    gtk_label_set_markup(GTK_LABEL(cal->mEvent_label), tmp);
    g_free(tmp);

    gtk_box_pack_start(GTK_BOX(cal->mEvent_vbox), cal->mEvent_label
            , FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(cal->mEvent_label), 0, 0.5);
    cal->mEvent_scrolledWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cal->mEvent_scrolledWin)
            , GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(
            cal->mEvent_scrolledWin), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(cal->mEvent_vbox), cal->mEvent_scrolledWin
            , TRUE, TRUE, 0);
    cal->mEvent_rows_vbox = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(
            GTK_SCROLLED_WINDOW(cal->mEvent_scrolledWin), cal->mEvent_rows_vbox);
}

/**********************************************************************
 * This routine is called from ical-code xfical_alarm_build_list_internal
 * and ical files are already open at that time. So make sure ical files
 * are opened before and closed after this call.
 **********************************************************************/
void build_mainbox_info(void)
{
#undef P_N
#define P_N "build_mainbox_info: "
    CalWin *cal = g_par.xfcal;
    char      *s_time;
    char      a_day[9];  /* yyyymmdd */
    struct tm *t, tt;
    xfical_type ical_type;
    gchar file_type[8];
    gint i;
    GList *todo_list=NULL;
    GList *event_list=NULL;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif

    if (g_par.show_todos) {
        t = orage_localtime();
        s_time = orage_tm_time_to_icaltime(t);
        strncpy(a_day, s_time, 8);
        a_day[8] = '\0';

        ical_type = XFICAL_TYPE_TODO;
        /* first search base orage file */
        strcpy(file_type, "O00.");
        insert_rows(&todo_list, a_day, ical_type, file_type);
        /* then process all foreign files */
        for (i = 0; i < g_par.foreign_count; i++) {
            g_sprintf(file_type, "F%02d.", i);
            insert_rows(&todo_list, a_day, ical_type, file_type);
        }
    }
    if (g_par.show_events) {
        gtk_calendar_get_date(GTK_CALENDAR(cal->mCalendar)
                , &(tt.tm_year), &(tt.tm_mon), &(tt.tm_mday));
        tt.tm_year -= 1900;
        s_time = orage_tm_time_to_icaltime(&tt);
        strncpy(a_day, s_time, 8);
        a_day[8] = '\0';
    
        ical_type = XFICAL_TYPE_EVENT;
        strcpy(file_type, "O00.");
        insert_rows(&event_list, a_day, ical_type, file_type);
        for (i = 0; i < g_par.foreign_count; i++) {
            g_sprintf(file_type, "F%02d.", i);
            insert_rows(&event_list, a_day, ical_type, file_type);
        }
    }
    if (todo_list) {
        gtk_widget_destroy(cal->mTodo_vbox);
        create_mainbox_todo_info();
        todo_list = g_list_sort(todo_list, list_order);
        g_list_foreach(todo_list, (GFunc)info_process
                , cal->mTodo_rows_vbox);
        g_list_free(todo_list);
        todo_list = NULL;
        gtk_widget_show_all(cal->mTodo_vbox);
    }
    else {
        gtk_widget_hide_all(cal->mTodo_vbox);
    }
    if (event_list) {
        gtk_widget_destroy(cal->mEvent_vbox);
        create_mainbox_event_info();
        event_list = g_list_sort(event_list, list_order);
        g_list_foreach(event_list, (GFunc)info_process
                , cal->mEvent_rows_vbox);
        g_list_free(event_list);
        event_list = NULL;
        gtk_widget_show_all(cal->mEvent_vbox);
    }
    else {
        gtk_widget_hide_all(cal->mEvent_vbox);
    }
}

void build_mainWin()
{
#undef P_N
#define P_N "build_mainWin: "
    GdkPixbuf *orage_logo;
    CalWin *cal = g_par.xfcal;
    GdkColormap *pic1_cmap;

#ifdef ORAGE_DEBUG
    orage_message(-100, P_N);
#endif
    pic1_cmap = gdk_colormap_get_system();
    gdk_color_parse("red", &cal->mRed);
    gdk_colormap_alloc_color(pic1_cmap, &cal->mRed, FALSE, TRUE);
    gdk_color_parse("blue", &cal->mBlue);
    gdk_colormap_alloc_color(pic1_cmap, &cal->mBlue, FALSE, TRUE);

    /* using static icon here since this dynamic icon is not updated
     * when date changes. Could be added, but not worth it.
     * Dynamic icon is used in systray and about windows */
    orage_logo = orage_create_icon(cal, TRUE, 48, 48); 
    cal->mAccel_group = gtk_accel_group_new();
    cal->Tooltips = gtk_tooltips_new();

    gtk_window_set_title(GTK_WINDOW(cal->mWindow), _("Orage"));
    gtk_window_set_position(GTK_WINDOW(cal->mWindow), GTK_WIN_POS_NONE);
    gtk_window_set_resizable(GTK_WINDOW(cal->mWindow), FALSE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(cal->mWindow), TRUE);

    if (orage_logo != NULL) {
        gtk_window_set_icon(GTK_WINDOW(cal->mWindow), orage_logo);
        gtk_window_set_default_icon(orage_logo);
        g_object_unref(orage_logo);
    }

    /* Build the vertical box */
    cal->mVbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(cal->mWindow), cal->mVbox);
    gtk_widget_show(cal->mVbox);

    /* Build the menu */
    build_menu();

    /* Build the calendar */
    cal->mCalendar = gtk_calendar_new();
    gtk_box_pack_start(GTK_BOX(cal->mVbox), cal->mCalendar, TRUE, TRUE, 0);
    gtk_calendar_set_display_options(GTK_CALENDAR(cal->mCalendar)
            , GTK_CALENDAR_SHOW_HEADING | GTK_CALENDAR_SHOW_DAY_NAMES
            | GTK_CALENDAR_SHOW_WEEK_NUMBERS);
    gtk_widget_show(cal->mCalendar);

    /* Build the Info boxes */
    create_mainbox_todo_info();
    create_mainbox_event_info();

    /* Signals */
    g_signal_connect((gpointer) cal->mCalendar, "day_selected_double_click"
            , G_CALLBACK(mCalendar_day_selected_double_click_cb)
            , (gpointer) cal);
    g_signal_connect((gpointer) cal->mCalendar, "day_selected"
            , G_CALLBACK(mCalendar_day_selected_cb)
            , (gpointer) cal);

    g_signal_connect((gpointer) cal->mCalendar, "month-changed"
            , G_CALLBACK(mCalendar_month_changed_cb), (gpointer) cal);

    gtk_window_add_accel_group(GTK_WINDOW(cal->mWindow), cal->mAccel_group);

    if (g_par.pos_x || g_par.pos_y)
        gtk_window_move(GTK_WINDOW(cal->mWindow), g_par.pos_x, g_par.pos_y);
    /*
    gtk_window_stick(GTK_WINDOW(cal->mWindow));
    */
}
