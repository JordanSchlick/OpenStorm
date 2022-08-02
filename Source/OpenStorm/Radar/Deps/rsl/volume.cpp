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
 * Volume functions coded in this file:
 *
 *   Default DZ_F and DZ_INVF.  For VR, SW, CZ, ZT, DR and LR too.
 *   Volume *RSL_new_volume(int max_sweeps);
 *   Sweep *RSL_new_sweep(int max_rays);
 *   Ray *RSL_new_ray(int max_bins);
 *   Ray *RSL_clear_ray(Ray *r);
 *   Sweep *RSL_clear_sweep(Sweep *s);
 *   Volume *RSL_clear_volume(Volume *v);
 *   void RSL_free_ray(Ray *r);
 *   void RSL_free_sweep(Sweep *s);
 *   void RSL_free_volume(Volume *v);
 *   Ray *RSL_copy_ray(Ray *r);
 *   Sweep *RSL_copy_sweep(Sweep *s);
 *   Volume *RSL_copy_volume(Volume *v);
 *   Ray *RSL_get_ray_from_sweep(Sweep *s, float azim);
 *   float RSL_get_value_from_sweep(Sweep *s, float azim, float r);
 *   float RSL_get_value_from_ray(Ray *ray, float r);
 *   float RSL_get_value_at_h(Volume *v, float azim, float grnd_r, float h);
 *   Sweep *RSL_get_sweep(Volume *v, float elev);
 *   Ray *RSL_get_ray(Volume *v, float elev, float azimuth);
 *   float RSL_get_value(Volume *v, float elev, float azimuth, float range);
 *   Ray *RSL_get_ray_above(Volume *v, Ray *current_ray);
 *   Ray *RSL_get_ray_below(Volume *v, Ray *current_ray);
 *   Ray *RSL_get_matching_ray(Volume *v, Ray *ray);
 *   int RSL_get_sweep_index_from_volume
 *
 * See image_gen.c for the Volume image generation functions.
 *
 * See doc/rsl_index.html for HTML formatted documentation.
 * 
 * All routines herein coded, unless otherwise stated, by:
 *     John Merritt
 *     Space Applications Corporation
 *
 *  Contributions by
 *  Dennis Flanigan, Jr.
 *  flanigan@lance.colostate.edu
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
int strcasecmp(const char *s1, const char *s2);

#define USE_RSL_VARS
#include "rsl.h"

#define bin_azimuth(x, dx) (float)((float)x/dx)
#define bin_elevation(x, dx) (float)((float)x/dx)
#define bin_range(x, dx) (float)((float)x/dx)

extern int radar_verbose_flag;

/* Internal storage conversion functions. These may be any conversion and
 * may be dynamically defined; based on the input data conversion.  If you
 * change any of the reserved values, ie. values that cannot be converted
 * from/to internal storage (BADVAL, APFLAG, etc), be sure to change all
 * functions, both XX_F and XX_INVF to handle it.  It is best to have the
 * reserved values stored at 0, 1, 2, on up.  That way you merely need to 
 * provide an offset to the actual conversion function.  See 'rsl.h'
 * for the definition of the reserved values.  Currently: BADVAL, RFVAL,
 * APFLAG, NOECHO.  
 *
 * The conversion functions may NOT be macros.
 */

/* Modification to properly convert DM field (DEK 21 Nov 2012) */

#ifdef USE_TWO_BYTE_PRECISION
#define F_FACTOR 100.0
#define F_DR_FACTOR 1000.0
#define F_DZ_RANGE_OFFSET 50
#define F_DM_RANGE_OFFSET 130   /* DEK */
#else
#define F_FACTOR 2.0
#define F_DR_FACTOR 10.0
#define F_DZ_RANGE_OFFSET 32
#endif

/* #define F_VR_OFFSET 63.5 */
#define F_VR_OFFSET 127.0
#define F_DR_OFFSET 12.0

/* IMPORTANT: This is the offset from reserved values. This
 *            number must be exactly (or >=) the number of
 *            reserved values in XX_F and XX_INVF.
 *
 * You must change nsig_to_radar.c where F_OFFSET is used for optimization.
 */
#define F_OFFSET 4


float DZ_F(Range x) {
  if (x >= F_OFFSET) /* This test works when Range is unsigned. */
    return (((float)x-F_OFFSET)/F_FACTOR - F_DZ_RANGE_OFFSET);  /* Default wsr88d. */
  if (x == 0) return BADVAL;
  if (x == 1) return RFVAL;
  if (x == 2) return APFLAG;
  if (x == 3) return NOECHO;
  return BADVAL;  /* Can't get here, but quiets the compiler. */
}


float DM_F(Range x) {
  if (x >= F_OFFSET) /* This test works when Range is unsigned. */
        return (((float)x-F_OFFSET)/F_FACTOR - F_DM_RANGE_OFFSET);  /* DEK  */
  if (x == 0) return BADVAL;
  if (x == 1) return RFVAL;
  if (x == 2) return APFLAG;
  if (x == 3) return NOECHO;
  return BADVAL;  /* Can't get here, but quiets the compiler. */
}


float VR_F(Range x) {
  float val;
  if (x >= F_OFFSET) { /* This test works when Range is unsigned. */
    val = (((float)x-F_OFFSET)/F_FACTOR - F_VR_OFFSET);  /* Default wsr88d coding. */
    /*  fprintf(stderr, "x=%d, val=%f\n", x, val); */
    return val;
  }
  if (x == 0) return BADVAL;
  if (x == 1) return RFVAL;
  if (x == 2) return APFLAG;
  if (x == 3) return NOECHO;
  return BADVAL;  /* Can't get here, but quiets the compiler. */
}

float DR_F(Range x) {    /* Differential reflectivity */
  float val;
  if (x >= F_OFFSET) { /* This test works when Range is unsigned. */
    val = (((float)x-F_OFFSET)/F_DR_FACTOR - F_DR_OFFSET);
    return val;
  }
  if (x == 0) return BADVAL;
  if (x == 1) return RFVAL;
  if (x == 2) return APFLAG;
  if (x == 3) return NOECHO;
  return BADVAL;  /* Can't get here, but quiets the compiler. */
}

float LR_F(Range x) {/* From MCTEX */
  if (x >= F_OFFSET)  /* This test works when Range is unsigned. */
    return (float) (x - 250.)/6.;
  if (x == 0) return BADVAL;
  if (x == 1) return RFVAL;
  if (x == 2) return APFLAG;
  if (x == 3) return NOECHO;
  return BADVAL;
 }

float HC_F(Range x) {  /* HydroClass (Sigmet) */
  if (x == 0) return BADVAL;
  return (float)x;
}

/****************************
 Sigmet RhoHV : one_byte values
> RohHV = sqrt((N-1)/253)
> 0     bad/no value
> 1     0.0000
> 2     0.0629
> 128   0.7085
> 253   0.9980
> 254   1.0000
*******************************/
float RH_F(Range x) {
  if (x == 0) return BADVAL;
  /* return (float)(sqrt((double)((x-1.0)/253.0))); */
  return ((float)x-1.0) / 100.;
}

