/*      Orage - Calendar and alarm handler
 *
 * Copyright (c) 2005-2007 Juha Kautto  (juha at xfce.org)
 * Copyright (c) 2003-2006 Mickael Graf (korbinus at xfce.org)
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
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfcegui4/dialogs.h>
#ifdef HAVE_NOTIFY
#include <libnotify/notify.h>
#endif

#include "functions.h"
#include "mainbox.h"
#include "ical-code.h"
#include "event-list.h"
#include "appointment.h"
#include "reminder.h"
#include "xfce_trayicon.h"
#include "tray_icon.h"
#include "parameters.h"

void create_notify_reminder(alarm_struct *alarm);

static void child_setup_async(gpointer user_data)
{
#if defined(HAVE_SETSID) && !defined(G_OS_WIN32)
    setsid();
#endif
}

static void child_watch_cb(GPid pid, gint status, gpointer data)
{
    gboolean *sound_active = (gboolean *)data;

    waitpid(pid, NULL, 0);
    g_spawn_close_pid(pid);
    *sound_active = FALSE;
}

gboolean orage_exec(const char *cmd, gboolean *sound_active, GError **error)
{
    char **argv;
    gboolean success;
    int spawn_flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;
    GPid pid;

    if (!g_shell_parse_argv(cmd, NULL, &argv, error)) 
        return FALSE;

    if (!argv || !argv[0])
        return FALSE;

    success = g_spawn_async(NULL, argv, NULL, spawn_flags
            , child_setup_async, NULL, &pid, error);
    if (success)
        *sound_active = TRUE;
    g_child_watch_add(pid, child_watch_cb, sound_active);
    g_strfreev(argv);

    return success;
}

static void alarm_free_memory(alarm_struct *alarm)
{
    if (!alarm->display_orage && !alarm->display_notify)
        /* if both visuals are gone we can't stop audio anymore, so stop it 
         * now before it is too late */
        alarm->repeat_cnt = 0;
    if (!alarm->display_orage && !alarm->display_notify && !alarm->audio) {
        /* all gone, need to clean memory */
        g_string_free(alarm->uid, TRUE);
        if (alarm->title != NULL)
            g_string_free(alarm->title, TRUE);
        if (alarm->description != NULL)
            g_string_free(alarm->description, TRUE);
        if (alarm->sound != NULL)
            g_string_free(alarm->sound, TRUE);
        g_free(alarm->active_alarm);
        g_free(alarm);
    }
}

static gboolean sound_alarm(gpointer data)
{
    alarm_struct *alarm = (alarm_struct *)data;
    GError *error = NULL;
    gboolean status;
    GtkWidget *stop;

    /* note: -1 loops forever */
    if (alarm->repeat_cnt != 0) {
        if (alarm->active_alarm->sound_active) {
            return(TRUE);
        }
        status = orage_exec(alarm->sound->str
                , &alarm->active_alarm->sound_active, &error);
        if (!status) {
            g_warning("reminder: play failed (%si)", alarm->sound->str);
            alarm->repeat_cnt = 0; /* one warning is enough */
        }
        else if (alarm->repeat_cnt > 0)
            alarm->repeat_cnt--;
    }
    else { /* repeat_cnt == 0 */
        if (alarm->display_orage 
        && ((stop = alarm->active_alarm->stop_noise_reminder) != NULL)) {
            gtk_widget_set_sensitive(GTK_WIDGET(stop), FALSE);
        }
#ifdef HAVE_NOTIFY
        if (alarm->display_notify) {
            /* We need to remove the silence button from notify window.
             * This is not nice method, but it is not possible to access
             * the timeout so we just need to start it from all over */
            notify_notification_close(alarm->active_alarm->active_notify, NULL);
            create_notify_reminder(alarm);
        }
#endif
        alarm_free_memory(alarm);
        status = FALSE; /* no more alarms, end timeouts */
    }
        
    return(status);
}

static void create_sound_reminder(alarm_struct *alarm)
{
    g_string_prepend(alarm->sound, " \"");
    g_string_prepend(alarm->sound, g_par.sound_application);
    g_string_append(alarm->sound, "\"");
    alarm->active_alarm->sound_active = FALSE;
    if (alarm->repeat_cnt == 0) {
        alarm->repeat_cnt++; /* need to do it once */
    }

    g_timeout_add(alarm->repeat_delay*1000
            , (GtkFunction) sound_alarm
            , (gpointer) alarm);
}

