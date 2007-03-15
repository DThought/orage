/* appointment.c
 *
 * Copyright (C) 2004-2005 Mickaël Graf <korbinus at xfce.org>
 * Copyright (C) 2005 Juha Kautto <juha at xfce.org>
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

#define _XOPEN_SOURCE /* glibc2 needs this */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib/gprintf.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-client.h>

#include "mainbox.h"
#include "event-list.h"
#include "appointment.h"
#include "ical-code.h"
#include "functions.h"

#define AVAILABILITY_ARRAY_DIM 2
#define RECUR_ARRAY_DIM 5
#define ALARM_ARRAY_DIM 11
#define FILETYPE_SIZE 38

extern char *local_icaltimezone_location;
extern gboolean local_icaltimezone_utc;

void
fill_appt_window(appt_win *apptw, char *action, char *par);

enum {
    LOCATION,
    LOCATION_ENG,
    N_COLUMNS
};

gboolean 
ical_to_year_month_day_hour_minute(char *ical
    , int *year, int *month, int *day, int *hour, int *minute)
{
    int i;

    i = sscanf(ical, "%04d%02d%02dT%02d%02d", year, month, day, hour, minute);
    switch (i) {
        case 3: /* date */
            *hour = -1;
            *minute = -1;
            break;
        case 5: /* time */
            break;
        default: /* error */
            g_warning("ical_to_year_month_day_hour_minute: ical error %s %d"
                    , ical, i);
            return FALSE;
            break;
    }
    return TRUE;
}

struct tm
display_to_year_month_day(const char *display)
{
    const char *date_format;
    char *ret;
    struct tm d = {0,0,0,0,0,0,0,0,0};

    date_format = _("%m/%d/%Y");
    if ((ret = strptime(display, date_format, &d)) == NULL)
        g_error("Orage: display_to_year_month_day wrong format (%s)"
                , display);
    else if (strlen(ret))
        g_error("Orage: display_to_year_month_day too long format (%s)"
                , display);
    return(d);
}

char * 
year_month_day_to_display(int year, int month, int day)
{
    const char *date_format;
    static char result[32];
    struct tm d = {0,0,0,0,0,0,0,0,0};

    date_format = _("%m/%d/%Y");
    d.tm_mday = day;
    d.tm_mon = month - 1;
    d.tm_year = year - 1900;

    if (strftime(result, 32, date_format, &d))
        return(result);
    else {
        g_error("Orage:year_month_day_to_display too long string in strftime");
        return(NULL);
    }
}

void
mark_appointment_changed(appt_win *apptw)
{
    if (!apptw->appointment_changed) {
        apptw->appointment_changed = TRUE;
        gtk_widget_set_sensitive(apptw->appRevert, TRUE);
        gtk_widget_set_sensitive(apptw->appFileRevert_menuitem, TRUE);
    }
}

void
mark_appointment_unchanged(appt_win *apptw)
{
    if (apptw->appointment_changed) {
        apptw->appointment_changed = FALSE;
        gtk_widget_set_sensitive(apptw->appRevert, FALSE);
        gtk_widget_set_sensitive(apptw->appFileRevert_menuitem, FALSE);
    }
}

void
set_time_sensitivity(appt_win *apptw)
{
    gboolean dur_act;
    gboolean allDay_act;

    dur_act = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appDur_checkbutton));
    allDay_act = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton));

    if (allDay_act) {
        gtk_widget_set_sensitive(apptw->appStartTime_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->appStartTime_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->appStartTimezone_button, FALSE);
        gtk_widget_set_sensitive(apptw->appEndTime_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->appEndTime_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->appEndTimezone_button, FALSE);
        gtk_widget_set_sensitive(apptw->appDur_spin_hh, FALSE);
        gtk_widget_set_sensitive(apptw->appDur_spin_hh_label, FALSE);
        gtk_widget_set_sensitive(apptw->appDur_spin_mm, FALSE);
        gtk_widget_set_sensitive(apptw->appDur_spin_mm_label, FALSE);
        if (dur_act) {
            gtk_widget_set_sensitive(apptw->appEndDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd_label, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->appEndDate_button, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd_label, FALSE);
        }
    }
    else {
        gtk_widget_set_sensitive(apptw->appStartTime_spin_hh, TRUE);
        gtk_widget_set_sensitive(apptw->appStartTime_spin_mm, TRUE);
        gtk_widget_set_sensitive(apptw->appStartTimezone_button, TRUE);
        if (dur_act) {
            gtk_widget_set_sensitive(apptw->appEndDate_button, FALSE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_mm, FALSE);
            gtk_widget_set_sensitive(apptw->appEndTimezone_button, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd_label, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_hh, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_hh_label, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_mm, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_mm_label, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->appEndDate_button, TRUE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_hh, TRUE);
            gtk_widget_set_sensitive(apptw->appEndTime_spin_mm, TRUE);
            gtk_widget_set_sensitive(apptw->appEndTimezone_button, TRUE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_dd_label, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_hh, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_hh_label, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_mm, FALSE);
            gtk_widget_set_sensitive(apptw->appDur_spin_mm_label, FALSE);
        }
    }
}

void
set_repeat_sensitivity(appt_win *apptw)
{
    gint freq, i;
    
    freq = gtk_combo_box_get_active((GtkComboBox *)apptw->appRecur_freq_cb);
    if (freq == XFICAL_FREQ_NONE) {
        gtk_widget_set_sensitive(apptw->appRecur_limit_rb, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_count_rb, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_count_spin, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_count_label, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_until_rb, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_until_button, FALSE);
        for (i=0; i <= 6; i++) {
            gtk_widget_set_sensitive(apptw->appRecur_byday_cb[i], FALSE);
            gtk_widget_set_sensitive(apptw->appRecur_byday_spin[i], FALSE);
        }
        gtk_widget_set_sensitive(apptw->appRecur_int_spin, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_int_spin_label1, FALSE);
        gtk_widget_set_sensitive(apptw->appRecur_int_spin_label2, FALSE);
    }
    else {
        gtk_widget_set_sensitive(apptw->appRecur_limit_rb, TRUE);
        gtk_widget_set_sensitive(apptw->appRecur_count_rb, TRUE);
        if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_count_rb))) {
            gtk_widget_set_sensitive(apptw->appRecur_count_spin, TRUE);
            gtk_widget_set_sensitive(apptw->appRecur_count_label, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->appRecur_count_spin, FALSE);
            gtk_widget_set_sensitive(apptw->appRecur_count_label, FALSE);
        }
        gtk_widget_set_sensitive(apptw->appRecur_until_rb, TRUE);
        if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_until_rb))) {
            gtk_widget_set_sensitive(apptw->appRecur_until_button, TRUE);
        }
        else {
            gtk_widget_set_sensitive(apptw->appRecur_until_button, FALSE);
        }
        for (i=0; i <= 6; i++) {
            gtk_widget_set_sensitive(apptw->appRecur_byday_cb[i], TRUE);
        }
        if (freq == XFICAL_FREQ_MONTHLY || freq == XFICAL_FREQ_YEARLY) {
            for (i=0; i <= 6; i++) {
                gtk_widget_set_sensitive(apptw->appRecur_byday_spin[i], TRUE);
            }
        }
        else {
            for (i=0; i <= 6; i++) {
                gtk_widget_set_sensitive(apptw->appRecur_byday_spin[i], FALSE);
            }
        }
        gtk_widget_set_sensitive(apptw->appRecur_int_spin, TRUE);
        gtk_widget_set_sensitive(apptw->appRecur_int_spin_label1, TRUE);
        gtk_widget_set_sensitive(apptw->appRecur_int_spin_label2, TRUE);
    }
}

void
on_appTitle_entry_changed_cb(GtkEditable *entry, gpointer user_data)
{
    gchar *title, *application_name;
    const gchar *appointment_name;
    appt_win *apptw = (appt_win *)user_data;

    appointment_name = gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry);
    application_name = _("Orage");

    if(strlen((char *)appointment_name) > 0)
        title = g_strdup_printf("%s - %s", appointment_name, application_name);
    else
        title = g_strdup_printf("%s", application_name);

    gtk_window_set_title(GTK_WINDOW(apptw->appWindow), (const gchar *)title);

    g_free(title);
    mark_appointment_changed((appt_win *)user_data);
}

