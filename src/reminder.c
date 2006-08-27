/* reminder.c
 *
 * (C) 2003-2006 Mickaël Graf
 * (C) 2005-2006 Juha Kautto 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
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

#include "mainbox.h"
#include "event-list.h"
#include "appointment.h"
#include "reminder.h"
#include "ical-code.h"
#include "functions.h"
#include "tray_icon.h"
#include "xfce_trayicon.h"

extern GList *alarm_list;
extern XfceTrayIcon *trayIcon;

static gchar *play_cmd = NULL;

void set_play_command(gchar *cmd)
{
    if (play_cmd)
        g_free(play_cmd);
    play_cmd = g_strdup(cmd);
}

static void
child_setup_async(gpointer user_data)
{
#if defined(HAVE_SETSID) && !defined(G_OS_WIN32)
    setsid();
#endif
}

static void
child_watch_cb(GPid pid, gint status, gpointer data)
{
    gboolean *sound_active = (gboolean *)data;

    waitpid(pid, NULL, 0);
    g_spawn_close_pid(pid);
    *sound_active = FALSE;
}

gboolean 
orage_exec(const char *cmd, gboolean *sound_active, GError **error)
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

gboolean
orage_sound_alarm(gpointer data)
{
    GError *error = NULL;
    gboolean status;
    xfce_audio_alarm_type *audio_alarm = (xfce_audio_alarm_type *) data;
    GtkWidget *wReminder;
    GtkWidget *stop;

    /* note: -1 loops forever */
    if (audio_alarm->cnt != 0) {
        /*
        status = xfce_exec(audio_alarm->play_cmd, FALSE, FALSE, &error);
        */
        if (audio_alarm->sound_active) {
            return(TRUE);
        }
        status = orage_exec(audio_alarm->play_cmd, &audio_alarm->sound_active
                , &error);
        if (!status) {
            g_warning("reminder: play failed (%si)", audio_alarm->play_cmd);
            audio_alarm->cnt = 0; /* one warning is enough */
        }
        else if (audio_alarm->cnt > 0)
            audio_alarm->cnt--;
    }
    else { /* cnt == 0 */
        if ((wReminder = audio_alarm->wReminder) != NULL) {
            g_object_steal_data(G_OBJECT(wReminder), "AUDIO ACTIVE");
            stop = g_object_get_data(G_OBJECT(wReminder), "AUDIO STOP");
            gtk_widget_set_sensitive(GTK_WIDGET(stop), FALSE);
        }
        g_free(audio_alarm->play_cmd);
        g_free(data);
        status = FALSE; /* no more alarms */
    }
        
    return(status);
}

xfce_audio_alarm_type *
create_soundReminder(alarm_struct *alarm, GtkWidget *wReminder)
{
    xfce_audio_alarm_type *audio_alarm;

    audio_alarm =  g_new(xfce_audio_alarm_type, 1);
    audio_alarm->play_cmd = g_strconcat(play_cmd, " \"", alarm->sound->str, "\"", NULL);
    audio_alarm->delay = alarm->repeat_delay;
    audio_alarm->sound_active = FALSE;
    if ((audio_alarm->cnt = alarm->repeat_cnt) == 0) {
        audio_alarm->cnt++;
        audio_alarm->wReminder = NULL;
    }
    else { /* repeat alarm */
        audio_alarm->wReminder = wReminder;
        g_object_set_data(G_OBJECT(wReminder), "AUDIO ACTIVE", audio_alarm);
    }

    g_timeout_add(alarm->repeat_delay*1000
        , (GtkFunction) orage_sound_alarm
        , (gpointer) audio_alarm);

    return(audio_alarm);
}

void
on_btOkReminder_clicked(GtkButton *button, gpointer user_data)
{
  GtkWidget *wReminder = (GtkWidget *)user_data;

  gtk_widget_destroy(wReminder); /* destroy the specific appointment window */
}

