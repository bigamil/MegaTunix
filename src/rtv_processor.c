/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * Most of this file contributed by Perry Harrington
 * slight changes applied (naming, addition ofbspot 1-3 vars)
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#include <assert.h>
#include <config.h>
#include <configfile.h>
#include <defines.h>
#include <debugging.h>
#include <dep_processor.h>
#include <enums.h>
#include <glade/glade.h>
#include <lookuptables.h>
#include "../mtxmatheval/mtxmatheval.h"
#include <rtv_processor.h>
#include <stdlib.h>
#include <structures.h>


/*!
 \brief process_rt_vars() processes incoming realtime variables. It's a pretty
 complex function so read the sourcecode.. ;)
 \param incoming (void *) pointer to the raw incoming data
 */
void process_rt_vars(void *incoming)
{
	extern struct Rtv_Map *rtv_map;
	extern struct Firmware_Details *firmware;
	extern gint temp_units;
	guchar *raw_realtime = incoming;
	GObject * object = NULL;
	gchar * expr = NULL;
	gint num_raw =  0;
	GList * list= NULL;
	gint i = 0;
	gint j = 0;
	gfloat x = 0;
	gint offset = 0;
	gfloat result = 0.0;
	gfloat tmpf = 0.0;
	gboolean temp_dep = FALSE;
	void *evaluator = NULL;
	GTimeVal timeval;
	gint ts_position;
	gint hist_position;
	gint hist_max;
	gfloat *history = NULL;
	gchar *special = NULL;

	num_raw = firmware->rtvars_size;
	if (num_raw != rtv_map->raw_total)
	{
		dbg_func(g_strdup_printf(__FILE__": process_rt_vars()\n\tlength of buffer(%i) and realtime map raw_length(%i)\n\tDO NOT match, critical ERROR!\n",num_raw,rtv_map->raw_total),CRITICAL);
		return;
	}
	/* Store timestamps in ringbuffer */
	ts_position = rtv_map->ts_position;
	timeval = rtv_map->ts_array[ts_position];
	g_get_current_time(&timeval);
	rtv_map->ts_array[ts_position] = timeval;
	ts_position++;
	/* wrap around.. */
	if (ts_position >= rtv_map->ts_max)
		ts_position = 0;
	rtv_map->ts_position = ts_position;
	
	
	for (i=0;i<num_raw;i++)
	{
		/* Get list of derived vars for raw offset "i" */
		list = g_array_index(rtv_map->rtv_array,GList *,i);
		if (list == NULL) /* no derived vars for this variable */
			continue;
		list = g_list_first(list);
		for (j=0;j<g_list_length(list);j++)
		{
			history = NULL;
			special = NULL;
			object=(GObject *)g_list_nth_data(list,j);
			if (!object)
			{
				dbg_func(g_strdup_printf(__FILE__": rtv_processor()\n\t Object bound to list at offset %i is invalid!!!!\n",i),CRITICAL);
				continue;
			}
			temp_dep = (gboolean)g_object_get_data(object,"temp_dep");
			special = (gchar *)g_object_get_data(object,"special");
			if (special)
			{
				tmpf = handle_special(object,special);
				goto store_it;
			}
			evaluator = (void *)g_object_get_data(object,"ul_evaluator");
			if (!evaluator)
			{
				expr = g_object_get_data(object,"ul_conv_expr");
				if (expr == NULL)
				{
					dbg_func(g_strdup_printf(__FILE__": process_rt_vars()\n\t \"ul_conv_expr\" was NULL for control \"%s\", EXITING!\n",(gchar *)g_object_get_data(object,"internal_name")),CRITICAL);
					exit (-3);
				}
				evaluator = evaluator_create(expr);
				if (!evaluator)
					dbg_func(g_strdup_printf(__FILE__": rtv_processor()\n\t Creating of evaluator for function \"%s\" FAILED!!!\n\n",expr),CRITICAL);
				assert(evaluator);
				g_object_set_data(object,"ul_evaluator",evaluator);
			}
			else
				assert(evaluator);
			offset = (gint)g_object_get_data(object,"offset");
			if (g_object_get_data(object,"complex_expr"))
			{
				tmpf = handle_complex_expr(object,incoming,UPLOAD);
				goto store_it;
			}

			if (g_object_get_data(object,"lookuptable"))
			{
				dbg_func(g_strdup_printf(__FILE__": process_rt_vars()\n\tgetting Lookuptable for var using offset %i\n",offset),COMPLEX_EXPR);
				x = lookup_data(object,raw_realtime[offset]);
			}
			else
			{
				dbg_func(g_strdup_printf(__FILE__": process_rt_vars()\n\tNo Lookuptable needed for var using offset %i\n",offset),COMPLEX_EXPR);
				x = raw_realtime[offset];
			}


			dbg_func(g_strdup_printf(__FILE__": process_rt_vars()\n\texpression is %s\n",evaluator_get_string(evaluator)),COMPLEX_EXPR);
			tmpf = evaluator_evaluate_x(evaluator,x);
store_it:
			if (temp_dep)
			{
				if (temp_units == CELSIUS)
					result = (tmpf-32)*(5.0/9.0);
				else
					result = tmpf;
			}
			else
				result = tmpf;
			history = (gfloat *)g_object_get_data(object,"history");
			hist_position = (gint)g_object_get_data(object,"hist_position");
			hist_max = (gint)g_object_get_data(object,"hist_max");
			/* Store data in ringbuffer */
			history[hist_position] = result;
			g_object_set_data(object,"last_entry",GINT_TO_POINTER(hist_position));
			hist_position++;
			/* wrap around.. */
			if (hist_position >= hist_max)
				hist_position = 0;
			g_object_set_data(object,"hist_position",GINT_TO_POINTER(hist_position));


			//printf("Result of %s is %f\n",(gchar *)g_object_get_data(object,"internal_name"),result);

		}
	}
	return;
}


