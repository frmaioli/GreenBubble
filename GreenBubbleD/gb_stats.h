/*
 * gb_stats.h:
 *	Configuration handling for GreenBubble project
 *
 * Copyright (c) 2018-2019 Fabiano R. Maioli <frmaioli@gmail.com>
 ***********************************************************************
 *    GreenBubble is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    GreenBubble is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with GreenBubble.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#ifndef GB_STATS_H
#define GB_STATS_H

#include <gb_main.h>

void cfg_big_json_test(gbCfg_t *cfg);
void gb_stats_init(gbSts_t *sts);
void gb_stats_decref(gbSts_t *sts);
void gb_get_status(gbSts_t *sts);

#endif //GB_STATS_H
