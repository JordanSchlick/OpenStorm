#ifndef _wsr88d
#define _wsr88d
#include <stdio.h>

/* Modify the following to point to the file that contains the 
 * nexrad (wsr88d) location information for each radar site.
 * The directory should be the same as the LIBDIR in the makefile.
 */
#ifndef WSR88D_SITE_INFO_FILE
#define WSR88D_SITE_INFO_FILE "/home/kelley/trmm/lib/wsr88d_locations.dat"
#endif
/*===============================================================*/
; typedef struct {
	char archive2[8];    /* Always ARCHIVE2 */
	char site_id[4];     /* 4-leter site ID.  e.g. KLMB */
	char tape_num[6];    /* NCDC tape number. e.g. N00001 */
	char b1;             /* Blank. */
	char date[9];        /* Date tape written. dd-MMM-yy e.g. 19-FEB-93 */
	char b2;             /* Blank. */
	char time[8];        /* Time tape written.  hh:mm:ss.  e.g. 10:22:59 */
	char b3;             /* Blank. */
	char data_center[5]; /* Data Center writing tape: RDASC or NCDC. */
	char wban_num[5];    /* WBAN number of this NEXRAD site.  This is a 
				unique 5-digit number assigned at
				NCDC.  Numbers are contained in the NCDC
				NEXRAD Station History file.  The file
				also contains the four letter site ID,
				Latitude, Longitude, Elevation, and
				common location name. */
	char tape_mode[5];   /* Tape output mode.  Current values are 8200, 8500,
				8500c. */
	char volume_num[5];  /* A volume number to be used for copies and
				extractions of data from tapes.  The form
				would be VOL01, VOL02, VOL03 ... VOLnn. */
	char b4[6];          /* Blank. Available for future use. */
	char b5[31552];      /* May be used for internal controls or
				other information at each archive center.
				Information of value to users will be
				documented at the time of tape shipment. */
} Wsr88d_tape_header;
							

/* Title record structure for nexrad archive2 data file.
	 The first record of each nexrad data file is a title record */
typedef struct
	 {
	 char     filename[9];
	 char     ext[3];
	 int      file_date;    /* modified Julian date */
	 int      file_time;    /* milliseconds of day since midnight */
	 char     unused1[4];
	 }
nr_archive2_title;

/* message packet structure for nexrad radar data */
typedef struct
	 {
	 short    ctm[6];    /* not used */

	 /* halfword 7 : message header information */
	 short    msg_size;  /* # halfwords from here to end of record? */
	 short    msg_type;  /* Digital Radar Data.  This message may contain
			* a combination of either reflectivity,
			* aliased velocity, or spectrum width.
			*/
	 short    id_seq;    /* I.d. Seq = 0 to 7FFF, then roll over to 0 */
	 short    msg_date;  /* modified Julian date from 1/1/70 */
	 int      msg_time;  /* packet generation time in ms past midnite */
	 short    num_seg;
	 short    seg_num;

	 /* halfword 15 : data header information */
	 int      ray_time;  /* collection time for this ray in ms */
	 short    ray_date;  /* modified Julian date for this ray */
	 short    unam_rng;  /* unambiguous range */
	 short    azm;       /* coded azimuth angle */
	 short    ray_num;   /* ray no. within elevation scan */
	 short    ray_status;/* ray status flag */
	 short    elev;      /* coded elevation angle */
	 short    elev_num;  /* elevation no. within volume scan */
	 
	 /* halfword 24 : gate/bin information*/
	 short    refl_rng;   /* range to first gate of refl data */
	 short    dop_rng;    /* range to first gate of doppler data */
	 short    refl_size;  /* refl data gate size */
	 short    dop_size;   /* doppler data gate size */
	 short    num_refl;   /* no. of reflectivity gates */
	 short    num_dop;    /* no. of doppler gates */
	 
	 /* halfword 30 */
	 short    sec_num;    /* sector no. within cut */
	 float    sys_cal;    /* gain calibration constant */

	 /* halfword 33 : data parameters */
	 short    refl_ptr;   /* reflectivity data ptr */ 
	 short    vel_ptr;    /* velocity data ptr */
	 short    spc_ptr;    /* spectrum width ptr */
	 short    vel_res;    /* Doppler velocity resolution */
	 short    vol_cpat;   /* volume coverage pattern */
	 short    unused1[4];
	 
	 /* halfword 42 : data pointers for Archive II playback */
	 short    ref_ptrp;
	 short    vel_ptrp;
	 short    spc_ptrp;
	 
	 /* halfword 45 */
	 short    nyq_vel;    /* Nyquist velocity */
	 short    atm_att;    /* atmospheric attenuation factor */
	 short    min_dif;

	 /* halfwords 48 to 64 */
	short    unused2[17];
	 
	 /* halfwords 65 to 1214 */
	 unsigned char     data[2300];

	 /* last 4 bytes : frame check sequence */
	 unsigned char     fts[4];
	 }
