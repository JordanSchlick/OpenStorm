/*
		NASA/TRMM, Code 910.1.
		This is the TRMM Office Radar Software Library.
		Copyright (C) 1996  John H. Merritt
												Space Applications Corporation
												Vienna, Virginia, a NASA/GSFC on-site contractor.

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
#ifndef _rsl_h
#define _rsl_h

/* Are we building the library? */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define RSL_VERSION_STR "v1.50"

/**********************************************************************/
/* Configure: Define USE_TWO_BYTE_PRECISION to have RSL store internal*/
/*            values with two bytes.  Otherwise, use one byte.        */
/*            It is a good idea to use two byte precision.            */
/*            See 'makefile' for an additional explaination.          */
/*                                                                    */
/**********************************************************************/
#define USE_TWO_BYTE_PRECISION


/**********************************************************************/
/* Configure: Define the file name of the red,green, and blue color   */
/*            color tables.  This maps reflectance to color.          */
/*            This should be $(LIBDIR)/colors, from the makefile      */
/*            so you shouldn't have to modify anything here.          */
/**********************************************************************/
#ifndef COLORDIR
#define COLORDIR "/home/kelley/trmm/lib/colors"
#endif

/* These are the color table indexes. See RSL_set/get_color_table. */
#define RSL_RED_TABLE   0
#define RSL_GREEN_TABLE 1
#define RSL_BLUE_TABLE  2

/* The default color tables for reflectivity, velocity, spectral width,
 * height, rainfall, and zdr.
 */
#define REFL_RED_FILE   COLORDIR "/red_reflectivity.clr"
#define REFL_GREEN_FILE COLORDIR "/grn_reflectivity.clr"
#define REFL_BLUE_FILE  COLORDIR "/blu_reflectivity.clr"
#define VEL_RED_FILE    COLORDIR "/red_velocity.clr"
#define VEL_GREEN_FILE  COLORDIR "/grn_velocity.clr"
#define VEL_BLUE_FILE   COLORDIR "/blu_velocity.clr"
#define SW_RED_FILE     COLORDIR "/red_spectral_width.clr"
#define SW_GREEN_FILE   COLORDIR "/grn_spectral_width.clr"
#define SW_BLUE_FILE    COLORDIR "/blu_spectral_width.clr"
#define HEIGHT_RED_FILE   COLORDIR "/red_height.clr"
#define HEIGHT_GREEN_FILE COLORDIR "/grn_height.clr"
#define HEIGHT_BLUE_FILE  COLORDIR "/blu_height.clr"
#define RAINFALL_RED_FILE   COLORDIR "/red_rainfall.clr"
#define RAINFALL_GREEN_FILE COLORDIR "/grn_rainfall.clr"
#define RAINFALL_BLUE_FILE  COLORDIR "/blu_rainfall.clr"

/* Added by D. Wolff 07/31/97 */
#define ZDR_RED_FILE   COLORDIR "/red_zdr.clr"
#define ZDR_GREEN_FILE COLORDIR "/grn_zdr.clr"
#define ZDR_BLUE_FILE  COLORDIR "/blu_zdr.clr"

/*************************************************************************/
/*  You should not have to change anything below this line.
 *  The rest is my fault.
 */
/*************************************************************************/
#include <stdio.h>

/*
 * Magic numbers.  These are used to uniquely identify the type of 
 * values present in a particular structure: Volume, Sweep, Ray.
 * The magic numbers V_DZ, V_VR, V_SW, etc.  represent Volume magic
 * numbers for the field types DZ, VR, SW, etc.  Similiar magic numbers
 * are implimented for Sweep, and Ray.  The magic number is the first word
 * of the data structure.  These magic number may determine which conversion
 * function is used, see volume.c and the end of this file for the
 * conversion functions. (As of v0.39 -- NOT YET IMPLEMENTED.)
 */
enum Rsl_magic_num {
	V_DZ, V_VR, V_SW, V_CZ, V_ZT, V_DR, V_LR,
	S_DZ, S_VR, S_SW, S_CZ, S_ZT, S_DR, S_LR,
	R_DZ, R_VR, R_SW, R_CZ, R_ZT, R_DR, R_LR
};

/* File format types recognized by RSL. */
enum File_type {UNKNOWN, WSR88D_FILE, UF_FILE, LASSEN_FILE,
								TOGA_FILE, NSIG_FILE_V1, NSIG_FILE_V2,
								RSL_FILE, MCGILL_FILE, HDF_FILE, RAPIC_FILE,
								RADTEC_FILE, DORADE_FILE, RAINBOW_FILE};

/* Pick a BADVAL that is out of range. That is, the range
 * of the conversion cannot include these reserved values.
 * Typically, pick a number that cannot be stored in the Range data type.
 */
#ifdef USE_TWO_BYTE_PRECISION
	typedef unsigned short Range;
#define BADVAL  (float)0x20000
#else
	typedef unsigned char Range;
#define BADVAL  (float)0500     /* non-meaningful value (500 octal) */
#endif

#define RFVAL (BADVAL-1) /* Range folded value.  See rfival. */
#define APFLAG (BADVAL-2)
#define NOTFOUND_H (BADVAL-3)
#define NOTFOUND_V (BADVAL-4)
#define NOECHO (BADVAL-5) /* For nsig and UF -32, for kwaj -30 */
#define RSL_SPEED_OF_LIGHT 299792458.0 /* m/s */