/***************************** 
  Sigmet PhiDP : one_byte values
> PhiDP (mod 180) = 180 * ((N-1)/254)
> The range is from 0 to 180 degrees in steps of 0.71 as follows.
> 0     bad/no value
> 1     0.00 deg
> 2     0.71 deg
> 101   70.87 deg
> 254   179.29 deg
******************************/
float PH_F(Range x) {
  if (x == 0) return BADVAL;
  /* Sigmet method */
  return (360.*(x-1.))/65534.;
 }

/* TODO: Should this be 5. cm. instead of 0.5?  Or maybe 10. cm.? */
float rsl_kdp_wavelen = 0.5; /* Default radar wavelen = .5 cm.  See
                              * nsig_to_radar.c for the code that sets this.
                              */
/* KD_F for 1 or 2 byte. */
float KD_F(Range x)
{
  if (x == 0) return BADVAL;
  /* Sigmet method */
  return ((float)x-32768.)/100.;
}

/* Normalized Coherent Power (DORADE) */
float NP_F(Range x)
{
  if (x == 0) return BADVAL;
  return (float)(x - 1) / 100.;
}

/* Standard Deviation (for Dual-pole QC testing.) */
float SD_F(Range x)
{
  if (x == 0) return BADVAL;
  return ((float)x-1.) / 100.;
}

/* Signal to Noise Ratio */
float SN_F(Range x) {
  if (x == 0) return BADVAL;
  return ((float)x-32768.)/100.;
}

/* Signal Quality Index */
float SQ_F(Range x)
{
  if (x == 0) return BADVAL;
  return (float)(x-1) / 65533.;
}

float TI_F(Range x)
{
  if (x >= F_OFFSET) return (float)(x);
  if (x == 0) return BADVAL;
  if (x == 1) return RFVAL;
  if (x == 2) return APFLAG;
  if (x == 3) return NOECHO;
  return BADVAL;
}

float SW_F(Range x) {  return VR_F(x); }
float CZ_F(Range x) {  return DZ_F(x); }
float ZT_F(Range x) {  return DZ_F(x); }
float ZD_F(Range x) {  return DR_F(x); } /* Differential reflectivity */
float CD_F(Range x) {  return DR_F(x); } /* Differential reflectivity */
float XZ_F(Range x) {  return DZ_F(x); }
float MZ_F(Range x) {  return (float)x; } /* DZ Mask */
float MD_F(Range x) {  return MZ_F(x); }  /* ZD Mask */
float ZE_F(Range x) {  return DZ_F(x); }
float VE_F(Range x) {  return VR_F(x); }
/* float DM_F(Range x) {  return DZ_F(x); }   DEK */
float DX_F(Range x) {  return DZ_F(x); }
float CH_F(Range x) {  return DZ_F(x); }
float AH_F(Range x) {  return DZ_F(x); }
float CV_F(Range x) {  return DZ_F(x); }
float AV_F(Range x) {  return DZ_F(x); }
float VS_F(Range x) {  return VR_F(x); }
float VL_F(Range x) {  return VR_F(x); }
float VG_F(Range x) {  return VR_F(x); }
float VT_F(Range x) {  return VR_F(x); }
float VC_F(Range x) {  return VR_F(x); }




/* Unfortunately, floats are stored differently than ints/shorts.  So,
 * we cannot simply set up a switch statement and we must test for
 * all the special cases first.  We must test for exactness.
 */
Range DZ_INVF(float x)
{
  if (x == BADVAL) return (Range)0;
  if (x == RFVAL)  return (Range)1;
  if (x == APFLAG)  return (Range)2;
  if (x == NOECHO)  return (Range)3;
  if (x < -F_DZ_RANGE_OFFSET) return (Range)0;
  return (Range)(F_FACTOR*(x+F_DZ_RANGE_OFFSET)+.5 + F_OFFSET); /* Default wsr88d. */
}

Range DM_INVF(float x)
{
  if (x == BADVAL) return (Range)0;
  if (x == RFVAL)  return (Range)1;
  if (x == APFLAG)  return (Range)2;
  if (x == NOECHO)  return (Range)3;
  return (Range)(F_FACTOR*(x+F_DM_RANGE_OFFSET)+.5 + F_OFFSET); /* DEK  */
}


Range VR_INVF(float x)
{
  if (x == BADVAL) return (Range)0;
  if (x == RFVAL)  return (Range)1;
  if (x == APFLAG)  return (Range)2;
  if (x == NOECHO)  return (Range)3;
  if (x < -F_VR_OFFSET)    return (Range)0;
  return (Range)(F_FACTOR*(x+F_VR_OFFSET)+.5 + F_OFFSET); /* Default wsr88d coding. */
}

Range DR_INVF(float x)     /* Differential reflectivity */
{
  if (x == BADVAL) return (Range)0;
  if (x == RFVAL)  return (Range)1;
  if (x == APFLAG) return (Range)2;
  if (x == NOECHO) return (Range)3;
  if (x < -F_DR_OFFSET)    return (Range)0;
  return (Range)(F_DR_FACTOR*(x + F_DR_OFFSET) + F_OFFSET + 0.5);
}

Range HC_INVF(float x)  /* HydroClass (Sigmet) */
{
  if (x == BADVAL) return (Range)0;
  return (Range)(x + 0.5); /* Round */
}

Range LR_INVF(float x) /* MCTEX */
{
  if (x == BADVAL) return (Range)0;
  if (x == RFVAL)  return (Range)1;
  if (x == APFLAG)  return (Range)2;
  if (x == NOECHO)  return (Range)3;
  return (Range)((6.*x + 250) + 0.5); /* Round */
}

/**************************
 Sigmet RhoHV : one_byte values
> RohHV = sqrt((N-1)/253)
> 0     bad/no value
> 1     0.0000
> 2     0.0629
> 128   0.7085
> 253   0.9980
> 254   1.0000
****************************/
/* RH_INVF for 1 or 2 byte data. */
Range RH_INVF(float x) {
  if (x == BADVAL) return (Range)0;
  /* return (Range)(x * x * 253.0 + 1.0 + 0.5); */
  return (Range)(x * 100. + 1. +.5);
}

/******************************
 Sigmet PhiDP : one_byte values
> PhiDP (mod 180) = 180 * ((N-1)/254)
> The range is from 0 to 180 degrees in steps of 0.71 as follows.
> 0     bad/no value
> 1     0.00 deg
> 2     0.71 deg
> 101   70.87 deg
> 254   179.29 deg
*******************************/
Range PH_INVF(float x) {
  if (x == BADVAL) return (Range)0;
  return (Range)(x*65534./360. + 1.0 + 0.5);
}


/* KD_INVF for 1 or 2 byte data. */
Range KD_INVF(float x) {
  if (x == BADVAL) return (Range)0;
  return (Range)(x * 100. + 32768. + 0.5);
}

