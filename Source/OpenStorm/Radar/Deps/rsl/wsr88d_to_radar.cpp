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

#include "rsl.h"
#include "wsr88d.h"


#ifdef _WIN32
#define strdup _strdup
#endif

extern int radar_verbose_flag;
/*
 * These externals can be found in the wsr88d library; secret code.
 */
void print_head(Wsr88d_file_header wsr88d_file_header);
void clear_sweep(Wsr88d_sweep *wsr88d_sweep, int x, int n);
void free_and_clear_sweep(Wsr88d_sweep *wsr88d_sweep, int x, int n);
/*
 * Secretly in uf_to_radar.c
 */
Volume *copy_sweeps_into_volume(Volume *new_volume, Volume *old_volume);


Radar *wsr88d_load_m31_into_radar(Wsr88d_file *wf);

/* Function to specify keeping the extra split-cut inserted into middle of
 * volume scan when SAILS is in effect for VCPs 12 and 212.
 */
static int keep_sails = 0;
void RSL_wsr88d_keep_sails()
{
		keep_sails = 1;
}

/* Function to specify that all sweeps are to be stored as read.  Don't combine
 * split-cuts by elevation or remove short-range reflectivity in Doppler cuts.
 */
void RSL_wsr88d_asis()
{
		RSL_wsr88d_merge_split_cuts_off();
		RSL_wsr88d_keep_sails();
}

void float_to_range(float *x, Range *c, int n, Range (*function)(float x) )
{
	while (n--) {
		if (*x == WSR88D_BADVAL) *c = function(BADVAL);
		else if (*x == WSR88D_RFVAL) *c = function(RFVAL);
		else *c = function(*x);
		c++; x++;
	}
}


void wsr88d_remove_sails_sweep(Radar* radar)
{
		/* Remove SAILS sweeps.  For VCPs 12 and 212 only. */

		int i, j;
		int sails_loc[4];
		int isails, nsails;

		if (radar->h.vcp != 12 && radar->h.vcp != 212) return;

		for (j = 0; j < MAX_RADAR_VOLUMES; j++) {
				if (radar->v[j]) {
						/* Find and save SAILS indices. */
						nsails = 0;
						for (i = 1; i < radar->v[j]->h.nsweeps; i++) {
								/* If this sweep's elevation is less than previous sweep,
									 remove this sweep. */
								if (radar->v[j]->sweep[i]->h.elev <
										radar->v[j]->sweep[i - 1]->h.elev) {
										sails_loc[nsails++] = i;
								}
						}
						/* Remove sweeps at SAILS indices. */
						if (nsails > 0) {
								for (isails = 0; isails < nsails; isails++) {
										RSL_free_sweep(radar->v[j]->sweep[sails_loc[isails]]);
										radar->v[j]->sweep[sails_loc[isails]] = NULL;
								}
						}
				} /* if radar->v[j] not NULL */
		} /* for volumes */
		/* Push down the sweeps to remove gaps in radar structure. */
		if (nsails > 0) {
				radar = RSL_prune_radar(radar);
				fprintf(stderr, "Removed %d SAILS sweep%s.\n", nsails,
						(nsails > 1) ? "s" : "");  /* Thanks K&R */
				fprintf(stderr, "Call RSL_keep_sails() before RSL_anyformat_to_radar() "
						"to keep SAILS sweeps.\n");
		}
}