typedef struct {
	int   month; /* Time for this ray; month (1-12). */
	int   day;   /* Time for this ray; day (1-31).   */
	int   year;  /* Time for this ray; year (eg. 1993). */
	int   hour;  /* Date for this ray; hour (0-23). */
	int   minute;/* Date for this ray; minute (0-59).*/
	float sec;   /* Date for this ray; second + fraction of second. */
	float unam_rng;  /* Unambiguous range. (KM). */
	float azimuth;   /* Azimuth angle (degrees). Must be positive.
										* 0=North, 90=east, -90/270=west.
										* This angle is the mean azimuth for the whole ray.
										* E.g. for NSIG the beginning and end azimuths are averaged.
										*/
	int   ray_num;    /* Ray no. within elevation scan. */
	float elev;       /* Elevation angle. (degrees). */
	int   elev_num;   /* Elevation no. within volume scan. */
	
	int   range_bin1; /* Range to first gate.(meters) */
	int   gate_size;  /* Data gate size (meters)*/
	
	float  vel_res;   /* Doppler velocity resolution */
	float sweep_rate; /* Sweep rate. Full sweeps/min. */
	
	int prf;          /* Pulse repetition frequency, in Hz. */
	int prf2;         /* Second PRF, for Sigmet dual PRF */
	float azim_rate;  /* Sweep rate in degrees/sec. */
	float fix_angle;  /* Elevation angle for the sweep. (degrees). */
	float pitch;      /* Pitch angle. */
	float roll;       /* Roll  angle. */
	float heading;    /* Heading. */
	float pitch_rate; /* (angle/sec) */
	float roll_rate;  /* (angle/sec) */
	float heading_rate; /* (angle/sec) */
	float lat;          /* Latitude (degrees) */
	float lon;          /* Longitude (degrees) */
	int   alt;          /* Altitude (m) */
	float rvc;          /* Radial velocity correction (m/sec) */
	float vel_east;     /* Platform velocity to the east (negative for west) (m/sec) */
	float vel_north;    /* Platform velocity to the north (negative south) (m/sec) */
	float vel_up;       /* Platform velocity toward up (negative down) (m/sec) */
	int   pulse_count; /* Pulses used in a single dwell time. */
	float pulse_width; /* Pulse width (micro-sec). */
	float beam_width;  /* Beamwidth in degrees. */
	float frequency;   /* Carrier freq. GHz. */
	float wavelength;  /* Wavelength. Meters. */
	float nyq_vel;     /* Nyquist velocity. m/s */
	float (*f)(Range x);       /* Data conversion function. f(x). */
	Range (*invf)(float x);    /* Data conversion function. invf(x). */
	int   nbins;               /* Number of array elements for 'Range'. */
} Ray_header;


typedef struct {              
	 Ray_header h;
	 Range *range;    /* range[0..nbins-1] 
										 * For wsr88d file:
										 * 0..460 for reflectivity, 0..920 for velocity and
										 * spectrum width.
										 */
	 } Ray;


typedef struct _azimuth_hash {
	Ray *ray; 
	struct _azimuth_hash *next, *ray_high, *ray_low;
} Azimuth_hash;

typedef struct {
	Azimuth_hash **indexes;
	int nindexes;
} Hash_table;


typedef struct {
	int sweep_num;   /* Integer sweep number. */
	float elev;      /* Elevation angle for sweep. Value is -999 for RHI. */
	float azimuth;   /* Azimuth for RHI. Value is -999 for PPI. */
	float beam_width;  /* This is in the ray header too. */
	float vert_half_bw;  /* Vertical beam width divided by 2 */
	float horz_half_bw;  /* Horizontal beam width divided by 2 */

	int nrays;
	float (*f)(Range x);       /* Data conversion function. f(x). */
	Range (*invf)(float x);    /* Data conversion function. invf(x). */
} Sweep_header;

typedef struct {           
	Sweep_header h;   
	Ray **ray;               /* ray[0..nrays-1]. */
} Sweep;

typedef struct {
	char *type_str;  /* One of:'Reflectivity', 'Velocity' or 'Spectrum width' */
	int nsweeps;
	float calibr_const;        /* Calibration constant.  HDF specific. */
	float (*f)(Range x);       /* Data conversion function. f(x). */
	Range (*invf)(float x);    /* Data conversion function. invf(x). */
} Volume_header;

typedef struct {
		Volume_header h;           /* Specific info for each elev. */
		Sweep **sweep;             /* sweep[0..nsweeps-1]. */
} Volume;



typedef Range Carpi_value;
typedef Range Cappi_value;

typedef struct {
	int   month;            /* (1-12). */
	int   day;              /* (1-31).   */
	int   year;             /* (eg. 1993). */
	int   hour;             /* (0-23). */
	int   minute;           /* (0-59).*/
	float sec;              /* second + fraction of second. */
	float dx, dy;           /* Size of cell in km. */
	int   nx, ny;           /* Number of cells. */
	int   radar_x, radar_y; /* Location of center of radar. */
	float height;           /* Height of this Carpi. */
	float lat, lon;         /* Lat/lon of lower left corner of Carpi. */
	char  radar_type[50];   /* Radar types. */
	int   field_type;       /* Same as for Radar. */
	int   interp_method;    /* ??? string describing interpolation method. */
	float (*f)(Carpi_value x);    /* Data conversion function. f(x). */
	Carpi_value (*invf)(float x); /* Data conversion function. invf(x). */
	Carpi_value **data;     /* data[ny][nx] */
} Carpi;