/* Standard Deviation (for Dual-pole QC testing.) */
Range SD_INVF(float x)
{
  if (x == BADVAL) return (Range)0;
  return (Range)(x * 100. + 1. + .5);
}

/* Signal to Noise Ratio */
Range SN_INVF(float x) {
  if (x == BADVAL) return (Range)0;
  return (Range)(x * 100. + 32768. + 0.5);
}

/* Signal Quality Index */
Range SQ_INVF(float x)
{
  if (x == BADVAL) return (Range)0;
  return (Range)(x * 65533. + 1. +.5);
}

/* Normalized Coherent Power (DORADE) */
Range NP_INVF(float x)
{
  if (x == BADVAL) return (0);
  return (Range)(x * 100. + 1.);
}

Range TI_INVF(float x) /* MCTEX */
{
  if (x == BADVAL) return (Range)0;
  if (x == RFVAL)  return (Range)1;
  if (x == APFLAG)  return (Range)2;
  if (x == NOECHO)  return (Range)3;
  return (Range)(x);
}


Range SW_INVF(float x) {  return VR_INVF(x); }
Range CZ_INVF(float x) {  return DZ_INVF(x); }
Range ZT_INVF(float x) {  return DZ_INVF(x); }
Range ZD_INVF(float x) {  return DR_INVF(x); } /* Differential reflectivity */
Range CD_INVF(float x) {  return DR_INVF(x); } /* Differential reflectivity */
Range XZ_INVF(float x) {  return DZ_INVF(x); }
Range MZ_INVF(float x) {  return (Range)x;   } /* DZ Mask */
Range MD_INVF(float x) {  return MZ_INVF(x); } /* ZD Mask */
Range ZE_INVF(float x) {  return DZ_INVF(x); }
Range VE_INVF(float x) {  return VR_INVF(x); }
/* Range DM_INVF(float x) {  return DZ_INVF(x); }  DEK */
Range DX_INVF(float x) {  return DZ_INVF(x); }
Range CH_INVF(float x) {  return DZ_INVF(x); }
Range AH_INVF(float x) {  return DZ_INVF(x); }
Range CV_INVF(float x) {  return DZ_INVF(x); }
Range AV_INVF(float x) {  return DZ_INVF(x); }
Range VS_INVF(float x) {  return VR_INVF(x); }
Range VL_INVF(float x) {  return VR_INVF(x); }
Range VG_INVF(float x) {  return VR_INVF(x); }
Range VT_INVF(float x) {  return VR_INVF(x); }
Range VC_INVF(float x) {  return VR_INVF(x); }



/**********************************************************************/
/*        M E M O R Y    M A N A G E M E N T   R O U T I N E S        */
/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*                        new_volume                                  */
/*                        new_sweep                                   */
/*                        new_ray                                     */
/*                                                                    */
/**********************************************************************/
Volume *RSL_new_volume(int max_sweeps)
{
  /*
   * A volume consists of a header section and an array of sweeps.
   */
  Volume *v;
  v = (Volume *)calloc(1, sizeof(Volume));
  if (v == NULL) perror("RSL_new_volume");
  v->sweep = (Sweep **) calloc(max_sweeps, sizeof(Sweep*));
  if (v->sweep == NULL) perror("RSL_new_volume, Sweep*");
  v->h.nsweeps = max_sweeps; /* A default setting. */
  return v;
}

/*
 * The 'Sweep_list' structure is internal to RSL.  It maintains a list
 * of sweeps allocated and it contains pointers to a hash table of Rays
 * separately for each sweep.  There is no reason to access this internal
 * structure except when optimizing new RSL routines that access Rays.
 * Otherwise, the RSL interfaces should suffice.
 *
 * The hash table is a means of finding rays, by azimuth, quickly.
 * To find a ray is simple:  use the hash function to get close
 * to the ray, if not right on it the first time.  Collisions of rays in
 * the hash table are handled by a link list of rays from a hash entry.
 * Typically, the first ray of the sweep is not the ray with the smallest
 * azimuth angle.  We are confident that the order of Rays in the Sweep
 * is by azimuth angle, but that cannot be guarenteed.  Therefore, this
 * hash scheme is required.
 *
 * The 'Sweep_list' contains the address of the memory allocated to
 * sweep.  The list is sorted by addresses.  There is no
 * memory limit to the number of sweeps.  If the number of sweeps exceeds
 * the current allocation for the Sweep_list, then a new Sweep_list is
 * allocated, which is bigger, and the old list copied to it.
 *
 * Sweep_list is at least as long as the number of sweeps allocated.
 */

typedef struct {
  Sweep *s_addr;
  Hash_table *hash;
} Sweep_list;

/*
 * By design of RSL, this should be "#define STATIC static"
 *
 * It is OK to "#define STATIC static", but, if you do, then
 * the examples (run by run_tests in examples/) will fail for
 * those programs that test these data structures.  I normally,
 * don't set this #define, for that reason.
 */

#define STATIC
STATIC int RSL_max_sweeps = 0; /* Initial allocation for sweep_list.
                                * RSL_new_sweep will allocate the space first
                                * time around.
                                */
STATIC int RSL_nsweep_addr = 0; /* A count of sweeps in the table. */
STATIC Sweep_list *RSL_sweep_list = NULL;
STATIC int RSL_nextents = 0;

void FREE_HASH_NODE(Azimuth_hash *node)
{
  if (node == NULL) return;
  FREE_HASH_NODE(node->next); /* Tail recursive link list removal. */
  free(node);
}

void FREE_HASH_TABLE(Hash_table *table)
{
  int i;
  if (table == NULL) return;
  for (i=0; i<table->nindexes; i++)
    FREE_HASH_NODE(table->indexes[i]); /* A possible linked list of Rays. */
  free(table->indexes);
  free(table);
}

void REMOVE_SWEEP(Sweep *s)
{
  int i;
  int j;
  /* Find where it goes, split the list and slide the tail down one. */
  for (i=0; i<RSL_nsweep_addr; i++)
    if (s == RSL_sweep_list[i].s_addr) break;

  if (i == RSL_nsweep_addr) return; /* Not found. */
  /* This sweep is at 'i'. */ 
  /* Deallocate the memory for the hash table. */
  FREE_HASH_TABLE(RSL_sweep_list[i].hash);

  RSL_nsweep_addr--;
  for (j=i; j<RSL_nsweep_addr; j++)
    RSL_sweep_list[j] = RSL_sweep_list[j+1];

  RSL_sweep_list[RSL_nsweep_addr].s_addr = NULL;
  RSL_sweep_list[RSL_nsweep_addr].hash = NULL;
}
  

