/*
 * Copyright (C) 2002-2011 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute, etc. this as long as all the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#ifndef __MS2_MENU_HANDLERS_H__
#define __MS2_MENU_HANDLERS_H__

#include <defines.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <threads.h>

/* Prototypes */
void ecu_plugin_menu_setup(GladeXML *);
gboolean show_tps_calibrator_window(GtkWidget *, gpointer);
gboolean show_ms2_afr_calibrator_window(GtkWidget *, gpointer);
gboolean show_ms2_therm_table_generator_window(GtkWidget *, gpointer);
gboolean show_sensor_calibration_help(GtkWidget *, gpointer);
gboolean show_sensor_calibrator_window(GtkWidget *, gpointer);
gboolean show_battery_calibrator_window(GtkWidget *, gpointer);
gboolean ms2_reinit(GtkWidget *, gpointer);
gboolean ms2_reboot(GtkWidget *, gpointer);
gboolean therm_set_state(gpointer, GtkWidget *);
/* Prototypes */

#endif