void
on_btStopNoiseReminder_clicked(GtkButton *button, gpointer user_data)
{
  xfce_audio_alarm_type *audio_alarm = (xfce_audio_alarm_type *)user_data;

  audio_alarm->cnt = 0;
  gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

void
on_btOpenReminder_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *wReminder = (GtkWidget *)user_data;
    gchar *uid;
    appt_win *app;
                                                                                
    if ((uid = (gchar *)g_object_get_data(G_OBJECT(wReminder), "ALARM_UID"))
        != NULL) {
        app = create_appt_win("UPDATE", uid, NULL);
        gtk_widget_show(app->appWindow);
    }
}

void
on_destroy(GtkWidget *wReminder, gpointer user_data)
{
    xfce_audio_alarm_type *audio_alarm = (xfce_audio_alarm_type *)user_data;

    if (g_object_get_data(G_OBJECT(wReminder), "AUDIO ACTIVE") != NULL) {
        audio_alarm->cnt = 0;
        audio_alarm->wReminder = NULL; /* window is being distroyed */
    }
    g_free(g_object_get_data(G_OBJECT(wReminder), "ALARM_UID")); /* free uid */
}

void
create_wReminder(alarm_struct *alarm)
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
  char heading[200];
  xfce_audio_alarm_type *audio_alarm;
  gchar *alarm_uid;

  wReminder = gtk_dialog_new();
  gtk_widget_set_size_request(wReminder, 300, 250);
  strcpy(heading,  _("Reminder "));
  gtk_window_set_title(GTK_WINDOW(wReminder),  heading);
  gtk_window_set_position(GTK_WINDOW(wReminder), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(wReminder), FALSE);
  gtk_window_set_resizable(GTK_WINDOW(wReminder), TRUE);
  gtk_window_set_keep_above(GTK_WINDOW(wReminder), TRUE);

  vbReminder = GTK_DIALOG(wReminder)->vbox;
  gtk_widget_show(vbReminder);

  strncat(heading, alarm->title->str, 50);
  hdReminder = xfce_create_header(NULL, heading);
  gtk_widget_show(hdReminder);
  gtk_box_pack_start(GTK_BOX(vbReminder), hdReminder, FALSE, TRUE, 0);

  swReminder = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swReminder)
          , GTK_SHADOW_NONE);
  gtk_widget_show(swReminder);
  gtk_box_pack_start(GTK_BOX(vbReminder), swReminder, TRUE, TRUE, 5);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swReminder)
          , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  lbReminder = gtk_label_new(alarm->description->str);
  gtk_label_set_line_wrap(GTK_LABEL(lbReminder), TRUE);
  gtk_widget_show(lbReminder);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swReminder)
          , lbReminder);

  daaReminder = GTK_DIALOG(wReminder)->action_area;
  gtk_dialog_set_has_separator(GTK_DIALOG(wReminder), FALSE);
  gtk_widget_show(daaReminder);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(daaReminder), GTK_BUTTONBOX_END);

  btOpenReminder = gtk_button_new_from_stock("gtk-open");
  gtk_widget_show(btOpenReminder);
  gtk_dialog_add_action_widget(GTK_DIALOG(wReminder), btOpenReminder
          , GTK_RESPONSE_OK);

  btOkReminder = gtk_button_new_from_stock("gtk-close");
  gtk_widget_show(btOkReminder);
  gtk_dialog_add_action_widget(GTK_DIALOG(wReminder), btOkReminder
          , GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(btOkReminder, GTK_CAN_DEFAULT);

  alarm_uid = g_strdup(alarm->uid->str);
  g_object_set_data(G_OBJECT(wReminder), "ALARM_UID", alarm_uid);
  g_signal_connect((gpointer) btOpenReminder, "clicked",
            G_CALLBACK(on_btOpenReminder_clicked), wReminder);

  g_signal_connect((gpointer) btOkReminder, "clicked",
		    G_CALLBACK(on_btOkReminder_clicked), wReminder);

  if (alarm->audio) {
    audio_alarm = create_soundReminder(alarm, wReminder);
    if (alarm->repeat_cnt != 0) {
        btStopNoiseReminder = gtk_button_new_from_stock("gtk-stop");
        gtk_widget_show(btStopNoiseReminder);
        gtk_dialog_add_action_widget(GTK_DIALOG(wReminder), btStopNoiseReminder
                , GTK_RESPONSE_OK);
        g_object_set_data(G_OBJECT(wReminder), "AUDIO STOP"
                , btStopNoiseReminder);

        g_signal_connect((gpointer) btStopNoiseReminder, "clicked",
		    G_CALLBACK(on_btStopNoiseReminder_clicked), audio_alarm);

        g_signal_connect(G_OBJECT(wReminder), "destroy",
		    G_CALLBACK(on_destroy), audio_alarm);
    }
  }

  gtk_widget_show(wReminder);
}