Wsr88d_packet;

/* structure for the radar site parameters */
typedef struct radar_site {
		int number;        /* arbitrary number of this radar site */
		char name[4];      /* Nexrad site name */
		char city[15];     /* nearest city to  radaar site */
		char state[2];     /* state of radar site */
		int latd;   /* degrees of latitude of site */
		int latm;   /* minutes of latitude of site */
		int lats;   /* seconds of latitude of site */
		int lond;   /* degrees of longitude of site */
		int lonm;   /* minutes of longitude of site */
		int lons;   /* seconds of longitude of site */
		int height; /* height of site in meters above sea level*/
		int bwidth; /* bandwidth of site (mhz) */
		int spulse; /* length of short pulse (ns)*/
		int lpulse; /* length of long pulse (ns) */
} Wsr88d_site_info;

typedef struct {
	FILE *fptr;
} Wsr88d_file;

#define PACKET_SIZE 2432
typedef Wsr88d_packet Wsr88d_ray;    /* Same thing, different name. */


#define MAX_RAYS_IN_SWEEP 400
typedef struct {
	Wsr88d_ray *ray[MAX_RAYS_IN_SWEEP];  /* Expected maximum is around 366. */
} Wsr88d_sweep;

typedef union {
	nr_archive2_title title;
	Wsr88d_packet p;
} Wsr88d_file_header;

typedef struct {
	int dummy;     /* Structure not used yet. */
} Wsr88d_header;

typedef struct {
	int dummy;     /* Structure not used yet. */
} Wsr88d_ray_header;

/* Selected so we can boolean or use them for a data mask. */
#define WSR88D_DZ 0x1
#define WSR88D_VR 0x2
#define WSR88D_SW 0x4
#define WSR88D_BADVAL  0500   /* non-meaningful value (500 octal) */
#define WSR88D_RFVAL (WSR88D_BADVAL-1) /* ival = 0 means below SNR,
																							 = 1 means range folded. */


/***********************************************************************/
/*                                                                     */
/*                   Function specification.                           */
/*                                                                     */
/***********************************************************************/
Wsr88d_file *wsr88d_open(char *filename);
int wsr88d_perror(char *message);
int wsr88d_close(Wsr88d_file *wf);
int wsr88d_read_file_header(Wsr88d_file *wf,
				Wsr88d_file_header *wsr88d_file_header);
int wsr88d_read_tape_header(char *first_file,
				Wsr88d_tape_header *wsr88d_tape_header);
int wsr88d_read_sweep(Wsr88d_file *wf, Wsr88d_sweep *wsr88d_sweep);
int wsr88d_read_ray(Wsr88d_file *wf, Wsr88d_ray *wsr88d_ray);
int wsr88d_read_ray_header(Wsr88d_file *wf,
				Wsr88d_ray_header *wsr88d_ray_header);
int wsr88d_ray_to_float(Wsr88d_ray *ray,
				int THE_DATA_WANTED, float v[], int *n);
float wsr88d_get_nyquist(Wsr88d_ray *ray);
float wsr88d_get_atmos_atten_factor(Wsr88d_ray *ray);
float wsr88d_get_velocity_resolution(Wsr88d_ray *ray);
int   wsr88d_get_volume_coverage(Wsr88d_ray *ray);
float wsr88d_get_elevation_angle(Wsr88d_ray *ray);
float wsr88d_get_azimuth(Wsr88d_ray *ray);
float wsr88d_get_range(Wsr88d_ray *ray);
void  wsr88d_get_date(Wsr88d_ray *ray, int *mm, int *dd, int *yy);
void  wsr88d_get_time(Wsr88d_ray *ray, int *hh, int *mm, int *ss, float *fsec);
Wsr88d_site_info *wsr88d_get_site(char *in_sitenm); /* Courtesy of Dan Austin. */
int *wsr88d_get_vcp_info(int vcp_num,int el_num);  /* Courtesy of Dan Austin. */
float wsr88d_get_fix_angle(Wsr88d_ray *ray);
int   wsr88d_get_pulse_count(Wsr88d_ray *ray);
float wsr88d_get_azimuth_rate(Wsr88d_ray *ray);
float wsr88d_get_pulse_width(Wsr88d_ray *ray);
float wsr88d_get_prf(Wsr88d_ray *ray);
float wsr88d_get_prt(Wsr88d_ray *ray);
float wsr88d_get_wavelength(Wsr88d_ray *ray);
float wsr88d_get_frequency(Wsr88d_ray *ray);

int no_command (char *cmd);
FILE *uncompress_pipe (FILE *fp);
FILE *compress_pipe (FILE *fp);
int rsl_pclose(FILE *fp);

#endif
