/* appointment.c
 *
 * Copyright (C) 2004-2005 Micka�l Graf <korbinus at xfce.org>
 * Parts of the code below are copyright (C) 2005 Juha Kautto <kautto.juha at kolumbus.fi>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/netk-trayicon.h>
#include <libxfcegui4/libxfcegui4.h>
#include <libxfce4mcs/mcs-client.h>

#include "appointment.h"
#include "ical-code.h"
//#include "support.h"

#define DATE_SEPARATOR "-"
#define MAX_APP_LENGTH 4096
#define RCDIR          "xfce4" G_DIR_SEPARATOR_S "xfcalendar"
#define APPOINTMENT_FILE "appointments.ics"


void
on_appClose_clicked_cb(GtkButton *button, gpointer user_data)
{

  appt_win *apptw = (appt_win *)user_data;

  appt_type *appt = g_new(appt_type, 1); 
  gchar *new_uid;

  gint StartHour_value,
    StartMinutes_value,
    EndHour_value,
    EndMinutes_value;

  G_CONST_RETURN gchar *winTitle;

  gchar *Note_value;

  char a_day[10];

  char *cyear, *cmonth, *cday;
  int iyear, imonth, iday;

  GtkTextIter start, end;

  winTitle = gtk_window_get_title(GTK_WINDOW(apptw->appWindow));

  cyear   = strtok ((char *) winTitle, DATE_SEPARATOR);
  cmonth  = strtok (NULL, DATE_SEPARATOR);
  cday    = strtok (NULL, DATE_SEPARATOR);

  iyear = atoi(cyear);
  imonth = atoi(cmonth);
  iday = atoi(cday);

  appt->title = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appTitle_entry);
  g_warning("Title: %s\n", appt->title);

  appt->location = (gchar *) gtk_entry_get_text((GtkEntry *)apptw->appLocation_entry);
  g_warning("Location: %s\n", appt->location);

  StartHour_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appStartHour_spinbutton);
  StartMinutes_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appStartMinutes_spinbutton);
  g_sprintf(appt->starttime, XF_APP_TIME_FORMAT
	    , iyear, imonth, iday
        , StartHour_value, StartMinutes_value, 0);
  g_warning("Start: %s\n", appt->starttime);

  EndHour_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appEndHour_spinbutton);
  EndMinutes_value = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appEndMinutes_spinbutton);
  g_sprintf(appt->endtime, XF_APP_TIME_FORMAT
	    , iyear, imonth, iday
        , EndHour_value, EndMinutes_value, 0);
  g_warning("End: %s\n", appt->endtime);

  appt->alarm = gtk_spin_button_get_value_as_int((GtkSpinButton *)apptw->appAlarm_spinbutton);
  g_warning("Alarm: %d\n", appt->alarm);

  appt->alarmTimeType = gtk_combo_box_get_active((GtkComboBox *)apptw->appAlarmTimeType_combobox);
  g_warning("Time Type: %d\n", appt->alarmTimeType);

  appt->availability = gtk_combo_box_get_active((GtkComboBox *)apptw->appAvailability_cb);
  g_warning("Availability: %d\n", appt->availability);

  gtk_text_buffer_get_bounds(gtk_text_view_get_buffer((GtkTextView *)apptw->appNote_textview), 
			     &start, 
			     &end);
  appt->note = gtk_text_iter_get_text(&start, &end);
  g_warning("Note: %s\n", appt->note);

  /* Here we try to save the event... */
  if (open_ical_file()){

     g_sprintf(a_day, XF_APP_DATE_FORMAT
	       , iyear, imonth, iday);

     g_warning("Date: %s\n", a_day);

     g_warning("Removing :%s \n", apptw->xf_uid);
     xf_del_ical_app(apptw->xf_uid);
     g_warning("Removed :%s \n", apptw->xf_uid);
     new_uid = xf_add_ical_app(appt);
     g_warning("New ical uid: %s \n", new_uid);

     close_ical_file();
  }

  gtk_widget_destroy(apptw->appWindow);

}

void
on_appRemove_clicked_cb(GtkButton *button, gpointer user_data)
{

  appt_win *apptw = (appt_win *)user_data;

  if (open_ical_file()){
    xf_del_ical_app(apptw->xf_uid);
     close_ical_file();
  }
  gtk_widget_destroy(apptw->appWindow);

}

