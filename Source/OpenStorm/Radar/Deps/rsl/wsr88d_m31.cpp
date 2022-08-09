/*
    NASA/TRMM, Code 613
    This is the TRMM Office Radar Software Library.
    Copyright (C) 2008
            Bart Kelley
	    SSAI
	    Lanham, Maryland

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the
    Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
    Boston, MA  02110-1301, USA.
*/


/* 
 * This file contains routines for processing Message Type 31, the digital
 * radar message type introduced in WSR-88D Level II Build 10.  For more
 * information, see the "Interface Control Document for the RDA/RPG" at the
 * WSR-88D Radar Operations Center web site.
 */

#include "rsl.h"
#include "wsr88d.h"
#include <string.h>


#ifdef _WIN32
#define strdup _strdup
#endif

/* Data descriptions in the following data structures are from the "Interface
 * Control Document for the RDA/RPG", Build 10.0 Draft, WSR-88D Radar
 * Operations Center.
 */

typedef struct {
    short rpg[6]; /* 12 bytes inserted by RPG Communications Mgr. Ignored. */
    unsigned short msg_size;  /* Message size for this segment, in halfwords */
    unsigned char  channel;   /* RDA Redundant Channel */
    unsigned char  msg_type;  /* Message type. For example, 31 */
    unsigned short id_seq;    /* Msg seq num = 0 to 7FFF, then roll over to 0 */
    unsigned short msg_date;  /* Modified Julian date from 1/1/70 */
    unsigned int   msg_time;  /* Packet generation time in ms past midnight */
    unsigned short num_segs;  /* Number of segments for this message */
    unsigned short seg_num;   /* Number of this segment */
} Wsr88d_msg_hdr;

typedef struct {
    char radar_id[4];
    unsigned int   ray_time; /* Data collection time in milliseconds past midnight GMT */
    unsigned short ray_date; /* Julian date - 2440586.5 (1/01/1970) */
    unsigned short azm_num ; /* Radial number within elevation scan */
    float azm;      /* Azimuth angle in degrees (0 to 359.956055) */
    unsigned char compression_code; /* 0 = uncompressed, 1 = BZIP2, 2 = zlib */
    unsigned char spare; /* for word alignment */
    unsigned short radial_len; /* radial length in bytes, including data header block */
    unsigned char azm_res;
    unsigned char radial_status;
    unsigned char elev_num;
    unsigned char sector_cut_num;
    float elev;  /* Elevation angle in degrees (-7.0 to 70.0) */
    unsigned char radial_spot_blanking;
    unsigned char azm_indexing_mode;
    unsigned short data_block_count;
    /* Data Block Indexes */
    unsigned int vol_const;
    unsigned int elev_const;
    unsigned int radial_const;
    unsigned int field1;
    unsigned int field2;
    unsigned int field3;
    unsigned int field4;
    unsigned int field5;
    unsigned int field6;
} Ray_header_m31;  /* Called Data Header Block in RDA/RPG document. */

typedef struct {
    char dataname[4];
    unsigned int  reserved;
    unsigned short ngates;
    short range_first_gate;
    short range_samp_interval;
    short thresh_not_overlayed;
    short snr_thresh;
    unsigned char controlflag;
    unsigned char datasize_bits;
    float scale;
    float offset;
} Data_moment_hdr;

#define MAX_RADIAL_LENGTH 14288

typedef struct {
    Ray_header_m31 ray_hdr;
    float unamb_rng;
    float nyq_vel;
    unsigned char data[MAX_RADIAL_LENGTH];
} Wsr88d_ray_m31;



void wsr88d_swap_m31_hdr(Wsr88d_msg_hdr *msghdr)
{
    swap_2_bytes(&msghdr->msg_size);
    swap_2_bytes(&msghdr->id_seq);
    swap_2_bytes(&msghdr->msg_date);
    swap_4_bytes(&msghdr->msg_time);
    swap_2_bytes(&msghdr->num_segs);
    swap_2_bytes(&msghdr->seg_num);
}