/** Cappi data structure info **/
/*  Paul A. Kucera            **/

/* Element in location array of Cappi data structure.
 * Each element is elvation and range to data value.
 */
typedef struct 
	 {
	 float elev;        /* elevation angle */
	 float srange;       /* slant range !!! */
	 }
Er_loc;

/* Cappi data structure.
 */
typedef struct {
	int    month;       /* Begin time for this Cappi; month (1-12). */
	int    day;         /* Begin time for this Cappi; day (1-31).   */
	int    year;        /* Begin time for this Cappi; year (eg. 1993). */
	int    hour;        /* Begin date for this Cappi; hour (0-23). */
	int    minute;      /* Begin date for this Cappi; minute (0-59).*/
	float  sec;         /* Begin date for this Cappi; second + frac. of second.*/
	float  height;        /* Height for this Cappi in m */
	float  lat;
	float  lon;
	int    field_type;    /* Value of Constant ??_INDEX */
	char   radar_type[50];   /* Value of Constant radar->h.radar_type */
	int    interp_method; /* ??? string describing interpolation method. */
	Er_loc *loc;          /* elevation and range coordinate array */
	Sweep  *sweep;        /* Pointers to rays of data */
} Cappi;

/* The Cube data type. */

typedef Range Cube_value;
typedef Range Slice_value;

typedef struct 
{
		float lat, lon;
		float dx, dy, dz;
		int nx, ny, nz;
		char *data_type;
		Carpi **carpi;         /* Pointers to carpi[0] thru carpi[nz-1] */
} Cube;

typedef struct
{
		float dx, dy;
		int nx, ny;
		char *data_type;
	float (*f)(Slice_value x);    /* Data conversion function. f(x). */
	Slice_value (*invf)(float x); /* Data conversion function. invf(x). */
		Slice_value **data;           /* data[ny][nx]. */
} Slice;

typedef struct {
	int nbins;
	int low;
	int hi;
	int ucount;
	int ccount;
	int *data;
} Histogram;

enum scan_mode {PPI, RHI};

typedef struct {
	int month, day, year;
	int hour, minute;
	float sec; /* Second plus fractional part. */
	char radar_type[50]; /* Type of radar.  Use for QC-ing the data.
												* Supported types are:
												* "wsr88d", "lassen", "uf",
												* "nsig", "mcgill",
												* "kwajalein", "rsl", "toga",
												* "rapic", (rapic is Berrimah Austrailia)
												* "radtec", (SPANDAR radar at Wallops Is, VA)
												* "dorade",
												* "south_africa".
												* Set by appropriate ingest routine.
												*/
	int nvolumes;

	int number;        /* arbitrary number of this radar site */
	char name[8];      /* Nexrad site name */
	char radar_name[8]; /* Radar name. */
	char project[24];   /* Project identifier. */
	char city[15];     /* nearest city to  radar site */
	char state[3];     /* state of radar site */
	char country[15];
	int latd;   /* degrees of latitude of site */
	int latm;   /* minutes of latitude of site */
	int lats;   /* seconds of latitude of site */
	int lond;   /* degrees of longitude of site */
	int lonm;   /* minutes of longitude of site */
	int lons;   /* seconds of longitude of site */
	int height; /* height of site in meters above sea level*/
	int spulse; /* length of short pulse (ns)*/
	int lpulse; /* length of long pulse (ns) */
	int scan_mode; /* 0 = PPI, 1 = RHI */
	int vcp;    /* Volume Coverage Pattern (for WSR-88D only) */
} Radar_header;


typedef struct {
	Radar_header h;
	Volume **v;  /* Array 0..nvolumes-1 of pointers to Volumes.
											* 0 = DZ_INDEX = reflectivity.
											* 1 = VR_INDEX = velocity.
											* 2 = SW_INDEX = spectrum_width.
											* 3 = CZ_INDEX = corrected reflectivity.
											* 4 = ZT_INDEX = uncorrected reflectivity.
											* 5 = DR_INDEX = differential refl.
											* 6 = LR_INDEX = another differential refl.
											* 7 = ZD_INDEX = another differential refl.
											* 8 = DM_INDEX = received power.
											* 9 = RH_INDEX = RhoHV: Horz-Vert power corr coeff
											*10 = PH_INDEX = PhiDP: Differential phase angle
											*11 = XZ_INDEX = X-band reflectivity.
											*12 = CD_INDEX = Corrected DR.
											*13 = MZ_INDEX = DZ mask for 1C-51 HDF.
											*14 = MR_INDEX = DR mask for 1C-51 HDF.
											*15 = ZE_INDEX = Edited reflectivity.
											*16 = VE_INDEX = Edited velocity.
											*17 = KD_INDEX = KDP: Specific differential phase, deg/km.
											*18 = TI_INDEX = TIME (unknown)  for MCTEX data.
											*19 = DX_INDEX
											*20 = CH_INDEX
											*21 = AH_INDEX
											*22 = CV_INDEX
											*23 = AV_INDEX
											*24 = SQ_INDEX = Signal Quality Index (Sigmet)
											*25 = VS_INDEX = Radial Velocity Combined  (DORADE)
											*26 = VL_INDEX = Radial Velocity Combined  (DORADE)
											*27 = VG_INDEX = Radial Velocity Combined  (DORADE)
											*28 = VT_INDEX = Radial Velocity Combined  (DORADE)
											*29 = NP_INDEX = Normalized Coherent Power (DORADE)
											*30 = HC_INDEX = HydroClass (Sigmet)
											*31 = VC_INDEX = Radial Velocity Corrected (Sigmet)
											*32 = V2_INDEX = Radial Velocity for VCP 121 second Doppler cut.
											*33 = S2_INDEX = Spectrum Width  for VCP 121 second Doppler cut.
											*34 = V3_INDEX = Radial Velocity for VCP 121 third Doppler cut.
											*35 = S3_INDEX = Spectrum Width  for VCP 121 third Doppler cut.
											*42 = ET_INDEX = Total Power Enhanced (Sigmet)
											*43 = EZ_INDEX = Clutter Corr. Reflectivity Enhanced (Sigmet)
								*/

} Radar;