void fill_appt_window(appt_win *appt, char *action, char *par)
{
  char appt_date[12], start_hh[3], start_mi[3], end_hh[3], end_mi[3];
  gint i, j;
  GtkTextBuffer *tb;
  appt_type *appt_data;

  appt_data = xf_alloc_ical_app();

    if (strcmp(action, "NEW") == 0) {
    /* par contains XF_APP_DATE_FORMAT (yyyymmdd) date for new appointment */
        for (i = 0, j = 0; i <= 10; i++) { /* yyyymmdd -> yyyy-mm-dd */
            if ((i == 4) || (i == 7))
                appt_date[i] = '-';
            else
                appt_date[i] = par[j++];
        }
        gtk_window_set_title (GTK_WINDOW (appt->appWindow), appt_date);
    }
    else if (strcmp(action, "UPDATE") == 0) {
    /* par contains ical uid */
        if (!open_ical_file()) {
            g_warning("ical file open failed\n");
            return;
        }
        if ((appt_data = xf_get_ical_app(par)) == NULL) {
            g_warning("appointment not found\n");
            return;
        }

	appt->xf_uid = g_strdup(appt_data->uid);
	g_warning("id: %s \n", appt->xf_uid);

        for (i = 0, j = 0; i <= 9; i++) { /* yyyymmdd -> yyyy-mm-dd */
            if ((i == 4) || (i == 7))
                appt_date[i] = '-';
            else
                appt_date[i] = appt_data->starttime[j++];
        }
        appt_date[10] = '\0';
        gtk_window_set_title (GTK_WINDOW (appt->appWindow), appt_date);
        if (appt_data->title)
            gtk_entry_set_text(GTK_ENTRY(appt->appTitle_entry), appt_data->title);
        if (appt_data->location)
            gtk_entry_set_text(GTK_ENTRY(appt->appLocation_entry), appt_data->location);
        if (strlen( appt_data->starttime) > 6 ) {
            start_hh[0]= appt_data->starttime[9];
            start_hh[1]= appt_data->starttime[10];
            start_hh[2]= '\0';
            start_mi[0]= appt_data->starttime[11];
            start_mi[1]= appt_data->starttime[12];
            start_mi[2]= '\0';
            gtk_spin_button_set_value(
                      GTK_SPIN_BUTTON(appt->appStartHour_spinbutton)
                    , (gdouble) atoi(start_hh));
            gtk_spin_button_set_value(
                      GTK_SPIN_BUTTON(appt->appStartMinutes_spinbutton)
                    , (gdouble) atoi(start_mi));
        }
        if (strlen( appt_data->endtime) > 6 ) {
            end_hh[0]= appt_data->endtime[9];
            end_hh[1]= appt_data->endtime[10];
            end_hh[2]= '\0';
            end_mi[0]= appt_data->endtime[11];
            end_mi[1]= appt_data->endtime[12];
            end_mi[2]= '\0';
            gtk_spin_button_set_value(
                      GTK_SPIN_BUTTON(appt->appEndHour_spinbutton)
                    , (gdouble) atoi(end_hh));
            gtk_spin_button_set_value(
                      GTK_SPIN_BUTTON(appt->appEndMinutes_spinbutton)
                    , (gdouble) atoi(end_mi));
        }
	if (appt_data->alarm){
	  gtk_spin_button_set_value(
				    GTK_SPIN_BUTTON(appt->appAlarm_spinbutton)
				    , (gdouble) appt_data->alarm);
	}
	if (appt_data->alarmTimeType != -1){
	  gtk_combo_box_set_active(GTK_COMBO_BOX(appt->appAlarmTimeType_combobox)
				   , appt_data->alarmTimeType);
	}
	if (appt_data->availability != -1){
	  gtk_combo_box_set_active(GTK_COMBO_BOX(appt->appAvailability_cb)
				   , appt_data->availability);
	}
	if (appt_data->note){
	  tb = gtk_text_view_get_buffer((GtkTextView *)appt->appNote_textview);
	  gtk_text_buffer_set_text(tb, (const gchar *) appt_data->note, -1);
	}
        close_ical_file();
    }
    else
    g_error("unknown parameter\n");

}

