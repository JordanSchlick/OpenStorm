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
 * The PRUNE functions eliminate NULL or non-present (no data present)
 * substructures.  This tightens the structures for output.  Development
 * was sparked by failing NCAR UF ingest programs; they cannot handle
 * 0 sized volumes/sweeps/rays dispite conformity to the UF specification.
 *
 * These routines free memory that is pruned.
 *
 * John H. Merritt
 * Space Applications Corporation
 * December 12, 1995
 */

#include "rsl.h"
extern int radar_verbose_flag;

Ray *RSL_prune_ray(Ray *ray)
{
  if (ray == NULL) return NULL;
  if (ray->h.nbins > 0) return ray;
  RSL_free_ray(ray);
  return NULL;
}

Sweep *RSL_prune_sweep(Sweep *s)
{
  int i, j;

  if (s == NULL) return NULL;
  if (s->h.nrays == 0) {
	RSL_free_sweep(s);
	return NULL;
  }
/*
 * Squash out all dataless rays.  'j' is the index for the squashed (pruned)
 * rays.
 */
  for (i=0,j=0; i<s->h.nrays; i++)
	if ((s->ray[i] = RSL_prune_ray(s->ray[i])))
	  s->ray[j++] = s->ray[i]; /* Keep this ray. */

  if (j==0) {
	RSL_free_sweep(s);
	return NULL; /* All rays were pruned. */
  }
  for (i=j; i<s->h.nrays; i++) s->ray[i] = NULL;
  s->h.nrays = j;
  return s;
}

Volume *RSL_prune_volume(Volume *v)
{
  int i, j;

  if (v == NULL) return NULL;
  if (v->h.nsweeps == 0) {
	RSL_free_volume(v);
	return NULL;
  }
/*
 * Squash out all dataless sweeps.  'j' is the index for sweep containing data.
 */
  for (i=0,j=0; i<v->h.nsweeps; i++)
	if ((v->sweep[i] = RSL_prune_sweep(v->sweep[i])))
	  v->sweep[j++] = v->sweep[i]; /* Keep this sweep. */

  if (j==0) {
	RSL_free_volume(v);
	return NULL; /* All sweeps were pruned. */
  }
  for (i=j; i<v->h.nsweeps; i++) v->sweep[i] = NULL;
  v->h.nsweeps = j;
  return v;
}

Radar *RSL_prune_radar(Radar *radar)
{
  int i;
  /* Volume indexes are fixed so we just prune the substructures. */
  if (radar == NULL) return NULL;
  for (i=0; i<radar->h.nvolumes; i++)
	radar->v[i] = RSL_prune_volume(radar->v[i]);

  return radar;
}