void
appDur_checkbutton_clicked_cb(GtkCheckButton *cb, gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

void
app_recur_checkbutton_clicked_cb(GtkCheckButton *checkbutton
        , gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
    set_repeat_sensitivity((appt_win *)user_data);
}

void
app_recur_feature_checkbutton_clicked_cb(GtkCheckButton *checkbutton
        , gpointer user_data)
{
    appt_win *apptw=(appt_win *)user_data;

    mark_appointment_changed((appt_win *)user_data);
    if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->appRecur_feature_normal_rb))) {
        gtk_widget_hide(apptw->appRecur_int_label);
        gtk_widget_hide(apptw->appRecur_int_hbox);
        gtk_widget_hide(apptw->appRecur_byday_label);
        gtk_widget_hide(apptw->appRecur_byday_hbox);
        gtk_widget_hide(apptw->appRecur_byday_spin_label);
        gtk_widget_hide(apptw->appRecur_byday_spin_hbox);
    }
    else {
        gtk_widget_show(apptw->appRecur_int_label);
        gtk_widget_show(apptw->appRecur_int_hbox);
        gtk_widget_show(apptw->appRecur_byday_label);
        gtk_widget_show(apptw->appRecur_byday_hbox);
        gtk_widget_show(apptw->appRecur_byday_spin_label);
        gtk_widget_show(apptw->appRecur_byday_spin_hbox);
    }
}

void
app_checkbutton_clicked_cb(GtkCheckButton *checkbutton, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_appNote_buffer_changed_cb(GtkTextBuffer *buffer, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_app_entry_changed_cb(GtkEditable *entry, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_app_combobox_changed_cb(GtkComboBox *cb, gpointer user_data)
{
    set_repeat_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

void
on_app_spin_button_changed_cb(GtkSpinButton *cb, gpointer user_data)
{
    mark_appointment_changed((appt_win *)user_data);
}

void
on_appSound_button_clicked_cb(GtkButton *button, gpointer user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    GtkWidget *file_chooser;
    GtkFileFilter *filter;
    gchar *appSound_entry_filename;
    gchar *sound_file;
    const gchar *filetype[FILETYPE_SIZE] = {
        "*.aiff", "*.al", "*.alsa", "*.au", "*.auto", "*.avr",
        "*.cdr", "*.cvs", "*.dat", "*.vms", "*.gsm", "*.hcom", 
        "*.la", "*.lu", "*.maud", "*.mp3", "*.nul", "*.ogg", "*.ossdsp",
        "*.prc", "*.raw", "*.sb", "*.sf", "*.sl", "*.smp", "*.sndt", 
        "*.sph", "*.8svx", "*.sw", "*.txw", "*.ub", "*.ul", "*.uw",
        "*.voc", "*.vorbis", "*.vox", "*.wav", "*.wve"};
    register int i;

    appSound_entry_filename = g_strdup(gtk_entry_get_text(
            (GtkEntry *)apptw->appSound_entry));
    file_chooser = gtk_file_chooser_dialog_new(_("Select a file..."),
            GTK_WINDOW (apptw->appWindow),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
            GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
            NULL);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("Sound Files"));
    for (i = 0; i < FILETYPE_SIZE; i++) {
        gtk_file_filter_add_pattern(filter, filetype[i]);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("All Files"));
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(file_chooser), filter);

    gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(file_chooser)
            , PACKAGE_DATA_DIR "/orage/sounds", NULL);

    if (strlen(appSound_entry_filename) > 0)
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(file_chooser)
                , (const gchar *) appSound_entry_filename);
    else
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(file_chooser)
            , PACKAGE_DATA_DIR "/orage/sounds");

    if (gtk_dialog_run(GTK_DIALOG(file_chooser)) == GTK_RESPONSE_ACCEPT) {
        sound_file = 
            gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_chooser));
        if (sound_file) {
            gtk_entry_set_text(GTK_ENTRY(apptw->appSound_entry), sound_file);
            gtk_editable_set_position(GTK_EDITABLE(apptw->appSound_entry), -1);
        }
    }

    gtk_widget_destroy(file_chooser);
    g_free(appSound_entry_filename);
}

void
on_appAllDay_clicked_cb(GtkCheckButton *checkbutton, gpointer user_data)
{
    set_time_sensitivity((appt_win *)user_data);
    mark_appointment_changed((appt_win *)user_data);
}

void free_apptw(appt_win *apptw)
{
    gtk_widget_destroy(apptw->appWindow);
    gtk_object_destroy(GTK_OBJECT(apptw->appTooltips));
    g_free(apptw->xf_uid);
    g_free(apptw->par);
    xfical_appt_free(apptw->appt);
    g_free(apptw);
}

gboolean
appWindow_check_and_close(appt_win *apptw)
{
    gint result;

    if (apptw->appointment_changed == TRUE) {
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
             _("Warning"),
             GTK_STOCK_DIALOG_WARNING,
             _("The appointment information has been modified."),
             _("Do you want to continue?"),
             GTK_STOCK_YES, GTK_RESPONSE_ACCEPT,
             GTK_STOCK_NO, GTK_RESPONSE_CANCEL,
             NULL);

        if (result == GTK_RESPONSE_ACCEPT)
            free_apptw(apptw);
    }
    else
        free_apptw(apptw);
    return TRUE;
}

gboolean
on_appWindow_delete_event_cb(GtkWidget *widget, GdkEvent *event
    , gpointer user_data)
{
    return appWindow_check_and_close((appt_win *) user_data);
}

gboolean 
orage_validate_datetime(appt_win *apptw, appt_data *appt)
{
    gint result;

    if (xfical_compare_times(appt) > 0) {
        result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
                _("Warning"),
                GTK_STOCK_DIALOG_WARNING,
                _("The end of this appointment is earlier than the beginning."),
                NULL,
                GTK_STOCK_OK,
                GTK_RESPONSE_ACCEPT,
                NULL);
        return FALSE;
    }
    else {
        return TRUE;
    }
}

/*
 * fill_appt_from_apptw
 * This function fills an appointment with the content of an appointment window
 */
gboolean
fill_appt_from_apptw(appt_data *appt, appt_win *apptw)
{
    GtkTextIter start, end;
    const char *time_format="%H:%M";
    struct tm current_t;
    gchar starttime[6], endtime[6];
    gint i;

    /* Next line is fix for bug 2811.
     * We need to make sure spin buttons do not have values which are not
     * yet updated = visible.
     * gtk_spin_button_update call would do it, but it seems to cause
     * crash if done in on_app_spin_button_changed_cb and here we should
     * either do it for all spin fields, which takes too many lines of code.
     * if (GTK_WIDGET_HAS_FOCUS(apptw->StartTime_spin_hh))
     *      gtk_spin_button_update;
     * So we just change the focus field and then spin button value gets set.
     * It would be nice to not to have to change the field though.
     */
    gtk_widget_grab_focus(apptw->appTitle_entry);
    /*Get the title */
    appt->title = g_strdup(gtk_entry_get_text(
            (GtkEntry *)apptw->appTitle_entry));
    /* Get the location */
    appt->location = g_strdup(gtk_entry_get_text(
            (GtkEntry *)apptw->appLocation_entry));

    /* Get if the appointment is for the all day */
    appt->allDay = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton));

    /* Get the start date and time and timezone */
    current_t = display_to_year_month_day(gtk_button_get_label(
            GTK_BUTTON(apptw->appStartDate_button)));
    g_sprintf(starttime, "%02d:%02d"
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appStartTime_spin_hh))
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appStartTime_spin_mm)));
    strptime(starttime, time_format, &current_t);
    g_sprintf(appt->starttime, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);

    /* Get the end date and time and timezone */
    current_t = display_to_year_month_day(gtk_button_get_label(
            GTK_BUTTON(apptw->appEndDate_button)));
    g_sprintf(endtime, "%02d:%02d"
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appEndTime_spin_hh))
            , gtk_spin_button_get_value_as_int(
                    GTK_SPIN_BUTTON(apptw->appEndTime_spin_mm)));
    strptime(endtime, time_format, &current_t);
    g_sprintf(appt->endtime, XFICAL_APPT_TIME_FORMAT
            , current_t.tm_year + 1900, current_t.tm_mon + 1, current_t.tm_mday
            , current_t.tm_hour, current_t.tm_min, 0);

    /* Get the duration */
    appt->use_duration = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->appDur_checkbutton));
    appt->duration = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appDur_spin_dd)) * 24*60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appDur_spin_hh)) *    60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appDur_spin_mm)) *       60
                    ;

    if(!orage_validate_datetime(apptw, appt))
        return(FALSE);

    /* Get the availability */
    appt->availability = gtk_combo_box_get_active(
            (GtkComboBox *)apptw->appAvailability_cb);

    /* Get the notes */
    gtk_text_buffer_get_bounds(
            gtk_text_view_get_buffer((GtkTextView *)apptw->appNote_textview)
            , &start, &end);
    appt->note = gtk_text_iter_get_text(&start, &end);

    /* Get when the reminder will show up */
    appt->alarmtime = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appAlarm_spin_dd)) * 24*60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appAlarm_spin_hh)) *    60*60
                    + gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appAlarm_spin_mm)) *       60
                    ;

    /* Which sound file will be played */
    appt->sound = g_strdup(gtk_entry_get_text(
                (GtkEntry *)apptw->appSound_entry));

    /* Get if the alarm will repeat until someone shuts it off */
    appt->alarmrepeat = gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->appSoundRepeat_checkbutton));

    /* Get the recurrence */
    appt->freq = gtk_combo_box_get_active(
            (GtkComboBox *)apptw->appRecur_freq_cb);

    /* Get the recurrence interval */
    appt->interval = gtk_spin_button_get_value_as_int(
            GTK_SPIN_BUTTON(apptw->appRecur_int_spin));

    /* Get the recurrence ending */
    if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_limit_rb))) {
        appt->recur_limit = 0;    /* no limit */
        appt->recur_count = 0;    /* special: means no repeat count limit */
        appt->recur_until[0] = 0; /* special: means no until time limit */
    }
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_count_rb))) {
        appt->recur_limit = 1;    /* count limit */
        appt->recur_count = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->appRecur_count_spin));
        appt->recur_until[0] = 0; /* special: means no until time limit */
    }
    else if (gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_until_rb))) {
        appt->recur_limit = 2;    /* until limit */
        appt->recur_count = 0;    /* special: means no repeat count limit */

        current_t = display_to_year_month_day(gtk_button_get_label(
                GTK_BUTTON(apptw->appRecur_until_button)));
        g_sprintf(appt->recur_until, XFICAL_APPT_TIME_FORMAT
                , current_t.tm_year + 1900, current_t.tm_mon + 1
                , current_t.tm_mday, 23, 59, 10);
    }
    else
        g_warning("fill_appt_from_apptw: coding error, illegal recurrence");

    /* Get the recurrence weekdays */
    for (i=0; i <= 6; i++) {
        appt->recur_byday[i] = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_byday_cb[i]));
    /* Get the recurrence weekday selection for month/yearly case */
        appt->recur_byday_cnt[i] = gtk_spin_button_get_value_as_int(
                GTK_SPIN_BUTTON(apptw->appRecur_byday_spin[i]));
    }

    return(TRUE);
}

