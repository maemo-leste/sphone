/**
 * @file datapipe.c
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
#include <glib.h>
#include "datapipe.h"
#include "sphone-log.h"

/**
 * Execute the filters of a datapipe
 *
 * @param datapipe The datapipe to execute
 * @param indata The input data to run through the datapipe
 * @return The processed data
 */
static gconstpointer execute_datapipe_filters(datapipe_struct *const datapipe, gpointer indata)
{
	gpointer (*filter)(gpointer data, gpointer user_data);
	gpointer data = indata;

	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return NULL;
	}

	for (int i = 0; (filter = g_slist_nth_data(datapipe->filters, i)) != NULL; i++) {
		gpointer tmp = filter(data, g_slist_nth_data(datapipe->filters_user_data, i));

		data = tmp;
	}

	return data;
}

/**
 * Execute the output triggers of a datapipe
 *
 * @param datapipe The datapipe to execute
 * @param indata The input data to run through the datapipe
 */
void execute_datapipe_output_triggers(const datapipe_struct *const datapipe, gconstpointer indata)
{
	void (*trigger)(gconstpointer data, gpointer user_data);

	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return;
	}

	for (int i = 0; (trigger = g_slist_nth_data(datapipe->output_triggers, i)) != NULL; i++)
		trigger(indata, g_slist_nth_data(datapipe->triggers_user_data, i));
}

/**
 * Execute the datapipe
 *
 * @param datapipe The datapipe to execute
 * @param indata The input data to run through the datapipe
 * @param use_cache USE_CACHE to use data from cache,
 *                  USE_INDATA to use indata
 * @param cache_indata CACHE_INDATA to cache the indata,
 *                     DONT_CACHE_INDATA to keep the old data
 * @return The processed data
 */
gconstpointer execute_datapipe(datapipe_struct *const datapipe, gpointer indata)
{
	gconstpointer data = NULL;

	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return NULL;
	}

	data = execute_datapipe_filters(datapipe, indata);

	execute_datapipe_output_triggers(datapipe, data);

	datapipe->last_data = data;

	return data;
}


int datapipe_get_last_data_int(datapipe_struct *const datapipe)
{
	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return -1;
	}
	return GPOINTER_TO_INT(datapipe->last_data);
}

/**
 * Append a filter to an existing datapipe
 *
 * @param datapipe The datapipe to manipulate
 * @param filter The filter to add to the datapipe
 */
void append_filter_to_datapipe(datapipe_struct *const datapipe,
							   gpointer (*filter)(gpointer data, gpointer user_data),
							   gpointer user_data)
{
	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return;
	}

	if (filter == NULL) {
		sphone_log(LL_ERR, "%s called without a valid filter", __func__);
		return;
	}

	datapipe->filters = g_slist_append(datapipe->filters, filter);
	datapipe->filters_user_data = g_slist_append(datapipe->filters_user_data, user_data);
}

/**
 * Remove a filter from an existing datapipe
 * Non-existing filters are ignored
 *
 * @param datapipe The datapipe to manipulate
 * @param filter The filter to remove from the datapipe
 */
void remove_filter_from_datapipe(datapipe_struct *const datapipe, gpointer (*filter)(gpointer data, gpointer user_data))
{
	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return;
	}

	if (filter == NULL) {
		sphone_log(LL_ERR, "%s called without a valid filter", __func__);
		return;
	}

	gpointer (*ifilter)(gpointer data, gpointer user_data) = NULL;
	for (int i = 0; (ifilter = g_slist_nth_data(datapipe->filters, i)) != NULL; i++) {
		if(ifilter == filter) {
			datapipe->filters = g_slist_remove(datapipe->filters, filter);
			datapipe->filters_user_data = 
				g_slist_remove(datapipe->filters_user_data, g_slist_nth_data(datapipe->filters_user_data, i));
			break;
		}
	}

	/* Did we remove any entry? */
	if (!ifilter)
		sphone_log(LL_WARN, "Trying to remove non-existing filter");
}

/**
 * Append an output trigger to an existing datapipe
 *
 * @param datapipe The datapipe to manipulate
 * @param trigger The trigger to add to the datapipe
 */
void append_trigger_to_datapipe(datapipe_struct *const datapipe,
								void (*trigger)(gconstpointer data, gpointer user_data),
								gpointer user_data)
{
	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return;
	}

	if (trigger == NULL) {
		sphone_log(LL_ERR, "%s called without a valid trigger", __func__);
		return;
	}

	datapipe->output_triggers = g_slist_append(datapipe->output_triggers, trigger);
	datapipe->triggers_user_data = g_slist_append(datapipe->triggers_user_data, user_data);
}

/**
 * Remove an output trigger from an existing datapipe
 * Non-existing triggers are ignored
 *
 * @param datapipe The datapipe to manipulate
 * @param trigger The trigger to remove from the datapipe
 */
void remove_trigger_from_datapipe(datapipe_struct *const datapipe, void (*trigger)(gconstpointer data, gpointer user_data))
{
	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return;
	}

	if (trigger == NULL) {
		sphone_log(LL_ERR, "%s called without a valid trigger", __func__);
		return;
	}

	void (*itrigger)(gconstpointer data, gpointer user_data) = NULL;
	for (int i = 0; (itrigger = g_slist_nth_data(datapipe->output_triggers, i)) != NULL; i++) {
		if(itrigger == trigger) {
			datapipe->output_triggers = g_slist_remove(datapipe->output_triggers, trigger);
			datapipe->triggers_user_data = 
				g_slist_remove(datapipe->triggers_user_data, g_slist_nth_data(datapipe->triggers_user_data, i));
			break;
		}
	}

	/* Did we remove any entry? */
	if (!itrigger)
		sphone_log(LL_WARN, "Trying to remove non-existing trigger");
}

/**
 * Initialise a datapipe
 *
 * @param datapipe The datapipe to manipulate
 */
void setup_datapipe(datapipe_struct *const datapipe)
{
	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return;
	}

	datapipe->filters = NULL;
	datapipe->filters_user_data = NULL;
	datapipe->output_triggers = NULL;
	datapipe->triggers_user_data = NULL;
	datapipe->last_data = NULL;
}

/**
 * Deinitialize a datapipe
 *
 * @param datapipe The datapipe to manipulate
 */
void free_datapipe(datapipe_struct *const datapipe)
{
	if (datapipe == NULL) {
		sphone_log(LL_ERR,
			"free_datapipe() called "
			"without a valid datapipe");
		return;
	}

	/* Warn about still registered filters/triggers */
	if (datapipe->filters != NULL) {
		sphone_log(LL_INFO,
			"free_datapipe() called on a datapipe that "
			"still has registered filter(s)");
	}

	if (datapipe->output_triggers != NULL) {
		sphone_log(LL_INFO,
			"free_datapipe() called on a datapipe that "
			"still has registered output_trigger(s)");
	}
}