/**********************************************************************/
/*                                                                    */
/* done 3/23         wsr88d_load_sweep_into_volume                    */
/*                                                                    */
/*  By: John Merritt                                                  */
/*      Space Applications Corporation                                */
/*      March 3, 1994                                                 */
/**********************************************************************/
int wsr88d_load_sweep_into_volume(Wsr88d_sweep ws,
											 Volume *v, int nsweep, unsigned int vmask)
{
	int i;
	int iray;
	float v_data[1000];
	Range  c_data[1000];
	int n;

	int mon, day, year;
	int hh, mm, ss;
	float fsec;
	int vol_cpat;

	Ray *ray_ptr;
	Range (*invf)(float x);
	float (*f)(Range x);

	/* Allocate memory for MAX_RAYS_IN_SWEEP rays. */
	v->sweep[nsweep] = RSL_new_sweep(MAX_RAYS_IN_SWEEP);
	if (v->sweep[nsweep] == NULL) {
		perror("wsr88d_load_sweep_into_volume: RSL_new_sweep");
		return -1;
	}

	v->sweep[nsweep]->h.elev = 0;
	v->sweep[nsweep]->h.nrays = 0;
	f = (float (*)(Range x))NULL;
	invf = (Range (*)(float x))NULL;
	if (vmask & WSR88D_DZ) { invf = DZ_INVF; f = DZ_F; }
	if (vmask & WSR88D_VR) { invf = VR_INVF; f = VR_F; }
	if (vmask & WSR88D_SW) { invf = SW_INVF; f = SW_F; }
	
	v->h.invf = invf;
	v->h.f    = f;
	v->sweep[nsweep]->h.invf = invf;
	v->sweep[nsweep]->h.f    = f;

	for (i=0,iray=0; i<MAX_RAYS_IN_SWEEP; i++) {
		if (ws.ray[i] != NULL) {
			wsr88d_ray_to_float(ws.ray[i], vmask, v_data, &n);
			float_to_range(v_data, c_data, n, invf);
			if (n > 0) {
				wsr88d_get_date(ws.ray[i], &mon, &day, &year);
				wsr88d_get_time(ws.ray[i], &hh, &mm, &ss, &fsec);
				/*
			fprintf(stderr,"n %d, mon %d, day %d, year %d,  hour %d, min %d, sec %d, fsec %f\n",
						n, mon, day, year, hh, mm, ss, fsec);
						*/
				/*
				 * Load the sweep/ray headar information.
				 */
				
				v->sweep[nsweep]->ray[iray] = RSL_new_ray(n);
				/*(Range *)calloc(n, sizeof(Range)); */
				
				ray_ptr = v->sweep[nsweep]->ray[iray]; /* Make code below readable. */
				ray_ptr->h.f        = f;
				ray_ptr->h.invf     = invf;
				ray_ptr->h.month    = mon;
				ray_ptr->h.day      = day;
				ray_ptr->h.year     = year + 1900; /* Yes 1900 makes this year 2000 compliant, due to wsr88d using unix time(). */
				ray_ptr->h.hour     = hh;
				ray_ptr->h.minute   = mm;
				ray_ptr->h.sec      = ss + fsec;
				ray_ptr->h.unam_rng = wsr88d_get_range   (ws.ray[i]);
				ray_ptr->h.azimuth  = wsr88d_get_azimuth (ws.ray[i]);
/* -180 to +180 is converted to 0 to 360 */
				if (ray_ptr->h.azimuth < 0) ray_ptr->h.azimuth += 360;
				ray_ptr->h.ray_num  = ws.ray[i]->ray_num;
				ray_ptr->h.elev       = wsr88d_get_elevation_angle(ws.ray[i]);
				ray_ptr->h.elev_num   = ws.ray[i]->elev_num;
				if (vmask & WSR88D_DZ) {
					ray_ptr->h.range_bin1 = ws.ray[i]->refl_rng;
					ray_ptr->h.gate_size  = ws.ray[i]->refl_size;
				} else {
					ray_ptr->h.range_bin1 = ws.ray[i]->dop_rng;
					ray_ptr->h.gate_size  = ws.ray[i]->dop_size;
				}
				ray_ptr->h.vel_res  = wsr88d_get_velocity_resolution(ws.ray[i]);
				vol_cpat = wsr88d_get_volume_coverage(ws.ray[i]);
				switch (vol_cpat) {
				case 11: ray_ptr->h.sweep_rate = 16.0/5.0;  break;
				case 12: ray_ptr->h.sweep_rate = 17.0/4.2;  break;
				case 21: ray_ptr->h.sweep_rate = 11.0/6.0;  break;
				case 31: ray_ptr->h.sweep_rate =  8.0/10.0; break;
				case 32: ray_ptr->h.sweep_rate =  7.0/10.0; break;
				case 121:ray_ptr->h.sweep_rate = 20.0/5.5;  break;
				default: ray_ptr->h.sweep_rate =  0.0; break;
				}

				ray_ptr->h.nyq_vel  = wsr88d_get_nyquist(ws.ray[i]);
				ray_ptr->h.azim_rate   = wsr88d_get_azimuth_rate(ws.ray[i]);
				ray_ptr->h.fix_angle   = wsr88d_get_fix_angle(ws.ray[i]);
				ray_ptr->h.pulse_count = wsr88d_get_pulse_count(ws.ray[i]);
				ray_ptr->h.pulse_width = wsr88d_get_pulse_width(ws.ray[i]);
				ray_ptr->h.beam_width  = .95;
				ray_ptr->h.prf         = wsr88d_get_prf(ws.ray[i]);
				ray_ptr->h.frequency   = wsr88d_get_frequency(ws.ray[i]);
				ray_ptr->h.wavelength = 0.1071; /* Previously called
																		 * wsr88d_get_wavelength(ws.ray[i]).
																		 * See wsr88d.c for explanation.
												 */

				/* It is no coincidence that the 'vmask' and wsr88d datatype
				 * values are the same.  We expect 'vmask' to be one of
				 * REFL_MASK, VEL_MASK, or SW_MASK.  These match WSR88D_DZ,
				 * WSR88D_VR, and WSR88D_SW in the wsr88d library.
				 */
				ray_ptr->h.nbins = n;
				memcpy(ray_ptr->range, c_data, n*sizeof(Range));
				v->sweep[nsweep]->h.nrays = iray+1;
				v->sweep[nsweep]->h.elev += ray_ptr->h.elev;
				v->sweep[nsweep]->h.sweep_num = ray_ptr->h.elev_num;
				iray++;
			}
		}
	}
	v->sweep[nsweep]->h.beam_width = .95;
	v->sweep[nsweep]->h.vert_half_bw = .475;
	v->sweep[nsweep]->h.horz_half_bw = .475;
	/* Now calculate the mean elevation angle for this sweep. */
	if (v->sweep[nsweep]->h.nrays > 0)
		v->sweep[nsweep]->h.elev /= v->sweep[nsweep]->h.nrays;
	else {
		RSL_free_sweep(v->sweep[nsweep]); /* No rays loaded, free this sweep. */
		v->sweep[nsweep] = NULL;
	}
	
	return 0;
}