void
on_appFileClose_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    appWindow_check_and_close((appt_win *) user_data);
}

gboolean 
save_xfical_from_appt_win(appt_win *apptw)
{
    gboolean ok = FALSE;
    appt_data *appt = apptw->appt;

    if (fill_appt_from_apptw(appt, apptw)) {
        /* Here we try to save the event... */
        if (xfical_file_open()) {
            if (apptw->appointment_add) {
                apptw->xf_uid = g_strdup(xfical_appt_add(appt));
                ok = (apptw->xf_uid ? TRUE : FALSE);
                if (ok) {
                    apptw->appointment_add = FALSE;
                    gtk_widget_set_sensitive(apptw->appDuplicate, TRUE);
                    gtk_widget_set_sensitive(apptw->appFileDuplicate_menuitem
                            , TRUE);
                    g_message("Orage **: Added: %s", apptw->xf_uid);
                }
                else
                    g_warning("Addition failed: %s", apptw->xf_uid);
            }
            else {
                ok = xfical_appt_mod(apptw->xf_uid, appt);
                if (ok)
                    g_message("Orage **: Modified: %s", apptw->xf_uid);
                else
                    g_warning("Modification failed: %s", apptw->xf_uid);
            }
            xfical_file_close();
        }

        if (ok) {
            apptw->appointment_new = FALSE;
            mark_appointment_unchanged(apptw);
            if (apptw->eventlist != NULL)
                refresh_eventlist_win((eventlist_win *)apptw->eventlist);
            xfcalendar_mark_appointments();
        }
    }

    return (ok);
}

void
on_appFileSave_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    save_xfical_from_appt_win((appt_win *)user_data);
}

void
on_appSave_clicked_cb(GtkButton *button, gpointer user_data)
{
    save_xfical_from_appt_win((appt_win *)user_data);
}

void
save_xfical_from_appt_win_and_close(appt_win *apptw)
{
    if (save_xfical_from_appt_win(apptw))
        free_apptw(apptw);
}

void 
on_appFileSaveClose_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    save_xfical_from_appt_win_and_close((appt_win *)user_data);
}

void
on_appSaveClose_clicked_cb(GtkButton *button, gpointer user_data)
{
    save_xfical_from_appt_win_and_close((appt_win *)user_data);
}

void
delete_xfical_from_appt_win(appt_win *apptw)
{
    gint result;

    result = xfce_message_dialog(GTK_WINDOW(apptw->appWindow),
             _("Warning"),
             GTK_STOCK_DIALOG_WARNING,
             _("This appointment will be permanently removed."),
             _("Do you want to continue?"),
             GTK_STOCK_YES,
             GTK_RESPONSE_ACCEPT,
             GTK_STOCK_NO,
             GTK_RESPONSE_REJECT,
             NULL);
                                 
    if (result == GTK_RESPONSE_ACCEPT) {
        if (!apptw->appointment_add)
            if (xfical_file_open()) {
                result = xfical_appt_del(apptw->xf_uid);
                xfical_file_close();
                if (result)
                    g_message("Orage **: Removed: %s", apptw->xf_uid);
                else
                    g_warning("Removal failed: %s", apptw->xf_uid);
            }

        if (apptw->eventlist != NULL)
            refresh_eventlist_win((eventlist_win *)apptw->eventlist);
        xfcalendar_mark_appointments();

        free_apptw(apptw);
    }
}

void
on_appDelete_clicked_cb(GtkButton *button, gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *)user_data);
}

void
on_appFileDelete_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    delete_xfical_from_appt_win((appt_win *) user_data);
}

void 
duplicate_xfical_from_appt_win(appt_win *apptw)
{
    gint x, y;
    appt_win *apptw2;

    apptw2 = create_appt_win("COPY", apptw->xf_uid, apptw->eventlist);
    gtk_window_get_position(GTK_WINDOW(apptw->appWindow), &x, &y);
    gtk_window_move(GTK_WINDOW(apptw2->appWindow), x+20, y+20);
}

void
on_appFileDuplicate_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    duplicate_xfical_from_appt_win((appt_win *)user_data);
}

void
on_appDuplicate_clicked_cb(GtkButton *button, gpointer user_data)
{
    duplicate_xfical_from_appt_win((appt_win *)user_data);
}

void
revert_xfical_to_last_saved(appt_win *apptw)
{
    if (!apptw->appointment_new) {
        fill_appt_window(apptw, "UPDATE", apptw->xf_uid);
    }
    else {
        fill_appt_window(apptw, "NEW", apptw->par);
    }
}

void
on_appFileRevert_menu_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    revert_xfical_to_last_saved((appt_win *) user_data);
}

void
on_appRevert_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    revert_xfical_to_last_saved((appt_win *) user_data);
}

void
on_appStartEndDate_clicked_cb(GtkWidget *button, gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    GtkWidget *selDate_Window_dialog;
    GtkWidget *selDate_Calendar_calendar;
    gint result;
    guint year, month, day;
    char *date_to_display=NULL; 
    struct tm *t;
    struct tm current_t;

    selDate_Window_dialog = gtk_dialog_new_with_buttons(
            _("Pick the date"), GTK_WINDOW(apptw->appWindow),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            _("Today"), 
            1,
            GTK_STOCK_OK,
            GTK_RESPONSE_ACCEPT,
            NULL);

    selDate_Calendar_calendar = gtk_calendar_new();
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(selDate_Window_dialog)->vbox)
            , selDate_Calendar_calendar);

    current_t = display_to_year_month_day(gtk_button_get_label(
            GTK_BUTTON(button)));
    xfcalendar_select_date(GTK_CALENDAR(selDate_Calendar_calendar)
            , current_t.tm_year+1900, current_t.tm_mon, current_t.tm_mday);
    gtk_widget_show_all(selDate_Window_dialog);

    result = gtk_dialog_run(GTK_DIALOG(selDate_Window_dialog));
    switch(result){
        case GTK_RESPONSE_ACCEPT:
            gtk_calendar_get_date(GTK_CALENDAR(selDate_Calendar_calendar)
                    , &year, &month, &day);
            date_to_display = year_month_day_to_display(year, month + 1, day);
            break;
        case 1:
            t = orage_localtime();
            date_to_display = year_month_day_to_display(t->tm_year + 1900
                    , t->tm_mon + 1, t->tm_mday);
            break;
        case GTK_RESPONSE_DELETE_EVENT:
        default:
            date_to_display = (gchar *)gtk_button_get_label(
                    GTK_BUTTON(button));
            break;
    }
    if (g_ascii_strcasecmp((gchar *)date_to_display
            , (gchar *)gtk_button_get_label(GTK_BUTTON(button))) != 0) {
        mark_appointment_changed(apptw);
    }
    gtk_button_set_label(GTK_BUTTON(button), (const gchar *)date_to_display);
    gtk_widget_destroy(selDate_Window_dialog);
}

