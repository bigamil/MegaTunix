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

#include <comms_gui.h>
#include <config.h>
#include <dataio.h>
#include <defines.h>
#include <debugging.h>
#include <errno.h>
#include <fcntl.h>
#include <notifications.h>
#include <runtime_gui.h>
#include <serialio.h>
#include <string.h>
#include <structures.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <termios.h>
#include <threads.h>
#include <unistd.h>


extern gboolean raw_reader_running;
extern GtkWidget *comms_view;
extern struct DynamicMisc misc;
struct Serial_Params *serial_params;
gboolean connected;
       
void open_serial(gchar * port_name)
{
	/* We are using DOS/Win32 style com port numbers instead of unix
	 * style as its easier to think of COM1 instead of /dev/ttyS0
	 * thus com1=/dev/ttyS0, com2=/dev/ttyS1 and so on 
	 */
	gint fd = -1;
	gchar *device = NULL;	/* temporary unix name of the serial port */
	gchar * err_text = NULL;
	serial_params->port_name = g_strdup(port_name); 

	device = g_strdup(port_name);
	/* Open Read/Write and NOT as the controlling TTY in nonblock mode */
	//fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
	/* Blocking mode... */
	fd = open(device, O_RDWR | O_NOCTTY );
	if (fd > 0)
	{
		/* SUCCESS */
		/* NO Errors occurred opening the port */
		serial_params->open = TRUE;
		serial_params->fd = fd;
		/* Save serial port status */
		tcgetattr(serial_params->fd,&serial_params->oldtio);
		dbg_func(g_strdup_printf(__FILE__" open_serial(), %s Opened Successfully\n",device),SERIAL_GEN);
		update_logbar(comms_view,NULL,g_strdup_printf("%s Opened Successfully\n",device),TRUE,FALSE);
	}
	else
	{
		/* FAILURE */
		/* An Error occurred opening the port */
		serial_params->open = FALSE;
		err_text = (gchar *)g_strerror(errno);
		dbg_func(g_strdup_printf(__FILE__": open_serial(),\n\tError Opening \"%s\", Error Code: \"%s\"\n",device,err_text),CRITICAL);

		update_logbar(comms_view,"warning",g_strdup_printf("Error Opening %s Error Code: %s \n",device,err_text),TRUE,FALSE);
		g_free(err_text);
	}

	g_free(device);
	return;
}
	
void setup_serial_params()
{
	if (serial_params->open == FALSE)
		return;
	/* Sets up serial port for the modes we want to use. 
	 * NOTE: Original serial tio params are stored and restored 
	 * in the open_serial() and close_serial() functions.
	 */ 

	/*clear struct for new settings*/
	memset(&serial_params->newtio, 0, sizeof(serial_params->newtio)); 
	/* 
	 * BAUDRATE: Set bps rate. You could also use cfsetispeed and 
	 * cfsetospeed
	 * CRTSCTS : output hardware flow control (only used if the cable has
	 * all necessary lines. See sect. 7 of Serial-HOWTO)
	 * CS8     : 8n1 (8bit,no parity,1 stopbit)
	 * CLOCAL  : local connection, no modem contol
	 * CREAD   : enable receiving characters
	 */

	/* Set baud (posix way) */
	cfsetispeed(&serial_params->newtio, BAUDRATE);
	cfsetospeed(&serial_params->newtio, BAUDRATE);

	/* Set additional flags, note |= syntax.. */
	serial_params->newtio.c_cflag |= CLOCAL | CREAD;
	/* Mask and set to 8N1 mode... */
	serial_params->newtio.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
	serial_params->newtio.c_cflag |= CS8;

	/* RAW Input */
	serial_params->newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* Disable software flow control */
	serial_params->newtio.c_iflag &= ~(IXON | IXOFF );

	/* Set raw output */
	serial_params->newtio.c_oflag &= ~OPOST;

	/* 
	   initialize all control characters 
	   default values can be found in /usr/include/termios.h, and are given
	   in the comments, but we don't need them here
	 */
	serial_params->newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
	serial_params->newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	serial_params->newtio.c_cc[VERASE]   = 0;     /* del */
	serial_params->newtio.c_cc[VKILL]    = 0;     /* @ */
	serial_params->newtio.c_cc[VEOF]     = 0;     /* Ctrl-d */
	serial_params->newtio.c_cc[VEOL]     = 0;     /* '\0' */
	//serial_params->newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	serial_params->newtio.c_cc[VMIN]     = 0;     /* blocking read until 1 character arrives */
	serial_params->newtio.c_cc[VTIME]    = 5;     /* inter-character timer unused */

	tcsetattr(serial_params->fd,TCSAFLUSH,&serial_params->newtio);

	return;
}

void close_serial()
{
	if (serial_params->open == FALSE)
		return;
	tcsetattr(serial_params->fd,TCSAFLUSH,&serial_params->oldtio);
	close(serial_params->fd);
	serial_params->fd = -1;
	serial_params->open = FALSE;
	connected = FALSE;
	gtk_widget_set_sensitive(misc.status[STAT_CONNECTED],
			connected);
	//	gtk_widget_set_sensitive(misc.ww_status[STAT_CONNECTED],
	//			connected);

	/* An Closing the comm port */
	dbg_func(__FILE__": close_serial(), COM Port Closed\n",SERIAL_GEN);
	update_logbar(comms_view,NULL,g_strdup_printf("COM Port Closed\n"),TRUE,FALSE);
}

void set_ms_page(gint ms_page)
{
	gint res = 0;
	gchar buf;
	gchar *tmpbuf = NULL;

	if ((ms_page > 1) || (ms_page < 0))
	{
		tmpbuf = g_strdup_printf(__FILE__": page choice %i is out of range(0,1)\n",ms_page);
		dbg_func(tmpbuf,CRITICAL);
		g_free(tmpbuf);
	}

	buf = ms_page & 0x01;
	res = write(serial_params->fd,"P",1);
	if (res != 1)
		dbg_func(__FILE__": FAILURE sending \"P\" (change page) command to ECU \n",CRITICAL);
	res = write(serial_params->fd,&buf,1);
	if (res != 1)
	{
		tmpbuf = g_strdup_printf(__FILE__": FAILURE changing page on ECU to %i\n",ms_page);
		dbg_func(tmpbuf,CRITICAL);
		g_free(tmpbuf);
	}
}