/*
 * DZ     Reflectivity (dBZ), may contain some     DZ_INDEX
 *        signal-processor level QC and/or      
 *        filters. This field would contain 
 *        Darwin's CZ, or WSR88D's standard 
 *        reflectivity. In other words, unless
 *        the field is described otherwise, it
 *        should always go here. In essence, this
 *        is the "cleanest" reflectivity field
 *        for a radar.
 *
 * VR     Radial Velocity (m/s)                    VR_INDEX
 *
 * SW     Spectral Width (m2/s2)                   SW_INDEX
 *
 * CZ     QC Reflectivity (dBZ), contains
 *        post-processed QC'd data                 CZ_INDEX
 *
 * ZT     Total Reflectivity (dBZ)                 ZT_INDEX
 *        Reflectivity unfiltered for clutter...
 *        This is UZ in UF files.
 *
 * DR     Differential reflectivity                DR_INDEX
 *        DR and LR are for dual-polarization
 *        radars only. Unitless or in dB.
 *
 * LR     Another form of differential ref         LR_INDEX
 *        called LDR, not sure of units
 *
 * ZD     ZDR: Reflectivity Depolarization Ratio   ZD_INDEX
 *        ZDR = 10log(ZH/ZV)  (dB)
 *
 * DM     Received power measured by the radar.    DM_INDEX
 *        Units are dBm.
 *
 * RH     RhoHV: Horz-Vert power correlation       RH_INDEX
 *        coefficient. (0 to 1) See volume.c
 *
 * PH     PhiDP: Differential phase angle.         PH_INDEX
 *        (0 to 180 deg in steps of 0.71)
 *        See volume.c
 *
 * XZ     X-band reflectivity                      XZ_INDEX
 *
 * CD     Corrected ZD reflectivity (differential) CD_INDEX
 *        contains QC'ed data
 *
 * MZ     DZ mask volume for HDF 1C-51 product.    MZ_INDEX
 *
 * MD     ZD mask volume for HDF 1C-51 product.    MD_INDEX
 *
 * ZE     Edited Reflectivity.                     ZE_INDEX
 *
 * VE     Edited Velocity.                         VE_INDEX
 *
 * KD     KDP (deg/km) Differencial Phase          KD_INDEX
 *            (Sigmet, Lassen)
 *
 * TI     TIME (unknown)  for MCTEX data.          TI_INDEX
 *
 * SQ     SQI: Signal Quality Index. (Sigmet)      SQ_INDEX
 *        Decimal fraction from 0 to 1, where 0
 *        is noise, 1 is noiseless.
 *
 * VS     Radial Velocity, Short PRT (m/s) (DORADE)   VS_INDEX
 *
 * VL     Radial Velocity, Long PRT (m/s)  (DORADE)   VL_INDEX
 *
 * VG     Radial Velocity, combined (m/s)  (DORADE)   VG_INDEX
 *
 * VT     Radial Velocity, combined (m/s)  (DORADE)   VT_INDEX
 *
 * NP     Normalized Coherent Power.       (DORADE)   NP_INDEX
 *
 * HC     HydroClass: enumerated class.    (Sigmet)   HC_INDEX
 *
 * VC     Radial Velocity corrected for    (Sigmet)   VC_INDEX
 *        Nyquist unfolding. 
 */

/*
 * The number of *_INDEX must never exceed MAX_RADAR_VOLUMES.
 * Increase MAX_RADAR_VOLUMES appropriately, for new ingest formats.
 *
 * Also, when adding new *_INDEXes, you must update the following three arrays
 * located near the end of this file: RSL_ftype, RSL_f_list, and  RSL_invf_list.
 * You also need to modify volume.c, updating the initialization of array
 * rsl_qfield by adding a '1' for each new volume index.
 */

#define MAX_RADAR_VOLUMES 47