void
on_appTimezone_clicked_cb(GtkWidget *button,  appt_win *apptw
        , gchar **tz)
{
#define MAX_AREA_LENGTH 20
    GtkTreeStore *store;
    GtkTreeIter iter1, iter2;
    GtkWidget *tree;
    GtkCellRenderer *rend;
    GtkTreeViewColumn *col;
    GtkWidget *window;
    GtkWidget *sw;
    xfical_timezone_array tz_a;
    int i, j, result;
    char area_old[MAX_AREA_LENGTH], *loc, *loc_eng, *loc_int;
    GtkTreeSelection *sel;
    GtkTreeModel     *model;
    GtkTreeIter       iter;

    /* enter data */
    store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    strcpy(area_old, "S T a R T");
    tz_a = xfical_get_timezones();
    for (i=0; i < tz_a.count-2; i++) {
        /* first area */
        if (! g_str_has_prefix(tz_a.city[i], area_old)) {
            for (j=0; tz_a.city[i][j] != '/' && j < MAX_AREA_LENGTH; j++) {
                area_old[j] = tz_a.city[i][j];
            }
            if (j < MAX_AREA_LENGTH)
                area_old[j] = 0;
            else
                g_warning("on_appStartEndTimezone_clicked_cb: too long line in zones.tab %s", tz_a.city[i]);

            gtk_tree_store_append(store, &iter1, NULL);
            gtk_tree_store_set(store, &iter1
                    , LOCATION, _(area_old)
                    , LOCATION_ENG, area_old
                    , -1);
        }
        /* then city translated and in base form used internally */
        gtk_tree_store_append(store, &iter2, &iter1);
        gtk_tree_store_set(store, &iter2
                , LOCATION, _(tz_a.city[i]) 
                , LOCATION_ENG, tz_a.city[i] 
                , -1);
    }
         
    /* create view */
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);

    rend = gtk_cell_renderer_text_new();
    col  = gtk_tree_view_column_new_with_attributes(_("Location")
                , rend, "text", LOCATION_ENG, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);
    gtk_tree_view_column_set_visible(col, FALSE);

    /* show it */
    window =  gtk_dialog_new_with_buttons(_("Pick timezone")
            , GTK_WINDOW(apptw->appWindow)
            , GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
            , _("UTC"), 1
            , _("floating"), 2
            , GTK_STOCK_OK, GTK_RESPONSE_ACCEPT
            , NULL);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), sw, TRUE, TRUE, 0);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 500);

    gtk_widget_show_all(window);
    do {
        result = gtk_dialog_run(GTK_DIALOG(window));
        switch (result) {
            case GTK_RESPONSE_ACCEPT:
                sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
                if (gtk_tree_selection_get_selected(sel, &model, &iter))
                    if (gtk_tree_model_iter_has_child(model, &iter))
                        result = 0;
                    else {
                        gtk_tree_model_get(model, &iter, LOCATION, &loc, -1);
                        gtk_tree_model_get(model, &iter, LOCATION_ENG
                                , &loc_eng, -1);
                    }
                else {
                    loc = g_strdup(_(*tz));
                    loc_eng = g_strdup((*tz));
                }
                break;
            case 1:
                loc = g_strdup(_("UTC"));
                loc_eng = g_strdup("UTC");
                break;
            case 2:
                loc = g_strdup(_("floating"));
                loc_eng = g_strdup("floating");
                break;
            default:
                loc = g_strdup(_(*tz));
                loc_eng = g_strdup((*tz));
                break;
        }
    } while (result == 0) ;
    if (g_ascii_strcasecmp((gchar *)loc
            , (gchar *)gtk_button_get_label(GTK_BUTTON(button))) != 0) {
        mark_appointment_changed(apptw);
    }
    gtk_button_set_label(GTK_BUTTON(button), loc);
    if (*tz)
        g_free(*tz);
    *tz = g_strdup(loc_eng);
    g_free(loc);
    g_free(loc_eng);
    gtk_widget_destroy(window);
}

static void on_appStartTimezone_clicked_cb(GtkWidget *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    appt_data *appt;

    appt = apptw->appt;
    on_appTimezone_clicked_cb(button, apptw, &appt->start_tz_loc);
}

static void on_appEndTimezone_clicked_cb(GtkWidget *button
        , gpointer *user_data)
{
    appt_win *apptw = (appt_win *)user_data;
    appt_data *appt;

    appt = apptw->appt;
    on_appTimezone_clicked_cb(button, apptw, &appt->end_tz_loc);
}

void
fill_appt_window_times(appt_win *apptw, appt_data *appt)
{
    char *startdate_to_display, *enddate_to_display;
    int year, month, day, hours, minutes;

    /* all day ? */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->appAllDay_checkbutton), appt->allDay);

    /* start time */
    if (strlen(appt->starttime) > 6 ) {
        ical_to_year_month_day_hour_minute(appt->starttime
                , &year, &month, &day, &hours, &minutes);
        startdate_to_display = year_month_day_to_display(year, month, day);
        gtk_button_set_label(GTK_BUTTON(apptw->appStartDate_button)
                , (const gchar *)startdate_to_display);

        if (hours > -1 && minutes > -1) {
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appStartTime_spin_hh), (gdouble)hours);
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appStartTime_spin_mm), (gdouble)minutes);
        }
        if (appt->start_tz_loc) {
            gtk_button_set_label(GTK_BUTTON(apptw->appStartTimezone_button)
                    , _(appt->start_tz_loc));
        }
        else /* we should never get here */
            g_warning("fill_appt_window_times: s_tz is null");
    }
    else
        g_warning("fill_appt_window_times: starttime wrong %s", appt->uid);

    /* end time */
    if (strlen(appt->endtime) > 6 ) {
        ical_to_year_month_day_hour_minute(appt->endtime
                , &year, &month, &day, &hours, &minutes);
        enddate_to_display = year_month_day_to_display(year, month, day);
        gtk_button_set_label(GTK_BUTTON(apptw->appEndDate_button)
                , (const gchar *)enddate_to_display);

        if (hours > -1 && minutes > -1) {
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appEndTime_spin_hh), (gdouble)hours);
            gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appEndTime_spin_mm), (gdouble)minutes);
        }
        if (appt->end_tz_loc) {
            gtk_button_set_label(GTK_BUTTON(apptw->appEndTimezone_button)
                    , _(appt->end_tz_loc));
        }
        else /* we should never get here */
            g_warning("fill_appt_window_times: e_tz is null");
    }
    else
        g_warning("fill_appt_window_times: endtime wrong %s", appt->uid);

    /* duration */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(apptw->appDur_checkbutton)
            , appt->use_duration);
    day = appt->duration/(24*60*60);
    hours = (appt->duration-day*(24*60*60))/(60*60);
    minutes = (appt->duration-day*(24*60*60)-hours*(60*60))/(60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appDur_spin_dd)
            , (gdouble)day);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appDur_spin_hh)
            , (gdouble)hours);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appDur_spin_mm)
            , (gdouble)minutes);
}

