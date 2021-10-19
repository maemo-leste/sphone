/*
 * sphone
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * Copyright (C) Carl Philipp Klemm 2021 <carl@uvos.xyz>
 * 
 * sphone is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * sphone is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <gtk/gtk.h>
#include <stdbool.h>
#include "rtconf.h"

#define ARRAY_SIZE(x)   (sizeof(x)/sizeof(x[0]))
#define TEST_BIT(x,addr) (1UL & (addr[x/8] >> (x & 0xff)))

void utils_start_ringing(const gchar *dial);
void utils_stop_ringing(const gchar *dial);
int utils_ringing_status(void);
void utils_sms_notify(void);

#endif