int INSERT_SWEEP(Sweep *s)
{
  Sweep_list *new_list;
  int i,j;

  if (RSL_nsweep_addr >= RSL_max_sweeps) { /* Current list is too small. */
    RSL_nextents++;
    new_list = (Sweep_list *) calloc(100*RSL_nextents, sizeof(Sweep_list));
    if (new_list == NULL) {
      perror("INSERT_SWEEP");
      exit(2);
    }
    /* Copy the old list to the new one. */
    for (i=0; i<RSL_max_sweeps; i++) new_list[i] = RSL_sweep_list[i];
    RSL_max_sweeps = 100*RSL_nextents;
    free(RSL_sweep_list);
    RSL_sweep_list = new_list;
  }
  /* Find where it goes, split the list and slide the tail down one. */
  for (i=0; i<RSL_nsweep_addr; i++)
    if (s < RSL_sweep_list[i].s_addr) break;
    
  /* This sweep goes at 'i'. But first we must split the list. */
  for (j=RSL_nsweep_addr; j>i; j--)
    RSL_sweep_list[j] = RSL_sweep_list[j-1];

  RSL_sweep_list[i].s_addr = s;
  RSL_sweep_list[i].hash = NULL;
  RSL_nsweep_addr++;
  return i;
}

int SWEEP_INDEX(Sweep *s)
{
  /* Locate the sweep in the RSL_sweep_list.  Return the index. */
  /* Simple linear search; but this will be a binary search. */
  int i;
  for (i=0; i<RSL_nsweep_addr; i++)
    if (s == RSL_sweep_list[i].s_addr) return i;
  return -1;
}

Sweep *RSL_new_sweep(int max_rays)
{
  /*
   * A sweep consists of a header section and an array of rays.
   */
  Sweep *s;
  s = (Sweep  *)calloc(1, sizeof(Sweep));
  if (s == NULL) perror("RSL_new_sweep");
  INSERT_SWEEP(s);
  s->ray = (Ray **) calloc(max_rays, sizeof(Ray*));
  if (s->ray == NULL) perror("RSL_new_sweep, Ray*");
  s->h.nrays = max_rays; /* A default setting. */
  s->h.elev = -999.;
  s->h.azimuth = -999.;
  return s;
}

Ray *RSL_new_ray(int max_bins)
{
  /*
   * A ray consists of a header section and an array of Range types (floats).
   */
  Ray *r;
  r = (Ray *)calloc(1, sizeof(Ray));
  if (r == NULL) perror("RSL_new_ray");
  r->range = (Range *) calloc(max_bins, sizeof(Range));
  if (r->range == NULL) perror("RSL_new_ray, Range");
  r->h.nbins = max_bins; /* A default setting. */
/*  fprintf(stderr,"range[0] = %x, range[%d] = %x\n", &r->range[0], max_bins-1, &r->range[max_bins-1]);*/
  return r;
}

/**********************************************************************/
/*                                                                    */
/*                      clear_ray                                     */
/*                      clear_sweep                                   */
/*                      clear_volume                                  */
/*                                                                    */
/**********************************************************************/
Ray *RSL_clear_ray(Ray *r)
{
  if (r == NULL) return r;
  memset(r->range, 0, sizeof(Range)*r->h.nbins);
  return r;
}
Sweep *RSL_clear_sweep(Sweep *s)
{
  int i;
  if (s == NULL) return s;
  for (i=0; i<s->h.nrays; i++) {
    RSL_clear_ray(s->ray[i]);
  }
  return s;
}
Volume *RSL_clear_volume(Volume *v)
{
  int i;
  if (v == NULL) return v;
  for (i=0; i<v->h.nsweeps; i++) {
    RSL_clear_sweep(v->sweep[i]);
  }
  return v;
}
/**********************************************************************/
/*                                                                    */
/*                       free_ray                                     */
/*                       free_sweep                                   */
/*                       free_volume                                  */
/*                                                                    */
/**********************************************************************/
void RSL_free_ray(Ray *r)
{
  if (r == NULL) return;
  if (r->range) free(r->range);
  free(r);
}
void RSL_free_sweep(Sweep *s)
{
  int i;
  if (s == NULL) return;
  for (i=0; i<s->h.nrays; i++) {
    RSL_free_ray(s->ray[i]);
  }
  if (s->ray) free(s->ray);
  REMOVE_SWEEP(s); /* Remove from internal Sweep list. */
  free(s);
}
void RSL_free_volume(Volume *v)
{
  int i;
  if (v == NULL) return;

  for (i=0; i<v->h.nsweeps; i++)
     {
     RSL_free_sweep(v->sweep[i]);
     }
  if (v->sweep) free(v->sweep);
  if (v->h.type_str) free(v->h.type_str);
  free(v);
}

/**********************************************************************/
/*                                                                    */
/*                       copy_ray                                     */
/*                       copy_sweep                                   */
/*                       copy_volume                                  */
/*                                                                    */
/**********************************************************************/
Ray *RSL_copy_ray(Ray *r)
{
  Ray *new_ray;

  if (r == NULL) return NULL;
  new_ray = RSL_new_ray(r->h.nbins);
  new_ray->h = r->h;
  memcpy(new_ray->range, r->range, r->h.nbins*sizeof(Range));
  return new_ray;
}
Sweep *RSL_copy_sweep(Sweep *s)
{
  int i;
  Sweep *n_sweep;

  if (s == NULL) return NULL;
  n_sweep = RSL_new_sweep(s->h.nrays);
  if (n_sweep == NULL) return NULL;
  n_sweep->h = s->h;

  for (i=0; i<s->h.nrays; i++) {
    n_sweep->ray[i] = RSL_copy_ray(s->ray[i]);
  }
  return n_sweep;
}



Volume *RSL_copy_volume(Volume *v)
{
  int i;
  Volume *new_vol;

  if (v == NULL) return NULL;
  new_vol = RSL_new_volume(v->h.nsweeps);
  new_vol->h = v->h;

  for (i=0; i<v->h.nsweeps; i++) {
    new_vol->sweep[i] = RSL_copy_sweep(v->sweep[i]);
  }
  return new_vol;
}


/**********************************************************************/
/**********************************************************************/
/*              G E N E R A L    F U N C T I O N S                    */
/**********************************************************************/
/**********************************************************************/

double angle_diff(float x, float y)
{
  double d;
  d = fabs((double)(x - y));
  if (d > 180) d = 360 - d;
  return d;
}

/**********************************************************************/
/*                                                                    */
/*                 RSL_get_next_cwise_ray                             */
/* Dennis Flanigan                                                    */
/* Mods by John Merritt 10/20/95                                      */
/**********************************************************************/
Ray *RSL_get_next_cwise_ray(Sweep *s, Ray *ray)
{
  /* The fastest way to do this is to gain access to the hash table
   * which maintains a linked list of sorted rays.
   */
  Hash_table *hash_table;
  Azimuth_hash *closest;
  int hindex;
  float ray_angle;

  if (s == NULL) return NULL;
  if (ray == NULL) return NULL;
  /* Find a non-NULL index close to hindex that we want. */
  hash_table = hash_table_for_sweep(s);
  if (hash_table == NULL) return NULL; /* Nada. */
  ray_angle = ray->h.azimuth;
  hindex = hash_bin(hash_table,ray_angle); 
  
  /* Find hash entry with closest Ray */
  closest = the_closest_hash(hash_table->indexes[hindex],ray_angle);
 
  return closest->ray_high->ray;
}