appt_data *
fill_appt_window_get_appt(char *action, char *par)
{
    appt_data *appt=NULL;
    struct tm *t;
    gchar today[9];

    if (strcmp(action, "NEW") == 0) {
/* par contains XFICAL_APPT_DATE_FORMAT (yyyymmdd) date for NEW appointment */
        appt = xfical_appt_alloc();
        t = orage_localtime();
        g_sprintf(today, "%04d%02d%02d", t->tm_year+1900, t->tm_mon+1
                , t->tm_mday);
        /* If we're today, we propose an appointment the next half-hour */
        /* hour 24 is wrong, we use 00 */
        if (strcmp(par, today) == 0 && t->tm_hour < 23) { 
            if(t->tm_min <= 30){
                g_sprintf(appt->starttime,"%sT%02d%02d00"
                            , par, t->tm_hour, 30);
                g_sprintf(appt->endtime,"%sT%02d%02d00"
                            , par, t->tm_hour + 1, 00);
            }
            else{
                g_sprintf(appt->starttime,"%sT%02d%02d00"
                            , par, t->tm_hour + 1, 00);
                g_sprintf(appt->endtime,"%sT%02d%02d00"
                            , par, t->tm_hour + 1, 30);
            }
        }
        /* otherwise we suggest it at 09:00 in the morning. */
        else {
            g_sprintf(appt->starttime,"%sT090000", par);
            g_sprintf(appt->endtime,"%sT093000", par);
        }
        if (local_icaltimezone_utc) {
            appt->start_tz_loc = g_strdup("UTC");
            appt->end_tz_loc = g_strdup("UTC");
        }
        else if (local_icaltimezone_location) {
            appt->start_tz_loc = g_strdup(local_icaltimezone_location);
            appt->end_tz_loc = g_strdup(local_icaltimezone_location);
        }
        appt->duration = 30;
    }
    else if ((strcmp(action, "UPDATE") == 0) 
          || (strcmp(action, "COPY") == 0)) {
        /* par contains ical uid */
        if (!xfical_file_open()) {
            g_error("ical file open failed\n");
            return(NULL);
        }
        if ((appt = xfical_appt_get(par)) == NULL) {
            g_message("Orage **: appointment not found");
            xfical_file_close();
            return(NULL);
        }
        xfical_file_close();
    }
    else {
        g_error("unknown parameter\n");
        return(NULL);
    }

    return(appt);
}

void
fill_appt_window(appt_win *apptw, char *action, char *par)
{
    int year, month, day, hours, minutes;
    GtkTextBuffer *tb;
    appt_data *appt;
    struct tm *t;
    char *untildate_to_display;
    int i;

    g_message("Orage **: %s appointment: %s", action, par);
    if ((appt = fill_appt_window_get_appt(action, par)) == NULL) {
        g_error("fill_appt_window: Appointment initialization failed");
        return;
    }
    apptw->appt = appt; 
    /* let's make sure timezones are never null. 
     * Remember: null means use floating */
    if (!appt->start_tz_loc)
        appt->start_tz_loc = g_strdup("floating");
    if (!appt->end_tz_loc)
        appt->end_tz_loc = g_strdup("floating");


    /* first flags */
    apptw->xf_uid = g_strdup(appt->uid);
    apptw->par = g_strdup((const gchar *)par);
    apptw->appointment_changed = FALSE;
    if (strcmp(action, "NEW") == 0) {
        apptw->appointment_add = TRUE;
        apptw->appointment_new = TRUE;
    }
    else if (strcmp(action, "UPDATE") == 0) {
        apptw->appointment_add = FALSE;
        apptw->appointment_new = FALSE;
    }
    else if (strcmp(action, "COPY") == 0) {
            /* COPY uses old uid as base and adds new, so
             * add == TRUE && new == FALSE */
        apptw->appointment_add = TRUE;
        apptw->appointment_new = FALSE;
    }
    else {
        g_error("unknown parameter\n");
        return;
    }
    /* we only want to enable duplication if we are working with an old
     * existing app (=not adding new) */
    gtk_widget_set_sensitive(apptw->appDuplicate, !apptw->appointment_add);
    gtk_widget_set_sensitive(apptw->appFileDuplicate_menuitem
            , !apptw->appointment_add);

    /* window title */
    gtk_window_set_title(GTK_WINDOW(apptw->appWindow)
            , _("New appointment - Orage"));

    /********************* GENERAL tab *********************/
    /* appointment name */
    gtk_entry_set_text(GTK_ENTRY(apptw->appTitle_entry)
            , (appt->title ? appt->title : ""));

    /* location */
    gtk_entry_set_text(GTK_ENTRY(apptw->appLocation_entry)
            , (appt->location ? appt->location : ""));

    /* times */
    fill_appt_window_times(apptw, appt);

    /* availability */
    if (appt->availability != -1) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->appAvailability_cb)
                   , appt->availability);
    }

    /* note */
    tb = gtk_text_view_get_buffer((GtkTextView *)apptw->appNote_textview);
    gtk_text_buffer_set_text(tb
            , (appt->note ? (const gchar *) appt->note : ""), -1);

    /********************* ALARM tab *********************/
    /* alarmtime (comes in seconds) */
    day = appt->alarmtime/(24*60*60);
    hours = (appt->alarmtime-day*(24*60*60))/(60*60);
    minutes = (appt->alarmtime-day*(24*60*60)-hours*(60*60))/(60);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appAlarm_spin_dd)
                , (gdouble)day);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appAlarm_spin_hh)
                , (gdouble)hours);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(apptw->appAlarm_spin_mm)
                , (gdouble)minutes);

    /* alarm sound */
    gtk_entry_set_text(GTK_ENTRY(apptw->appSound_entry)
            , (appt->sound ? appt->sound : ""));

    /* alarm repeat */
    gtk_toggle_button_set_active(
            GTK_TOGGLE_BUTTON(apptw->appSoundRepeat_checkbutton)
                    , appt->alarmrepeat);

    /********************* RECURRENCE tab *********************/
    /* recurrence */
    gtk_combo_box_set_active(GTK_COMBO_BOX(apptw->appRecur_freq_cb)
            , appt->freq);
    switch(appt->recur_limit) {
        case 0: /* no limit */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->appRecur_limit_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->appRecur_count_spin), (gdouble)1);
            t = orage_localtime();
            untildate_to_display = year_month_day_to_display(t->tm_year+1900
                    , t->tm_mon+1, t->tm_mday);
            gtk_button_set_label(GTK_BUTTON(apptw->appRecur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        case 1: /* count */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->appRecur_count_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->appRecur_count_spin)
                    , (gdouble)appt->recur_count);
            t = orage_localtime();
            untildate_to_display = year_month_day_to_display(t->tm_year+1900
                    , t->tm_mon+1, t->tm_mday);
            gtk_button_set_label(GTK_BUTTON(apptw->appRecur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        case 2: /* until */
            gtk_toggle_button_set_active(
                    GTK_TOGGLE_BUTTON(apptw->appRecur_until_rb), TRUE);
            gtk_spin_button_set_value(
                    GTK_SPIN_BUTTON(apptw->appRecur_count_spin), (gdouble)1);
            ical_to_year_month_day_hour_minute(appt->recur_until
                    , &year, &month, &day, &hours, &minutes);
            untildate_to_display = year_month_day_to_display(year, month, day);
            gtk_button_set_label(GTK_BUTTON(apptw->appRecur_until_button)
                    , (const gchar *)untildate_to_display);
            break;
        default: /* error */
            g_warning("fill_appt_window: Unsupported recur_limit %d",
                    appt->recur_limit);
    }

    /* weekdays */
    for (i=0; i <= 6; i++) {
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_byday_cb[i])
                , appt->recur_byday[i]);
        /* which weekday of month / year */
        gtk_spin_button_set_value(
                GTK_SPIN_BUTTON(apptw->appRecur_byday_spin[i])
                , (gdouble)appt->recur_byday_cnt[i]);
    }

    /* interval */
    gtk_spin_button_set_value(
            GTK_SPIN_BUTTON(apptw->appRecur_int_spin)
            , (gdouble)appt->interval);

    if (appt->interval > 1
    || !appt->recur_byday[0] || !appt->recur_byday[1] || !appt->recur_byday[2] 
    || !appt->recur_byday[3] || !appt->recur_byday[4] || !appt->recur_byday[5] 
    || !appt->recur_byday[6]) {
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_feature_advanced_rb), TRUE);
    }
    else {
        gtk_toggle_button_set_active(
                GTK_TOGGLE_BUTTON(apptw->appRecur_feature_normal_rb), TRUE);
    }

    set_time_sensitivity(apptw);
    set_repeat_sensitivity(apptw);
    mark_appointment_unchanged(apptw);
}