void wsr88d_swap_m31_ray_hdr(Ray_header_m31 *ray_hdr)
{
    int *data_ptr;

    swap_4_bytes(&ray_hdr->ray_time);
    swap_2_bytes(&ray_hdr->ray_date);
    swap_2_bytes(&ray_hdr->azm_num);
    swap_4_bytes(&ray_hdr->azm);
    swap_2_bytes(&ray_hdr->radial_len);
    swap_4_bytes(&ray_hdr->elev);
    swap_2_bytes(&ray_hdr->data_block_count);
    data_ptr = (int *) &ray_hdr->vol_const;
    for (; data_ptr <= (int *) &ray_hdr->field6; data_ptr++)
	swap_4_bytes(data_ptr);
}


void wsr88d_swap_data_hdr(Data_moment_hdr *this_field)
{
    short *halfword;
    halfword = (short *) &this_field->ngates;
    for (; halfword < (short *) &this_field->controlflag; halfword++)
	swap_2_bytes(halfword);
    swap_4_bytes(&this_field->scale);
    swap_4_bytes(&this_field->offset);
}


float wsr88d_get_angle(short bitfield)
{
    short mask = 1;
    int i;
    float angle = 0.;
    float value[13] = {0.043945, 0.08789, 0.17578, 0.35156, .70313, 1.40625,
	2.8125, 5.625, 11.25, 22.5, 45., 90., 180.};

    /* Find which bits are set and sum corresponding values to get angle. */

    bitfield = bitfield >> 3;  /* 3 least significant bits aren't used. */
    for (i = 0; i < 13; i++) {
	if (bitfield & mask) angle += value[i];
	bitfield = bitfield >> 1;
    }
    return angle;
}


float wsr88d_get_azim_rate(short bitfield)
{
    short mask = 1;
    int i;
    float rate = 0.;
    float value[12] = {0.0109863, 0.021972656, 0.043945, 0.08789, 0.17578,
	0.35156, .70313, 1.40625, 2.8125, 5.625, 11.25, 22.5};

    /* Find which bits are set and sum corresponding values to get rate. */

    bitfield = bitfield >> 3;  /* 3 least significant bits aren't used. */
    for (i = 0; i < 12; i++) {
	if (bitfield & mask) rate += value[i];
	bitfield = bitfield >> 1;
    }
    if (bitfield >> 15) rate = -rate;
    return rate;
}

#define WSR88D_MAX_SWEEPS 30

typedef struct {
    int vcp;
    int num_cuts;
    float vel_res;
    float fixed_angle[WSR88D_MAX_SWEEPS];
    float azim_rate[WSR88D_MAX_SWEEPS];
    int waveform[WSR88D_MAX_SWEEPS];
    int super_res_ctrl[WSR88D_MAX_SWEEPS];
    int surveil_prf_num[WSR88D_MAX_SWEEPS];
    int doppler_prf_num[WSR88D_MAX_SWEEPS];
} VCP_data;

//static VCP_data vcp_data;

void wsr88d_get_vcp_data(short *msgtype5, VCP_data* vcp_data)
{
    short azim_rate, fixed_angle, vel_res;
    short sres_and_survprf; /* super res ctrl and surveil prf, one byte each */
    short chconf_and_waveform;
    int i;
    
    vcp_data->vcp = (unsigned short) msgtype5[2];
    vcp_data->num_cuts = msgtype5[3];
    if (little_endian()) {
        swap_2_bytes(&vcp_data->vcp);
        swap_2_bytes(&vcp_data->num_cuts);
    }
    vel_res = msgtype5[5];
    if (little_endian()) swap_2_bytes(&vel_res);
    vel_res = vel_res >> 8;
    if (vel_res == 2) vcp_data->vel_res = 0.5;
    else if (vel_res == 4) vcp_data->vel_res = 1.0;
    else vcp_data->vel_res = 0.0;
    /* Get elevation related information for each sweep. */
    for (i=0; i < vcp_data->num_cuts; i++) {
        fixed_angle = msgtype5[11 + i*23];
        azim_rate = msgtype5[15 + i*23];
        chconf_and_waveform = msgtype5[12 + i*23];
        sres_and_survprf = msgtype5[13 + i*23];
        vcp_data->doppler_prf_num[i] = msgtype5[23 + i*23];
        if (little_endian()) {
            swap_2_bytes(&fixed_angle);
            swap_2_bytes(&azim_rate);
            swap_2_bytes(&chconf_and_waveform);
            swap_2_bytes(&sres_and_survprf);
            swap_2_bytes(&vcp_data->doppler_prf_num[i]);
        }
        vcp_data->fixed_angle[i] = wsr88d_get_angle(fixed_angle);
        vcp_data->azim_rate[i] = wsr88d_get_azim_rate(azim_rate);
        vcp_data->waveform[i] = chconf_and_waveform & 0xff;
        vcp_data->super_res_ctrl[i] = sres_and_survprf >> 8;
        vcp_data->surveil_prf_num[i] = sres_and_survprf & 0xff;
    }
}