#ifdef HAVE_NOTIFY
static void notify_closed(NotifyNotification *n, gpointer par)
{
    alarm_struct *alarm = (alarm_struct *)par;

    alarm->display_notify = FALSE; /* I am gone */
    alarm_free_memory(alarm);
}

static void notify_action_open(NotifyNotification *n, const char *action
        , gpointer par)
{
    alarm_struct *alarm = (alarm_struct *)par;

    create_notify_reminder(alarm);
    create_appt_win("UPDATE", alarm->uid->str, NULL);
}

static void notify_action_silence(NotifyNotification *n, const char *action
        , gpointer par)
{
    alarm_struct *alarm = (alarm_struct *)par;

    alarm->repeat_cnt = 0;
    create_notify_reminder(alarm);
}
#endif

void create_notify_reminder(alarm_struct *alarm) 
{
#ifdef HAVE_NOTIFY
    char heading[250];
    NotifyNotification *n;

    if (!notify_init("Orage")) {
        g_warning("Notify init failed\n");
        return;
    }

    strncpy(heading,  _("Reminder "), 199);
    strncat(heading, alarm->title->str, 50);
    n = notify_notification_new(heading, alarm->description->str, NULL, NULL);
    alarm->active_alarm->active_notify = n;
    if (g_par.trayIcon && NETK_IS_TRAY_ICON(g_par.trayIcon->tray)) 
        notify_notification_attach_to_widget(n, g_par.trayIcon->image);

    if (alarm->notify_timeout == -1)
        notify_notification_set_timeout(n, NOTIFY_EXPIRES_NEVER);
    else if (alarm->notify_timeout == 0)
        notify_notification_set_timeout(n, NOTIFY_EXPIRES_DEFAULT);
    else
        notify_notification_set_timeout(n, alarm->notify_timeout*1000);

    notify_notification_add_action(n, "open", _("Open")
            , (NotifyActionCallback)notify_action_open
            , alarm, NULL);
    if ((alarm->audio) && (alarm->repeat_cnt > 1)) {
        notify_notification_add_action(n, "stop", "Silence"
                , (NotifyActionCallback)notify_action_silence
                , alarm, NULL);
    }
    (void)g_signal_connect(G_OBJECT(n), "closed"
           , G_CALLBACK(notify_closed), alarm);

    if (!notify_notification_show(n, NULL)) {
        g_warning("failed to send notification");
    }
#else
    g_warning("libnotify not linked in. Can't use notifications.");
#endif
}

static void destroy_orage_reminder(GtkWidget *wReminder, gpointer user_data)
{
    alarm_struct *alarm = (alarm_struct *)user_data;

    alarm->display_orage = FALSE; /* I am gone */
    alarm_free_memory(alarm);
}

static void on_btStopNoiseReminder_clicked(GtkButton *button
        , gpointer user_data)
{
    alarm_struct *alarm = (alarm_struct *)user_data;

    alarm->repeat_cnt = 0;
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

static void on_btOkReminder_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *wReminder = (GtkWidget *)user_data;

    gtk_widget_destroy(wReminder); /* destroy the specific appointment window */
}

static void on_btOpenReminder_clicked(GtkButton *button, gpointer user_data)
{
    alarm_struct *alarm = (alarm_struct *)user_data;

    create_appt_win("UPDATE", alarm->uid->str, NULL);
}