void
create_appt_win_menu(appt_win *apptw)
{
    GtkWidget *menu_separator;

    /* Menu bar */
    apptw->appMenubar = gtk_menu_bar_new();
    gtk_box_pack_start(GTK_BOX(apptw->appVBox1), apptw->appMenubar
            , FALSE, FALSE, 0);

    /* File menu stuff */
    apptw->appFile_menu = xfcalendar_menu_new(_("_File"), apptw->appMenubar);

    apptw->appFileSave_menuitem = 
            xfcalendar_image_menu_item_new_from_stock("gtk-save"
                    , apptw->appFile_menu, apptw->appAccelgroup);

    apptw->appFileSaveClose_menuitem = 
            xfcalendar_menu_item_new_with_mnemonic(_("Sav_e and close")
                    , apptw->appFile_menu);
    gtk_widget_add_accelerator(apptw->appFileSaveClose_menuitem
            , "activate", apptw->appAccelgroup
            , GDK_w, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);

    menu_separator = xfcalendar_separator_menu_item_new(apptw->appFile_menu);

    apptw->appFileRevert_menuitem = 
            xfcalendar_image_menu_item_new_from_stock("gtk-revert-to-saved"
                    , apptw->appFile_menu, apptw->appAccelgroup);

    apptw->appFileDuplicate_menuitem = 
            xfcalendar_menu_item_new_with_mnemonic(_("D_uplicate")
                    , apptw->appFile_menu);
    gtk_widget_add_accelerator(apptw->appFileDuplicate_menuitem
            , "activate", apptw->appAccelgroup
            , GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

    menu_separator = xfcalendar_separator_menu_item_new(apptw->appFile_menu);

    apptw->appFileDelete_menuitem = 
            xfcalendar_image_menu_item_new_from_stock("gtk-delete"
                    , apptw->appFile_menu, apptw->appAccelgroup);

    menu_separator = xfcalendar_separator_menu_item_new(apptw->appFile_menu);

    apptw->appFileClose_menuitem = 
            xfcalendar_image_menu_item_new_from_stock("gtk-close"
                    , apptw->appFile_menu, apptw->appAccelgroup);
}

void
create_appt_win_buttons(appt_win *apptw)
{
    gint i = 0;
    GtkWidget *toolbar_separator;

    /* Handle box and toolbar */
    /*
    apptw->appHandleBox = gtk_handle_box_new();
    gtk_box_pack_start(GTK_BOX(apptw->appVBox1), apptw->appHandleBox
            , FALSE, FALSE, 0);

    apptw->appToolbar = gtk_toolbar_new();
    gtk_container_add(GTK_CONTAINER (apptw->appHandleBox), apptw->appToolbar);
    */
    apptw->appToolbar = gtk_toolbar_new();
    gtk_box_pack_start(GTK_BOX(apptw->appVBox1), apptw->appToolbar
            , FALSE, FALSE, 0);
    gtk_toolbar_set_tooltips(GTK_TOOLBAR(apptw->appToolbar), TRUE);

    /* Add buttons to the toolbar */
    apptw->appSave = xfcalendar_toolbar_append_button(apptw->appToolbar
            , "gtk-save", apptw->appTooltips, _("Save"), i++);

    apptw->appSaveClose = xfcalendar_toolbar_append_button(apptw->appToolbar
            , "gtk-close", apptw->appTooltips, _("Save and close"), i++);
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(apptw->appSaveClose)
            , _("Save and close"));
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(apptw->appSaveClose), TRUE);

    toolbar_separator = xfcalendar_toolbar_append_separator(apptw->appToolbar
            , i++);

    apptw->appRevert = xfcalendar_toolbar_append_button(apptw->appToolbar
            , "gtk-revert-to-saved", apptw->appTooltips, _("Revert"), i++);

    apptw->appDuplicate = xfcalendar_toolbar_append_button(apptw->appToolbar
            , "gtk-copy", apptw->appTooltips, _("Duplicate"), i++);
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(apptw->appDuplicate)
            , _("Duplicate"));

    toolbar_separator = xfcalendar_toolbar_append_separator(apptw->appToolbar
            , i++);

    apptw->appDelete = xfcalendar_toolbar_append_button(apptw->appToolbar
            , "gtk-delete", apptw->appTooltips, _("Delete"), i++);
}

void
create_appt_win_tab_general(appt_win *apptw)
{
    GtkWidget *label;
    char *availability_array[AVAILABILITY_ARRAY_DIM] = {
            _("Free"), _("Busy")};

    apptw->appTableGeneral = xfcalendar_table_new(8, 2);
    apptw->appGeneral_notebook_page = 
        xfce_create_framebox_with_content(NULL, apptw->appTableGeneral);
    apptw->appGeneral_tab_label = gtk_label_new(_("General"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->appNotebook)
            , apptw->appGeneral_notebook_page, apptw->appGeneral_tab_label);

    apptw->appTitle_label = gtk_label_new(_("Title "));
    apptw->appTitle_entry = gtk_entry_new();
    xfcalendar_table_add_row(apptw->appTableGeneral
            , apptw->appTitle_label, apptw->appTitle_entry, 0
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));

    apptw->appLocation_label = gtk_label_new(_("Location"));
    apptw->appLocation_entry = gtk_entry_new();
    xfcalendar_table_add_row(apptw->appTableGeneral
            , apptw->appLocation_label, apptw->appLocation_entry, 1
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));

    apptw->appAllDay_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("All day event"));
    xfcalendar_table_add_row(apptw->appTableGeneral
            , NULL, apptw->appAllDay_checkbutton, 2
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));

    apptw->appStart = gtk_label_new(_("Start"));
    apptw->appStartDate_button = gtk_button_new();
    apptw->appStartTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->appStartTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    apptw->appStartTimezone_button = gtk_button_new();
    apptw->appStartTime_hbox = xfcalendar_datetime_hbox_new(
            apptw->appStartDate_button, 
            apptw->appStartTime_spin_hh, 
            apptw->appStartTime_spin_mm, 
            apptw->appStartTimezone_button);
    xfcalendar_table_add_row(apptw->appTableGeneral
            , apptw->appStart, apptw->appStartTime_hbox, 3
            , (GtkAttachOptions) (GTK_SHRINK | GTK_FILL)
            , (GtkAttachOptions) (GTK_SHRINK | GTK_FILL));

    apptw->appEnd = gtk_label_new(_("End"));
    apptw->appEndDate_button = gtk_button_new();
    apptw->appEndTime_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    apptw->appEndTime_spin_mm = gtk_spin_button_new_with_range(0, 59, 1);
    apptw->appEndTimezone_button = gtk_button_new();
    apptw->appEndTime_hbox = xfcalendar_datetime_hbox_new(
            apptw->appEndDate_button, 
            apptw->appEndTime_spin_hh, 
            apptw->appEndTime_spin_mm, 
            apptw->appEndTimezone_button);
    xfcalendar_table_add_row(apptw->appTableGeneral
            , apptw->appEnd, apptw->appEndTime_hbox, 4
            , (GtkAttachOptions) (GTK_SHRINK | GTK_FILL)
            , (GtkAttachOptions) (GTK_SHRINK | GTK_FILL));

    apptw->appDur_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appDur_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Duration"));
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox), apptw->appDur_checkbutton
            , FALSE, FALSE, 0);
    apptw->appDur_spin_dd = gtk_spin_button_new_with_range(0, 1000, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appDur_spin_dd), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox), apptw->appDur_spin_dd
            , FALSE, FALSE, 5);
    apptw->appDur_spin_dd_label = gtk_label_new(_("days"));
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox)
            , apptw->appDur_spin_dd_label, FALSE, FALSE, 0);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox), label, FALSE, FALSE, 5);
    apptw->appDur_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appDur_spin_hh), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox), apptw->appDur_spin_hh
            , FALSE, FALSE, 5);
    apptw->appDur_spin_hh_label = gtk_label_new(_("hours"));
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox)
            , apptw->appDur_spin_hh_label, FALSE, FALSE, 0);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox), label, FALSE, FALSE, 5);
    apptw->appDur_spin_mm = gtk_spin_button_new_with_range(0, 59, 5);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appDur_spin_mm), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->appDur_hbox), apptw->appDur_spin_mm
            , FALSE, FALSE, 5);
    apptw->appDur_spin_mm_label = gtk_label_new(_("mins"));
    gtk_box_pack_start(GTK_BOX (apptw->appDur_hbox)
            , apptw->appDur_spin_mm_label, FALSE, FALSE, 0);
    xfcalendar_table_add_row(apptw->appTableGeneral
            , NULL, apptw->appDur_hbox, 5
            , (GtkAttachOptions) (GTK_FILL)
            , (GtkAttachOptions) (GTK_FILL));
    
    apptw->appAvailability = gtk_label_new(_("Availability"));
    apptw->appAvailability_cb = gtk_combo_box_new_text();
    xfcalendar_combo_box_append_array(apptw->appAvailability_cb
            , availability_array, AVAILABILITY_ARRAY_DIM);
    xfcalendar_table_add_row(apptw->appTableGeneral
            , apptw->appAvailability, apptw->appAvailability_cb, 6
            , (GtkAttachOptions) (GTK_FILL)
            , (GtkAttachOptions) (GTK_FILL));

    apptw->appNote = gtk_label_new(_("Note"));
    apptw->appNote_Scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(apptw->appNote_Scrolledwindow)
            , GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(
            GTK_SCROLLED_WINDOW(apptw->appNote_Scrolledwindow)
            , GTK_SHADOW_IN);
    apptw->appNote_buffer = gtk_text_buffer_new(NULL);
    apptw->appNote_textview = 
            gtk_text_view_new_with_buffer(
                    GTK_TEXT_BUFFER(apptw->appNote_buffer));
    gtk_container_add(GTK_CONTAINER(apptw->appNote_Scrolledwindow)
            , apptw->appNote_textview);
    xfcalendar_table_add_row(apptw->appTableGeneral
            , apptw->appNote, apptw->appNote_Scrolledwindow, 7
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL));
    gtk_widget_grab_focus(apptw->appTitle_entry);
}