void get_wsr88d_unamb_and_nyq_vel(Wsr88d_ray_m31 *wsr88d_ray, float *unamb_rng,
	float *nyq_vel)
{
    int dindex, found;
    short nyq_vel_sh, unamb_rng_sh;

    found = 0;
    dindex = wsr88d_ray->ray_hdr.radial_const;
    if (strncmp((char *) &wsr88d_ray->data[dindex], "RRAD", 4) == 0) found = 1;
    else {
	dindex = wsr88d_ray->ray_hdr.elev_const;
	if (strncmp((char *) &wsr88d_ray->data[dindex], "RRAD", 4) == 0)
	    found = 1;
	else {
	    dindex = wsr88d_ray->ray_hdr.vol_const;
	    if (strncmp((char *) &wsr88d_ray->data[dindex], "RRAD", 4) == 0)
		found = 1;
	}
    }
    if (found) {
	memcpy(&unamb_rng_sh, &wsr88d_ray->data[dindex+6], 2);
	memcpy(&nyq_vel_sh, &wsr88d_ray->data[dindex+16], 2);
	if (little_endian()) {
	    swap_2_bytes(&unamb_rng_sh);
	    swap_2_bytes(&nyq_vel_sh);
	}
	*unamb_rng = unamb_rng_sh / 10.;
	*nyq_vel = nyq_vel_sh / 100.;
    } else {
	*unamb_rng = 0.;
	*nyq_vel = 0.;
    }
}


int read_wsr88d_ray_m31(Wsr88d_file *wf, int msg_size,
	Wsr88d_ray_m31 *wsr88d_ray)
{
    int n;
    float nyq_vel, unamb_rng;

    /* Read wsr88d ray. */

    n = fread(wsr88d_ray->data, msg_size, 1, wf->fptr);
    if (n < 1) {
	fprintf(stderr,"read_wsr88d_ray_m31: Read failed.\n");
	return 0;
    }

    /* Copy data header block to ray header structure. */
    memcpy(&wsr88d_ray->ray_hdr, &wsr88d_ray->data, sizeof(Ray_header_m31));

    if (little_endian()) wsr88d_swap_m31_ray_hdr(&wsr88d_ray->ray_hdr);

    /* Retrieve unambiguous range and Nyquist velocity here so that we don't
     * have to do it for each data moment later.
     */
    get_wsr88d_unamb_and_nyq_vel(wsr88d_ray, &unamb_rng, &nyq_vel);
    wsr88d_ray->unamb_rng = unamb_rng;
    wsr88d_ray->nyq_vel = nyq_vel;

    return 1;
}


