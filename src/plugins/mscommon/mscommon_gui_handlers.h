/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
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

#ifndef __MSCOMMON_GUI_HANDLERS_H__
#define __MSCOMMON_GUI_HANDLERS_H__

#include <configfile.h>
#include <enums.h>
#include <gtk/gtk.h>

/* Externs */
extern gboolean (*get_symbol_f)(const gchar *, void **);
extern void (*thread_update_logbar_f)(const gchar *, const gchar *, gchar *, gboolean, gboolean);
extern GtkWidget *(*lookup_widget_f)(const gchar *);
extern gboolean (*set_file_api_f)(ConfigFile *, gint , gint );
extern gboolean (*get_file_api_f)(ConfigFile *, gint *, gint *);
extern void (*stop_tickler_f)(gint);
extern void (*start_tickler_f)(gint);
extern gchar **(*parse_keys_f)(const gchar *, gint *, const gchar * );
extern void(*set_title_f)(const gchar *);
extern glong (*get_extreme_from_size_f)(DataSize, Extreme);
extern gfloat (*convert_after_upload_f)(GtkWidget *);
extern gint (*convert_before_download_f)(GtkWidget *, gfloat);
extern gboolean (*std_entry_handler_f)(GtkWidget *, gpointer);
extern gboolean (*entry_changed_handler_f)(GtkWidget *, gpointer);
extern GdkColor (*get_colors_from_hue_f)(gfloat, gfloat, gfloat);
extern guint (*get_bitshift_f)(guint);
extern GList *(*get_list_f)(gchar *);
void (*update_widget_f)(gpointer, gpointer);
extern gboolean (*lookup_current_value_f)(const gchar *, gfloat *);
extern gboolean (*search_model_f)(GtkTreeModel *, GtkWidget *, GtkTreeIter *);
extern gint (*get_choice_count_f)(GtkTreeModel *);
extern void (*set_reqfuel_color_f)(GuiColor, gint);
/* Externs */

/* Prototypes */
gboolean common_entry_handler(GtkWidget *, gpointer);
gboolean common_bitmask_button_handler(GtkWidget *, gpointer);
gboolean common_slider_handler(GtkWidget *, gpointer);
gboolean common_button_handler(GtkWidget *, gpointer);
gboolean common_combo_handler(GtkWidget *, gpointer);

gboolean common_spin_handler(GtkWidget *, gpointer);
void set_widget_labels(const gchar *);
void swap_labels(const gchar *, gboolean);
void switch_labels(gpointer, gpointer);
gboolean force_update_table(gpointer);
gboolean trigger_group_update(gpointer);
gboolean update_multi_expression(gpointer);
/* Prototypes */


#endif