void
create_appt_win_tab_alarm(appt_win *apptw)
{
    GtkWidget *label;

    apptw->appTableAlarm = xfcalendar_table_new(8, 2);
    apptw->appAlarm_notebook_page = 
        xfce_create_framebox_with_content(NULL, apptw->appTableAlarm);
    apptw->appAlarm_tab_label = gtk_label_new(_("Alarm"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->appNotebook)
            , apptw->appAlarm_notebook_page, apptw->appAlarm_tab_label);

    apptw->appAlarm = gtk_label_new(_("Alarm"));
    apptw->appAlarm_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appAlarm_spin_dd = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appAlarm_spin_dd), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->appAlarm_hbox)
            , apptw->appAlarm_spin_dd, FALSE, FALSE, 0);
    label = gtk_label_new(_("days"));
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 5);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 10);
    apptw->appAlarm_spin_hh = gtk_spin_button_new_with_range(0, 23, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appAlarm_spin_hh), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox)
            , apptw->appAlarm_spin_hh, FALSE, FALSE, 0);
    label = gtk_label_new(_("hours"));
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 5);
    label = gtk_label_new(" ");
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 10);
    apptw->appAlarm_spin_mm = gtk_spin_button_new_with_range(0, 59, 5);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appAlarm_spin_mm), TRUE);
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), apptw->appAlarm_spin_mm
            , FALSE, FALSE, 0);
    label = gtk_label_new(_("mins"));
    gtk_box_pack_start(GTK_BOX (apptw->appAlarm_hbox), label, FALSE, FALSE, 5);
    xfcalendar_table_add_row(apptw->appTableAlarm
            , apptw->appAlarm, apptw->appAlarm_hbox, 1
            , (GtkAttachOptions) (GTK_FILL)
            , (GtkAttachOptions) (GTK_FILL));

    apptw->appSound_label = gtk_label_new(_("Sound"));
    apptw->appSound_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_set_spacing(GTK_BOX(apptw->appSound_hbox), 6);
    apptw->appSound_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(apptw->appSound_hbox), apptw->appSound_entry
            , TRUE, TRUE, 0);
    apptw->appSound_button = gtk_button_new_from_stock("gtk-find");
    gtk_box_pack_start(GTK_BOX(apptw->appSound_hbox), apptw->appSound_button
            , FALSE, TRUE, 0);
    xfcalendar_table_add_row(apptw->appTableAlarm
            , apptw->appSound_label, apptw->appSound_hbox, 2
            , (GtkAttachOptions) (GTK_FILL)
            , (GtkAttachOptions) (GTK_FILL));

    apptw->appSoundRepeat_checkbutton = 
            gtk_check_button_new_with_mnemonic(_("Repeat alarm sound"));
    xfcalendar_table_add_row(apptw->appTableAlarm
            , NULL, apptw->appSoundRepeat_checkbutton, 3
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));
}

void
create_appt_win_tab_recurrence(appt_win *apptw)
{
    gint row=1, i;
    char *recur_freq_array[RECUR_ARRAY_DIM] = {
        _("None"), _("Daily"), _("Weekly"), _("Monthly"), _("Yearly")};
    char *weekday_array[7] = {
        _("Mon"), _("Tue"), _("Wed"), _("Thu"), _("Fri"), _("Sat"), _("Sun")};

    apptw->appTableRecur = xfcalendar_table_new(8, 2);
    apptw->appRecur_notebook_page = 
            xfce_create_framebox_with_content(NULL, apptw->appTableRecur);
    apptw->appRecur_tab_label = gtk_label_new(_("Recurrence"));

    gtk_notebook_append_page(GTK_NOTEBOOK(apptw->appNotebook)
            , apptw->appRecur_notebook_page, apptw->appRecur_tab_label);

    apptw->appRecur_feature_label = gtk_label_new(_("Complexity"));
    apptw->appRecur_feature_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appRecur_feature_normal_rb = gtk_radio_button_new_with_label(NULL
            , _("Basic"));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_feature_hbox)
            , apptw->appRecur_feature_normal_rb, FALSE, FALSE, 15);
    apptw->appRecur_feature_advanced_rb = 
            gtk_radio_button_new_with_mnemonic_from_widget(
                    GTK_RADIO_BUTTON(apptw->appRecur_feature_normal_rb)
            , _("Advanced"));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_feature_hbox)
            , apptw->appRecur_feature_advanced_rb, FALSE, FALSE, 15);
    xfcalendar_table_add_row(apptw->appTableRecur
            , apptw->appRecur_feature_label, apptw->appRecur_feature_hbox, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));
    gtk_tooltips_set_tip(apptw->appTooltips, apptw->appRecur_feature_normal_rb
            , _("Use this if you want regular repeating event"), NULL);
    gtk_tooltips_set_tip(apptw->appTooltips, apptw->appRecur_feature_advanced_rb
            , _("Use this if you need complex times like:\n Every second week or \n Every Saturday and Sunday or \n First Tuesday every month")
            , NULL);

    apptw->appRecur_freq_label = gtk_label_new(_("Frequency"));
    apptw->appRecur_freq_cb = gtk_combo_box_new_text();
    xfcalendar_combo_box_append_array(apptw->appRecur_freq_cb
            , recur_freq_array, RECUR_ARRAY_DIM);
    xfcalendar_table_add_row(apptw->appTableRecur
            , apptw->appRecur_freq_label, apptw->appRecur_freq_cb, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (GTK_FILL));

    apptw->appRecur_int_label = gtk_label_new(_("Interval"));
    apptw->appRecur_int_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appRecur_int_spin_label1 = gtk_label_new(_("Each"));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_int_hbox)
            , apptw->appRecur_int_spin_label1, FALSE, FALSE, 0);
    apptw->appRecur_int_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appRecur_int_spin), TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_int_hbox)
            , apptw->appRecur_int_spin, FALSE, FALSE, 5);
    apptw->appRecur_int_spin_label2 = gtk_label_new(_("occurrence"));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_int_hbox)
            , apptw->appRecur_int_spin_label2, FALSE, FALSE, 0);
    xfcalendar_table_add_row(apptw->appTableRecur
            , apptw->appRecur_int_label, apptw->appRecur_int_hbox, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));
    gtk_tooltips_set_tip(apptw->appTooltips, apptw->appRecur_int_spin
            , _("Limit frequency to certain interval. For example:\n Every third day: Frequency = Daily and Interval = 3")
            , NULL);

    apptw->appRecur_limit_label = gtk_label_new(_("Limit"));
    apptw->appRecur_limit_rb = gtk_radio_button_new_with_label(NULL
            , _("Repeat forever"));
    xfcalendar_table_add_row(apptw->appTableRecur
            , apptw->appRecur_limit_label, apptw->appRecur_limit_rb, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));

    apptw->appRecur_count_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appRecur_count_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->appRecur_limit_rb), _("Repeat "));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_count_hbox)
            , apptw->appRecur_count_rb, FALSE, FALSE, 0);
    apptw->appRecur_count_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(apptw->appRecur_count_spin)
            , TRUE);
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_count_hbox)
            , apptw->appRecur_count_spin, FALSE, FALSE, 0);
    apptw->appRecur_count_label = gtk_label_new(_("times"));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_count_hbox)
            , apptw->appRecur_count_label, FALSE, FALSE, 5);
    xfcalendar_table_add_row(apptw->appTableRecur
            , NULL, apptw->appRecur_count_hbox, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));

    apptw->appRecur_until_hbox = gtk_hbox_new(FALSE, 0);
    apptw->appRecur_until_rb = gtk_radio_button_new_with_mnemonic_from_widget(
            GTK_RADIO_BUTTON(apptw->appRecur_limit_rb), _("Repeat until "));
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_until_hbox)
            , apptw->appRecur_until_rb, FALSE, FALSE, 0);
    apptw->appRecur_until_button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(apptw->appRecur_until_hbox)
            , apptw->appRecur_until_button, FALSE, FALSE, 0);
    xfcalendar_table_add_row(apptw->appTableRecur
            , NULL, apptw->appRecur_until_hbox, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));

    apptw->appRecur_byday_label = gtk_label_new(_("Weekdays"));
    apptw->appRecur_byday_hbox = gtk_hbox_new(TRUE, 0);
    for (i=0; i <= 6; i++) {
        apptw->appRecur_byday_cb[i] = 
                gtk_check_button_new_with_mnemonic(weekday_array[i]);
        gtk_box_pack_start(GTK_BOX(apptw->appRecur_byday_hbox)
                , apptw->appRecur_byday_cb[i], FALSE, FALSE, 0);
    }
    xfcalendar_table_add_row(apptw->appTableRecur
            , apptw->appRecur_byday_label, apptw->appRecur_byday_hbox, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));

    apptw->appRecur_byday_spin_label = gtk_label_new(_("Which day"));
    apptw->appRecur_byday_spin_hbox = gtk_hbox_new(TRUE, 0);
    for (i=0; i <= 6; i++) {
        apptw->appRecur_byday_spin[i] = 
                gtk_spin_button_new_with_range(-9, 9, 1);
        gtk_box_pack_start(GTK_BOX(apptw->appRecur_byday_spin_hbox)
                , apptw->appRecur_byday_spin[i], FALSE, FALSE, 0);
        gtk_tooltips_set_tip(apptw->appTooltips, apptw->appRecur_byday_spin[i]
                , _("Specify which weekday for monthly and yearly events.\n For example:\n Second Wednesday each month:\n\tFrequency = Monthly,\n\tWeekdays = check only Wednesday,\n\tWhich day = select 2 from the number below Wednesday")
                , NULL);
    }
    xfcalendar_table_add_row(apptw->appTableRecur
            , apptw->appRecur_byday_spin_label
            , apptw->appRecur_byday_spin_hbox, row++
            , (GtkAttachOptions) (GTK_EXPAND | GTK_FILL)
            , (GtkAttachOptions) (0));
}

