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
 * Creates an array that is located in each sweep structure that is 
 * a hash for quick azimuth angle lookups.  These routines in concert
 * with rewrites of the 'RSL_get_value...' routines dramatically speed
 * up the RSL.
 *
 * The following routines are affected by this change.  Just so you don't
 * think I'm absentminded, I list what might be affected and illustrate that
 * they're not.
 *
 *   Routine                Affected?
 *   ---------------        ------------
 *   RSL_copy_sweep         No.
 *   RSL_clear_sweep        No.
 *   RSL_read_radar         No.
 *   RSL_write_radar        No.
 *   RSL_uf_to_radar        No.
 *   RSL_nsig_to_radar      No.
 *   RSL_toga_to_radar      No.
 *   RSL_lassen_to_radar    No.
 *   RSL_new_sweep          Yes.
 *   RSL_get_value_from_ray Yes.
 *   RSL_get...             No.
 
 *
 * By: John Merritt
 *     Space Applications Corporation
 *     October 10, 1995
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rsl.h"
extern int radar_verbose_flag;

/*********************************************************************/
/*                                                                   */
/*          set_high_and_low_pointers                                */
/*                                                                   */
/* By Dennis Flanigan                                                */
/* 5/2/95                                                            */
/*********************************************************************/
static void set_high_and_low_pointers(Hash_table *table)
   {
	 /* This function modifies its argument. */

   Azimuth_hash *last,*current,*ray_prev_ind;
   Azimuth_hash *one,*two,*three,*four,*very_first,*very_last;
   int not_sorted_flag;
   int i;

   /* Set ray_high,ray_low pointers in hash data structures.
    */
   if (table == NULL) return;
   very_first = NULL;
   very_last = NULL;
   last = NULL;
   current = NULL;
   ray_prev_ind = NULL;
   for(i = 0; i < table->nindexes;i++)
      {
      if(table->indexes[i] != NULL)
         {
         if(ray_prev_ind != NULL)
            {
            /* Connect last ray in last index to first ray in 
             * this index.
             */
            ray_prev_ind->ray_high = table->indexes[i];
            table->indexes[i]->ray_low = ray_prev_ind;
            }

         /* Connect all entries in this indexes.  Don't worry
          * about ray angle order right now.
          */
         current = table->indexes[i];
         while(current->next)
            {
            last = current;
            current = current->next;
		   
            last->ray_high = current;
            current->ray_low = last;
            }

         /* Set ray_high and ray_low so that entries are "sorted" */
         not_sorted_flag = 1;  /* TRUE */
         while(not_sorted_flag)
            {
            current = table->indexes[i];
            not_sorted_flag = 0;  /* FALSE */
            while(current->next)
               {
               /* last = current; */
               current = current->next;
			  
               if(current->ray_low != NULL)
                  {
                  /* Someday want to test ray_angle */
                  if(current->ray_low->ray->h.azimuth > current->ray->h.azimuth)
                     {
                     /* Need to switch the ray_high and ray_low
                      * pointers so that order is one three two four.
                      */
                     not_sorted_flag = 1;  /* TRUE */
                     one   = current->ray_low->ray_low;
                     two   = current->ray_low;
                     three = current;
                     four  = current->ray_high;
					
                     if(one != NULL) one->ray_high = three;
                     two->ray_low = three;
                     two->ray_high = four;
                     three->ray_low = one;
                     three->ray_high = two;
                     if(four != NULL) four->ray_low = two;
                     }
                  }
               }
            }

         /* Save highest ray angle  entry in index so that it may 
          * be connected to first entry of next index.  The
          * very_last pointer will be used to set very_first
          * pointer.
          */
         current = table->indexes[i];
         while(current->ray_high) current = current->ray_high;
         ray_prev_ind = current;
         very_last = current;

         /* If this is first non-NULL hash index, save lowest
          * ray angle.
          */
         if(very_first == NULL)
            {
            current = table->indexes[i];
            while(current->ray_low) current = current->ray_low;
            very_first = current;
            }
         }
      }

   /* Tie the first and last azimuth_hash's together.
    * (Future If PPI_SWEEP statement....)
    */
   very_first->ray_low = very_last;
   very_last->ray_high = very_first;
   }



static Azimuth_hash *hash_add_node(Azimuth_hash *node, void *ray)
{
  Azimuth_hash *new_node;
  
  new_node = (Azimuth_hash *)calloc(1, sizeof(Azimuth_hash));
  if (new_node == NULL) perror("hash_add_node");
  else {
   new_node->ray = (Ray*)ray;
   new_node->next = node;
 }
  return new_node;
}

Hash_table *construct_sweep_hash_table(Sweep *s)
{
  Hash_table *hash_table;
  int i, iazim;
  Ray *ray;
  float res;
  
  if (s == NULL) return NULL;
  hash_table = (Hash_table *) calloc(1, sizeof(Hash_table));
  hash_table->nindexes = s->h.nrays;
  if (hash_table->nindexes < 0) {
	fprintf(stderr, "Unable to construct sweep hash table because nrays = %d\n", s->h.nrays);
	fprintf(stderr, "FATAL error... unable to continue.\n");
	//exit(-1);
  }

  res = 360.0 / hash_table->nindexes;
  /* Check that this makes sense with beam width. */
  if ((res > 2*s->h.beam_width) && (s->h.beam_width != 0)) { 
	/* Problem.  Too few rays so force a
	 * reasonable hash table size
	 */
	hash_table->nindexes = 360.0 / s->h.beam_width;
	res = s->h.beam_width;
  }
  hash_table->indexes = (Azimuth_hash **)calloc(hash_table->nindexes, sizeof(Azimuth_hash *));
  if (hash_table->indexes == NULL) {
	if (radar_verbose_flag) perror("construct_sweep_hash_table");
	return hash_table;
  }
  
  for (i=0; i<s->h.nrays; i++) {
	ray = s->ray[i];
	if (ray == NULL) continue;
	iazim = (int)(ray->h.azimuth/res + res/2.0); /* Centered on bin. */
	if (iazim >= hash_table->nindexes) iazim -= hash_table->nindexes ;
	/*	fprintf(stderr,"ray# %d, azim %f, iazim %d\n", ray->h.ray_num, ray->h.azimuth, iazim); */
	if (iazim > hash_table->nindexes || iazim < 0) {
	  if (radar_verbose_flag){
		fprintf(stderr,"ERROR: ");
		fprintf(stderr,"ray# %d, azim %f, iazim %d, nrays %d, nindexes %d\n", ray->h.ray_num, ray->h.azimuth, iazim, s->h.nrays, hash_table->nindexes);
   }
	} else
      hash_table->indexes[iazim] = hash_add_node(hash_table->indexes[iazim], ray);

  }
  
  set_high_and_low_pointers(hash_table);
  return hash_table;
}


