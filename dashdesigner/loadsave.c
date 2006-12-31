
#include "../include/defines.h"
#include <events.h>
#include <loadsave.h>
#include <getfiles.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <xml.h>


EXPORT gboolean load_handler(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog = NULL;
#ifndef __WIN32__
	gchar * tmpbuf = NULL;
#endif
	gchar * p_dir = NULL;

	dialog = gtk_file_chooser_dialog_new ("Open File",
			NULL,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	p_dir = g_strconcat(HOME(),PSEP,".MegaTunix",PSEP,DASHES_DIR,NULL);
#ifndef __WIN32__
	/* System wide dir */
	tmpbuf = g_strconcat(DATA_DIR,PSEP,DASHES_DIR,NULL);
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog),tmpbuf,NULL);
	g_free(tmpbuf);
#else
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(dialog),p_dir);
#endif
	/* Personal dir */
	gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(dialog),p_dir,NULL);
	g_free(p_dir);
	setup_file_filters(GTK_FILE_CHOOSER(dialog));

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		import_dash_xml(filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
	return TRUE;
}


EXPORT gboolean save_as_handler(GtkWidget *widget, gpointer data)
{
	save_handler(widget,GINT_TO_POINTER(TRUE));
	return TRUE;
}

EXPORT gboolean save_handler(GtkWidget *widget, gpointer data)
{
	extern GladeXML *main_xml;
	GtkWidget *dash;
	GtkWidget *dialog = NULL;
	gchar * tmpbuf = NULL;
	gchar * filename = NULL;
	gchar *defdir = NULL;

	defdir = g_strconcat(HOME(),PSEP, ".MegaTunix",PSEP,DASHES_DIR, NULL);

	dialog = gtk_file_chooser_dialog_new ("Save File",
			NULL,
			GTK_FILE_CHOOSER_ACTION_SAVE,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
			NULL);
	setup_file_filters(GTK_FILE_CHOOSER(dialog));
#if GTK_MINOR_VERSION >= 8
	if (gtk_minor_version >= 8)
		gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
#endif

	dash = glade_xml_get_widget(main_xml,"dashboard");
	filename = g_object_get_data(G_OBJECT(dash),"dash_xml_filename");
	if ((filename != NULL) && ((gint)data == FALSE)) /* Saving PRE-existin*/
	{
		tmpbuf = g_strconcat(defdir,PSEP,filename,NULL);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog),tmpbuf);
		g_free(tmpbuf);
	}
	else	/* NEW Document (Save As) */
	{
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),defdir);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "NEW_DASH_NAME.xml");
	}
//	g_free(filename);
	g_free(defdir);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		gchar *tmp;
		tmp = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		export_dash_xml(tmp);
		g_free (tmp);
	}
	gtk_widget_destroy (dialog);
	return TRUE;

}

void setup_file_filters(GtkFileChooser *chooser)
{
	GtkFileFilter * filter = NULL;
	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(filter),"*.*");
	gtk_file_filter_set_name(GTK_FILE_FILTER(filter),"All Files");
	gtk_file_chooser_add_filter(chooser,filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(GTK_FILE_FILTER(filter),"*.xml");
	gtk_file_filter_set_name(GTK_FILE_FILTER(filter),"XML Files");
	gtk_file_chooser_add_filter(chooser,filter);
	gtk_file_chooser_set_filter(chooser,filter);
	return ;
}