#define DZ_INDEX 0
#define VR_INDEX 1
#define SW_INDEX 2
#define CZ_INDEX 3
#define ZT_INDEX 4
#define DR_INDEX 5
#define LR_INDEX 6
#define ZD_INDEX 7
#define DM_INDEX 8
#define RH_INDEX 9
#define PH_INDEX 10
#define XZ_INDEX 11
#define CD_INDEX 12
#define MZ_INDEX 13
#define MD_INDEX 14
#define ZE_INDEX 15
#define VE_INDEX 16
#define KD_INDEX 17
#define TI_INDEX 18
#define DX_INDEX 19
#define CH_INDEX 20
#define AH_INDEX 21
#define CV_INDEX 22
#define AV_INDEX 23
#define SQ_INDEX 24
#define VS_INDEX 25
#define VL_INDEX 26
#define VG_INDEX 27
#define VT_INDEX 28
#define NP_INDEX 29
#define HC_INDEX 30
#define VC_INDEX 31
#define V2_INDEX 32
#define S2_INDEX 33
#define V3_INDEX 34
#define S3_INDEX 35
#define CR_INDEX 36
#define CC_INDEX 37
#define PR_INDEX 38
#define SD_INDEX 39
#define ZZ_INDEX 40
#define RD_INDEX 41
#define ET_INDEX 42
#define EZ_INDEX 43
#define TV_INDEX 44
#define ZV_INDEX 45
#define SN_INDEX 46


/* Prototypes for functions. */
/* Alphabetical and grouped by object returned. */


Radar *RSL_africa_to_radar(char *infile);
Radar *RSL_anyformat_to_radar(char *infile, ...);
Radar *RSL_dorade_to_radar(char *infile);
Radar *RSL_fix_radar_header(Radar *radar);
Radar *RSL_get_window_from_radar(Radar *r, float min_range, float max_range,float low_azim, float hi_azim);
Radar *RSL_hdf_to_radar(char *infile);
Radar *RSL_hdf_to_radar_unQC(char *infile);
Radar *RSL_kwaj_to_radar(char *infile);
Radar *RSL_lassen_to_radar(char *infile);
Radar *RSL_mcgill_to_radar(char *infile);
Radar *RSL_new_radar(int nvolumes);
Radar *RSL_nsig_to_radar(char *infile);
Radar *RSL_nsig2_to_radar(char *infile);
Radar *RSL_prune_radar(Radar *radar);
Radar *RSL_radtec_to_radar(char *infile);
Radar *RSL_rainbow_to_radar(char *infile);
Radar *RSL_rapic_to_radar(char *infile);
Radar *RSL_read_radar(char *infile);
Radar *RSL_sort_radar(Radar *r);
Radar *RSL_toga_to_radar(char *infile);
Radar *RSL_uf_to_radar(char *infile);
Radar *RSL_uf_to_radar_fp(FILE *fp);
Radar *RSL_wsr88d_to_radar(char *infile, char *call_or_first_tape_file);

Volume *RSL_clear_volume(Volume *v);
Volume *RSL_copy_volume(Volume *v);
Volume *RSL_fix_volume_header(Volume *v);
Volume *RSL_get_volume(Radar *r, int type_wanted);
Volume *RSL_get_window_from_volume(Volume *v, float min_range, float max_range, float low_azim, float hi_azim);
Volume *RSL_new_volume(int max_sweeps);
Volume *RSL_prune_volume(Volume *v);
Volume *RSL_read_volume(FILE *fp);
Volume *RSL_reverse_sweep_order(Volume *v);
Volume *RSL_sort_rays_in_volume(Volume *v);
Volume *RSL_sort_sweeps_in_volume(Volume *v);
Volume *RSL_sort_volume(Volume *v);
Volume *RSL_volume_z_to_r(Volume *z_volume, float k, float a);

Sweep *RSL_clear_sweep(Sweep *s);
Sweep *RSL_copy_sweep(Sweep *s);
Sweep *RSL_fix_sweep_header(Sweep *sweep);
Sweep *RSL_get_closest_sweep(Volume *v,float sweep_angle,float limit);
Sweep *RSL_get_eth_sweep(Volume *v,float et_point,float max_range);
Sweep *RSL_get_first_sweep_of_volume(Volume *v);
Sweep *RSL_get_sweep(Volume *v, float elev);
Sweep *RSL_get_window_from_sweep(Sweep *s, float min_range, float max_range, float low_azim, float hi_azim);

Sweep *RSL_new_sweep(int max_rays);
Sweep *RSL_prune_sweep(Sweep *s);
Sweep *RSL_read_sweep (FILE *fp);
Sweep *RSL_sort_rays_in_sweep(Sweep *s);
Sweep *RSL_sort_rays_by_time(Sweep *s);
Sweep *RSL_sweep_z_to_r(Sweep *z_sweep, float k, float a);

