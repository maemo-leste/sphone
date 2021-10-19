/**
 * @file datapipe.h
 * Sphone datapipe
 * @author Carl Klemm <carl@uvos.xyz>
 *
 * sphone is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * sphone is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with sphone.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _DATAPIPE_H_
#define _DATAPIPE_H_

#include <glib.h>

/**
 * Datapipe structure
 *
 * Only access this struct through the functions
 */
typedef struct {
	GSList *filters;		//< The filters 
	GSList *output_triggers;	//< Triggers
} datapipe_struct;

// Datapipe execution
gconstpointer execute_datapipe(datapipe_struct *const datapipe, gpointer indata);
void execute_datapipe_output_triggers(const datapipe_struct *const datapipe, gconstpointer indata);

// Filters 
void append_filter_to_datapipe(datapipe_struct *const datapipe, gpointer (*filter)(gpointer data));
void remove_filter_from_datapipe(datapipe_struct *const datapipe, gpointer (*filter)(gpointer data));

// triggers
void append_trigger_to_datapipe(datapipe_struct *const datapipe, void (*trigger)(gconstpointer data));
void remove_trigger_from_datapipe(datapipe_struct *const datapipe, void (*trigger)(gconstpointer data));

void setup_datapipe(datapipe_struct *const datapipe);
void free_datapipe(datapipe_struct *const datapipe);

#endif // _DATAPIPE_H_
