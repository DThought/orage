/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

GtkWidget*
create_XFCalendar (void)
{
  GtkWidget *XFCalendar;
  GtkWidget *vbox1;
  GtkWidget *menubar1;
  GtkWidget *menuitem4;
  GtkWidget *menuitem4_menu;
  GtkWidget *quit1;
  GtkWidget *menuitem7;
  GtkWidget *menuitem7_menu;
  GtkWidget *about1;
  GtkWidget *calendar1;
  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();

  XFCalendar = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (XFCalendar), _("XFCalendar"));
  gtk_window_set_position (GTK_WINDOW (XFCalendar), GTK_WIN_POS_MOUSE);
  gtk_window_set_resizable (GTK_WINDOW (XFCalendar), FALSE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (XFCalendar), TRUE);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (XFCalendar), vbox1);

  menubar1 = gtk_menu_bar_new ();
  gtk_widget_show (menubar1);
  gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

  menuitem4 = gtk_menu_item_new_with_mnemonic (_("_File"));
  gtk_widget_show (menuitem4);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem4);

  menuitem4_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem4), menuitem4_menu);

  quit1 = gtk_image_menu_item_new_from_stock ("gtk-quit", accel_group);
  gtk_widget_show (quit1);
  gtk_container_add (GTK_CONTAINER (menuitem4_menu), quit1);

  menuitem7 = gtk_menu_item_new_with_mnemonic (_("_Help"));
  gtk_widget_show (menuitem7);
  gtk_container_add (GTK_CONTAINER (menubar1), menuitem7);

  menuitem7_menu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem7), menuitem7_menu);

  about1 = gtk_menu_item_new_with_mnemonic (_("_About"));
  gtk_widget_show (about1);
  gtk_container_add (GTK_CONTAINER (menuitem7_menu), about1);

  calendar1 = gtk_calendar_new ();
  gtk_widget_show (calendar1);
  gtk_box_pack_start (GTK_BOX (vbox1), calendar1, TRUE, TRUE, 0);
  gtk_calendar_display_options (GTK_CALENDAR (calendar1),
                                GTK_CALENDAR_SHOW_HEADING
                                | GTK_CALENDAR_SHOW_DAY_NAMES
                                | GTK_CALENDAR_SHOW_WEEK_NUMBERS
                                | GTK_CALENDAR_WEEK_START_MONDAY);

  g_signal_connect ((gpointer) XFCalendar, "delete_event",
                    G_CALLBACK (on_XFCalendar_delete_event),
                    NULL);
  g_signal_connect ((gpointer) quit1, "activate",
                    G_CALLBACK (on_quit1_activate),
                    NULL);
  g_signal_connect ((gpointer) about1, "activate",
                    G_CALLBACK (on_about1_activate),
                    NULL);
  g_signal_connect ((gpointer) calendar1, "day_selected_double_click",
                    G_CALLBACK (on_calendar1_day_selected_double_click),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (XFCalendar, XFCalendar, "XFCalendar");
  GLADE_HOOKUP_OBJECT (XFCalendar, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT (XFCalendar, menubar1, "menubar1");
  GLADE_HOOKUP_OBJECT (XFCalendar, menuitem4, "menuitem4");
  GLADE_HOOKUP_OBJECT (XFCalendar, menuitem4_menu, "menuitem4_menu");
  GLADE_HOOKUP_OBJECT (XFCalendar, quit1, "quit1");
  GLADE_HOOKUP_OBJECT (XFCalendar, menuitem7, "menuitem7");
  GLADE_HOOKUP_OBJECT (XFCalendar, menuitem7_menu, "menuitem7_menu");
  GLADE_HOOKUP_OBJECT (XFCalendar, about1, "about1");
  GLADE_HOOKUP_OBJECT (XFCalendar, calendar1, "calendar1");

  gtk_window_add_accel_group (GTK_WINDOW (XFCalendar), accel_group);

  return XFCalendar;
}

GtkWidget*
create_wAppointment (void)
{
  GtkWidget *wAppointment;
  GtkWidget *vbox2;
  GtkWidget *handlebox1;
  GtkWidget *toolbar1;
  GtkWidget *tmp_toolbar_icon;
  GtkWidget *btSave;
  GtkWidget *btClose;
  GtkWidget *vseparator1;
  GtkWidget *btDelete;
  GtkWidget *scrolledwindow1;
  GtkWidget *textview1;

  wAppointment = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (wAppointment, 300, 250);
  gtk_window_set_title (GTK_WINDOW (wAppointment), _("Appointment"));
  gtk_window_set_position (GTK_WINDOW (wAppointment), GTK_WIN_POS_MOUSE);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (wAppointment), TRUE);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (wAppointment), vbox2);

  handlebox1 = gtk_handle_box_new ();
  gtk_widget_show (handlebox1);
  gtk_box_pack_start (GTK_BOX (vbox2), handlebox1, FALSE, FALSE, 0);

  toolbar1 = gtk_toolbar_new ();
  gtk_widget_show (toolbar1);
  gtk_container_add (GTK_CONTAINER (handlebox1), toolbar1);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar1), GTK_TOOLBAR_ICONS);

  tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-save", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1)));
  btSave = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("btSave"),
                                _("Save"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_label_set_use_underline (GTK_LABEL (((GtkToolbarChild*) (g_list_last (GTK_TOOLBAR (toolbar1)->children)->data))->label), TRUE);
  gtk_widget_show (btSave);

  tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-close", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1)));
  btClose = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("btClose"),
                                _("Close"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_label_set_use_underline (GTK_LABEL (((GtkToolbarChild*) (g_list_last (GTK_TOOLBAR (toolbar1)->children)->data))->label), TRUE);
  gtk_widget_show (btClose);

  vseparator1 = gtk_vseparator_new ();
  gtk_widget_show (vseparator1);
  gtk_toolbar_append_widget (GTK_TOOLBAR (toolbar1), vseparator1, NULL, NULL);

  tmp_toolbar_icon = gtk_image_new_from_stock ("gtk-clear", gtk_toolbar_get_icon_size (GTK_TOOLBAR (toolbar1)));
  btDelete = gtk_toolbar_append_element (GTK_TOOLBAR (toolbar1),
                                GTK_TOOLBAR_CHILD_BUTTON,
                                NULL,
                                _("Clear"),
                                _("Clear"), NULL,
                                tmp_toolbar_icon, NULL, NULL);
  gtk_label_set_use_underline (GTK_LABEL (((GtkToolbarChild*) (g_list_last (GTK_TOOLBAR (toolbar1)->children)->data))->label), TRUE);
  gtk_widget_show (btDelete);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox2), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  textview1 = gtk_text_view_new ();
  gtk_widget_show (textview1);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), textview1);

  g_signal_connect ((gpointer) wAppointment, "delete_event",
                    G_CALLBACK (on_wAppointment_delete_event),
                    NULL);
  g_signal_connect ((gpointer) btSave, "clicked",
                    G_CALLBACK (on_btSave_clicked),
                    NULL);
  g_signal_connect ((gpointer) btClose, "clicked",
                    G_CALLBACK (on_btClose_clicked),
                    NULL);
  g_signal_connect ((gpointer) btDelete, "clicked",
                    G_CALLBACK (on_btDelete_clicked),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (wAppointment, wAppointment, "wAppointment");
  GLADE_HOOKUP_OBJECT (wAppointment, vbox2, "vbox2");
  GLADE_HOOKUP_OBJECT (wAppointment, handlebox1, "handlebox1");
  GLADE_HOOKUP_OBJECT (wAppointment, toolbar1, "toolbar1");
  GLADE_HOOKUP_OBJECT (wAppointment, btSave, "btSave");
  GLADE_HOOKUP_OBJECT (wAppointment, btClose, "btClose");
  GLADE_HOOKUP_OBJECT (wAppointment, vseparator1, "vseparator1");
  GLADE_HOOKUP_OBJECT (wAppointment, btDelete, "btDelete");
  GLADE_HOOKUP_OBJECT (wAppointment, scrolledwindow1, "scrolledwindow1");
  GLADE_HOOKUP_OBJECT (wAppointment, textview1, "textview1");

  return wAppointment;
}