/**********************************************************************/
/*                                                                    */
/*                 RSL_get_next_ccwise_ray                            */
/* JHM 10/20/95                                                       */
/**********************************************************************/
Ray *RSL_get_next_ccwise_ray(Sweep *s, Ray *ray)
{
  /* The fastest way to do this is to gain access to the hash table
   * which maintains a linked list of sorted rays.
   */
  Hash_table *hash_table;
  Azimuth_hash *closest;
  int hindex;
  float ray_angle;

  if (s == NULL) return NULL;
  if (ray == NULL) return NULL;
  /* Find a non-NULL index close to hindex that we want. */
  hash_table = hash_table_for_sweep(s);
  if (hash_table == NULL) return NULL; /* Nada. */
  ray_angle = ray->h.azimuth;  
  hindex = hash_bin(hash_table,ray_angle); 
  
  /* Find hash entry with closest Ray */
  closest = the_closest_hash(hash_table->indexes[hindex],ray_angle);
 
  return closest->ray_low->ray;
}


/******************************************
 *                                        *
 * cwise_angle_diff                       *
 *                                        *
 * Dennis Flanigan,Jr. 5/17/95            *
 ******************************************/
double cwise_angle_diff(float x,float y)
   {
   /* Returns the clockwise angle difference of x to y.
    * If x = 345 and y = 355 return 10.  
    * If x = 345 and y = 335 return 350
    */
   double d;
   
   d = (double)(y - x);
   if (d < 0) d += 360;
   return d;
   }

/******************************************
 *                                        *
 * ccwise_angle_diff                      *
 *                                        *
 * Dennis Flanigan,Jr. 5/17/95            *
 ******************************************/
double ccwise_angle_diff(float x,float y)
   {
   /* Returns the counterclockwise angle differnce of x to y.
    * If x = 345 and y = 355 return 350.  
    * If x = 345 and y = 335 return 10
    */
   double d;
   
   d = (double)(x - y);
   if (d < 0) d += 360;
   return d;
   }

/*****************************************
 *                                       *
 * the_closest_hash                      *
 *                                       *
 * Dennis Flanigan,Jr. 4/29/95           *
 *****************************************/
Azimuth_hash *the_closest_hash(Azimuth_hash *hash, float ray_angle)
   {
   /* Return the hash pointer with the minimum ray angle difference. */

   double clow,chigh,cclow;
   Azimuth_hash *high,*low;

   if (hash == NULL) return NULL;

   /* Set low pointer to hash index with ray angle just below
    * requested angle and high pointer to just above requested
    * angle.
    */

   /* set low and high pointers to initial search locations*/
   low = hash;         
   high = hash->ray_high;
   
   /* Search until clockwise angle to high is less then clockwise
    * angle to low.
    */

   clow   = cwise_angle_diff(ray_angle,low->ray->h.azimuth);
   chigh  = cwise_angle_diff(ray_angle,high->ray->h.azimuth);
   cclow  = ccwise_angle_diff(ray_angle,low->ray->h.azimuth);

   while((chigh > clow) && (clow != 0))
      {
      if (clow < cclow)
         {
         low  = low->ray_low;
         high = low->ray_high;  /* Not the same low as line before ! */
         }
      else
         {
         low  = low->ray_high;
         high = low->ray_high;  /* Not the same low as line before ! */
         }

      clow   = cwise_angle_diff(ray_angle,low->ray->h.azimuth);
      chigh  = cwise_angle_diff(ray_angle,high->ray->h.azimuth);
      cclow  = ccwise_angle_diff(ray_angle,low->ray->h.azimuth);
      }

   if(chigh <= cclow)
      {
      return high;
      }
   else
      {
      return low;
      }
   }


/*******************************************************************/
/*                                                                 */
/*       get_closest_sweep_index                                   */
/*                                                                 */
/* Dennis Flanigan, Jr.  5/15/95                                   */
/*******************************************************************/
int get_closest_sweep_index(Volume *v,float sweep_angle)
   {
   Sweep *s;
   int i,ci;
   float delta_angle = 91;
   float check_angle;
   
   if(v == NULL) return -1;

   ci = 0;

   for (i=0; i<v->h.nsweeps; i++)
      {
      s = v->sweep[i];
      if (s == NULL) continue;
      check_angle = fabs((double)(s->h.elev - sweep_angle));

      if(check_angle <= delta_angle) 
         {
         delta_angle = check_angle;
         ci = i;
         }
      }

   return ci;
   }





/********************************************************************/
/*                                                                  */
/*     RSL_get_closest_sweep                                        */
/*                                                                  */
/*  Dennis Flanigan, Jr. 5/15/95                                    */
/********************************************************************/
Sweep *RSL_get_closest_sweep(Volume *v,float sweep_angle,float limit)
   {
   /* Find closest sweep to requested angle.  Assume PPI sweep for
    * now. Meaning: sweep_angle represents elevation angle from 
    * 0->90 degrees
    */
   Sweep *s;
   float delta_angle;
   int ci;
   
   if (v == NULL) return NULL;
   
   if((ci = get_closest_sweep_index(v,sweep_angle)) < 0)
      {
      return NULL;
      }

   s = v->sweep[ci];

   delta_angle = fabs((double)(s->h.elev - sweep_angle));

   if( delta_angle <= limit)
      {
      return s;
      }
   else
      {
      return NULL;
      }
   }

/**********************************************************************/
/*     These are more specific routines to make coding hierarchical.  */
/*                                                                    */
/* done 4/7/95    Ray   *RSL_get_ray_from_sweep                       */
/* done 3/31      float  RSL_get_value_from_sweep                     */
/* done 3/31      float  RSL_get_value_from_ray                       */
/* done 4/1       float  RSL_get_value_at_h                           */
/*                                                                    */
/**********************************************************************/
Ray *RSL_get_ray_from_sweep(Sweep *s, float ray_angle)
{
  /* Locate the Ray * for ray_angle in the sweep. */
  
  /* Sanity checks. */
  if (s == NULL) return NULL;
  if (ray_angle < 0) ray_angle += 360.0; /* Only positive angles. */
  if (ray_angle >= 360) ray_angle -= 360;
  
  return RSL_get_closest_ray_from_sweep(s,ray_angle,s->h.horz_half_bw);
}

/**********************************************
 *                                            *
 *           hash_bin                         *
 *                                            *
 * Dennis Flanigan, Jr. 4/27/95               *
 **********************************************/
int hash_bin(Hash_table *table,float angle)
{
  /* Internal Routine to calculate the hashing bin index 
   * of a given angle.
   */
  int hash;
  float res;
  
  res = 360.0/table->nindexes;
  hash = (int)(angle/res + res/2.0);/*Centered about bin.*/
  
  if(hash >= table->nindexes) hash = hash - table->nindexes;
  
  /* Could test see which direction is closer, but 
   * why bother?
   */
  while(table->indexes[hash] == NULL) {
    hash++;
    if(hash >= table->nindexes) hash = 0;
  }
  
  return hash;
}

