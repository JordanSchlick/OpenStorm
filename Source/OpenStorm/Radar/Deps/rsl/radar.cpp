/*
    NASA/TRMM, Code 910.1.
    This is the TRMM Office Radar Software Library.
    Copyright (C) 1996, 1997
            John H. Merritt
            Space Applications Corporation
            Vienna, Virginia

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/*
 * Radar routines coded in this file:
 *
 *   RSL_radar_verbose_on();  
 *   RSL_radar_verbose_off();
 *   Radar *RSL_new_radar(int nvolumes);
 *   void RSL_free_radar(Radar *r);
 *   Radar *RSL_clear_radar(Radar *r);
 *   Volume *RSL_get_volume(Radar *r, int type_wanted);
 *   Radar *RSL_wsr88d_to_radar(char *infile, unsigned int data_mask);
 *
 * Internal routines:
 *   print_vect(float v[], int istart, int istop);
 *   void radar_load_date_time(Radar *radar);
 *   int wsr88d_load_sweep_into_volume(Wsr88d_sweep ws,
 *   					   Volume *v, int nsweep, unsigned int vmask);
 *
 * Radar routines not coded in this file:
 *
 *   Radar *RSL_read_radar(char *infile);
 *   int RSL_write_radar(Radar *radar, char *outfile);
 *   Radar *RSL_clear_radar(Radar *r);
 *   void RSL_radar_to_uf(Radar *r, char *outfile);
 *   Radar *RSL_uf_to_radar(char *infile);
 *
 * See the file radar.ez and version.notes for more detailed documentation.
 *
 * All routines herein coded, unless otherwise stated, by:
 *     John Merritt
 *     Space Applications Corporation
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rsl.h"

void RSL_print_version()
{
  printf("RSL version %s.\n", RSL_VERSION_STR);
}

/* Debug printing global variable: radar_verbose_flag */
int radar_verbose_flag = 0;

void RSL_radar_verbose_on()
{
  radar_verbose_flag = 1;
}
void RSL_radar_verbose_off()
{
  radar_verbose_flag = 0;
}

void print_vect(float v[], int istart, int istop)
{
  int i;
  for (i=istart; i<=istop; i++)
	fprintf(stderr,"v[%d] = %f\n", i, v[i]);
}


/**********************************************************************/
/*                                                                    */
/*               RSL_get_nyquist_from_radar                           */
/*                                                                    */
/**********************************************************************/
float RSL_get_nyquist_from_radar(Radar *radar)
{
  /* Find a velocity volume.
   * Find first sweep in that volume.
   * Find first ray in that sweep.
   * Return the nyquist velocity.
   *
   * This code required for loading nyquist value in non-velocity
   * volumes;  UF output is affected by this in a good way.
   */
  Volume *vol;
  Ray *ray;
  
  if (radar == NULL) return 0.0;
  if (radar->h.nvolumes <= VR_INDEX) return 0.0;

  vol = radar->v[VR_INDEX];
  ray = RSL_get_first_ray_of_volume(vol);
  if (ray == NULL) return 0.0;
  return ray->h.nyq_vel;
}

/**********************************************************************/
/*                                                                    */
/* done 3/30         new_radar()                                      */
/* done 3/30         free_radar()                                     */
/* done 4/21         clear_radar()                                    */
/*                                                                    */
/**********************************************************************/
Radar *RSL_new_radar(int nvolumes)
{
  Radar *r;
  r = (Radar *) calloc(1, sizeof(Radar));
  r->v = (Volume **) calloc(nvolumes, sizeof(Volume *));
  r->h.nvolumes = nvolumes;
  r->h.scan_mode = PPI; /* default PPI is enum constant defined in rsl.h */
  return r;
}

void RSL_free_radar(Radar *r)
{
  int i;

  /* Chase down all the pointers and free everything in sight. */
  if (r) {
	for (i=0; i<r->h.nvolumes; i++)
	  RSL_free_volume(r->v[i]);
	if (r->v) free(r->v);
	free(r);
  }
}

Radar *RSL_clear_radar(Radar *r)
{
  int i;

  if (r == NULL) return r;
  for (i=0; i<r->h.nvolumes; i++)
	RSL_clear_volume(r->v[i]);

  return r;
}

/**********************************************************************/
/*                                                                    */
/* done 8/26         radar_load_date_time                             */
/*                                                                    */
/**********************************************************************/
void radar_load_date_time(Radar *radar)
{
  /* Search for the first existing ray of the first sweep of the first
   * volume; steal that information.
   */

  int i;
  Ray *first_ray;

  radar->h.month = 0;
  radar->h.day   = 0;
  radar->h.year  = 0;
  radar->h.hour  = 0;
  radar->h.minute= 0;
  radar->h.sec= 0.0;
  first_ray = NULL;

  for (i=0; i<MAX_RADAR_VOLUMES; i++) {
	if (radar->v[i] != NULL) {
	  first_ray = RSL_get_first_ray_of_volume(radar->v[i]);
	  if (first_ray) {
		radar->h.month = first_ray->h.month;
		radar->h.day   = first_ray->h.day;
		radar->h.year  = first_ray->h.year;
		radar->h.hour  = first_ray->h.hour;
		radar->h.minute= first_ray->h.minute;
		radar->h.sec   = first_ray->h.sec;
		return;
	  }
	}
  }
}

/**********************************************************************/
/*                                                                    */
/* done 3/30     Volume *RSL_get_volume                               */
/*                                                                    */
/**********************************************************************/
Volume *RSL_get_volume(Radar *r, int type_wanted)
{
  return r->v[type_wanted];
}