Ray *RSL_clear_ray(Ray *r);
Ray *RSL_copy_ray(Ray *r);
Ray *RSL_get_closest_ray_from_sweep(Sweep *s,float ray_angle,float limit);
Ray *RSL_get_first_ray_of_sweep(Sweep *s);
Ray *RSL_get_first_ray_of_volume(Volume *v);
Ray *RSL_get_closest_ray_from_sweep(Sweep *s,float ray_angle,float limit);
Ray *RSL_get_next_ccwise_ray(Sweep *s, Ray *ray);
Ray *RSL_get_next_cwise_ray(Sweep *s, Ray *ray);
Ray *RSL_get_ray(Volume *v, float elev, float azimuth);
Ray *RSL_get_ray_above(Volume *v, Ray *current_ray);
Ray *RSL_get_ray_below(Volume *v, Ray *current_ray);
Ray *RSL_get_ray_from_sweep(Sweep *s, float azim);
Ray *RSL_get_window_from_ray(Ray *r, float min_range, float max_range, float low_azim, float hi_azim);
Ray *RSL_new_ray(int max_bins);
Ray *RSL_prune_ray(Ray *ray);
Ray *RSL_ray_z_to_r(Ray *z_ray, float k, float a);
Ray *RSL_read_ray   (FILE *fp);


float RSL_area_of_ray(Ray *r, float lo, float hi, float min_range, float max_range);
float RSL_fraction_of_ray(Ray *r, float lo, float hi, float range);
float RSL_fraction_of_sweep(Sweep *s, float lo, float hi, float range);
float RSL_fraction_of_volume(Volume *v, float lo, float hi, float range);
float RSL_fractional_area_of_sweep(Sweep *s, float lo, float hi, float min_rng, float max_rng);
float RSL_get_echo_top_height(Volume *v,float azim,float grange, float et_point);
float RSL_get_linear_value(Volume *v,float srange,float azim,float elev,float limit);
float RSL_get_nyquist_from_radar(Radar *radar);
float RSL_get_range_of_range_index(Ray *ray, int index);
float RSL_get_value(Volume *v, float elev, float azimuth, float range);
float RSL_get_value_at_h(Volume *v, float azim, float grnd_r, float h);
float RSL_get_value_from_cappi(Cappi *cappi, float rng, float azm);
float RSL_get_value_from_ray(Ray *ray, float r);
float RSL_get_value_from_sweep(Sweep *s, float azim, float r);
float RSL_z_to_r(float z, float k, float a);

int RSL_fill_cappi(Volume *v, Cappi *cap, int method);
int RSL_get_ray_index_from_sweep(Sweep *s, float azim,int *next_closest);
int RSL_get_sweep_index_from_volume(Volume *v, float elev,int *next_closest);
int RSL_radar_to_hdf(Radar *radar, char *outfile);
int RSL_write_histogram(Histogram *histogram, char *outfile);
int RSL_write_ray(Ray *r, FILE *fp);
int RSL_write_sweep(Sweep *s, FILE *fp);
int RSL_write_radar(Radar *radar, char *outfile);
int RSL_write_radar_gzip(Radar *radar, char *outfile);
int RSL_write_volume(Volume *v, FILE *fp);

unsigned char *RSL_rhi_sweep_to_cart(Sweep *s, int xdim, int ydim, float range, 
																		 int vert_scale);
unsigned char *RSL_sweep_to_cart(Sweep *s, int xdim, int ydim, float range);

void RSL_add_dbz_offset_to_ray(Ray *r, float dbz_offset);
void RSL_add_dbz_offset_to_sweep(Sweep *s, float dbz_offset);
void RSL_add_dbz_offset_to_volume(Volume *v, float dbz_offset);
void RSL_bscan_ray(Ray *r, FILE *fp);
void RSL_bscan_sweep(Sweep *s, char *outfile);
void RSL_bscan_volume(Volume *v, char *basename);
void RSL_find_rng_azm(float *r, float *ang, float x, float y);
void RSL_fix_time (Ray *ray);
void RSL_float_to_char(float *x, Range *c, int n);

void RSL_free_cappi(Cappi *c);
void RSL_free_carpi(Carpi *carpi);
void RSL_free_cube(Cube *cube);
void RSL_free_histogram(Histogram *histogram);
void RSL_free_ray(Ray *r);
void RSL_free_slice(Slice *slice);
void RSL_free_sweep(Sweep *s);
void RSL_free_radar(Radar *r);
void RSL_free_volume(Volume *v);
void RSL_get_color_table(int icolor, char buffer[256], int *ncolors);
void RSL_get_groundr_and_h(float slant_r, float elev, float *gr, float *h);
void RSL_get_slantr_and_elev(float gr, float h, float *slant_r, float *elev);
void RSL_get_slantr_and_h(float gr, float elev, float *slant_r, float *h);
void RSL_load_color_table(char *infile, char buffer[256], int *ncolors);
void RSL_load_height_color_table();
void RSL_load_rainfall_color_table();
void RSL_load_refl_color_table();
void RSL_load_vel_color_table();
void RSL_load_sw_color_table();
void RSL_load_zdr_color_table();
void RSL_load_red_table(char *infile);
void RSL_load_green_table(char *infile);
void RSL_load_blue_table(char *infile);
void RSL_print_histogram(Histogram *histogram, int min_range, int max_range,
												 char *filename);
void RSL_print_version();
void RSL_radar_to_uf(Radar *r, char *outfile);
void RSL_radar_to_uf_gzip(Radar *r, char *outfile);
void RSL_radar_verbose_off(void);
void RSL_radar_verbose_on(void);
void RSL_read_these_sweeps(char *csweep, ...);
void RSL_rebin_velocity_ray(Ray *r);
void RSL_rebin_velocity_sweep(Sweep *s);
void RSL_rebin_velocity_volume(Volume *v);
void RSL_rebin_zdr_ray(Ray *r);
void RSL_rebin_zdr_sweep(Sweep *s);
void RSL_rebin_zdr_volume(Volume *v);
void RSL_rhi_sweep_to_gif(Sweep *s, char *outfile, int xdim, int ydim, float range, 
													int vert_scale);