void wsr88d_load_ray_hdr(Wsr88d_ray_m31 *wsr88d_ray, Ray *ray, VCP_data* vcp_data)
{
    int month, day, year, hour, minute, sec;
    float fsec;
    Wsr88d_ray m1_ray;
    Ray_header_m31 ray_hdr;

    ray_hdr = wsr88d_ray->ray_hdr;
    m1_ray.ray_date = ray_hdr.ray_date;
    m1_ray.ray_time = ray_hdr.ray_time;

    wsr88d_get_date(&m1_ray, &month, &day, &year);
    wsr88d_get_time(&m1_ray, &hour, &minute, &sec, &fsec);
    ray->h.year = year + 1900;
    ray->h.month = month;
    ray->h.day = day;
    ray->h.hour = hour;
    ray->h.minute = minute;
    ray->h.sec = sec + fsec;
    ray->h.azimuth = ray_hdr.azm;
    ray->h.ray_num = ray_hdr.azm_num;
    ray->h.elev = ray_hdr.elev;
    ray->h.elev_num = ray_hdr.elev_num;
    ray->h.unam_rng = wsr88d_ray->unamb_rng;
    ray->h.nyq_vel = wsr88d_ray->nyq_vel;
    int elev_index;
    elev_index = ray_hdr.elev_num - 1;
    ray->h.azim_rate = vcp_data->azim_rate[elev_index];
    ray->h.fix_angle = vcp_data->fixed_angle[elev_index];
    ray->h.vel_res = vcp_data->vel_res;
    if (ray_hdr.azm_res != 1)
	ray->h.beam_width = 1.0;
    else ray->h.beam_width = 0.5;

    /* For convenience, use message type 1 routines to get some values.
     * First load VCP and elevation numbers into a msg 1 ray.
     */
    m1_ray.vol_cpat = vcp_data->vcp;
    m1_ray.elev_num = ray_hdr.elev_num;
    m1_ray.unam_rng = (short) (wsr88d_ray->unamb_rng * 10.);
    m1_ray.nyq_vel = (short) wsr88d_ray->nyq_vel;
    /* Get values from message type 1 routines. */
    ray->h.frequency = wsr88d_get_frequency(&m1_ray);
    ray->h.pulse_width = wsr88d_get_pulse_width(&m1_ray);
    ray->h.pulse_count = wsr88d_get_pulse_count(&m1_ray);
    ray->h.prf = (int) wsr88d_get_prf(&m1_ray);
    ray->h.wavelength = 0.1071;
}


int wsr88d_get_vol_index(char* dataname)
{
    if (strncmp(dataname, "DREF", 4) == 0) return DZ_INDEX;
    if (strncmp(dataname, "DVEL", 4) == 0) return VR_INDEX;
    if (strncmp(dataname, "DSW",  3) == 0) return SW_INDEX;
    if (strncmp(dataname, "DZDR", 4) == 0) return DR_INDEX;
    if (strncmp(dataname, "DPHI", 4) == 0) return PH_INDEX;
    if (strncmp(dataname, "DRHO", 4) == 0) return RH_INDEX;

    return -1;
}

#define MAXRAYS_M31 800
#define MAXSWEEPS 30