GtkWidget*
create_wInfo (void)
{
  GtkWidget *wInfo;
  GtkWidget *dialog_vbox1;
  GtkWidget *lbAppName;
  GtkWidget *dialog_action_area1;
  GtkWidget *okbutton1;

  wInfo = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (wInfo), _("About..."));
  gtk_window_set_position (GTK_WINDOW (wInfo), GTK_WIN_POS_MOUSE);
  gtk_window_set_modal (GTK_WINDOW (wInfo), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (wInfo), FALSE);

  dialog_vbox1 = GTK_DIALOG (wInfo)->vbox;
  gtk_widget_show (dialog_vbox1);

  lbAppName = gtk_label_new (_("XFCalendar\nv. 0.1.1\n\nManage your time with XFce 4\n(c) 2003 Micka\303\253l Graf\n\nThis software is Released under the \nGeneral Public Licence.\n\nXFce 4 is (c) Olivier Fourdan"));
  gtk_widget_show (lbAppName);
  gtk_box_pack_start (GTK_BOX (dialog_vbox1), lbAppName, FALSE, FALSE, 0);

  dialog_action_area1 = GTK_DIALOG (wInfo)->action_area;
  gtk_widget_show (dialog_action_area1);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area1), GTK_BUTTONBOX_END);

  okbutton1 = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_show (okbutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (wInfo), okbutton1, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton1, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) wInfo, "delete_event",
                    G_CALLBACK (on_wInfo_delete_event),
                    NULL);
  g_signal_connect ((gpointer) okbutton1, "clicked",
                    G_CALLBACK (on_okbutton1_clicked),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (wInfo, wInfo, "wInfo");
  GLADE_HOOKUP_OBJECT_NO_REF (wInfo, dialog_vbox1, "dialog_vbox1");
  GLADE_HOOKUP_OBJECT (wInfo, lbAppName, "lbAppName");
  GLADE_HOOKUP_OBJECT_NO_REF (wInfo, dialog_action_area1, "dialog_action_area1");
  GLADE_HOOKUP_OBJECT (wInfo, okbutton1, "okbutton1");

  return wInfo;
}