appt_win *create_appt_win(char *action, char *par)
{
  appt_win *appt = g_new(appt_win, 1);

  appt->xf_uid = NULL;

  appt->appWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  /*
  gtk_window_set_title (GTK_WINDOW (appt->appWindow), appt_date);
  */
  gtk_window_set_default_size (GTK_WINDOW (appt->appWindow), 450, 325);

  appt->appVBox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (appt->appVBox1);
  gtk_container_add (GTK_CONTAINER (appt->appWindow), appt->appVBox1);

  appt->appTable = gtk_table_new (9, 2, FALSE);
  gtk_widget_show (appt->appTable);
  gtk_box_pack_start (GTK_BOX (appt->appVBox1), appt->appTable, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appTable), 10);
  gtk_table_set_row_spacings (GTK_TABLE (appt->appTable), 5);
  gtk_table_set_col_spacings (GTK_TABLE (appt->appTable), 5);

  appt->appTitle_label = gtk_label_new (_("Title "));
  gtk_widget_show (appt->appTitle_label);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appTitle_label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appTitle_label), 0, 0.5);

  appt->appLocation_label = gtk_label_new (_("Location"));
  gtk_widget_show (appt->appLocation_label);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appLocation_label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appLocation_label), 0, 0.5);

  appt->appStart = gtk_label_new (_("Start"));
  gtk_widget_show (appt->appStart);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appStart, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appStart), 0, 0.5);

  appt->appEnd = gtk_label_new (_("End"));
  gtk_widget_show (appt->appEnd);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appEnd, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appEnd), 0, 0.5);

  /*
  appt->appPrivate = gtk_label_new (_("Private"));
  gtk_widget_show (appt->appPrivate);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appPrivate, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appPrivate), 0, 0.5);
  */

  appt->appAlarm = gtk_label_new (_("Alarm"));
  gtk_widget_show (appt->appAlarm);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAlarm, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appAlarm), 0, 0.5);

  /*
  appt->appRecurrence = gtk_label_new (_("Recurrence"));
  gtk_widget_show (appt->appRecurrence);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appRecurrence, 0, 1, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appRecurrence), 0, 0.5);
  */

  appt->appAvailability = gtk_label_new (_("Availability"));
  gtk_widget_show (appt->appAvailability);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAvailability, 0, 1, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appAvailability), 0, 0.5);

  appt->appNote = gtk_label_new (_("Note"));
  gtk_widget_show (appt->appNote);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appNote, 0, 1, 8, 9,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appNote), 0, 0.5);

  appt->appTitle_entry = gtk_entry_new ();
  gtk_widget_show (appt->appTitle_entry);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appTitle_entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  appt->appLocation_entry = gtk_entry_new ();
  gtk_widget_show (appt->appLocation_entry);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appLocation_entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  /*
  appt->appPrivate_check = gtk_check_button_new_with_mnemonic ("");
  gtk_widget_show (appt->appPrivate_check);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appPrivate_check, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  */

  appt->appNote_Scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (appt->appNote_Scrolledwindow);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appNote_Scrolledwindow, 1, 2, 8, 9,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (appt->appNote_Scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (appt->appNote_Scrolledwindow), GTK_SHADOW_IN);

  appt->appNote_textview = gtk_text_view_new ();
  gtk_widget_show (appt->appNote_textview);
  gtk_container_add (GTK_CONTAINER (appt->appNote_Scrolledwindow), appt->appNote_textview);

  appt->appAvailability_cb = gtk_combo_box_new_text ();
  gtk_widget_show (appt->appAvailability_cb);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAvailability_cb, 1, 2, 7, 8,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAvailability_cb), _("Free"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAvailability_cb), _("Busy"));

  appt->appAlarm_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (appt->appAlarm_hbox);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appAlarm_hbox, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  appt->appAlarm_spinbutton_adj = gtk_adjustment_new (0, 0, 100, 1, 10, 10);
  appt->appAlarm_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appAlarm_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appAlarm_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appAlarm_hbox), appt->appAlarm_spinbutton, TRUE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appAlarm_spinbutton), TRUE);

  appt->appAlarm_fixed_1 = gtk_fixed_new ();
  gtk_widget_show (appt->appAlarm_fixed_1);
  gtk_box_pack_start (GTK_BOX (appt->appAlarm_hbox), appt->appAlarm_fixed_1, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appAlarm_fixed_1), 10);

  appt->appAlarmTimeType_combobox = gtk_combo_box_new_text ();
  gtk_widget_show (appt->appAlarmTimeType_combobox);
  gtk_box_pack_start (GTK_BOX (appt->appAlarm_hbox), appt->appAlarmTimeType_combobox, TRUE, TRUE, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarmTimeType_combobox), _("minute(s)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarmTimeType_combobox), _("hour(s)"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (appt->appAlarmTimeType_combobox), _("day(s)"));

  appt->appAlarm_fixed_2 = gtk_fixed_new ();
  gtk_widget_show (appt->appAlarm_fixed_2);
  gtk_box_pack_start (GTK_BOX (appt->appAlarm_hbox), appt->appAlarm_fixed_2, TRUE, TRUE, 0);

  appt->appStartTime_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (appt->appStartTime_hbox);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appStartTime_hbox, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);

  appt->appStartHour_spinbutton_adj = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
  appt->appStartHour_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appStartHour_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appStartHour_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartHour_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appStartHour_spinbutton), TRUE);

  appt->appStartColumn_label = gtk_label_new (_(" : "));
  gtk_widget_show (appt->appStartColumn_label);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartColumn_label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appStartColumn_label), 0.5, 0.43);

  appt->appStartMinutes_spinbutton_adj = gtk_adjustment_new (0, 0, 45, 15, 10, 10);
  appt->appStartMinutes_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appStartMinutes_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appStartMinutes_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartMinutes_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appStartMinutes_spinbutton), TRUE);

  appt->appStartTime_fixed = gtk_fixed_new ();
  gtk_widget_show (appt->appStartTime_fixed);
  gtk_box_pack_start (GTK_BOX (appt->appStartTime_hbox), appt->appStartTime_fixed, TRUE, TRUE, 0);

  appt->appEndTime_hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (appt->appEndTime_hbox);
  gtk_table_attach (GTK_TABLE (appt->appTable), appt->appEndTime_hbox, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  appt->appEndHour_spinbutton_adj = gtk_adjustment_new (0, 0, 23, 1, 10, 10);
  appt->appEndHour_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appEndHour_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appEndHour_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndHour_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appEndHour_spinbutton), TRUE);

  appt->appEndColumn_label = gtk_label_new (_(" : "));
  gtk_widget_show (appt->appEndColumn_label);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndColumn_label, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (appt->appEndColumn_label), 0.5, 0.43);

  appt->appEndMinutes_spinbutton_adj = gtk_adjustment_new (0, 0, 45, 15, 10, 10);
  appt->appEndMinutes_spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (appt->appEndMinutes_spinbutton_adj), 1, 0);
  gtk_widget_show (appt->appEndMinutes_spinbutton);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndMinutes_spinbutton, FALSE, TRUE, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (appt->appEndMinutes_spinbutton), TRUE);

  appt->appEndTime_fixed = gtk_fixed_new ();
  gtk_widget_show (appt->appEndTime_fixed);
  gtk_box_pack_start (GTK_BOX (appt->appEndTime_hbox), appt->appEndTime_fixed, TRUE, TRUE, 0);

  appt->appHBox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (appt->appHBox1);
  gtk_box_pack_start (GTK_BOX (appt->appVBox1), appt->appHBox1, FALSE, TRUE, 0);

  appt->appRemove = gtk_button_new_from_stock ("gtk-remove");
  gtk_widget_show (appt->appRemove);
  gtk_box_pack_start (GTK_BOX (appt->appHBox1), appt->appRemove, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appRemove), 10);

  appt->appBottom_fixed = gtk_fixed_new ();
  gtk_widget_show (appt->appBottom_fixed);
  gtk_box_pack_start (GTK_BOX (appt->appHBox1), appt->appBottom_fixed, TRUE, TRUE, 0);

  appt->appClose = gtk_button_new_from_stock ("gtk-close");
  gtk_widget_show (appt->appClose);
  gtk_box_pack_start (GTK_BOX (appt->appHBox1), appt->appClose, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (appt->appClose), 10);
  GTK_WIDGET_SET_FLAGS (appt->appClose, GTK_CAN_DEFAULT);

  gtk_widget_grab_default (appt->appClose);

  g_signal_connect ((gpointer) appt->appClose, "clicked",
		    G_CALLBACK (on_appClose_clicked_cb),
		    appt);

  g_signal_connect ((gpointer) appt->appRemove, "clicked",
		    G_CALLBACK (on_appRemove_clicked_cb),
		    appt);

    fill_appt_window(appt, action, par);


  return appt;
}
