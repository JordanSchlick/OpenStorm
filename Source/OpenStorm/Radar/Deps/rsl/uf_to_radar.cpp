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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <unistd.h>
#include <math.h>

/* This allows us to use RSL_ftype, RSL_f_list, RSL_invf_list from rsl.h. */
#define USE_RSL_VARS
#include "rsl.h"

extern int radar_verbose_flag;
/* Changed old buffer size (16384) for larger dualpol files.  BLK 5/18/2011 */
/* Changed old buffer size (20000) for larger dualpol files.  BLK 3/20/2014 */
typedef short UF_buffer[26000]; /* Some UF files are bigger than 4096
                                 * that the UF doc's specify.
                                 */

#define UF_MORE 0
#define UF_DONE 1

static float (*f)(Range x);
static Range (*invf)(float x);

Volume *reset_nsweeps_in_volume(Volume *volume)
{
  int i;
  if (volume == NULL) return NULL;
  for (i=volume->h.nsweeps; i>0; i--)
    if (volume->sweep[i-1] != NULL) {
      volume->h.nsweeps = i;
      break;
    }
  return volume;
}
Radar *reset_nsweeps_in_all_volumes(Radar *radar)
{
  int i;
  if (radar == NULL) return NULL;
  for (i=0; i<radar->h.nvolumes; i++)
    radar->v[i] = reset_nsweeps_in_volume(radar->v[i]);
  return radar;
}

Volume *copy_sweeps_into_volume(Volume *new_volume, Volume *old_volume)
{
  int i;
  int nsweeps;
  if (old_volume == NULL) return new_volume;
  if (new_volume == NULL) return new_volume;
  nsweeps = new_volume->h.nsweeps; /* Save before copying old header. */
  new_volume->h = old_volume->h;
  new_volume->h.nsweeps = nsweeps;
  for (i=0; i<old_volume->h.nsweeps; i++)
    new_volume->sweep[i] = old_volume->sweep[i]; /* Just copy pointers. */
  /* Free the old sweep array, not the pointers to sweeps. */
  free(old_volume->sweep);
  return new_volume;
}

void swap2(short *buf, int n)
{
  short *end_addr;
  end_addr = buf + n;
  while (buf < end_addr) {
    swap_2_bytes(buf);
    buf++;
  }
}

static void put_start_time_in_radar_header(Radar *radar)
{
  /* Get the earliest ray time and store it in radar header.
   * The search is necessary because rays are not always in chronological order.
   * For example, we have received data in which rays were apparently sorted by
   * azimuth in some upstream software.  This results in the ray times being out
   * of order, because a sweep rarely actually begins at zero degrees.
   *
   * Written by Bart Kelley, SSAI, June 19, 2013
   */

  int i = 0;
  Sweep *sweep;
  Ray   *ray;

  int prevdate, thisdate;
  float prevtime, thistime;

  /* Get first sweep of first available field. */
  for (i=0; i < MAX_RADAR_VOLUMES; i++) {
      if ((sweep = radar->v[i]->sweep[0]) != NULL) break;
  }
  /* This shouldn't happen. */
  if (i >= MAX_RADAR_VOLUMES) {
      fprintf(stderr,"put_start_time_in_radar_header: No radar volumes contain "
              "sweep at index 0.\n");
      return;
  }

  /* Get first ray and its time. */
  i = 0;
  while (!sweep->ray[i] && i < sweep->h.nrays) i++;
  ray = sweep->ray[i];
  prevdate = ray->h.year * 10000 + ray->h.month * 100 + ray->h.day;
  prevtime = ray->h.hour * 10000 + ray->h.minute * 100 + ray->h.sec;

  /* Compare times of remaining rays for earliest time. */
  for (i=0; i<sweep->h.nrays; i++) {
    ray = sweep->ray[i];
    thisdate = ray->h.year * 10000 + ray->h.month * 100 + ray->h.day;
    thistime = ray->h.hour * 10000 + ray->h.minute * 100 + ray->h.sec;
    if (thisdate == prevdate) {
      if (thistime < prevtime) prevtime = thistime;
    }
    else if (thisdate < prevdate) {
      prevdate = thisdate;
      prevtime = thistime;
    }
  }

  radar->h.year = prevdate / 10000;
  radar->h.month = prevdate / 100 % 100;
  radar->h.day = prevdate % 100;
  radar->h.hour = (int) prevtime / 10000;
  radar->h.minute = (int) prevtime / 100 % 100;
  radar->h.sec = fmod(prevtime,100.);
}