void RSL_select_fields(char *field_type, ...);
void RSL_set_color_table(int icolor, char buffer[256], int ncolors);
void RSL_sweep_to_gif(Sweep *s, char *outfile, int xdim, int ydim, float range);
void RSL_sweep_to_pgm(Sweep *s, char *outfile, int xdim, int ydim, float range);
void RSL_sweep_to_pict(Sweep *s, char *outfile, int xdim, int ydim, float range);
void RSL_sweep_to_ppm(Sweep *s, char *outfile, int xdim, int ydim, float range);
void RSL_volume_to_gif(Volume *v, char *basename, int xdim, int ydim, float range);
void RSL_volume_to_pgm(Volume *v, char *basename, int xdim, int ydim, float range);
void RSL_volume_to_pict(Volume *v, char *basename, int xdim, int ydim, float range);
void RSL_volume_to_ppm(Volume *v, char *basename, int xdim, int ydim, float range);
void RSL_write_gif(char *outfile, unsigned char *image,
									 int xdim, int ydim, char c_table[256][3]);
void RSL_write_pgm(char *outfile, unsigned char *image,
									 int xdim, int ydim);
void RSL_write_pict(char *outfile, unsigned char *image,
										int xdim, int ydim, char c_table[256][3]);
void RSL_write_ppm(char *outfile, unsigned char *image,
									 int xdim, int ydim, char c_table[256][3]);

Cappi *RSL_new_cappi(Sweep *sweep, float height);
Cappi *RSL_cappi_at_h(Volume  *v, float height, float max_range);

Carpi *RSL_cappi_to_carpi(Cappi *cappi, float dx, float dy,
													float lat, float lon,
													int nx, int ny, int radar_x, int radar_y);
Carpi *RSL_new_carpi(int nrows, int ncols);
Carpi *RSL_volume_to_carpi(Volume *v, float h, float grnd_r,
													 float dx, float dy, int nx, int ny,
													 int radar_x, int radar_y, float lat, float lon);

Cube *RSL_new_cube(int ncarpi);
Cube *RSL_volume_to_cube(Volume *v, float dx, float dy, float dz,
												 int nx, int ny, int nz, float grnd_r,
												 int radar_x, int radar_y, int radar_z);

Slice *RSL_new_slice(int nrows, int ncols);
Slice *RSL_get_slice_from_cube(Cube *cube, int x, int y, int z);


Histogram *RSL_allocate_histogram(int low, int hi);
Histogram *RSL_get_histogram_from_ray(Ray *ray, Histogram *histogram,
																			int low, int hi, int min_range,
																			int max_range);
Histogram *RSL_get_histogram_from_sweep(Sweep *sweep, Histogram *histogram, 
																				int low, int hi, int min_range,
																				int max_range);
Histogram *RSL_get_histogram_from_volume(Volume *volume, Histogram *histogram,
																				 int low, int hi, int min_range,
																				 int max_range);
Histogram *RSL_read_histogram(char *infile);

int no_command (char *cmd);
FILE *uncompress_pipe (FILE *fp);
FILE *compress_pipe (FILE *fp);
int rsl_pclose(FILE *fp);

/* Carpi image generation functions. These are modified clones of the
		 corresponding sweep image generation functions.
*/
unsigned char *RSL_carpi_to_cart(Carpi *carpi, int xdim, int ydim,
																																 float range);
void RSL_carpi_to_gif(Carpi *carpi, char *outfile, int xdim, int ydim,
																						float range);
void RSL_carpi_to_pict(Carpi *carpi, char *outfile, int xdim, int ydim,
																						 float range);
void RSL_carpi_to_ppm(Carpi *carpi, char *outfile, int xdim, int ydim,
																						float range);
void RSL_carpi_to_pgm(Carpi *carpi, char *outfile, int xdim, int ydim,
																						float range);

/* Internal storage conversion functions. These may be any conversion and
 * may be dynamically defined; based on the input data conversion.
 */
float DZ_F(Range x);
float VR_F(Range x);
float SW_F(Range x);
float CZ_F(Range x);
float ZT_F(Range x);
float DR_F(Range x);
float LR_F(Range x);
float ZD_F(Range x);
float DM_F(Range x);
float RH_F(Range x);
float PH_F(Range x);
float XZ_F(Range x);
float CD_F(Range x);
float MZ_F(Range x);
float MD_F(Range x);
float ZE_F(Range x);
float VE_F(Range x);
float KD_F(Range x);
float TI_F(Range x);
float DX_F(Range x);
float CH_F(Range x);
float AH_F(Range x);
float CV_F(Range x);
float AV_F(Range x);
float SQ_F(Range x);
float VS_F(Range x);
float VL_F(Range x);
float VG_F(Range x);
float VT_F(Range x);
float NP_F(Range x);
float HC_F(Range x);
float VC_F(Range x);
float SD_F(Range x);
float SN_F(Range x);