GtkWidget*
create_wClearWarn (void)
{
  GtkWidget *wClearWarn;
  GtkWidget *dialog_vbox2;
  GtkWidget *hbox1;
  GtkWidget *image1;
  GtkWidget *lbClearWarn;
  GtkWidget *dialog_action_area2;
  GtkWidget *cancelbutton1;
  GtkWidget *okbutton2;

  wClearWarn = gtk_dialog_new ();
  gtk_widget_set_size_request (wClearWarn, 250, 120);
  gtk_window_set_title (GTK_WINDOW (wClearWarn), _("Warning"));
  gtk_window_set_position (GTK_WINDOW (wClearWarn), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (wClearWarn), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (wClearWarn), FALSE);

  dialog_vbox2 = GTK_DIALOG (wClearWarn)->vbox;
  gtk_widget_show (dialog_vbox2);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (dialog_vbox2), hbox1, TRUE, TRUE, 0);

  image1 = gtk_image_new_from_stock ("gtk-dialog-warning", GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image1);
  gtk_box_pack_start (GTK_BOX (hbox1), image1, TRUE, TRUE, 0);

  lbClearWarn = gtk_label_new (_("You will remove all information \nassociated with this date."));
  gtk_widget_show (lbClearWarn);
  gtk_box_pack_start (GTK_BOX (hbox1), lbClearWarn, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (lbClearWarn), GTK_JUSTIFY_LEFT);

  dialog_action_area2 = GTK_DIALOG (wClearWarn)->action_area;
  gtk_widget_show (dialog_action_area2);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog_action_area2), GTK_BUTTONBOX_END);

  cancelbutton1 = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_show (cancelbutton1);
  gtk_dialog_add_action_widget (GTK_DIALOG (wClearWarn), cancelbutton1, GTK_RESPONSE_CANCEL);
  GTK_WIDGET_SET_FLAGS (cancelbutton1, GTK_CAN_DEFAULT);

  okbutton2 = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_show (okbutton2);
  gtk_dialog_add_action_widget (GTK_DIALOG (wClearWarn), okbutton2, GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS (okbutton2, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) cancelbutton1, "clicked",
                    G_CALLBACK (on_cancelbutton1_clicked),
                    NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (wClearWarn, wClearWarn, "wClearWarn");
  GLADE_HOOKUP_OBJECT_NO_REF (wClearWarn, dialog_vbox2, "dialog_vbox2");
  GLADE_HOOKUP_OBJECT (wClearWarn, hbox1, "hbox1");
  GLADE_HOOKUP_OBJECT (wClearWarn, image1, "image1");
  GLADE_HOOKUP_OBJECT (wClearWarn, lbClearWarn, "lbClearWarn");
  GLADE_HOOKUP_OBJECT_NO_REF (wClearWarn, dialog_action_area2, "dialog_action_area2");
  GLADE_HOOKUP_OBJECT (wClearWarn, cancelbutton1, "cancelbutton1");
  GLADE_HOOKUP_OBJECT (wClearWarn, okbutton2, "okbutton2");

  return wClearWarn;
}