void wsr88d_load_ray_into_radar(Wsr88d_ray_m31 *wsr88d_ray, int isweep,
	Radar *radar, VCP_data* vcp_data)
{
    /* Load data into ray structure for each data field. */

    int data_index;
    int *field_offset;
    int ifield, nfields;
    int iray;
    const int nconstblocks = 3;

    Data_moment_hdr data_hdr;
    int ngates, do_swap;
    int i, hdr_size;
    unsigned short item;
    float value, scale, offset;
    unsigned char *data;
    Range (*invf)(float x) = nullptr;
    float (*f)(Range x) = nullptr;
    Ray *ray;
    int vol_index, waveform;

    extern int rsl_qfield[]; /* See RSL_select_fields in volume.c */

    enum waveforms {surveillance=1, doppler_w_amb_res, doppler_no_amb_res,
	batch};

    int merging_split_cuts;

    merging_split_cuts =  wsr88d_merge_split_cuts_is_set();
    nfields = wsr88d_ray->ray_hdr.data_block_count - nconstblocks;
    field_offset = (int *) &wsr88d_ray->ray_hdr.radial_const;
    do_swap = little_endian();
    iray = wsr88d_ray->ray_hdr.azm_num - 1;

    for (ifield=0; ifield < nfields; ifield++) {
	    field_offset++;
	    data_index = *field_offset;
	    /* Get data moment header. */
	    hdr_size = sizeof(data_hdr);
        //fprintf(stderr, "%i %i %i\n", data_index, ifield, nfields);
        if (data_index + hdr_size <= MAX_RADIAL_LENGTH) {
            memcpy(&data_hdr, &wsr88d_ray->data[data_index], hdr_size);
        } else {
#ifdef VERBOSE
            fprintf(stderr, "wsr88d_load_ray_into_radar: field_offset too large %i\n", data_index);
#endif
            return;
        }
	    if (do_swap) wsr88d_swap_data_hdr(&data_hdr);
	    data_index += hdr_size;

	    vol_index = wsr88d_get_vol_index(data_hdr.dataname);
	    if (vol_index < 0) {
#ifdef VERBOSE
	        fprintf(stderr,"wsr88d_load_ray_into_radar: Unknown dataname %s.  "
		        "isweep = %d, iray = %d.\n", data_hdr.dataname, isweep,
		        iray);
#endif
	        return;
	    }

	    /* Is this field in the selected fields list? */
	    if (!rsl_qfield[vol_index]) continue;

	    switch (vol_index) {
	        case DZ_INDEX: f = DZ_F; invf = DZ_INVF; break;
	        case VR_INDEX: f = VR_F; invf = VR_INVF; break;
	        case SW_INDEX: f = SW_F; invf = SW_INVF; break;
	        case DR_INDEX: f = DR_F; invf = DR_INVF; break;
	        case PH_INDEX: f = PH_F; invf = PH_INVF; break;
	        case RH_INDEX: f = RH_F; invf = RH_INVF; break;
	    }

	    waveform = vcp_data->waveform[isweep];

	    /* If this field is reflectivity, check to see if it's from the velocity
             * sweep in a split cut.  If so, we normally skip it since we already
             * have reflectivity from the surveillance sweep.  It is kept only when
             * the user has turned off merging of split cuts.  We skip over this
             * field if all of the following are true: surveillance PRF number is 0,
             * waveform is Contiguous Doppler with Ambiguity Resolution (range
             * unfolding), and we're merging split cuts.
	     */
	    if (vol_index == DZ_INDEX && (vcp_data->surveil_prf_num[isweep] == 0 &&
		        vcp_data->waveform[isweep] == doppler_w_amb_res &&
		        merging_split_cuts))
	        continue;

	    /* Load the data for this field. */
	    if (radar->v[vol_index] == NULL) {
	        radar->v[vol_index] = RSL_new_volume(MAXSWEEPS);
	        radar->v[vol_index]->h.f = f;
	        radar->v[vol_index]->h.invf = invf;
                switch (vol_index) {
                    case DZ_INDEX:
                        radar->v[vol_index]->h.type_str = strdup("Reflectivity");
                        break;
                    case VR_INDEX:
                        radar->v[vol_index]->h.type_str = strdup("Velocity");
                        break;
                    case SW_INDEX:
                        radar->v[vol_index]->h.type_str = strdup("Spectrum width");
                        break;
                    case DR_INDEX:
                        radar->v[vol_index]->h.type_str = strdup("Differential "
                            "Reflectivity");
                        break;
                    case PH_INDEX:
                        radar->v[vol_index]->h.type_str = strdup("Differential "
                            "Phase (PhiDP)");
                        break;
                    case RH_INDEX:
                        radar->v[vol_index]->h.type_str = strdup("Correlation "
                            "Coefficient (RhoHV)");
                        break;
                }
	   
	    }
	    if (radar->v[vol_index]->sweep[isweep] == NULL) {
	        radar->v[vol_index]->sweep[isweep] = RSL_new_sweep(MAXRAYS_M31);
	        radar->v[vol_index]->sweep[isweep]->h.f = f;
	        radar->v[vol_index]->sweep[isweep]->h.invf = invf;
	    }
	    ngates = data_hdr.ngates;
	    ray = RSL_new_ray(ngates);

	    /* Convert data to float, then use range function to store in ray.
	     * Note: data range is 2-255. 0 means signal is below threshold, and 1
	     * means range folded.
	     */

	    offset = data_hdr.offset;
	    scale = data_hdr.scale;
	    if (data_hdr.scale == 0) scale = 1.0; 
	    data = &wsr88d_ray->data[data_index];
	    for (i = 0; i < ngates; i++) {
	        if (data_hdr.datasize_bits != 16) {
		    item = *data;
		    data++;
	        } else {
		    item = *(unsigned short *)data;
		    if (do_swap) swap_2_bytes(&item);
		    data += 2;
	        }
	        if (item > 1)
		    value = (item - offset) / scale;
	        else value = (item == 0) ? BADVAL : RFVAL;
	        ray->range[i] = invf(value);
	        ray->h.f = f;
	        ray->h.invf = invf;
	    }
	    wsr88d_load_ray_hdr(wsr88d_ray, ray, vcp_data);
	    ray->h.range_bin1 = data_hdr.range_first_gate;
	    ray->h.gate_size = data_hdr.range_samp_interval;
	    ray->h.nbins = ngates;
	    radar->v[vol_index]->sweep[isweep]->ray[iray] = ray;
	    radar->v[vol_index]->sweep[isweep]->h.nrays = iray+1;
    } /* for each data field */
}