static void create_orage_reminder(alarm_struct *alarm)
{
    GtkWidget *wReminder;
    GtkWidget *vbReminder;
    GtkWidget *lbReminder;
    GtkWidget *daaReminder;
    GtkWidget *btOpenReminder;
    GtkWidget *btStopNoiseReminder;
    GtkWidget *btOkReminder;
    GtkWidget *swReminder;
    GtkWidget *hdReminder;
    char heading[250];
    gchar *head2;

    wReminder = gtk_dialog_new();
    gtk_widget_set_size_request(wReminder, 300, 250);
    strncpy(heading,  _("Reminder "), 199);
    gtk_window_set_title(GTK_WINDOW(wReminder),  heading);
    gtk_window_set_position(GTK_WINDOW(wReminder), GTK_WIN_POS_CENTER);
    gtk_window_set_modal(GTK_WINDOW(wReminder), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(wReminder), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(wReminder), TRUE);

    vbReminder = GTK_DIALOG(wReminder)->vbox;

    strncat(heading, alarm->title->str, 50);
    head2 = g_markup_escape_text(heading, -1);
    hdReminder = xfce_create_header(NULL, head2);
    g_free(head2);
    gtk_box_pack_start(GTK_BOX(vbReminder), hdReminder, FALSE, TRUE, 0);

    swReminder = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swReminder)
            , GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(vbReminder), swReminder, TRUE, TRUE, 5);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swReminder)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    lbReminder = gtk_label_new(alarm->description->str);
    gtk_label_set_line_wrap(GTK_LABEL(lbReminder), TRUE);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swReminder)
            , lbReminder);

    daaReminder = GTK_DIALOG(wReminder)->action_area;
    gtk_dialog_set_has_separator(GTK_DIALOG(wReminder), FALSE);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(daaReminder), GTK_BUTTONBOX_END);

    btOpenReminder = gtk_button_new_from_stock("gtk-open");
    gtk_dialog_add_action_widget(GTK_DIALOG(wReminder), btOpenReminder
            , GTK_RESPONSE_OK);

    btOkReminder = gtk_button_new_from_stock("gtk-close");
    gtk_dialog_add_action_widget(GTK_DIALOG(wReminder), btOkReminder
            , GTK_RESPONSE_OK);
    GTK_WIDGET_SET_FLAGS(btOkReminder, GTK_CAN_DEFAULT);

    g_signal_connect((gpointer) btOpenReminder, "clicked"
            , G_CALLBACK(on_btOpenReminder_clicked), alarm);

    g_signal_connect((gpointer) btOkReminder, "clicked"
            , G_CALLBACK(on_btOkReminder_clicked), wReminder);

    if ((alarm->audio) && (alarm->repeat_cnt > 1)) {
        btStopNoiseReminder = gtk_button_new_from_stock("gtk-stop");
        alarm->active_alarm->stop_noise_reminder = btStopNoiseReminder;
        gtk_dialog_add_action_widget(GTK_DIALOG(wReminder)
                , btStopNoiseReminder, GTK_RESPONSE_OK);
        g_signal_connect((gpointer)btStopNoiseReminder, "clicked",
            G_CALLBACK(on_btStopNoiseReminder_clicked), alarm);
        g_signal_connect(G_OBJECT(wReminder), "destroy",
            G_CALLBACK(destroy_orage_reminder), alarm);
    }
    gtk_widget_show_all(wReminder);
}

void create_reminders(alarm_struct *alarm)
{
    alarm_struct *n_alarm;

    /* FIXME: instead of copying this new private version of the alarm,
     * g_list_remove(GList *g_par.alarm_list, gconstpointer alarm);
     * remove it and use the original. saves time */
    n_alarm = g_new0(alarm_struct, 1);
    /* alarm_time is not copied. It was only used to find out when alarm
     * happens and while we are here, it happened already */
    n_alarm->uid = g_string_new(alarm->uid->str);
    n_alarm->title = g_string_new(alarm->title->str);
    n_alarm->description = g_string_new(alarm->description->str);
    n_alarm->notify_timeout = alarm->notify_timeout;
    n_alarm->display = alarm->display;
    n_alarm->display_orage = alarm->display_orage;
    n_alarm->display_notify = alarm->display_notify;
    n_alarm->notify_timeout = alarm->notify_timeout;
    n_alarm->audio = alarm->audio;
    if (alarm->sound != NULL)
        n_alarm->sound = g_string_new(alarm->sound->str);
    n_alarm->repeat_cnt = alarm->repeat_cnt;
    n_alarm->repeat_delay = alarm->repeat_delay;
    if (n_alarm->display
    && (!n_alarm->display_orage && !n_alarm->display_notify))
        n_alarm->display_orage = TRUE;
    n_alarm->active_alarm = g_new0(active_alarm_struct, 1);

    if (n_alarm->audio)
        create_sound_reminder(n_alarm);
    if (n_alarm->display_orage)
        create_orage_reminder(n_alarm);
    if (n_alarm->display_notify)
        create_notify_reminder(n_alarm);
    /*
    if (alarm->display
    && (!alarm->display_orage && !alarm->display_notify))
        alarm->display_orage = TRUE;
    alarm->active_alarm = g_new0(active_alarm_struct, 1);

    if (alarm->audio)
        create_sound_reminder(alarm);
    if (alarm->display_orage)
        create_orage_reminder(alarm);
    if (alarm->display_notify)
        create_notify_reminder(alarm);
        */
}