appt_win 
*create_appt_win(char *action, char *par, eventlist_win *event_list)
{
    int i;
    appt_win *apptw = g_new(appt_win, 1);

    /* main window creation and base elements */
    apptw->xf_uid = NULL;
    apptw->eventlist = event_list;    /* Keep track of the parent, if any */
    apptw->appointment_changed = FALSE;

    apptw->appWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(apptw->appWindow), 450, 325);

    apptw->appTooltips = gtk_tooltips_new();
    apptw->appAccelgroup = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(apptw->appWindow)
            , apptw->appAccelgroup);

    apptw->appVBox1 = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(apptw->appWindow), apptw->appVBox1);

    create_appt_win_menu(apptw);

    create_appt_win_buttons(apptw);

    /* ********** Here begins tabs ********** */
    apptw->appNotebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(apptw->appVBox1), apptw->appNotebook);
    gtk_container_set_border_width(GTK_CONTAINER (apptw->appNotebook), 5);

    create_appt_win_tab_general(apptw);

    create_appt_win_tab_alarm(apptw);

    create_appt_win_tab_recurrence(apptw);

    /* signals */

    g_signal_connect((gpointer) apptw->appFileSave_menuitem, "activate",
            G_CALLBACK(on_appFileSave_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileSaveClose_menuitem, "activate",
            G_CALLBACK(on_appFileSaveClose_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileDuplicate_menuitem, "activate",
            G_CALLBACK(on_appFileDuplicate_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileRevert_menuitem, "activate",
            G_CALLBACK(on_appFileRevert_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileDelete_menuitem, "activate",
            G_CALLBACK(on_appFileDelete_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appFileClose_menuitem, "activate",
            G_CALLBACK(on_appFileClose_menu_activate_cb), apptw);

    g_signal_connect((gpointer) apptw->appAllDay_checkbutton, "clicked",
            G_CALLBACK(on_appAllDay_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appSound_button, "clicked",
            G_CALLBACK(on_appSound_button_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appSave, "clicked",
            G_CALLBACK(on_appSave_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appSaveClose, "clicked",
            G_CALLBACK(on_appSaveClose_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDelete, "clicked",
            G_CALLBACK(on_appDelete_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDuplicate, "clicked",
            G_CALLBACK(on_appDuplicate_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRevert, "clicked",
            G_CALLBACK(on_appRevert_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartDate_button, "clicked",
            G_CALLBACK(on_appStartEndDate_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndDate_button, "clicked",
            G_CALLBACK(on_appStartEndDate_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartTime_spin_hh, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartTime_spin_mm, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndTime_spin_hh, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndTime_spin_mm, "changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appStartTimezone_button, "clicked",
            G_CALLBACK(on_appStartTimezone_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appEndTimezone_button, "clicked",
            G_CALLBACK(on_appEndTimezone_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_checkbutton, "clicked",
            G_CALLBACK(appDur_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_spin_dd, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_spin_hh, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appDur_spin_mm, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appWindow, "delete-event",
            G_CALLBACK(on_appWindow_delete_event_cb), apptw);
    /* Take care of the title entry to build the appointment window title 
     * Beware: we are not using apptw->appTitle_entry as a GtkEntry here 
     * but as an interface GtkEditable instead.
     */
    g_signal_connect((gpointer) apptw->appTitle_entry, "changed",
            G_CALLBACK(on_appTitle_entry_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appLocation_entry, "changed",
            G_CALLBACK(on_app_entry_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appSoundRepeat_checkbutton, "clicked",
            G_CALLBACK(app_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_freq_cb, "changed",
            G_CALLBACK(on_app_combobox_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_limit_rb, "clicked",
            G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_count_rb, "clicked",
            G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_count_spin, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_until_rb, "clicked",
            G_CALLBACK(app_recur_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_feature_normal_rb, "clicked",
            G_CALLBACK(app_recur_feature_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_feature_advanced_rb, "clicked",
            G_CALLBACK(app_recur_feature_checkbutton_clicked_cb), apptw);

    g_signal_connect((gpointer) apptw->appRecur_until_button, "clicked",
            G_CALLBACK(on_appStartEndDate_clicked_cb), apptw);


    for (i=0; i <= 6; i++) {
        g_signal_connect((gpointer) apptw->appRecur_byday_cb[i], "clicked",
                G_CALLBACK(app_checkbutton_clicked_cb), apptw);
        g_signal_connect((gpointer) apptw->appRecur_byday_spin[i], 
                "value-changed",
                G_CALLBACK(on_app_spin_button_changed_cb), apptw);
    }
    g_signal_connect((gpointer) apptw->appRecur_int_spin, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appAvailability_cb, "changed",
            G_CALLBACK(on_app_combobox_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appAlarm_spin_dd, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appAlarm_spin_hh, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appAlarm_spin_mm, "value-changed",
            G_CALLBACK(on_app_spin_button_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appNote_buffer, "changed",
            G_CALLBACK(on_appNote_buffer_changed_cb), apptw);

    g_signal_connect((gpointer) apptw->appSound_entry, "changed",
            G_CALLBACK(on_app_entry_changed_cb), apptw);

    fill_appt_window(apptw, action, par);

    gtk_widget_show_all(apptw->appWindow);
    if (gtk_toggle_button_get_active(
            GTK_TOGGLE_BUTTON(apptw->appRecur_feature_normal_rb))) {
        gtk_widget_hide(apptw->appRecur_int_label);
        gtk_widget_hide(apptw->appRecur_int_hbox);
        gtk_widget_hide(apptw->appRecur_byday_label);
        gtk_widget_hide(apptw->appRecur_byday_hbox);
        gtk_widget_hide(apptw->appRecur_byday_spin_label);
        gtk_widget_hide(apptw->appRecur_byday_spin_hbox);
    }

    return apptw;
}
