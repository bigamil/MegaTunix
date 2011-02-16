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

#ifndef __FREEMS_COMMS_H__
#define __FEEEMS_COMMS_H__

#include <gtk/gtk.h>
#include <defines.h>
#include <enums.h>


/* Prototypes */
void *win32_reader(gpointer);
void *unix_reader(gpointer);
void freeems_serial_enable(void);
void freeems_serial_disable(void);
gboolean comms_test(void);
gboolean setup_rtv(void);
void signal_read_rtvars(void);
void *rtv_subscriber(gpointer);
void *serial_repair_thread(gpointer);
void send_to_ecu(gpointer, gint, gboolean);
void freeems_send_to_ecu(gint, gint, DataSize, gint, gboolean);
void send_to_ecu(gpointer, gint, gboolean);
/* Prototypes */

#endif