gboolean orage_alarm_clock(gpointer user_data)
{
    CalWin *xfcal = (CalWin *)user_data;
    struct tm *t;
    static guint previous_year=0, previous_month=0, previous_day=0;
    guint selected_year=0, selected_month=0, selected_day=0;
    guint current_year=0, current_month=0, current_day=0;
    GList *alarm_l;
    alarm_struct *cur_alarm;
    gboolean alarm_raised=FALSE;
    gboolean more_alarms=TRUE;
    gchar time_now[XFICAL_APPT_TIME_FORMAT_LEN];
    GString *tooltip=NULL;
    gint alarm_cnt=0;
    gint tooltip_alarm_limit=5;
    gint year, month, day, hour, minute, second;
    gint dd, hh, min;
    GDate *g_now, *g_alarm;
                                                                                
    t = orage_localtime();
  /* See if the day just changed and the former current date was selected */
    if (previous_day != t->tm_mday) {
        current_year  = t->tm_year + 1900;
        current_month = t->tm_mon;
        current_day   = t->tm_mday;
      /* Get the selected data from calendar */
        gtk_calendar_get_date(GTK_CALENDAR (xfcal->mCalendar),
                 &selected_year, &selected_month, &selected_day);
        if (selected_year == previous_year 
        && selected_month == previous_month 
        && selected_day == previous_day) {
            /* previous day was indeed selected, 
               keep it current automatically */
            orage_select_date(GTK_CALENDAR(xfcal->mCalendar)
                    , current_year, current_month, current_day);
        }
        previous_year  = current_year;
        previous_month = current_month;
        previous_day   = current_day;
        xfical_alarm_build_list(TRUE);  /* new alarm list when date changed */
        if (g_par.show_systray) {
            /* refresh date in tray icon */
            if (g_par.trayIcon && NETK_IS_TRAY_ICON(g_par.trayIcon->tray)) { 
                xfce_tray_icon_disconnect(g_par.trayIcon);
                destroy_TrayIcon(g_par.trayIcon);
            }
            g_par.trayIcon = create_TrayIcon(xfcal);
            xfce_tray_icon_connect(g_par.trayIcon);
        }
    }

    if (g_par.trayIcon && NETK_IS_TRAY_ICON(g_par.trayIcon->tray)) { 
        tooltip = g_string_new(_("Next active alarms:"));
    }
  /* Check if there are any alarms to show */
    alarm_l = g_par.alarm_list;
    for (alarm_l = g_list_first(alarm_l);
         alarm_l != NULL && more_alarms;
         alarm_l = g_list_next(alarm_l)) {
        /* remember that it is sorted list */
        cur_alarm = (alarm_struct *)alarm_l->data;
        g_sprintf(time_now, XFICAL_APPT_TIME_FORMAT, t->tm_year + 1900
                , t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
        if (strcmp(time_now, cur_alarm->alarm_time) > 0) {
            create_reminders(cur_alarm);
            alarm_raised = TRUE;
        }
        else /*if (strcmp(time_now, cur_alarm->alarm_time) <= 0) */ {
            /* check if this should be visible in systray icon tooltip */
            if (tooltip && (alarm_cnt < tooltip_alarm_limit)) {
                sscanf(cur_alarm->alarm_time, XFICAL_APPT_TIME_FORMAT
                        , &year, &month, &day, &hour, &minute, &second);
                g_now = g_date_new_dmy(t->tm_mday, t->tm_mon + 1
                        , t->tm_year + 1900);
                g_alarm = g_date_new_dmy(day, month, year);
                dd = g_date_days_between(g_now, g_alarm);
                hh = hour - t->tm_hour;
                min = minute - t->tm_min;
                if (min < 0) {
                    min += 60;
                    hh -= 1;
                }
                if (hh < 0) {
                    hh += 24;
                    dd -= 1;
                }
                g_date_free(g_now);
                g_date_free(g_alarm);

                g_string_append_printf(tooltip, 
                        _("\n%02d d %02d h %02d min to: %s"),
                        dd, hh, min, cur_alarm->title->str);
                alarm_cnt++;
            }
            else /* sorted so scan can be stopped */
                more_alarms = FALSE; 
        }
    }
    if (alarm_raised) { /* at least one alarm processed, need new list */
        xfical_alarm_build_list(FALSE); 
    }

    if (tooltip) {
        if (alarm_cnt == 0)
            g_string_append_printf(tooltip, _("\nNo active alarms found"));
        xfce_tray_icon_set_tooltip(g_par.trayIcon, tooltip->str, NULL);
        g_string_free(tooltip, TRUE);
    }
    /*
    if (g_par.trayIcon && NETK_IS_TRAY_ICON(g_par.trayIcon->tray)) { 
        build_tray_tooltip(time_now);
    }
    */
    return TRUE;
}

