/*
 * gb_config.c:
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

#include <string.h>

#include <gb_config.h>
#include <gb_led.h>

void dflt_config_init(gbCfg_t *cfg)
{
    cfg->ld_instant_mode = false;

    cfg->ld_instant[LD_WHITE].enable = false;    
    cfg->ld_instant[LD_WHITE].vset = 40; //Cant go lower Vin (36V) for BST900   
    cfg->ld_instant[LD_WHITE].cset = 0;

    cfg->ld_instant[LD_BLUE].enable = false;    
    cfg->ld_instant[LD_BLUE].vset = 0;    
    cfg->ld_instant[LD_BLUE].cset = 0;

    cfg->ld_instant[LD_RED].enable = false;    
    cfg->ld_instant[LD_RED].vset = 0;    
    cfg->ld_instant[LD_RED].cset = 0;

    {
        // (0 and 24hs will always repeat the number)
        unsigned char tmp[LD_NUMB][ROUT_STEP] = {
                //0hs, 3hs, 6hs, 9hs, 12hs, 15hs, 18hs, 21hs, 24hs
                { 0,   0,   0,   75,  100,  100,  75,   0,    0 }, //White
                { 0,   0,   0,   80,   40,   10,   0,   3,    0 }, //Blue
                { 0,   0,   0,    5,   15,   40,  80,   0,    0 }  //Red
                };
        memcpy(cfg->ld_spec, tmp, sizeof(cfg->ld_spec));
    }

    // Generate the numbers in between for ld_routine_perc
    // It will also set ld_routine_init to true
    ld_generate_points();

    return;
}