void wsr88d_load_sweep_header(Radar *radar, int isweep, VCP_data* vcp_data)
{
    int ivolume, nrays;
    Sweep *sweep;
    Ray *last_ray;

    for (ivolume=0; ivolume < MAX_RADAR_VOLUMES; ivolume++) {
	if (radar->v[ivolume] != NULL &&
		radar->v[ivolume]->sweep[isweep] != NULL) {
	    sweep = radar->v[ivolume]->sweep[isweep];
	    nrays = sweep->h.nrays;
	    if (nrays == 0) continue;
	    last_ray = sweep->ray[nrays-1];
	    sweep->h.sweep_num = last_ray->h.elev_num;
	    sweep->h.elev = vcp_data->fixed_angle[isweep];
	    sweep->h.beam_width = last_ray->h.beam_width;
	    sweep->h.vert_half_bw = sweep->h.beam_width / 2.;
	    sweep->h.horz_half_bw = sweep->h.beam_width / 2.;
	}
    }
}


Radar *wsr88d_load_m31_into_radar(Wsr88d_file *wf)
{
    Wsr88d_msg_hdr msghdr;
    Wsr88d_ray_m31 wsr88d_ray;
    short non31_seg_remainder[1202]; /* Remainder after message header */
    int end_of_vos = 0, isweep = 0;
    int msg_hdr_size, msg_size, n;
    int prev_elev_num = 1, prev_raynum = 0, raynum = 0;
    Radar *radar = NULL;
    enum radial_status {START_OF_ELEV, INTERMED_RADIAL, END_OF_ELEV, BEGIN_VOS,
        END_VOS};

    VCP_data vcp_data;

    /* Message type 31 is a variable length message.  All other types consist of
     * 1 or more segments of length 2432 bytes.  To handle all types, we read
     * the message header and check the type.  If not 31, then simply read
     * the remainder of the 2432-byte segment.  If it is 31, use the size given
     * in message header to determine how many bytes to read.
     */

    memset(&msghdr,0 , sizeof(msghdr));
    n = fread(&msghdr, sizeof(Wsr88d_msg_hdr), 1, wf->fptr);

    /* printf("msgtype = %d\n", msghdr.msg_type); */
    msg_hdr_size = sizeof(Wsr88d_msg_hdr) - sizeof(msghdr.rpg);

    radar = RSL_new_radar(MAX_RADAR_VOLUMES);
    memset(&wsr88d_ray, 0, sizeof(Wsr88d_ray_m31)); /* Initialize to be safe. */

    while (! end_of_vos) {
	if (msghdr.msg_type == 31) {
	    if (little_endian()) wsr88d_swap_m31_hdr(&msghdr);

	    /* Get size of the remainder of message.  The given size is in
	     * halfwords; convert it to bytes.
	     */
	    msg_size = (int) msghdr.msg_size * 2 - msg_hdr_size;

	    n = read_wsr88d_ray_m31(wf, msg_size, &wsr88d_ray);
	    if (n <= 0) return NULL;
	    raynum = wsr88d_ray.ray_hdr.azm_num;
	    if (raynum > MAXRAYS_M31) {
		fprintf(stderr,"Error: raynum = %d, exceeds MAXRAYS_M31"
			" (%d)\n", raynum, MAXRAYS_M31);
		fprintf(stderr,"isweep = %d\n", isweep);
		RSL_free_radar(radar);
		return NULL;
	    }

	    /* Check for an unexpected start of new elevation, and issue a
	     * warning if this has occurred.  This condition usually means
	     * less rays then expected in the sweep that just ended.
	     */
	    if (wsr88d_ray.ray_hdr.radial_status == START_OF_ELEV &&
		    wsr88d_ray.ray_hdr.elev_num-1 > isweep) {
		fprintf(stderr,"Warning: Radial status is Start-of-Elevation, "
			"but End-of-Elevation was not\n"
			"issued for elevation number %d.  Number of rays = %d"
			"\n", prev_elev_num, prev_raynum);
		wsr88d_load_sweep_header(radar, isweep, &vcp_data);
		isweep++;
		prev_elev_num = wsr88d_ray.ray_hdr.elev_num - 1;
	    }

            /* Check if this sweep number exceeds how many we allocated */
            if (isweep > MAXSWEEPS) {
		fprintf(stderr,"Error: isweep = %d, exceeds MAXSWEEPS (%d)\n", isweep, MAXSWEEPS);
		RSL_free_radar(radar);
		return NULL;                
            }

	    /* Load ray into radar structure. */
	    wsr88d_load_ray_into_radar(&wsr88d_ray, isweep, radar, &vcp_data);
	    prev_raynum = raynum;

	    /* Check for end of sweep */
	    if (wsr88d_ray.ray_hdr.radial_status == END_OF_ELEV) {
		wsr88d_load_sweep_header(radar, isweep, &vcp_data);
		isweep++;
		prev_elev_num = wsr88d_ray.ray_hdr.elev_num;
	    }
	}
	else { /* msg_type not 31 */
	    n = fread(&non31_seg_remainder, sizeof(non31_seg_remainder), 1,
		    wf->fptr);
	    if (n < 1) {
		fprintf(stderr,"Warning: load_wsr88d_m31_into_radar: ");
		if (feof(wf->fptr) != 0)
		    fprintf(stderr, "Unexpected end of file.\n");
		else
		    fprintf(stderr,"Read failed.\n");
		fprintf(stderr,"Current sweep index: %d\n"
			"Last ray read: %d\n", isweep, prev_raynum);
		wsr88d_load_sweep_header(radar, isweep, &vcp_data);
		return radar;
	    }
	    if (msghdr.msg_type == 5) {
		wsr88d_get_vcp_data(non31_seg_remainder, &vcp_data);
		radar->h.vcp = vcp_data.vcp;
	    }
	}

	/* If not at end of volume scan, read next message header. */
	if (wsr88d_ray.ray_hdr.radial_status != END_VOS) {
	    n = fread(&msghdr, sizeof(Wsr88d_msg_hdr), 1, wf->fptr);
	    if (n < 1) {
		fprintf(stderr,"Warning: load_wsr88d_m31_into_radar: ");
		if (feof(wf->fptr) != 0)
		    fprintf(stderr,"Unexpected end of file.\n");
		else fprintf(stderr,"Failed reading msghdr.\n");
		fprintf(stderr,"Current sweep index: %d\n"
			"Last ray read: %d\n", isweep, prev_raynum);
		wsr88d_load_sweep_header(radar, isweep, &vcp_data);
		end_of_vos = 1;
	    }
	}
	else {
	    end_of_vos = 1;
	    wsr88d_load_sweep_header(radar, isweep, &vcp_data);
	}
    }  /* while not end of vos */

    return radar;
}