/**********************************************************************/
/*                                                                    */
/*                     RSL_wsr88d_to_radar                            */
/*                                                                    */
/*  By: John Merritt                                                  */
/*      Space Applications Corporation                                */
/*      March 3, 1994                                                 */
/**********************************************************************/

Radar *RSL_wsr88d_to_radar(char *infile, char *call_or_first_tape_file)
/*
 * Gets all volumes from the nexrad file.  Input file is 'infile'.
 * Site information is extracted from 'call_or_first_tape_file'; this
 * is typically a disk file called 'nex.file.1'.
 *
 *  -or-
 *
 * Uses the string in 'call_or_first_tape_file' as the 4 character call sign
 * for the sight.  All UPPERCASE characters.  Normally, this call sign
 * is extracted from the file 'nex.file.1'.
 *
 * Returns a pointer to a Radar structure; that contains the different
 * Volumes of data.
 */
{
	Radar *radar;
	Volume *new_volume;
	Wsr88d_file *wf;
	Wsr88d_sweep wsr88d_sweep;
	Wsr88d_file_header wsr88d_file_header;
	Wsr88d_tape_header wsr88d_tape_header;
	int n;
	int nsweep;
	int i;
	int iv;
	int nvolumes;
	int volume_mask[] = {WSR88D_DZ, WSR88D_VR, WSR88D_SW};
	char *field_str[] = {(char*)"Reflectivity", (char*)"Velocity", (char*)"Spectrum width"};
	Wsr88d_site_info sitepDefualt = {};
	Wsr88d_site_info *sitep;
	char site_id_str[5];
	char *the_file;
	int expected_msgtype = 0;
	char version[8];
	int vnum;

	extern int rsl_qfield[]; /* See RSL_select_fields in volume.c */
	extern int *rsl_qsweep; /* See RSL_read_these_sweeps in volume.c */
	extern int rsl_qsweep_max;

	sitep = NULL;
/* Determine the site quasi automatically.  Here is the procedure:
 *    1. Determine if we have a call sign.
 *    2. Try reading 'call_or_first_tape_file' from disk.  This is done via
 *       wsr88d_read_tape_header.
 *    3. If no valid site info, abort.
 */
	if (call_or_first_tape_file == NULL) {
		//fprintf(stderr, "wsr88d_to_radar: No valid site ID info provided.\n");
		//return(NULL);
	} else if (strlen(call_or_first_tape_file) == 4)
		sitep =  wsr88d_get_site(call_or_first_tape_file);
	else if (strlen(call_or_first_tape_file) == 0) {
		//fprintf(stderr, "wsr88d_to_radar: No valid site ID info provided.\n");
		//return(NULL);
	}  

	if (sitep == NULL)
		if (wsr88d_read_tape_header(call_or_first_tape_file, &wsr88d_tape_header) > 0) {
			memcpy(site_id_str, wsr88d_tape_header.site_id, 4);
			sitep  = wsr88d_get_site(site_id_str);
		}
	if (sitep == NULL) {
			//fprintf(stderr,"wsr88d_to_radar: No valid site ID info found.\n");
				//return(NULL);
			sitep = &sitepDefualt;
	}
		if (radar_verbose_flag)
			fprintf(stderr,"SITE: %c%c%c%c\n", sitep->name[0], sitep->name[1],
						 sitep->name[2], sitep->name[3]);
	
		
	
	memset(&wsr88d_sweep, 0, sizeof(Wsr88d_sweep)); /* Initialize to 0 a 
																									 * heavily used variable.
																									 */

/* 1. Open the input wsr88d file. */
	if (infile == NULL) the_file = (char*)"stdin";  /* wsr88d.c understands this to
																						* mean read from stdin.
																						*/
	else the_file = infile;

	if ((wf = wsr88d_open(the_file)) == NULL) {
		wsr88d_perror(the_file);
		return NULL;
	}



/* 2. Read wsr88d headers. */
	/* Return # bytes, 0 or neg. on fail. */
	n = wsr88d_read_file_header(wf, &wsr88d_file_header);
	/*
	 * Get the expected digital radar message type based on version string
	 * from the Archive II header.  The message type is 31 for Build 10, and 1
	 * for prior builds.
	 */
	if (n > 0) {
			strncpy(version, wsr88d_file_header.title.filename, 8);
			if (strncmp(version,"AR2V",4) == 0) {
					sscanf(version, "AR2V%4d", &vnum);
					if (vnum > 1) expected_msgtype = 31;
					else expected_msgtype = 1;
			}
			else if (strncmp(version,"ARCHIVE2",8) == 0) expected_msgtype = 1;
	}

	if (n <= 0 || expected_msgtype == 0) {
			fprintf(stderr,"RSL_wsr88d_to_radar: ");
			if (n <= 0)
			fprintf(stderr,"wsr88d_read_file_header failed %i\n", n);
			else
			fprintf(stderr,"Archive II header contains unknown version "
					": '%s'\n", version);
			wsr88d_close(wf);
			return NULL;
	}

	if (radar_verbose_flag)
		print_head(wsr88d_file_header);


	if (expected_msgtype == 31) {
			/* Get radar for message type 31. */
			radar = wsr88d_load_m31_into_radar(wf);
			if (radar == NULL) return NULL;
	}
	else {
			/* Get radar for message type 1. */
			nvolumes = 3;
			/* Allocate all Volume pointers. */
			radar = RSL_new_radar(MAX_RADAR_VOLUMES);
			if (radar == NULL) return NULL;

		/* Clear the sweep pointers. */
			clear_sweep(&wsr88d_sweep, 0, MAX_RAYS_IN_SWEEP);

		/* Allocate a maximum of 30 sweeps for the volume. */
		/* Order is important.  WSR88D_DZ, WSR88D_VR, WSR88D_SW, is 
		 * assigned to the indexes DZ_INDEX, VR_INDEX and SW_INDEX respectively.
		 */ 

			for (iv=0; iv<nvolumes; iv++)
				if (rsl_qfield[iv]) radar->v[iv] = RSL_new_volume(20);


		/* LOOP until EOF */
			nsweep = 0;
			for (;(n = wsr88d_read_sweep(wf, &wsr88d_sweep)) > 0; nsweep++) {
				if (rsl_qsweep != NULL) {
					if (nsweep > rsl_qsweep_max) break;
					if (rsl_qsweep[nsweep] == 0) continue;
				}
				if (radar_verbose_flag)  
				fprintf(stderr,"Processing for SWEEP # %d\n", nsweep);

					/*  wsr88d_print_sweep_info(&wsr88d_sweep); */
				
				for (iv=0; iv<nvolumes; iv++) {
					if (rsl_qfield[iv]) {
						/* Exceeded sweep limit.
						 * Allocate more sweeps.
						 * Copy all previous sweeps.
						 */
						if (nsweep >= radar->v[iv]->h.nsweeps) {
							if (radar_verbose_flag)
								fprintf(stderr,"Exceeded sweep allocation of %d. "
										"Adding 20 more.\n", nsweep);
							new_volume = RSL_new_volume(radar->v[iv]->h.nsweeps+20);
							new_volume = copy_sweeps_into_volume(new_volume, radar->v[iv]);
							radar->v[iv] = new_volume;
						}
						if (wsr88d_load_sweep_into_volume(wsr88d_sweep,
							 radar->v[iv], nsweep, volume_mask[iv]) != 0) {
							RSL_free_radar(radar);
							return NULL;
						}
					}
				}
				if (nsweep == 0) {
					/* Get Volume Coverage Pattern number for radar header. */
					i=0;
					while (i < MAX_RAYS_IN_SWEEP && wsr88d_sweep.ray[i] == NULL) i++;
					if (i < MAX_RAYS_IN_SWEEP) radar->h.vcp = wsr88d_get_volume_coverage(
						wsr88d_sweep.ray[i]);
				}

				free_and_clear_sweep(&wsr88d_sweep, 0, MAX_RAYS_IN_SWEEP);
			}

			for (iv=0; iv<nvolumes; iv++) {
				if (rsl_qfield[iv]) {
					radar->v[iv]->h.type_str = strdup(field_str[iv]);
					radar->v[iv]->h.nsweeps = nsweep;
				}
			}
	}
	wsr88d_close(wf);

/*
 * Here we will assign the Radar_header information.  Take most of it
 * from an existing volume's header.  
 */
	radar_load_date_time(radar);  /* Magic :-) */

		radar->h.number = sitep->number;
		memcpy(&radar->h.name, sitep->name, sizeof(sitep->name));
		memcpy(&radar->h.radar_name, sitep->name, sizeof(sitep->name)); /* Redundant */
		memcpy(&radar->h.city, sitep->city, sizeof(sitep->city));
		memcpy(&radar->h.state, sitep->state, sizeof(sitep->state));
		strcpy(radar->h.radar_type, "wsr88d");
		radar->h.latd = sitep->latd;
		radar->h.latm = sitep->latm;
		radar->h.lats = sitep->lats;
		if (radar->h.latd < 0) { /* Degree/min/sec  all the same sign */
			radar->h.latm *= -1;
			radar->h.lats *= -1;
		}
		radar->h.lond = sitep->lond;
		radar->h.lonm = sitep->lonm;
		radar->h.lons = sitep->lons;
		if (radar->h.lond < 0) { /* Degree/min/sec  all the same sign */
			radar->h.lonm *= -1;
			radar->h.lons *= -1;
		}
		radar->h.height = sitep->height;
		radar->h.spulse = sitep->spulse;
		radar->h.lpulse = sitep->lpulse;
		if (sitep != &sitepDefualt) {
			free(sitep);
		}

	if (wsr88d_merge_split_cuts_is_set()) {
			radar = wsr88d_merge_split_cuts(radar);
			if ((radar->h.vcp == 12 || radar->h.vcp == 212) && !keep_sails) 
					wsr88d_remove_sails_sweep(radar);
	}
	return radar;
}