Range DZ_INVF(float x);
Range VR_INVF(float x);
Range SW_INVF(float x);
Range CZ_INVF(float x);
Range ZT_INVF(float x);
Range DR_INVF(float x);
Range LR_INVF(float x);
Range ZD_INVF(float x);
Range DM_INVF(float x);
Range RH_INVF(float x);
Range PH_INVF(float x);
Range XZ_INVF(float x);
Range CD_INVF(float x);
Range MZ_INVF(float x);
Range MD_INVF(float x);
Range ZE_INVF(float x);
Range VE_INVF(float x);
Range KD_INVF(float x);
Range TI_INVF(float x);
Range DX_INVF(float x);
Range CH_INVF(float x);
Range AH_INVF(float x);
Range CV_INVF(float x);
Range AV_INVF(float x);
Range SQ_INVF(float x);
Range VS_INVF(float x);
Range VL_INVF(float x);
Range VG_INVF(float x);
Range VT_INVF(float x);
Range NP_INVF(float x);
Range HC_INVF(float x);
Range VC_INVF(float x);
Range SD_INVF(float x);
Range SN_INVF(float x);


/* If you like these variables, you can use them in your application
 * by defining USE_RSL_VARS before #include "rsl.h"
 */
#ifdef USE_RSL_VARS
static char *RSL_ftype[] = {(char*)"DZ", (char*)"VR", (char*)"SW", (char*)"CZ", (char*)"ZT", (char*)"DR", 
														(char*)"LR", (char*)"ZD", (char*)"DM", (char*)"RH", (char*)"PH", (char*)"XZ", 
														(char*)"CD", (char*)"MZ", (char*)"MD", (char*)"ZE", (char*)"VE", (char*)"KD", 
														(char*)"TI", (char*)"DX", (char*)"CH", (char*)"AH", (char*)"CV", (char*)"AV",
														(char*)"SQ", (char*)"VS", (char*)"VL", (char*)"VG", (char*)"VT", (char*)"NP",
														(char*)"HC", (char*)"VC", (char*)"V2", (char*)"S2", (char*)"V3", (char*)"S3",
														(char*)"CR", (char*)"CC", (char*)"PR", (char*)"SD", (char*)"ZZ", (char*)"RD",
														(char*)"ET", (char*)"EZ", (char*)"TV", (char*)"ZV", (char*)"SN"};

static  float (*RSL_f_list[])(Range x) = {DZ_F, VR_F, SW_F, CZ_F, ZT_F, DR_F,
																					LR_F, ZD_F, DM_F, RH_F, PH_F, XZ_F,
																					CD_F, MZ_F, MD_F, ZE_F, VE_F, KD_F,
																					TI_F, DX_F, CH_F, AH_F, CV_F, AV_F,
																					SQ_F, VS_F, VL_F, VG_F, VT_F, NP_F,
																					HC_F, VC_F, VR_F, SW_F, VR_F, SW_F,
																					DZ_F, CZ_F, PH_F, SD_F, DZ_F, DZ_F,
																					ZT_F, DZ_F, ZT_F, DZ_F, SN_F};

static  Range (*RSL_invf_list[])(float x)
				 = {DZ_INVF, VR_INVF, SW_INVF, CZ_INVF, ZT_INVF, DR_INVF, 
						LR_INVF, ZD_INVF, DM_INVF, RH_INVF, PH_INVF, XZ_INVF, 
						CD_INVF, MZ_INVF, MD_INVF, ZE_INVF, VE_INVF, KD_INVF,
						TI_INVF, DX_INVF, CH_INVF, AH_INVF, CV_INVF, AV_INVF,
						SQ_INVF, VS_INVF, VL_INVF, VG_INVF, VT_INVF, NP_INVF,
						HC_INVF, VC_INVF, VR_INVF, SW_INVF, VR_INVF, SW_INVF,
						DZ_INVF, CZ_INVF, PH_INVF, SD_INVF, DZ_INVF, DZ_INVF,
						ZT_INVF, DZ_INVF, ZT_INVF, DZ_INVF, SN_INVF};
#endif
/* Secret routines that are quite useful and useful to developers. */
void radar_load_date_time(Radar *radar);
int big_endian(void);
int little_endian(void);
void swap_4_bytes(void *word);
void swap_2_bytes(void *word);
Hash_table *hash_table_for_sweep(Sweep *s);
int hash_bin(Hash_table *table,float angle);
Azimuth_hash *the_closest_hash(Azimuth_hash *hash, float ray_angle);
Hash_table *construct_sweep_hash_table(Sweep *s);
double       angle_diff(float x, float y);
int rsl_query_field(char *c_field);

/* Functions to control the handling of WSR-88D split cuts. */
void RSL_wsr88d_merge_split_cuts_on();
void RSL_wsr88d_merge_split_cuts_off();
void RSL_wsr88d_keep_short_refl();
int wsr88d_merge_split_cuts_is_set();

Radar *wsr88d_merge_split_cuts(Radar *radar);
void RSL_wsr88d_asis();
void RSL_wsr88d_keep_sails();

/* Debugging prototypes. */
void poke_around_volume(Volume *v);

/* SYSTEM: left out prototypes? */
extern int pclose (FILE *f); /* From stdio.h */

#endif
