/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */


#include <args.h>
#include <comms_gui.h>
#include <config.h>
#include <conversions.h>
#include <core_gui.h>
#include <defines.h>
#include <debugging.h>
#include <dispatcher.h>
#include <enums.h>
#include <errno.h>
#include <gdk/gdkgl.h>
#include <getfiles.h>
#include <glade/glade.h>
#include <gui_handlers.h>
#include <init.h>
#include <locale.h>
#include <locking.h>
#include <main.h>
#include <serialio.h>
#include <stringmatch.h>
#include <threads.h>
#include <timeout_handlers.h>
#include <xmlcomm.h>

GThread * thread_dispatcher_id = NULL;
gboolean ready = FALSE;
gint pf_dispatcher_id = -1;
gint gui_dispatcher_id = -1;
gboolean gl_ability = FALSE;
Serial_Params *serial_params = NULL;
GAsyncQueue *io_data_queue = NULL;
GAsyncQueue *slave_msg_queue = NULL;
GAsyncQueue *pf_dispatch_queue = NULL;
GAsyncQueue *gui_dispatch_queue = NULL;
GCond *statuscounts_cond = NULL;
GCond *gui_dispatch_cond = NULL;
GCond *pf_dispatch_cond = NULL;
GObject *global_data = NULL;

/*!
 \brief main() is the typical main function in a C program, it performs
 all core initialization, loading of all main parameters, initializing handlers
 and entering gtk_main to process events until program close
 \param argc (gint) count of command line arguments
 \param argv (char **) array of command line args
 \returns TRUE
 */
gint main(gint argc, gchar ** argv)
{
	gchar * filename = NULL;

	setlocale(LC_ALL,"");
#ifdef __WIN32__
	bindtextdomain(PACKAGE, "C:\\Program Files\\MegaTunix\\dist\\locale");
#else
	bindtextdomain(PACKAGE, LOCALEDIR);
#endif
	textdomain (PACKAGE);

	if(!g_thread_supported())
		g_thread_init(NULL);
	gdk_threads_init();
	
	statuscounts_cond = g_cond_new();
	gui_dispatch_cond = g_cond_new();
	pf_dispatch_cond = g_cond_new();
	gdk_threads_enter();
	gtk_init(&argc, &argv);
	glade_init();

	gl_ability = gdk_gl_init_check(&argc, &argv);

	/* For testing if gettext works
	printf(_("Hello World!\n"));
	*/

	global_data = g_object_new(GTK_TYPE_INVISIBLE,NULL);
	g_object_ref(global_data);
	gtk_object_sink(GTK_OBJECT(global_data));
	handle_args(argc,argv);

	/* This will exit mtx if the locking fails! */
	create_mtx_lock();

	/* Allocate memory  */
	serial_params = g_malloc0(sizeof(Serial_Params));

	open_debug();		/* Open debug log */
	init();			/* Initialize global vars */
	make_megasquirt_dirs();	/* Create config file dirs if missing */
	/* Build table of strings to enum values */
	build_string_2_enum_table();

	filename = get_file(g_build_filename(INTERROGATOR_DATA_DIR,"comm.xml",NULL),NULL);
	load_comm_xml(filename);
	g_free(filename);

	/* Create Queue to listen for commands */
	io_data_queue = g_async_queue_new();
	slave_msg_queue = g_async_queue_new();
	pf_dispatch_queue = g_async_queue_new();
	gui_dispatch_queue = g_async_queue_new();

	read_config();
	setup_gui();		

	gtk_rc_parse_string("style \"override\"\n{\n\tGtkTreeView::horizontal-separator = 0\n\tGtkTreeView::vertical-separator = 0\n}\nwidget_class \"*\" style \"override\"");
	/* Startup the serial General I/O handler.... */
	thread_dispatcher_id = g_thread_create(thread_dispatcher,
			NULL, /* Thread args */
			TRUE, /* Joinable */
			NULL); /*GError Pointer */

	pf_dispatcher_id = g_timeout_add(20,(GSourceFunc)pf_dispatcher,NULL);
	gui_dispatcher_id = g_timeout_add(35,(GSourceFunc)gui_dispatcher,NULL);

	/* Kickoff fast interrogation */
	gdk_threads_add_timeout(500,(GSourceFunc)early_interrogation,NULL);

	ready = TRUE;
	gtk_main();
	gdk_threads_leave();
	return (0) ;
}