/* These are used in uf_into_radar, set in caller RSL_uf_to_radar_fp. */
static int pulled_time_from_first_ray;
static int need_scan_mode;

/********************************************************************/
/*********************************************************************/
/*                                                                   */
/*                  uf_into_radar                                    */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      Space Applications Corporation                               */
/*      August 26, 1994                                              */
/*********************************************************************/
int uf_into_radar(UF_buffer uf, Radar **the_radar)
{
  
/* Missing data flag : -32768 when a signed short. */
#define UF_NO_DATA 0X8000
  
  /* Any convensions may be observed, however, Radial Velocity must be VE. */
  /* Typically:
   *    DM = Reflectivity (dB(mW)).
   *    DZ = Reflectivity (dBZ).
   *    VR = Radial Velocity.
   *    CZ = Corrected Reflectivity. (Quality controlled: AP removed, etc.)
   *    SW = Spectrum Width.
   *    DR = Differential Reflectivity.
   *    XZ = X-band Reflectivity.
   *
   * These fields may appear in any order in the UF file.
   *
   * RETURN:
   *   UF_DONE if we're done with the UF ingest.
   *   UF_MORE if we need more UF data.
   */
  
  /* These are pointers to various locations within the UF buffer 'uf'.
   * They are used to index the different components of the UF structure in
   * a manor consistant with the UF documentation.  For instance, uf_ma[1]
   * will be equivalenced to the second word (2 bytes/each) of the UF
   * buffer.
   */
  short *uf_ma;  /* Mandatory header block. */
  short *uf_lu;  /* Local Use header block.  */
  short *uf_dh;  /* Data header.  */
  short *uf_fh;  /* Field header. */
  short *uf_data; /* Data. */
  
  /* The length of each header. */
  int len_data, len_lu;
  
  int current_fh_index; 
  float scale_factor;
  
  int nfields, isweep, ifield, iray, i, j, m;
  char *field_type; /* For printing the field type upon error. */
  short proj_name[4];
  Ray *ray;
  Sweep *sweep;
  Radar *radar;
  float x;
  short missing_data;
  Volume *new_volume;
  int nbins;
  float frequency;
  extern int rsl_qfield[];
  extern int *rsl_qsweep; /* See RSL_read_these_sweeps in volume.c */
  extern int rsl_qsweep_max;

  radar = *the_radar;

/*
 * The organization of the Radar structure is by volumes, then sweeps, then
 * rays, then gates.  This is different from the UF file organization.
 * The UF format is sweeps, rays, then gates for all field types (volumes).
 */


/* Set up all the UF pointers. */
  uf_ma = uf;
  uf_lu = uf + uf_ma[3] - 1;
  uf_dh = uf + uf_ma[4] - 1;

  nfields =  uf_dh[0];
  isweep = uf_ma[9] - 1;

  if (rsl_qsweep != NULL) {
    if (isweep > rsl_qsweep_max) return UF_DONE;
    if (rsl_qsweep[isweep] == 0) return UF_MORE;
  }


/* Here is a sticky part.  We must make sure that if we encounter any
 * additional fields that were not previously present, that we are able
 * to load them.  This will require us to copy the entire radar structure
 * and whack off the old one.  But, we must be sure that it really is a
 * new field.  This is not so trivial as a couple of lines of code; I will
 * have to think about this a little bit more.  See STICKYSOLVED below.
 */
#ifdef STICKYSOLVED
  if (radar == NULL) radar = RSL_new_radar(nfields);
  /* Sticky solution here. */
#else
  if (radar == NULL) {
    radar = RSL_new_radar(MAX_RADAR_VOLUMES);
    *the_radar = radar;
    pulled_time_from_first_ray = 0;
    for (i=0; i<MAX_RADAR_VOLUMES; i++)
      if (rsl_qfield[i]) /* See RSL_select_fields in volume.c */
        radar->v[i] = RSL_new_volume(20);
  }
  
#endif

  if (need_scan_mode) {
    /* PPI and RHI are enum constants defined in rsl.h */
    if (uf_ma[34] == 1) radar->h.scan_mode = PPI;
    else if (uf_ma[34] == 3) radar->h.scan_mode = RHI;
    else {
      fprintf(stderr,"Warning: UF sweep mode = %d\n", uf_ma[34]);
      fprintf(stderr,"    Expected 1 or 3 (PPI or RHI)\n");
      fprintf(stderr,"    Setting radar->h.scan_mode to PPI\n");
      radar->h.scan_mode = PPI;
    }
    need_scan_mode = 0;
  }

/* For LITTLE ENDIAN:
 * WE "UNSWAP" character strings.  Because there are so few of them,
 * it is easier to catch them here.  The entire UF buffer is swapped prior
 * to entry to here, therefore, undo-ing these swaps; sets the
 * character strings right.
 */

  for (i=0; i<nfields; i++) {
    if (little_endian()) swap_2_bytes(&uf_dh[3+2*i]); /* Unswap. */
    ifield = -1;
    field_type = (char *)&uf_dh[3+2*i];
    for (j=0; j<MAX_RADAR_VOLUMES; j++) {
      if (strncmp(field_type, RSL_ftype[j], 2) == 0) {
        ifield = j;
        break;
      }
    }
    if (ifield < 0) { /* DON'T know how to handle this yet. */
      fprintf(stderr, "Unknown field type %c%c\n", (char)field_type[0],
              (char)field_type[1]);
      continue;
    }

    f = RSL_f_list[ifield];
    invf = RSL_invf_list[ifield];

    /* Do we place the data into this volume? */
    if (radar->v[ifield] == NULL) continue; /* Nope. */

    if (isweep >= radar->v[ifield]->h.nsweeps) { /* Exceeded sweep limit.
                                                  * Allocate more sweeps.
                                                  * Copy all previous sweeps.
                                                  */
      if (radar_verbose_flag)
        fprintf(stderr,"Exceeded sweep allocation of %d. Adding 20 more.\n", isweep);
      new_volume = RSL_new_volume(radar->v[ifield]->h.nsweeps+20);
      new_volume = copy_sweeps_into_volume(new_volume, radar->v[ifield]);
      radar->v[ifield] = new_volume;
    }

    if (radar->v[ifield]->sweep[isweep] == NULL) {
      if (radar_verbose_flag)
        fprintf(stderr,"Allocating new sweep for field %d, isweep %d\n", ifield, isweep);
      radar->v[ifield]->sweep[isweep] = RSL_new_sweep(1000);
      radar->v[ifield]->sweep[isweep]->h.nrays = 0; /* Increment this for each
                                                     * ray encountered.
                                                     */
      radar->v[ifield]->h.f = f;
      radar->v[ifield]->h.invf = invf;
      radar->v[ifield]->sweep[isweep]->h.f = f;
      radar->v[ifield]->sweep[isweep]->h.invf = invf;
      radar->v[ifield]->sweep[isweep]->h.sweep_num = uf_ma[9];
      radar->v[ifield]->sweep[isweep]->h.elev = uf_ma[35] / 64.0;
    }
    
    

    current_fh_index = uf_dh[4+2*i];
    uf_fh = uf + current_fh_index - 1;
    sweep = radar->v[ifield]->sweep[isweep];
    iray =  sweep->h.nrays;
    nbins = uf_fh[5];
    radar->v[ifield]->sweep[isweep]->ray[iray] = RSL_new_ray(nbins);
    ray   = radar->v[ifield]->sweep[isweep]->ray[iray];
    sweep->h.nrays += 1;


    if (ray) {
        /* 
         * ---- Beginning of MANDATORY HEADER BLOCK.
         */
      ray->h.ray_num = uf_ma[7];
      if (little_endian()) swap2(&uf_ma[10], 8);
      memcpy(radar->h.radar_name, &uf_ma[10], 8);
      if (little_endian()) swap2(&uf_ma[10], 8/2);
      memcpy(radar->h.name, &uf_ma[14], 8);
      if (little_endian()) swap2(&uf_ma[14], 8/2);
        
      /* All components of lat/lon are the same sign.  If not, then
       * what ever wrote the UF was in error.  A simple RSL program
       * can repair the damage, however, not here.
       */
      ray->h.lat = uf_ma[18] + uf_ma[19]/60.0 + uf_ma[20]/64.0/3600;
      ray->h.lon = uf_ma[21] + uf_ma[22]/60.0 + uf_ma[23]/64.0/3600;
      ray->h.alt      = uf_ma[24];
      ray->h.year     = uf_ma[25];
      if (ray->h.year < 1900) {
        ray->h.year += 1900;
        if (ray->h.year < 1980) ray->h.year += 100; /* Year >= 2000. */
      }
      ray->h.month    = uf_ma[26];
      ray->h.day      = uf_ma[27];
      ray->h.hour     = uf_ma[28];
      ray->h.minute   = uf_ma[29];
      ray->h.sec      = uf_ma[30];
      ray->h.azimuth  = uf_ma[32] / 64.0;

      /* If Local Use Header is present and contains azimuth, use that
       * azimuth for VR and SW. This is for WSR-88D, which runs separate
       * scans for DZ and VR/SW at the lower elevations, which means DZ
       * VR/SW and have different azimuths in the "same" ray.
       */
      len_lu = uf_ma[4] - uf_ma[3];
      if (len_lu == 2 && (ifield == VR_INDEX || ifield == SW_INDEX)) {
          if (strncmp((char *)uf_lu,"ZA",2) == 0 ||
              strncmp((char *)uf_lu,"AZ",2) == 0)
          ray->h.azimuth = uf_lu[1] / 64.0;
      }
      if (ray->h.azimuth < 0.) ray->h.azimuth += 360.; /* make it 0 to 360. */
      ray->h.elev     = uf_ma[33] / 64.0;
      ray->h.elev_num = sweep->h.sweep_num;
      ray->h.fix_angle  = sweep->h.elev = uf_ma[35] / 64.0;
      ray->h.azim_rate  = uf_ma[36] / 64.0;
      ray->h.sweep_rate = ray->h.azim_rate * (60.0/360.0);
      missing_data      = uf_ma[44];

      if (pulled_time_from_first_ray == 0) {
        radar->h.height = uf_ma[24];
        radar->h.latd = uf_ma[18];
        radar->h.latm = uf_ma[19];
        radar->h.lats = uf_ma[20] / 64.0;
        radar->h.lond = uf_ma[21];
        radar->h.lonm = uf_ma[22];
        radar->h.lons = uf_ma[23] / 64.0;
        /* Note that radar header time is now handled at end of ingest by
         * function put_start_time_in_radar_header().  The values below are
         * replaced. --BLK, 6/19/13
         */
        radar->h.year  = ray->h.year;
        radar->h.month = ray->h.month;
        radar->h.day   = ray->h.day;
        radar->h.hour  = ray->h.hour;
        radar->h.minute = ray->h.minute;
        radar->h.sec    = ray->h.sec;
        strcpy(radar->h.radar_type, "uf");
        pulled_time_from_first_ray = 1;
      }
      /*
       * ---- End of MANDATORY HEADER BLOCK.
       */
      
      /* ---- Optional header used for MCTEX files. */
        /* If this is a MCTEX file, the first 4 words following the
             mandatory header contain the string 'MCTEX'. */
      memcpy(proj_name, (short *)(uf + uf_ma[2] - 1), 8);
      if (little_endian()) swap2(proj_name, 4);


      /* ---- Local Use Header (if present) was checked during Mandatory
       *      Header processing above.
       */
      
      /* ---- Begining of FIELD HEADER. */
      uf_fh = uf+current_fh_index - 1;
      scale_factor      = uf_fh[1];
      ray->h.range_bin1 = uf_fh[2] * 1000.0 + uf_fh[3]; 
      ray->h.gate_size  = uf_fh[4];

      ray->h.nbins      = uf_fh[5];
      ray->h.pulse_width  = uf_fh[6]/(RSL_SPEED_OF_LIGHT/1.0e6);

        if (strncmp((char *)proj_name, "MCTEX", 5) == 0)  /* MCTEX? */
        {
            /* The beamwidth values are not correct in Mctex UF files. */
            ray->h.beam_width = 1.0;
            sweep->h.beam_width = ray->h.beam_width;
            sweep->h.horz_half_bw = ray->h.beam_width/2.0;
            sweep->h.vert_half_bw = ray->h.beam_width/2.0;
        }
        else  /* Not MCTEX */
        {
            ray->h.beam_width = uf_fh[7] / 64.0;
            sweep->h.beam_width = uf_fh[7]  / 64.0;
            sweep->h.horz_half_bw = uf_fh[7] / 128.0; /* DFF 4/4/95 */
            sweep->h.vert_half_bw = uf_fh[8] / 128.0; /* DFF 4/4/95 */
        }           
        /*      fprintf (stderr, "uf_fh[7] = %d, [8] = %d\n", (int)uf_fh[7], (int)uf_fh[8]); */
        if((int)uf_fh[7] == -32768) {
            ray->h.beam_width     = 1;
            sweep->h.beam_width   = 1;
            sweep->h.horz_half_bw = .5;
            sweep->h.vert_half_bw = .5;
        }
          
      frequency = uf_fh[9];
      /* This corrects an error in v1.43 and earlier where frequency was
       * multiplied by 64.  Correct units for UF are MHz; radar structure
       * uses GHz.
       */
      if (frequency < 1000.) frequency = frequency/64.;
      else frequency = frequency/1000.;
      ray->h.frequency    = frequency;
      ray->h.wavelength   = uf_fh[11] / 64.0 / 100.0;  /* cm to m. */
      ray->h.pulse_count  = uf_fh[12];
      if (ifield == DZ_INDEX || ifield == ZT_INDEX) {
        radar->v[ifield]->h.calibr_const  = uf_fh[16] / 100.0;
                            /* uf value scaled by 100 */
      }
      else {
        radar->v[ifield]->h.calibr_const  = 0.0;
      }
      if (uf_fh[17] == /*(short)*/UF_NO_DATA) x = 0;
      else x = uf_fh[17] / 1000000.0;  /* PRT in seconds. */
      if (x != 0) {
        ray->h.prf = 1/x;
        ray->h.unam_rng = RSL_SPEED_OF_LIGHT / (2.0 * ray->h.prf * 1000.0);
      }
      else {
        ray->h.prf = 0.0;
        ray->h.unam_rng = 0.0;
      }

      if (VR_INDEX == ifield || VE_INDEX == ifield) {
        ray->h.nyq_vel = uf_fh[19] / scale_factor;
      }
      
      /* ---- End of FIELD HEADER. */
      
      ray->h.f = f;
      ray->h.invf = invf;

      /* ---- Begining of FIELD DATA. */
      uf_data = uf+uf_fh[0] - 1;

      len_data = ray->h.nbins;  /* Known because of RSL_new_ray. */
      for (m=0; m<len_data; m++) {
        if (uf_data[m] == /*(short)*/UF_NO_DATA)
          ray->range[m] = invf(BADVAL); /* BADVAL */
        else {
          if(uf_data[m] == missing_data)
            ray->range[m] = invf(NOECHO); /* NOECHO */
          else
            ray->range[m] = invf((float)uf_data[m]/scale_factor);
        }
      }
    }
  }
  return UF_MORE;
}


