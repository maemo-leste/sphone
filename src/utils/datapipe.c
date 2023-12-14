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
#include <stdbool.h>
#include "datapipe.h"
#include "sphone-log.h"

struct callback {
	void *callback;
	void *data;
};

/**
 * Execute the filters of a datapipe
 *
 * @param datapipe The datapipe to execute
 * @param indata The input data to run through the datapipe
 * @return The processed data
 */
gconstpointer execute_datapipe_filters(datapipe_struct *const datapipe, gpointer indata)
{
	gpointer data = indata;
	bool entryNull = !indata;

	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return NULL;
	}

	struct callback *cb;
	for (int i = 0; (cb = g_slist_nth_data(datapipe->filters, i)) != NULL; i++) {
		gpointer (*filter)(gpointer data, gpointer user_data) = cb->callback;
		gpointer tmp = filter(data, cb->data);
		if(!entryNull && !tmp)
			return NULL;

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
	if (datapipe == NULL) {
		sphone_log(LL_ERR, "%s called without a valid datapipe", __func__);
		return;
	}

	struct callback *cb;
	for (int i = 0; (cb = g_slist_nth_data(datapipe->output_triggers, i)) != NULL; i++) {
		void (*trigger)(gconstpointer data, gpointer user_data) = cb->callback;
		trigger(indata, cb->data);
	}
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

	if(!data && indata)
		return NULL;

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
	
	struct callback *cb = g_malloc0(sizeof(*cb));
	cb->callback = filter;
	cb->data = user_data;
	datapipe->filters = g_slist_append(datapipe->filters, cb);
}

/**
 * Remove a filter from an existing datapipe
 * Non-existing filters are ignored
 *
 * @param datapipe The datapipe to manipulate
 * @param filter The filter to remove from the datapipe
 */
void remove_filter_from_datapipe(datapipe_struct *const datapipe, gpointer (*filter)(gpointer data, gpointer user_data),
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

	bool removed = false;
	for (GSList *element = datapipe->filters; element; element = element->next) {
		struct callback *cb = element->data;
		if(cb->callback == filter && user_data == cb->data) {
			datapipe->filters = g_slist_remove(datapipe->filters, cb);
			g_free(cb);
			removed = true;
			break;
		}
	}

	/* Did we remove any entry? */
	if (!removed)
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

	struct callback *cb = g_malloc0(sizeof(*cb));
	cb->callback = trigger;
	cb->data = user_data;
	datapipe->output_triggers = g_slist_append(datapipe->output_triggers, cb);
}

/**
 * Remove an output trigger from an existing datapipe
 * Non-existing triggers are ignored
 *
 * @param datapipe The datapipe to manipulate
 * @param trigger The trigger to remove from the datapipe
 */
void remove_trigger_from_datapipe(datapipe_struct *const datapipe, void (*trigger)(gconstpointer data, gpointer user_data),
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

	bool removed = false;
	for (GSList *element = datapipe->output_triggers; element; element = element->next) {
		struct callback *cb = element->data;
		if(cb->callback == trigger && user_data == cb->data) {
			datapipe->output_triggers = g_slist_remove(datapipe->output_triggers, cb);
			g_free(cb);
			removed = true;
			break;
		}
	}

	/* Did we remove any entry? */
	if (!removed)
		sphone_log(LL_WARN, "Trying to remove non-existing trigger. Offending callback: %p", trigger);
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
	datapipe->output_triggers = NULL;
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
		sphone_log(LL_WARN,
			"free_datapipe() called on a datapipe that "
			"still has registered filter(s). Offending callbacks:");
		for(GSList *element = datapipe->filters; element; element = element->next) {
			struct callback *cb = element->data;
			sphone_log(LL_WARN, "%p", cb->callback);
		}
	}

	if (datapipe->output_triggers != NULL) {
		sphone_log(LL_WARN,
			"free_datapipe() called on a datapipe that "
			"still has registered output_trigger(s). Offending callbacks:");
		for(GSList *element = datapipe->output_triggers; element; element = element->next) {
			struct callback *cb = element->data;
			sphone_log(LL_WARN, "%p", cb->callback);
		}
	}
}