Hash_table *hash_table_for_sweep(Sweep *s)
{
  int i;

  i = SWEEP_INDEX(s);
  if (i==-1) { /* Obviously, an unregistered sweep.  Most likely the
                * result of pointer assignments.
                */
    i = INSERT_SWEEP(s);
  }

  if (RSL_sweep_list[i].hash == NULL) { /* First time.  Construct the table. */
    RSL_sweep_list[i].hash = construct_sweep_hash_table(s);
  }

  return RSL_sweep_list[i].hash;
}  

/*********************************************************************/
/*                                                                   */
/*                    RSL_get_closest_ray_from_sweep                 */
/*                                                                   */
/* Dennis Flanigan  4/30/95                                          */
/*********************************************************************/
Ray *RSL_get_closest_ray_from_sweep(Sweep *s,float ray_angle, float limit)
   {
   /*
    * Return closest Ray in Sweep within limit (angle) specified
    * in parameter list.  Assume PPI mode.
    */
   int hindex;
   Hash_table *hash_table;
   Azimuth_hash *closest;
   double close_diff;

   if (s == NULL) return NULL;
   /* Find a non-NULL index close to hindex that we want. */
   hash_table = hash_table_for_sweep(s);
   if (hash_table == NULL) return NULL; /* Nada. */

   hindex = hash_bin(hash_table,ray_angle); 

   /* Find hash entry with closest Ray */
   closest = the_closest_hash(hash_table->indexes[hindex],ray_angle);
   
   /* Is closest ray within limit parameter ? If
    * so return ray, else return NULL.
    */

   close_diff = angle_diff(ray_angle,closest->ray->h.azimuth);
   
   if(close_diff <= limit) return closest->ray;
   
   return NULL;
 }


/*********************************************************************/
/*                                                                   */
/*                    Rsl_get_value_from_sweep                       */
/*                                                                   */
/*********************************************************************/
float RSL_get_value_from_sweep(Sweep *s, float azim, float r)
{
 /* Locate the polar point (r,azim) in the sweep. */
  Ray *ray;
  if (s == NULL) return BADVAL;
  ray = RSL_get_ray_from_sweep(s, azim);
  if (ray == NULL) return BADVAL;
  return RSL_get_value_from_ray(ray, r);
}


/*********************************************************************/
/*                                                                   */
/*                 RSL_get_range_of_range_index                      */
/* D. Flanigan 8/18/95                                               */
/*********************************************************************/
float RSL_get_range_of_range_index(Ray *ray, int index)
   {
   if (ray == NULL) return 0.0;
   if (index >= ray->h.nbins) return 0.0;
   return ray->h.range_bin1/1000.0 + index*ray->h.gate_size/1000.0;
   }


/************************************/
/*  RSL_get_value_from_ray          */
/*                                  */
/*  Updated 4/4/95  D. Flanigan     */
/*                                  */
/************************************/
float RSL_get_value_from_ray(Ray *ray, float r)
   {
   int bin_index;
   float rm;
   
   rm = r * 1000;

   if (ray == NULL) return BADVAL;

   if(ray->h.gate_size == 0)
      {
      if(radar_verbose_flag)
         {
         fprintf(stderr,"RSL_get_value_from_ray: ray->h.gate_size == 0\n");
         }
      return BADVAL;
      }
   
   /* range_bin1 is range to center of first bin */
   bin_index = (int)(((rm - ray->h.range_bin1)/ray->h.gate_size) + 0.5);

   /* Bin indexes go from 0 to nbins - 1 */
   if (bin_index >= ray->h.nbins || bin_index < 0) return BADVAL;

   return ray->h.f(ray->range[bin_index]);  
   }


/*********************************************************************/
/*                                                                   */
/*                    RSL_get_value_at_h                             */
/*                                                                   */
/*********************************************************************/
float RSL_get_value_at_h(Volume *v, float azim, float grnd_r, float h)
{
  float elev, r;

  RSL_get_slantr_and_elev(grnd_r, h, &r, &elev);
  return RSL_get_value(v, elev, azim, r);
}


/**********************************************************************/
/*     These take a Volume and return the appropriate structure.      */
/*                                                                    */
/* done 4/21/95  Sweep  *RSL_get_sweep                                */
/* done 4/1      Ray    *RSL_get_ray                                  */
/* done 4/1      float  *RSL_get_value                                */
/* done 5/3      Ray    *RSL_get_ray_above                            */
/* done 5/3      Ray    *RSL_get_ray_below                            */
/* done 5/12     Ray    *RSL_get_ray_from_other_volume                */
/*                                                                    */
/**********************************************************************/



/*********************************************************************/
/*                                                                   */
/*                    RSL_get_sweep                                  */
/*                                                                   */
/* Updated 5/15/95 Dennis Flanigan, Jr.                              */
/*********************************************************************/
Sweep *RSL_get_sweep(Volume *v, float sweep_angle)
   {
   /* Return a sweep with +/- 1/2 beam_width of 'elev', if found. */
   int   i = 0;
   
   if (v == NULL) return NULL;
   while(v->sweep[i] == NULL) i++;

   return RSL_get_closest_sweep(v,sweep_angle,v->sweep[i]->h.vert_half_bw);
   }


/*********************************************************************/
/*                                                                   */
/*                    RSL_get_ray                                    */
/*                                                                   */
/*********************************************************************/
Ray *RSL_get_ray(Volume *v, float elev, float azimuth)
{
  /* Locate 'elev' and 'azimuth' in the Volume v by a simple +/- epsilon on
   * the elevation angle and azimuth angle.
   */

  /*
   * 1. Locate sweep using azimuth; call RSL_get_sweep.
   * 2. Call RSL_get_ray_from_sweep
   */

  return  RSL_get_ray_from_sweep( RSL_get_sweep( v, elev ), azimuth );
}

/*********************************************************************/
/*                                                                   */
/*                    RSL_get_value                                  */
/*                                                                   */
/*********************************************************************/
float RSL_get_value(Volume *v, float elev, float azimuth, float range)
{
  /* Locate 'elev' and 'azimuth' and '<range' in the Volume v
   *  by a simple +/- epsilon on the elevation angle and azimuth angle
   *  and range.
   */

  /*
   * 1. Locate sweep using 'elev'.
   * 2. Call RSL_get_value_from_sweep
   */
  return RSL_get_value_from_sweep ( RSL_get_sweep (v, elev), azimuth, range );
}

/*********************************************************************/
/*                                                                   */
/*                    RSL_get_ray_above                              */
/*                                                                   */
/* Updated 5/15/95, Dennis Flanigan, Jr.                             */
/*********************************************************************/
Ray *RSL_get_ray_above(Volume *v, Ray *current_ray)
   {
   int i;

   if (v == NULL) return NULL;
   if (current_ray == NULL) return NULL;

   /* Find index of current Sweep */
   if(( i = get_closest_sweep_index(v,current_ray->h.elev)) < 0) return NULL;

   i++;
   while( i < v->h.nsweeps)
      {
      if(v->sweep[i] != NULL) break;
      i++;
      }

   if(i >= v->h.nsweeps) return NULL;

   return RSL_get_ray_from_sweep(v->sweep[i], current_ray->h.azimuth);
   }