/*********************************************************************/
/*                                                                   */
/*                  swap_uf_buffer                                   */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      Space Applications Corporation                               */
/*      October 4, 1994                                              */
/*********************************************************************/
void swap_uf_buffer(UF_buffer uf)
{
  short *addr_end;

  addr_end = uf + sizeof(UF_buffer)/sizeof(short);
  while (uf < addr_end)
    swap_2_bytes(uf++);
}

enum UF_type {NOT_UF, TRUE_UF, TWO_BYTE_UF, FOUR_BYTE_UF};


/*********************************************************************/
/*                                                                   */
/*                  RSL_uf_to_radar_fp                               */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      Space Applications Corporation                               */
/*      September 22, 1995                                           */
/*********************************************************************/
Radar *RSL_uf_to_radar_fp(FILE *fp)
{
  union {
    char buf[6];
    short sword;
    int word;
  } magic;
  Radar *radar;
  int nbytes;
  short sbytes;
  UF_buffer uf;
  enum UF_type uf_type;
#define NEW_BUFSIZ 16384

  radar = NULL;
  pulled_time_from_first_ray = 0;
  need_scan_mode = 1;

  /* setvbuf(fp,NULL,_IOFBF,(size_t)NEW_BUFSIZ); * Faster i/o? */
  if (fread(magic.buf, sizeof(char), 6, fp) <= 0) return NULL;
/*
 * Check for fortran record length delimeters, NCAR kludge.
 */
  if (strncmp("UF", magic.buf, 2) == 0) uf_type = TRUE_UF;
  else if (strncmp("UF", &magic.buf[2], 2) == 0) uf_type = TWO_BYTE_UF;
  else if (strncmp("UF", &magic.buf[4], 2) == 0) uf_type = FOUR_BYTE_UF;
  else uf_type = NOT_UF;
  
  switch (uf_type) {
  case FOUR_BYTE_UF:
    if (radar_verbose_flag) fprintf(stderr,"UF file with 4 byte FORTRAN record delimeters.\n");
    /* Handle first record specially, since we needed magic information. */
    nbytes = magic.word;
    if (little_endian()) swap_4_bytes(&nbytes);
    if (nbytes > sizeof(UF_buffer)) {
        fprintf(stderr,"\nRSL_uf_to_radar_fp: Record size (%d bytes) exceeds "
                "UF_buffer (%d bytes).\n", nbytes,(int)sizeof(UF_buffer));
        fprintf(stderr,"Increase size of UF_buffer in uf_to_radar.c\n");
        return NULL;
    }
    memcpy(uf, &magic.buf[4], 2);
    (void)fread(&uf[1], sizeof(char), nbytes-2, fp);
    if (little_endian()) swap_uf_buffer(uf);
    (void)fread(&nbytes, sizeof(int), 1, fp);
    if (uf_into_radar(uf, &radar) == UF_DONE) break;
    /* Now the rest of the file. */
    while(fread(&nbytes, sizeof(int), 1, fp) > 0) {
      if (little_endian()) swap_4_bytes(&nbytes);
      
      (void)fread(uf, sizeof(char), nbytes, fp);
      if (little_endian()) swap_uf_buffer(uf);
      
      (void)fread(&nbytes, sizeof(int), 1, fp);
      
      if (uf_into_radar(uf, &radar) == UF_DONE) break;
    }
    break;

  case TWO_BYTE_UF:
    if (radar_verbose_flag) fprintf(stderr,"UF file with 2 byte FORTRAN record delimeters.\n");
    /* Handle first record specially, since we needed magic information. */
    sbytes = magic.sword;
    if (little_endian()) swap_2_bytes(&sbytes);
    if (sbytes > sizeof(UF_buffer)) {
        fprintf(stderr,"\nRSL_uf_to_radar_fp: Record size (%d bytes) exceeds "
                "UF_buffer (%d bytes).\n", sbytes,(int)sizeof(UF_buffer));
        fprintf(stderr,"Increase size of UF_buffer in uf_to_radar.c\n");
        return NULL;
    }
    memcpy(uf, &magic.buf[2], 4);
    (void)fread(&uf[2], sizeof(char), sbytes-4, fp);
    if (little_endian()) swap_uf_buffer(uf);
    (void)fread(&sbytes, sizeof(short), 1, fp);
    uf_into_radar(uf, &radar);
    /* Now the rest of the file. */
    while(fread(&sbytes, sizeof(short), 1, fp) > 0) {
      if (little_endian()) swap_2_bytes(&sbytes);
      
      (void)fread(uf, sizeof(char), sbytes, fp);
      if (little_endian()) swap_uf_buffer(uf);
      
      (void)fread(&sbytes, sizeof(short), 1, fp);
      
      if (uf_into_radar(uf, &radar) == UF_DONE) break;
    }
    break;

  case TRUE_UF:
    if (radar_verbose_flag) fprintf(stderr,"UF file with no FORTRAN record delimeters.  Good.\n");
    /* Handle first record specially, since we needed magic information. */
    memcpy(&sbytes, &magic.buf[2], 2); /* Record length is in word #2. */
    if (little_endian()) swap_2_bytes(&sbytes); /* # of 2 byte words. */
    if (sbytes > sizeof(UF_buffer)) {
        fprintf(stderr,"\nRSL_uf_to_radar_fp: Record size (%d bytes) exceeds "
                "UF_buffer (%d bytes).\n", sbytes,(int)sizeof(UF_buffer));
        fprintf(stderr,"Increase size of UF_buffer in uf_to_radar.c\n");
        return NULL;
    }
    memcpy(uf, &magic.buf[0], 6);
    (void)fread(&uf[3], sizeof(short), sbytes-3, fp);
    if (little_endian()) swap_uf_buffer(uf);
    uf_into_radar(uf, &radar);
    /* Now the rest of the file. */
    while(fread(uf, sizeof(short), 2, fp) > 0) {
      memcpy(&sbytes, &uf[1], 2);  /* Record length is in word #2. */
      if (little_endian()) swap_2_bytes(&sbytes);
      
      (void)fread(&uf[2], sizeof(short), sbytes-2, fp);  /* Have words 1,2. */
      if (little_endian()) swap_uf_buffer(uf);
      
      if (uf_into_radar(uf, &radar) == UF_DONE) break;
    }
    break;
    
  case NOT_UF: return NULL; break;
  }
  radar = reset_nsweeps_in_all_volumes(radar);
  put_start_time_in_radar_header(radar);
  radar = RSL_prune_radar(radar);

  return radar;
}


/*********************************************************************/
/*                                                                   */
/*                  RSL_uf_to_radar                                  */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      Space Applications Corporation                               */
/*      September 22, 1995                                           */
/*********************************************************************/
Radar *RSL_uf_to_radar(char *infile)
{
/*
 * This routine ingests a UF file and fills the Radar structure.
 * This routine allocates space via the system routine malloc.
 *
 * If *infile is NULL, read from stdin.
 */
  FILE *fp;
  Radar *radar;
  
  radar = NULL;
  if (infile == NULL) {
    //int save_fd;
    //save_fd = dup(0);
    //fp = fdopen(save_fd, "r");
    fp = stdin;
  }  else if ((fp = fopen(infile, "r")) == NULL) {
    perror(infile);
    return radar;
  }
  fp = uncompress_pipe(fp); /* Transparently gunzip. */
  radar = RSL_uf_to_radar_fp(fp);
  rsl_pclose(fp);
    
  return radar;
}
