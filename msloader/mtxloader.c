#include <config.h>
#include <configfile.h>
#include <defines.h>
#include <fcntl.h>
#include <getfiles.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

EXPORT void load_firmware (GtkButton*);
EXPORT void abort_load (GtkButton*);
EXPORT gboolean leave (GtkWidget *, gpointer);
EXPORT gboolean about_popup (GtkWidget *, gpointer);
void textbuffer_changed(GtkTextBuffer *,gpointer);
void load_defaults();
void save_defaults();

GIOChannel *child_stdout = NULL;
GIOChannel *child_stdin = NULL;
GladeXML *xml = NULL;
GtkWidget *main_window = NULL;
GPid pid;

int main (int argc, char *argv[])
{
	GtkWidget *textview = NULL;
	GtkTextBuffer * textbuffer = NULL;
	GtkTextIter iter;
	GtkTextMark *mark;
	GtkWidget *dialog = NULL;
	gchar * filename = NULL;
	gchar * fname = NULL;
	gtk_init (&argc, &argv);
	fname = g_build_filename(GUI_DATA_DIR,"mtxloader.glade",NULL);
	filename = get_file(g_strdup(fname),NULL);
	if (!filename)
	{
		printf("ERROR! Could NOT locate %s\n",fname);
		g_free(fname);
		dialog = gtk_message_dialog_new_with_markup(NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_CLOSE,"\n<b>MegaTunix/mtxloader</b> doesn't appear to be installed correctly!\n\nDid you forget to run <i>\"sudo make install\"</i> ??\n\n");

		g_signal_connect(G_OBJECT(dialog),"response", G_CALLBACK(gtk_main_quit), dialog);
		g_signal_connect(G_OBJECT(dialog),"delete_event", G_CALLBACK(gtk_main_quit), dialog);
		g_signal_connect(G_OBJECT(dialog),"destroy_event", G_CALLBACK(gtk_main_quit), dialog);
		gtk_widget_show_all(dialog);
		gtk_main();
		exit(-1);
	}

	g_free(fname);
	xml = glade_xml_new (filename, NULL, NULL);
	g_free(filename);
	main_window = glade_xml_get_widget (xml, "main_window");
	textview = glade_xml_get_widget (xml, "textview");
	textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	mark = gtk_text_mark_new("end",FALSE);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(textbuffer),&iter);
	gtk_text_buffer_add_mark(GTK_TEXT_BUFFER(textbuffer),mark,&iter);
	g_signal_connect(G_OBJECT(textbuffer),"changed",G_CALLBACK(textbuffer_changed),NULL);
	glade_xml_signal_autoconnect (xml);
	load_defaults();
	gtk_widget_show_all (main_window);
	gtk_main ();

	return 0;
}

/* Parse a line of output, which may actually be more than one line. To handle
 * multiple lines, the string is split with g_strsplit(). */
static void
parse_output (gchar *line)
{
	gchar **lines, **ptr;
	gchar * tmpbuf = NULL;
	GtkWidget *textview;
	GtkTextIter iter;
	GtkTextBuffer * textbuffer = NULL;
	GtkWidget *parent = NULL;
	GtkAdjustment * adj = NULL;


	/* Load the list store, split the string into lines and create a pointer. */
	textview = glade_xml_get_widget (xml, "textview");
	lines = g_strsplit (line, "\n", -1);
	ptr = lines;

	/* Loop through each of the lines, parsing its content. */
	textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));
	while (*ptr != NULL)
	{
		/* If this is an empty string, move to the next string. */
		if (g_ascii_strcasecmp (*ptr, "") == 0)
		{
			++ptr;
			continue;
		}
		gtk_text_buffer_get_end_iter (textbuffer, &iter);
		tmpbuf = g_strdup_printf("%s\n",*ptr);
		printf("%s",tmpbuf);
		gtk_text_buffer_insert(textbuffer,&iter,tmpbuf,-1);
		g_free(tmpbuf);
		parent = gtk_widget_get_parent(textview);
		if (parent != NULL)
		{
			adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(parent));
			adj->value = adj->upper;
		}
		++ptr;
	}

	g_strfreev (lines);
	while (gtk_events_pending())
		gtk_main_iteration();
}

/* This is a watch function that is called when the IO channel has data to read. */
static gboolean
read_output (GIOChannel *channel,
		         GIOCondition condition,
		         gpointer data)
{
	GError *error = NULL;
	GIOStatus status;
	gchar *line;
	gsize len, term;

	/* Read the current line of data from the IO channel. */
	status = g_io_channel_read_line (channel, &line, &len, &term, &error);

	/* If some type of error has occurred, handle it. */
	if (status != G_IO_STATUS_NORMAL || line == NULL || error != NULL) 
	{
		if (error) 
		{
			g_warning ("Error reading IO channel: %s", error->message);
			g_error_free (error);
		}

		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "abort"), FALSE);
		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "abort_button"), FALSE);
		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "load"), TRUE);
		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "load_button"), TRUE);

		if (channel != NULL)
			g_io_channel_shutdown (channel, TRUE, NULL);
		channel = NULL;

		return FALSE;
	}

	/* Parse the line if an error has not occurred. */
	parse_output (line);
	g_free (line);

	return TRUE;
}