/*!
 \brief handle_complex_expr() handles a complex mathematcial expression for
 an variable represented by a GObject.
 \param object (GObject *) pointer to the object containing the conversion 
 expression and other relevant data
 \param incoming (void *) pointer to the raw data
 \param type (ConvType) enumeration stating if this is an upload or 
 download conversion
 \returns a float of the result of the mathematical expression
 */
gfloat handle_complex_expr(GObject *object, void * incoming,ConvType type)
{
	extern gint **ms_data;
	gchar **symbols = NULL;
	gchar **expr_types = NULL;
	guchar *raw_data = incoming;
	gint total_symbols = 0;
	gint i = 0;
	gint page = 0;
	gint offset = 0;
	gint bitmask = 0;
	gint bitshift = 0;
	void * evaluator = NULL;
	gchar **names = NULL;
	gdouble * values = NULL;
	gchar * tmpbuf = NULL;
	gdouble result = 0.0;


	symbols = (gchar **)g_object_get_data(object,"expr_symbols");
	expr_types = (gchar **)g_object_get_data(object,"expr_types");
	total_symbols = (gint)g_object_get_data(object,"total_symbols");

	names = g_malloc0(total_symbols*sizeof(gchar *));
	values = g_malloc0(total_symbols*sizeof(gdouble));

	for (i=0;i<total_symbols;i++)
	{
		page = 0;
		offset = 0;
		bitmask = 0;
		bitshift = 0;
		switch ((ComplexExprType)expr_types[i])
		{
			case VE_EMB_BIT:
				tmpbuf = g_strdup_printf("%s_page",symbols[i]);
				page = (gint) g_object_get_data(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_offset",symbols[i]);
				offset = (gint) g_object_get_data(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_bitmask",symbols[i]);
				bitmask = (gint) g_object_get_data(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_bitshift",symbols[i]);
				bitshift = (gint) g_object_get_data(object,tmpbuf);
				g_free(tmpbuf);
				names[i]=g_strdup(symbols[i]);
				values[i]=(gdouble)(((ms_data[page][offset])&bitmask) >> bitshift);
				dbg_func(g_strdup_printf(__FILE__": handle_complex_expr()\n\t Embedded bit, name: %s, value %f\n",names[i],values[i]),COMPLEX_EXPR);
				break;
			case VE_VAR:
				tmpbuf = g_strdup_printf("%s_page",symbols[i]);
				page = (gint) g_object_get_data(object,tmpbuf);
				g_free(tmpbuf);
				tmpbuf = g_strdup_printf("%s_offset",symbols[i]);
				offset = (gint) g_object_get_data(object,tmpbuf);
				g_free(tmpbuf);
				names[i]=g_strdup(symbols[i]);
				values[i]=(gdouble)ms_data[page][offset];
				dbg_func(g_strdup_printf(__FILE__": handle_complex_expr()\n\t VE Variable, name: %s, value %f\n",names[i],values[i]),COMPLEX_EXPR);
				break;
			case RAW_VAR:
				tmpbuf = g_strdup_printf("%s_offset",symbols[i]);
				offset = (gint) g_object_get_data(object,tmpbuf);
				g_free(tmpbuf);
				names[i]=g_strdup(symbols[i]);
				values[i]=(gdouble)raw_data[offset];
				dbg_func(g_strdup_printf(__FILE__": handle_complex_expr()\n\t RAW Variable, name: %s, value %f\n",names[i],values[i]),COMPLEX_EXPR);
		}

	}
	if (type == UPLOAD)
	{
		evaluator = (void *)g_object_get_data(object,"ul_evaluator");
		if (!evaluator)
			evaluator = evaluator_create(g_object_get_data(object,"ul_conv_expr"));
		g_object_set_data(object,"ul_evaluator",evaluator);
	}
	else if (type == DOWNLOAD)
	{
		evaluator = (void *)g_object_get_data(object,"dl_evaluator");
		if (!evaluator)
			evaluator = evaluator_create(g_object_get_data(object,"dl_conv_expr"));
		g_object_set_data(object,"dl_evaluator",evaluator);
	}
	else
		dbg_func(g_strdup_printf(__FILE__": handle_complex_expr()\n\tevaluator type undefined for %s\n",(gchar *)glade_get_widget_name(GTK_WIDGET(object))),CRITICAL);
	if (!evaluator)
	{
		dbg_func(g_strdup_printf(__FILE__": handle_complex_expr()\n\tevaluator missing for %s\n",(gchar *)glade_get_widget_name(GTK_WIDGET(object))),CRITICAL);
		exit (-1);
	}

	assert(evaluator);
	result = evaluator_evaluate(evaluator,total_symbols,names,values);
	for (i=0;i<total_symbols;i++)
	{
		dbg_func(g_strdup_printf(__FILE__": handle_complex_expr()\n\tkey %s value %f\n",names[i],values[i]),COMPLEX_EXPR);
		g_free(names[i]);
	}
	dbg_func(g_strdup_printf(__FILE__": handle_complex_expr()\n\texpression is %s\n",evaluator_get_string(evaluator)),COMPLEX_EXPR);
	g_free(names);
	g_free(values);
	return result;
}


/*!
 \brief handle_special() is used to handle special derived variables that
 DO NOT use any data fromthe realtime variables.  In this case it's only to
 create the high resoluation clock variable.
 \param object (GObject *) object representing this derived variable
 \param handler_name (gchar *) string name of special handler case to be done
 */
gfloat handle_special(GObject *object,gchar *handler_name)
{
	static GTimeVal now;
	static GTimeVal last;
	static gfloat cumu = 0.0;
	extern gboolean begin;

	if (g_strcasecmp(handler_name,"hr_clock")==0)
	{
		if (begin == TRUE)
		{       
			g_get_current_time(&now);
			last.tv_sec = now.tv_sec;
			last.tv_usec = now.tv_usec;
			begin = FALSE;
			return 0.0;
		}
		else
		{
			g_get_current_time(&now);
			cumu += (now.tv_sec-last.tv_sec)+
				((double)(now.tv_usec-last.tv_usec)/1000000.0);
			last.tv_sec = now.tv_sec;
			last.tv_usec = now.tv_usec;
			return cumu;
		}

	}
	else
		dbg_func(g_strdup_printf(__FILE__": handle_special()\n\t handler name is not recognized, \"%s\"\n",handler_name),CRITICAL);
	return 0.0;
}


/*!
 \brief lookup_current_value() gets the current value of the derived
 variable requested by name.
 \param internal_name (gchar *) name of the variable to get the data for.
 \param value (gflaot *) where to put the value
 \returns TRUE on successful lookup, FALSE on failure
 */
gboolean lookup_current_value(gchar *internal_name, gfloat *value)
{
	extern struct Rtv_Map *rtv_map;
	GObject * object = NULL;
	gfloat * history = NULL;
	gint last_entry = 0;
	
	object = g_hash_table_lookup(rtv_map->rtv_hash,internal_name);
	if (!object)
		return FALSE;
	history = (gfloat *)g_object_get_data(object,"history");
	last_entry = (gint)g_object_get_data(object,"last_entry");
	*value = history[last_entry];
	return TRUE;
}