/*********************************************************************/
/*                                                                   */
/*                    RSL_get_ray_below                              */
/*                                                                   */
/* Updated 5/15/95, Dennis Flanigan, Jr.                             */
/*********************************************************************/
Ray *RSL_get_ray_below(Volume *v, Ray *current_ray)
   {
   int i;
 
   if (v == NULL) return NULL;
   if (current_ray == NULL) return NULL;

   /* Find index of current Sweep */
   if(( i = get_closest_sweep_index(v,current_ray->h.elev)) < 0) return NULL;

   i--;
   while( i >= 0)
      {
      if(v->sweep[i] != NULL) break;
      i--;
      }

   if(i < 0) return NULL;

   return RSL_get_ray_from_sweep(v->sweep[i], current_ray->h.azimuth);
   }

/*********************************************************************/
/*                                                                   */
/*                    RSL_get_matching_ray                           */
/*                                                                   */
/*********************************************************************/
Ray *RSL_get_matching_ray(Volume *v, Ray *ray)
{

  /*
   * Locate the closest matching ray in the Volume 'v' to 'ray'.
   * Typically, use this function when finding a similiar ray in another
   * volume.
   */
  if (v == NULL) return NULL;
  if (ray == NULL) return NULL;

  return RSL_get_ray(v, ray->h.elev, ray->h.azimuth);
}

/*********************************************************************/
/*                                                                   */
/*                 RSL_get_first_ray_of_sweep                        */
/*                 RSL_get_first_ray_of_volume                       */
/*                                                                   */
/*********************************************************************/
Ray *RSL_get_first_ray_of_sweep(Sweep *s)
{
/* Because a sorting of azimuth angles may have been performed,
 * we need to test on the ray_num member and look for the smallest
 * one.  
 */
  int i;
  Ray *r;
  int smallest_ray_num;

  r = NULL;
  smallest_ray_num = 9999999;
  if (s == NULL) return r;
  for (i=0; i<s->h.nrays; i++)
    if (s->ray[i]) {
      if (s->ray[i]->h.ray_num <= 1) return s->ray[i];
      if (s->ray[i]->h.ray_num < smallest_ray_num) {
        r = s->ray[i];
        smallest_ray_num = r->h.ray_num;
      }
    }
  return r;
}

Ray *RSL_get_first_ray_of_volume(Volume *v)
{
  int i;
  if (v == NULL) return NULL;
  for (i=0; i<v->h.nsweeps; i++)
        if (v->sweep[i]) return RSL_get_first_ray_of_sweep(v->sweep[i]);
  return NULL;
}

/*********************************************************************/
/*                                                                   */
/*                 RSL_get_first_sweep_of_volume                     */
/*                                                                   */
/*********************************************************************/
Sweep *RSL_get_first_sweep_of_volume(Volume *v)
{
  int i;
  if (v == NULL) return NULL;
  for (i=0; i<v->h.nsweeps; i++)
    if (RSL_get_first_ray_of_sweep(v->sweep[i])) return v->sweep[i];
  return NULL;
}

#define N_SPECIAL_NAMES 2
/* 
 * Unfortunately in C, there is no way around initializing static
 * arrays by specifying repetition.
 *
 * There is another solution and that is to have RSL_new_radar set
 * a flag indicating if the application has called 'RSL_select_fields'
 * prior to calling the ingest routine.  I choose the static = {...}; method
 * for now.
 */

/* Could be static and force use of 'rsl_query_field' */
int rsl_qfield[MAX_RADAR_VOLUMES] = {
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1, 1, 1, 1,
  1, 1
 };


/*********************************************************************/
/*                                                                   */
/*                 RSL_select_fields                                 */
/*                                                                   */
/*********************************************************************/
void RSL_select_fields(char *field_type, ...)
{
  /*
   * 10/15/96
   * field_type = Case insensitive:
   *           "all"   - default, if never this routine is never called.
   *           "none"  - No fields are ingestd. Useful for getting header
   *                     information.
   *           "dz"    - Ingest DZ volume.
   *           "vr"    - Ingest VR volume.
   *            ...    - Just list additional fields.
   *           
   * The last argument must be NULL.  This signals this routine
   * when to stop parsing the field types.
   *
   * Action or side-effect:
   *   A second call to this fuction overrides any previous settings.
   *   In other words, multiple calls are not additive.  So, to get both
   *   DZ and VR volumes, use:
   *      RSL_select_fields("dz", "vr");  - Read both DZ and VR.
   *   and not:
   *      RSL_select_fields("dz");  -  Read only DZ.
   *      RSL_select_fields("vr");  -  Read only VR, no DZ.
   *
   * An RSL hidden array is set to flag which fields are selected.
   * This array is examined inside all ingest code.  It is not available
   * to the application.
   */

  va_list ap;
  char *c_field;
  int i;

  for (i=0; i<MAX_RADAR_VOLUMES; i++) rsl_qfield[i] = 0;

  /* # arguments, should be <= MAX_RADAR_VOLUMES, but we can handle
   * typo's and redundancies.  Each is processed in the order they 
   * appear.
   */
   
  c_field = field_type;
  va_start(ap, field_type);

  if (radar_verbose_flag) fprintf(stderr,"Selected fields for ingest:");
  while (c_field) {
    /* CHECK EACH FIELD. This is a fancier case statement than C provides. */
    if (radar_verbose_flag) fprintf(stderr," %s", c_field);
    if (strcasecmp(c_field, "all") == 0) {
      for (i=0; i<MAX_RADAR_VOLUMES; i++) rsl_qfield[i] = 1;
    } else if (strcasecmp(c_field, "none") == 0) {
      for (i=0; i<MAX_RADAR_VOLUMES; i++) rsl_qfield[i] = 0;
    } else {
    
      for (i=0; i<MAX_RADAR_VOLUMES; i++)
        if (strcasecmp(c_field, RSL_ftype[i]) == 0) {
          rsl_qfield[i] = 1;
          break; /* Break the for loop. */
        }
      
      if (i == MAX_RADAR_VOLUMES) {
        if (radar_verbose_flag)
          fprintf(stderr, "\nRSL_select_fields: Invalid field name <<%s>> specified.\n", c_field);
      }
    }
    c_field = va_arg(ap, char *);
  }

  if (radar_verbose_flag) fprintf(stderr,"\n");
  va_end(ap);

}


