/*
 * Copyright (C) 2002-2012 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
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

/*!
  \file src/plugins/freeems/freeems_gui_handlers.h
  \ingroup FreeEMSPlugin,Headers
  \brief FreeEMS Gui handlers
  \author David Andruczyk
  */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FREEEMS_GUI_HANDLERS_H__
#define __FREEEMS_GUI_HANDLERS_H__

#include <defines.h>
#include <gui_handlers.h>
#include <gtk/gtk.h>

/* Enumerations */
typedef enum
{
	SOFT_BOOT_ECU = LAST_GLOBAL_STD_HANDLER_ENUM + 1,
	HARD_BOOT_ECU,
	BENCHTEST_START,
	BENCHTEST_STOP,
	BENCHTEST_BUMP,
        GENERIC,
	NOOP
}FreeEMSCommonStdHandler;


typedef enum
{
	FREEEMS_TOGGLE = LAST_GLOBAL_TOGGLE_HANDLER_ENUM + 1
}FreeEMSCommonToggleHandler;
/* Enumerations */


/* Prototypes */
gboolean common_std_button_handler(GtkWidget *, gpointer);
gboolean common_toggle_button_handler(GtkWidget *, gpointer);
gboolean common_bitmask_button_handler(GtkWidget *, gpointer);
gboolean common_entry_handler(GtkWidget *, gpointer);
gboolean common_slider_handler(GtkWidget *, gpointer);
gboolean common_combo_handler(GtkWidget *, gpointer);
gboolean common_spin_button_handler(GtkWidget *, gpointer);

void common_gui_init(void);
void update_ecu_controls_pf(void);
void update_combo(GtkWidget *);
void update_entry(GtkWidget *);
void update_checkbutton(GtkWidget *);
void update_widget(gpointer, gpointer);
void get_essential_bits(GtkWidget *, gint *, gint *, gint *, gint *, gint *);
void get_essentials(GtkWidget *, gint *, gint *, DataSize *, gint *);

/* Prototypes */

#endif
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