/* Perform a ping operation when the Ping button is clicked. */
EXPORT void load_firmware (GtkButton *button)
{
	GtkWidget *port_entry = NULL;
	GtkWidget *file_button = NULL;
	gint argc, fin, fout, ret;
	const gchar *port;
	gchar *command = NULL;
	gchar *filename = NULL;
	gchar **argv = NULL;

	port_entry = glade_xml_get_widget (xml, "port_entry");
	file_button = glade_xml_get_widget (xml, "filechooser_button");
	port = gtk_entry_get_text(GTK_ENTRY(port_entry));
	filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_button));

	if (g_utf8_strlen(port, -1) == 0)
		return;
	if (filename == NULL)
		return;

	command = g_strdup_printf("msloader %s %s",port,filename);

	g_shell_parse_argv (command, &argc, &argv, NULL);
	ret = g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,&pid, NULL, &fout, NULL, NULL);

	g_strfreev (argv);
	g_free (command);

	if (!ret)
		g_warning ("The 'msloader' instruction has failed!");
	else
	{
		/* Disable the msloader button and enable the Abort button. */
		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "abort"), TRUE);
		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "abort_button"), TRUE);
		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "load"), FALSE);
		gtk_widget_set_sensitive (glade_xml_get_widget (xml, "load_button"), FALSE);

		/* Create a new IO channel and monitor it for data to read. */
		child_stdout = g_io_channel_unix_new (fout);
		child_stdin = g_io_channel_unix_new (fin);
		g_io_add_watch (child_stdout, G_IO_IN | G_IO_ERR | G_IO_HUP, read_output, NULL);
		g_io_channel_unref (child_stdout);
	}
}

/* Kill the current msloader process when the Stop button is pressed. */
EXPORT void abort_load (GtkButton *button)
{
  gtk_widget_set_sensitive (glade_xml_get_widget (xml, "abort"), FALSE);
  gtk_widget_set_sensitive (glade_xml_get_widget (xml, "abort_button"), FALSE);
  gtk_widget_set_sensitive (glade_xml_get_widget (xml, "load"), TRUE);
  gtk_widget_set_sensitive (glade_xml_get_widget (xml, "load_button"), TRUE);
  kill (pid, SIGINT);
}


EXPORT gboolean leave(GtkWidget * widget, gpointer data)
{
	save_defaults();
	gtk_main_quit();
	return FALSE;
}


/*!
 \brief about_popup makes the about tab and presents the MegaTunix logo
 */
EXPORT gboolean about_popup(GtkWidget *widget, gpointer data)
{
#if GTK_MINOR_VERSION >= 8
	if (gtk_minor_version >= 8)
	{
		gchar *authors[] = {"David J. Andruczyk",NULL};
		gchar *artists[] = {"Alan Barrow",NULL};
		gtk_show_about_dialog(NULL,
				"name","MegaTunix Firmware Loading Software",
				"version",VERSION,
				"copyright","David J. Andruczyk(2008)",
				"comments","MTXloader is a Graphical Firmware Loader designed to make it easy and (hopefully) intuitive to upgrade the firmware on your MS-I MegaSquirt powered vehicle.  Please send suggestions to the author for ways to improve mtxloader.",
				"license","GPL v2",
				"website","http://megatunix.sourceforge.net",
				"authors",authors,
				"artists",artists,
				"documenters",authors,
				NULL);
	}
#endif
	return TRUE;
}


void load_defaults()
{
	ConfigFile *cfgfile;
	gchar * filename = NULL;
	gchar * tmpbuf = NULL;
	filename = g_strconcat(HOME(), PSEP,".MegaTunix",PSEP,"config", NULL);
	cfgfile = cfg_open_file(filename);
	if (cfgfile)
	{
		if(cfg_read_string(cfgfile, "Serial", "override_port", &tmpbuf))
		{
			gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget (xml, "port_entry")),tmpbuf);
			g_free(tmpbuf);
		}
		if(cfg_read_string(cfgfile, "MTXLoader", "last_file", &tmpbuf))
		{
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(glade_xml_get_widget (xml, "filechooser_button")),tmpbuf);
			g_free(tmpbuf);
		}
		cfg_free(cfgfile);
		g_free(filename);
	}
}


void save_defaults()
{
	ConfigFile *cfgfile;
	gchar * filename = NULL;
	gchar * tmpbuf = NULL;
	filename = g_strconcat(HOME(), PSEP,".MegaTunix",PSEP,"config", NULL);
	cfgfile = cfg_open_file(filename);
	if (cfgfile)
	{
		tmpbuf = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(glade_xml_get_widget (xml, "filechooser_button")));
		if (tmpbuf)
			cfg_write_string(cfgfile, "MTXLoader", "last_file", tmpbuf);
		g_free(tmpbuf);
		cfg_write_file(cfgfile,filename);
		cfg_free(cfgfile);
		g_free(filename);
	}
}


void textbuffer_changed(GtkTextBuffer *buffer, gpointer data)
{
	GtkTextMark *insert, *end;
	GtkTextIter end_iter;
	GtkTextIter insert_iter;
	gchar * text = NULL;
	insert = gtk_text_buffer_get_insert (buffer);
	end = gtk_text_buffer_get_mark (buffer,"end");
	gtk_text_buffer_get_iter_at_mark(buffer,&insert_iter,insert);
	gtk_text_buffer_get_iter_at_mark(buffer,&end_iter,end);

	text = gtk_text_buffer_get_text(buffer,&insert_iter,&end_iter,TRUE);
	printf("insert text \"%s\"\n",text);
}