void
build_tray_tooltip()
{
    gint alarm_cnt=0;
    gint tooltip_alarm_limit=5;
    gint dd, hh, min;
    GString *tooltip=NULL;
    GList *alarm_l;
    alarm_struct *cur_alarm;
    /* GtkTooltipsData *cur_tooltip; */
     
    tooltip = g_string_new(_("Next active alarms:"));
    alarm_l = alarm_list;
    for (alarm_l = g_list_first(alarm_l);
         alarm_l != NULL && (alarm_cnt < tooltip_alarm_limit);
         alarm_l = g_list_next(alarm_l)) {
        cur_alarm = (alarm_struct *)alarm_l->data;
        if (xfical_duration(cur_alarm->alarm_time->str, &dd, &hh, &min)) {
            g_string_append_printf(tooltip, 
                    _("\n%02d d %02d h %02d min to: %s"),
                    dd, hh, min, cur_alarm->title->str);
            alarm_cnt++;
        }
    }
    if (alarm_cnt == 0)
        g_string_append_printf(tooltip, _("\nNo active alarms found"));

    /* would this check save CPU or not??
    cur_tooltip = gtk_tooltips_data_get(trayIcon->tray);
    if (cur_tooltip == NULL) 
    || (strcmp(tooltip->str, cur_tooltip->tip_text) != 0) {
        xfce_tray_icon_set_tooltip(trayIcon, tooltip->str, NULL);
    }
    */
    xfce_tray_icon_set_tooltip(trayIcon, tooltip->str, NULL);
    g_string_free(tooltip, TRUE);
}

gboolean
orage_alarm_clock(gpointer user_data)
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
            xfcalendar_select_date(GTK_CALENDAR(xfcal->mCalendar)
                    , current_year, current_month, current_day);
        }
        previous_year  = current_year;
        previous_month = current_month;
        previous_day   = current_day;
        xfical_alarm_build_list(TRUE);  /* new alarm list when date changed */
        if (NETK_IS_TRAY_ICON(trayIcon->tray)) { 
            /* refresh date in tray icon */
            xfce_tray_icon_disconnect(trayIcon);
            destroy_TrayIcon(trayIcon);
            trayIcon = create_TrayIcon(xfcal);
            xfce_tray_icon_connect(trayIcon);
        }
    }

  /* Check if there are any alarms to show */
    alarm_l = alarm_list;
    for (alarm_l = g_list_first(alarm_l);
         alarm_l != NULL && more_alarms;
         alarm_l = g_list_next(alarm_l)) {
        cur_alarm = (alarm_struct *)alarm_l->data;
        if (xfical_alarm_passed(cur_alarm->alarm_time->str)) {
            create_wReminder(cur_alarm);
            alarm_raised = TRUE;
        }
        else
            more_alarms = FALSE; /* it is sorted list, so we can stop */
    }
    if (alarm_raised) /* at least one alarm processed, need new list */
        xfical_alarm_build_list(FALSE); 

    if (NETK_IS_TRAY_ICON(trayIcon->tray)) { 
        build_tray_tooltip();
    }
    return TRUE;
}