int rsl_query_field(char *c_field)
{
  /*
   * 10/15/96
   * RSL interface, for library code developers, to rsl ingest code,
   * which is intended to be part of RSL ingest code, which access
   * the hidden array 'rsl_qfield' and reports if that field is to
   * be ingested.
   *
   * Return 1 if YES, meaning yes ingest this field type.
   * Return 0 if NO.
   */

  /*
   * All ingest code is meant to use this routine to decide whether
   * or not to allocate memory for a field type.  For data formats
   * that are very large, this will help optimize the ingest on 
   * small memory machines and hopefully avoid unnessary swapping.
   *
   * LASSEN is a good example where there may be 10 or 12 input field
   * types, but the application only wants 2 or 3 of them.
   *
   * The application interface is RSL_select_fields.
   */
  int i;
  
  /* Quiet the compilier when -pedantic. :-) */
  RSL_f_list[0] = RSL_f_list[0];
  RSL_invf_list[0] = RSL_invf_list[0];

  for (i=0; i<MAX_RADAR_VOLUMES; i++)
    if (strcasecmp(c_field, RSL_ftype[i]) == 0) {
      rsl_qfield[i] = 1;
      break; /* Break the for loop. */
    }
  
  if (i == MAX_RADAR_VOLUMES) { /* We should never see this message for
                                 * properly written ingest code.
                                 */
    fprintf(stderr, "rsl_query_field: Invalid field name <<%s>> specified.\n", c_field);
  }
  
  /* 'i' is the index. Is it set? */
  return rsl_qfield[i];
}


/* Could be static and force use of 'rsl_query_sweep' */
int *rsl_qsweep = NULL;  /* If NULL, then read all sweeps. Otherwise,
                          * read what is on the list.
                          */
#define RSL_MAX_QSWEEP 500 /* It'll be rediculious to have more. :-) */
int rsl_qsweep_max = RSL_MAX_QSWEEP;

/*********************************************************************/
/*                                                                   */
/*                 RSL_read_these_sweeps                             */
/*                                                                   */
/*********************************************************************/
void RSL_read_these_sweeps(char *csweep, ...)
{
  va_list ap;
  char *c_sweep;
  int i, isweep;

  /* "all", "none", "0", "1", "2", "3", ... */

  /* # arguments, should be <= 'max # sweeps expected', but, what is it?
   * We can handle typo's and redundancies.  Each is processed in the
   * order they appear.
   */
   
  c_sweep = csweep;
  va_start(ap, csweep);

  rsl_qsweep_max = -1;
  if (rsl_qsweep == NULL) 
    rsl_qsweep = (int *)calloc(RSL_MAX_QSWEEP, sizeof(int));

  /* else Clear the array - a second call to this function over-rides
   * any previous settings.  This holds even if the second call has
   * bad arguments.  
   */
  else 
    for(i = 0;i< RSL_MAX_QSWEEP; i++) 
      rsl_qsweep[i] = 0;


  if (radar_verbose_flag) fprintf(stderr,"Selected sweeps for ingest:");
  for (;c_sweep;    c_sweep = va_arg(ap, char *))
    {
    /* CHECK EACH FIELD. This is a fancier case statement than C provides. */
    if (radar_verbose_flag) fprintf(stderr," %s", c_sweep);
    if (strcasecmp(c_sweep, "all") == 0) {
      for (i=0; i<RSL_MAX_QSWEEP; i++) rsl_qsweep[i] = 1;
      rsl_qsweep_max = RSL_MAX_QSWEEP;
    } else if (strcasecmp(c_sweep, "none") == 0) {
     /* Commented this out to save runtime -GJW
      * rsl_qsweep[] already initialized to 0 above.
      *
      * for (i=0; i<RSL_MAX_QSWEEP; i++) rsl_qsweep[i] = 0;
      * rsl_qsweep_max = -1;
      */
    } else {
      i = sscanf(c_sweep,"%d", &isweep);
      if (i == 0) { /* No match, bad argument. */
        if (radar_verbose_flag) fprintf(stderr,"\nRSL_read_these_sweeps: bad parameter %s.  Ignoring.\n", c_sweep);
        continue;
      }

      if (isweep < 0 || isweep > RSL_MAX_QSWEEP) {
        if (radar_verbose_flag) fprintf(stderr,"\nRSL_read_these_sweeps: parameter %s not in [0,%d).  Ignoring.\n", c_sweep, RSL_MAX_QSWEEP);
        continue;
      }

      if (isweep > rsl_qsweep_max) rsl_qsweep_max = isweep;
      rsl_qsweep[isweep] = 1;
    }
  }

  if (radar_verbose_flag) fprintf(stderr,"\n");
  va_end(ap);
}

#include <time.h>
void RSL_fix_time (Ray *ray)
{
  struct tm the_time;
  float fsec;
  /* Fixes possible overflow values in month, day, year, hh, mm, ss */
  /* Normally, ss should be the overflow.  This code ensures end of 
   * month, year and century are handled correctly by using the Unix
   * time functions.
   */
  if (ray == NULL) return;
  memset(&the_time, 0, sizeof(struct tm));
  the_time.tm_sec = ray->h.sec;
  fsec = ray->h.sec - the_time.tm_sec;
  the_time.tm_min  = ray->h.minute;
  the_time.tm_hour = ray->h.hour;
  the_time.tm_mon  = ray->h.month - 1;
  the_time.tm_year = ray->h.year - 1900;
  the_time.tm_mday = ray->h.day;
  the_time.tm_isdst = -1;
  (void) mktime(&the_time);
  /* The time is fixed. */
  ray->h.sec    = the_time.tm_sec;
  ray->h.sec   += fsec;
  ray->h.minute = the_time.tm_min;
  ray->h.hour   = the_time.tm_hour;
  ray->h.month  = the_time.tm_mon + 1;
  ray->h.year   = the_time.tm_year + 1900;
  ray->h.day    = the_time.tm_mday;
  return;
}

/*********************************************************************/
/*                                                                   */
/*                 RSL_add_dbz_offset_to_ray                         */
/*                                                                   */
/*********************************************************************/
/*
  Add the calibration factor 'dbz_offset' to each ray bin which
  contains a valid value.
*/
void RSL_add_dbz_offset_to_ray(Ray *r, float dbz_offset)
{
  int ibin;
  float val; 

  if (r == NULL) return;
  for (ibin=0; ibin<r->h.nbins; ibin++)
  {
    val = r->h.f(r->range[ibin]);
    if ( val >= (float)NOECHO ) continue;  /* Invalid value */
    r->range[ibin] = r->h.invf(val + dbz_offset);
  }
}

/*********************************************************************/
/*                                                                   */
/*                 RSL_add_dbz_offset_to_sweep                       */
/*                                                                   */
/*********************************************************************/
void RSL_add_dbz_offset_to_sweep(Sweep *s, float dbz_offset)
{
  int iray;
  if (s == NULL) return;
  for (iray=0; iray<s->h.nrays; iray++)
    RSL_add_dbz_offset_to_ray(s->ray[iray], dbz_offset);
}

/*********************************************************************/
/*                                                                   */
/*                 RSL_add_dbz_offset_to_volume                      */
/*                                                                   */
/*********************************************************************/
void RSL_add_dbz_offset_to_volume(Volume *v, float dbz_offset)
{
  int isweep;
  if (v == NULL) return;
  for (isweep=0; isweep<v->h.nsweeps; isweep++)
    RSL_add_dbz_offset_to_sweep(v->sweep[isweep], dbz_offset);
}
