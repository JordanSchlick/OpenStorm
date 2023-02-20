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
 *------------------------------------------------------------
 * v1.14 5/12/95
 *------------------------------------------------------------
 *  Procedures:
 *   wsr88d_open
 *   wsr88d_close
 *   wsr88d_read_file_header
 *   wsr88d_read_tape_header
 *   wsr88d_read_sweep
 *   wsr88d_read_ray
 *   wsr88d_perror
 *   wsr88d_ray_to_float
 *
 *  Functions:
 *   wsr88d_get_nyquist
 *   wsr88d_get_atmos_atten_factor
 *   wsr88d_get_velocity_resolution
 *   wsr88d_get_volume_coverage
 *   wsr88d_get_elevation_angle
 *   wsr88d_get_azimuth
 *   wsr88d_get_range
 *   wsr88d_get_data
 *   wsr88d_get_time
 *   wsr88d_get_vcp_info(int vcp_num,int el_num)
 *   wsr88d_get_fix_angle(Wsr88d_ray *ray)
 *   wsr88d_get_pulse_count(Wsr88d_ray *ray)
 *   wsr88d_get_azimuth_rate(Wsr88d_ray *ray)
 *   wsr88d_get_pulse_width(Wsr88d_ray *ray)
 *   wsr88d_get_prt(Wsr88d_ray *ray)
 *   wsr88d_get_prf(Wsr88d_ray *ray)
 *   wsr88d_get_wavelength(Wsr88d_ray *ray)
 *   wsr88d_get_frequency(Wsr88d_ray *ray)
 * 
 *  Misc. routines: (canidates for possible inclusion into the library)
 *   print_head
 *   print_packet_info
 *   free_and_clear_sweep
 *   clear_sweep
 *   print_sweep_info
 *   Cvt_date                    <- From Dan Austin
 *   Cvt_time                    <- From Dan Austin
 *
 */ 


#include "wsr88d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>


#include <fcntl.h>
#include <sys/types.h>
//#include "bzip2/bzlib.h"
//#include "Misc/Compression.h"
#include "wsr88d_decode_ar2v.h"



static int little_endian(void)
{
	union {
		unsigned char byte[4];
		int val;
	} word;
	word.val = 0;
	word.byte[3] = 0x1;
	return word.val != 1;
}


static void swap_4_bytes(void *word)
{
	unsigned char *byte;
	unsigned char temp;
	byte    = (unsigned char*)word;
	temp    = byte[0];
	byte[0] = byte[3];
	byte[3] = temp;
	temp    = byte[1];
	byte[1] = byte[2];
	byte[2] = temp;
}

static void swap_2_bytes(void *word)
{
	unsigned char *byte;
	unsigned char temp;
	byte    = (unsigned char*)word;
	temp    = byte[0];
	byte[0] = byte[1];
	byte[1] = temp;
}
	
/**********************************************************************/
/*   D E B U G G I N G     R O U T I N E S    F O L L O W             */
/**********************************************************************/
/************************************************
 * Cvt_date-  convert the date in days since 1/1/70 (Julian) to mm/dd/yy 
 * parameters:
 * long int date_in - input julian date
 * returns: output date 
 * calls from: Cvt_pckt_hdr
 * calls to: none
 ************************************************/

#include <time.h>
int Cvt_date(long int date_in)
{
	int mm, dd, yy;
	time_t itime;
	struct tm *tm_time;
	itime = date_in - 1;
	itime *= 24*60*60; /* Seconds/day * days. */

	tm_time = gmtime(&itime);
	mm = tm_time->tm_mon+1;
	dd = tm_time->tm_mday;
	yy = tm_time->tm_year;

	return 10000.0*yy+100.0*mm+dd;
}

/************************************************
 * Cvt_time- converts 24 hr time in msecs after midnight to hhmmss
 * parameters:
 * long int time_in - input time in msecs after midnight
 * returns: double *time_out - output time
 * calls from: Cvt_pckt_hdr
 * calls to: none
 ************************************************/
float Cvt_time(long int time_in)
{
		double t;
		int hh,mm;

		t = time_in;
		t /= 1000.0;
		hh = t/3600;
		t -= hh*3600;
		mm = t/60;
		t -= mm*60;
		
		return hh*10000 + mm*100 + (float) t;
}
/**********************************************************************/
/*                                                                    */
/*  done 2/28        print_head                                       */
/*                                                                    */
/**********************************************************************/
void print_head(Wsr88d_file_header h)
{
	int i;
	fprintf(stderr,"Filename : ");
	for (i=0;i<9;i++) fprintf(stderr,"%c", h.title.filename[i]);   printf("\n");

	fprintf(stderr,"Extension: ");
	for (i=0;i<3;i++) fprintf(stderr,"%c", h.title.ext[i]);   printf("\n");

	fprintf(stderr,"Julian date: %d\n", Cvt_date(h.title.file_date));
	fprintf(stderr,"       time: %f\n", Cvt_time(h.title.file_time));

	
}

void print_packet_info(Wsr88d_packet *p)
{
	fprintf(stderr,"%5hd %5hd %5hd %5hd %5hd %5hd %5hd %10.3f %6d\n",
				 p->msg_type, p->id_seq, p->azm, p->ray_num, p->ray_status, p->elev, p->elev_num,
				 Cvt_time((int)p->ray_time), Cvt_date((int)p->ray_date));
}




/**********************************************************************/
/* End of debug routines.                                             */
/**********************************************************************/

void free_and_clear_sweep(Wsr88d_sweep *s, int low, int high)
{
/* Frees and sets the ray pointers to NULL.
 * Assumes that rays pointers have been allocated.
 */
	int i;
	for (i=low; i<high; i++)
		if (s->ray[i] != NULL) {
			free(s->ray[i]);
			s->ray[i] = NULL;
		}
}

void clear_sweep(Wsr88d_sweep *s, int low, int high)
{
/*
 * Simply set all sweep pointers to NULL.
 */
	int i;
	for (i=low; i<high; i++)
		s->ray[i] = NULL;
}

void wsr88d_print_sweep_info(Wsr88d_sweep *s)
{
	int i;

	fprintf(stderr,"Mtype    ID  azim  ray# rstat  elev elev#       time   date\n");
	fprintf(stderr,"----- ----- ----- ----- ----- ----- ----- ---------- ------\n");

	for (i=0; i<MAX_RAYS_IN_SWEEP; i++) {
		if (s->ray[i] != NULL) 
			print_packet_info((Wsr88d_packet *) s->ray[i]);
	}
}


// adapted from uncompress_pipe in gzip.c
//FILE* uncompress_pipe_ar2v(FILE* fp)
//{
	/* Pass the file pointed to by 'fp' through the bzip2 pipe. */

	//FILE *fpipe;
	//int save_fd;

	//if (no_command("wsr88d_decode_ar2v > /dev/null")){
	//  fprintf(stderr, "wsr88d_decode_ar2v not found, aborting ...\n");
	//  return fp;
	//}
	//save_fd = dup(0);
	//close(0); /* Redirect stdin for gzip. */
	//dup(fileno(fp));

	//fpipe = popen("wsr88d_decode_ar2v --stdout", "r");
	//if (fpipe == NULL) perror("uncompress_pipe_ar2v");
	//close(0);
	//dup(save_fd);
	//close(save_fd);
	//fclose(fp);
	//return fpipe;
//}


/**********************************************************************/
/*                                                                    */
/*  done 2/28             wsr88d_open                                 */
/*                                                                    */
/**********************************************************************/

Wsr88d_file *wsr88d_open(char *filename)
{
#ifdef _WIN32
	_set_fmode(_O_BINARY);
#endif // _WIN32

	Wsr88d_file *wf = (Wsr88d_file *)malloc(sizeof(Wsr88d_file));
	//int save_fd;

	//if ( strcmp(filename, "stdin") == 0 ) {
	//	save_fd = dup(0);
	//	wf->fptr = fdopen(save_fd,"r");
	//} else {
	//	wf->fptr = fopen(filename, "r");
	//}
	wf->fptr = fopen(filename, "r");

	if (wf->fptr == NULL){
		fprintf(stderr,"failed open file\n");
		return NULL;
	}

	// first check how the data are compressed by reading first few of magic bytes
	unsigned char hdrplus4[28];
	char bzmagic[4];
	int ar2v6bzip = 0;
	int isGzip = 0;
	fpos_t pos;
	fgetpos(wf->fptr, &pos);
	if (fread(hdrplus4, sizeof(hdrplus4), 1, wf->fptr) != 1) {
		fprintf(stderr,"failed to read first 28 bytes of Wsr88d file\n");
		fclose(wf->fptr);
		return NULL;
	}
	if (fread(bzmagic, sizeof(bzmagic), 1, wf->fptr) != 1) {
		fprintf(stderr,"failed to read bzip magic bytes from Wsr88d file\n");
		fclose(wf->fptr);
		return NULL;
	}
	// test for bzip2 magic.
	if (strncmp("BZ", bzmagic,2) == 0){
		ar2v6bzip = 1;
	}
	// test for gzip magic bytes
	if (hdrplus4[0] == 0x1f && hdrplus4[1] == 0x8b){
		isGzip = 1;
	}

	fclose(wf->fptr);

	// reopen the file
	//if ( strcmp(filename, "stdin") == 0 ) {
	//	 save_fd = dup(0);
	//	 wf->fptr = fdopen(save_fd,"r");
	//} else {
	//	 wf->fptr = fopen(filename, "r");
	//}
	wf->fptr = fopen(filename, "r");
 
	




	/*
	FILE* testFile = fopen(filename, "r");
	int totalSize = 0;
	int amountRead = 0;
	char buf[1024] = {};
	do {
		fprintf(stderr, "ftell: %i\n", (int)ftell(testFile));
		amountRead = fread(buf, 1, 1024, testFile);
		fprintf(stderr, "read: %i\n", amountRead);
		totalSize += amountRead;
	} while (amountRead != 0);
	fprintf(stderr, "ftell: %i\n", (int)ftell(testFile));
	fprintf(stderr, "file size: %i\n", totalSize);
	fclose(testFile);
	*/

	// decompress
	if(isGzip){
		//fprintf(stderr, "decode gzip here\n");
		wf->fptr = uncompress_pipe_gzip(wf->fptr);
	} else if(ar2v6bzip){
		wf->fptr = uncompress_pipe_ar2v(wf->fptr);
	}
	//else{
	//	 wf->fptr = uncompress_pipe(wf->fptr);
	//}
	
	


	if(wf->fptr == NULL){
		fprintf(stderr,"failed open file for decoding\n");
		return NULL;
	}

	#define NEW_BUFSIZ 16384
	setvbuf(wf->fptr,NULL,_IOFBF,(size_t)NEW_BUFSIZ); /* Faster i/o? */
	return wf;
}


/**********************************************************************/
/*                                                                    */
/*  done 2/28             wsr88d_perror                               */
/*                                                                    */
/**********************************************************************/
int wsr88d_perror(char *message)
{
/* 
 * I want to use a global 'wsr88d_errno' and
 * have this routine print an appropriate message.
 */

	/* This is a simple model now. */
	fprintf(stderr, "wsr88d_error: ");
	perror(message);
	return 0;
}

/**********************************************************************/
/*                                                                    */
/*  done 2/28             wsr88d_close                                */
/*                                                                    */
/**********************************************************************/
int wsr88d_close(Wsr88d_file *wf)
{
	int rc;
	rc = rsl_pclose(wf->fptr);
	free(wf);
	return rc;
}


/**********************************************************************/
/*                                                                    */
/*                     wsr88d_swap_file_header                        */
/*                                                                    */
/**********************************************************************/
void wsr88d_swap_file_header(Wsr88d_file_header *header)
{
	swap_4_bytes(&header->title.file_date);
	swap_4_bytes(&header->title.file_time);
}
	
/**********************************************************************/
/*                                                                    */
/*  done 2/28          wsr88d_read_file_header                        */
/*                                                                    */
/**********************************************************************/
int wsr88d_read_file_header(Wsr88d_file *wf,
														Wsr88d_file_header *wsr88d_file_header)
{
	int n;
	n = fread(&wsr88d_file_header->title,
						sizeof(wsr88d_file_header->title), 1, wf->fptr);
	if (little_endian())
		wsr88d_swap_file_header(wsr88d_file_header);
	return n;
}

/**********************************************************************/
/*                                                                    */
/*  done 8/18          wsr88d_read_tape_header                        */
/*                                                                    */
/**********************************************************************/
int wsr88d_read_tape_header(char *first_file,
														Wsr88d_tape_header *wsr88d_tape_header)
{
	FILE *fp;
	int n;
	char c;

	if ((fp = fopen(first_file, "r")) == NULL) {
		perror(first_file);
		return 0;
	}
	
	n = fread(wsr88d_tape_header, sizeof(Wsr88d_tape_header), 1, fp);
	if (n == 0) {
		fprintf(stderr, "WARNING: %s is smaller than 31616 bytes.  It is not a tape header file.\n", first_file);
	} else {
		/* Try to read one more character.  If we can, then this is not a 
		 * tape header file.  I suppose that we could look for '.' as the
		 * 9-th character and if it were there, then too this is not a tape
		 * header file.
		 */
		if (fread(&c, sizeof(char), 1, fp) > 0) {
			fprintf(stderr, "WARNING: %s is larger than 31616 bytes.  It is not a tape header file.\n", first_file);
			memset(wsr88d_tape_header, 0, sizeof(Wsr88d_tape_header));
			n = 0;
		} else { /* Ok so far. Now check the first 8 bytes for "ARCHIVE2" */
			if (strncmp(wsr88d_tape_header->archive2, "ARCHIVE2", 8) != 0) {
				fprintf(stderr, "WARNING: %s is 31616 bytes.  However, the first 8 bytes are not 'ARCHIVE2'.\n", first_file);
				memset(wsr88d_tape_header, 0, sizeof(Wsr88d_tape_header));
				n = 0;
			}
		}
		
	}
	fclose(fp);
	return n;
}



/**********************************************************************/
/*                                                                    */
/*  not done N/A       wsr88d_read_header                             */
/*                                                                    */
/**********************************************************************/
int wsr88d_read_header(Wsr88d_file *wf, Wsr88d_header *wsr88d_header)
{
	fprintf(stderr,"Routine: wsr88d_read_header\n");
	return 0;
}


/**********************************************************************/
/*                                                                    */
/*  done 3/2           wsr88d_read_sweep                              */
/*                                                                    */
/**********************************************************************/
int wsr88d_read_sweep(Wsr88d_file *wf, Wsr88d_sweep *wsr88d_sweep)
{
	int n;
	Wsr88d_ray wsr88d_ray;
	int nrec;
	int end_of_volume;
	int ray_num;

/* One sweep is defined as staying at the same RDA elevation number. */
/* We can read the file and check for that, however, we will need to
 * buffer our input.  The solution is to read the file and check the
 * radial status.  If it is '2' then we have reached the END OF ELEVATION.
 * Here is a complete list of radial status codes:
 *    0 = Start of new elevation.
 *    1 = Intermediate radial.
 *    2 = End of elevation.
 *    3 = Beginning of volume scan.
 *    4 = End of volume scan.
 */

/* Algorithm steps:
 *  1. Skip packets until.  Start of new elevation or
 *     Beginning of Volume scan.  STAT=0 or 3.
 *  2. Read until End of elevation.  STAT=2 or 4.  Skip message type != 1.
 */

	nrec = 0;
	ray_num = 0;
	n = wsr88d_read_ray(wf, &wsr88d_ray);

/* Step 1. */
	while ((wsr88d_ray.msg_type & 15) != 1 && n > 0) {
		/*
		fprintf(stderr,"SKIPPING packet: type %d, radial status %d\n",
					 wsr88d_ray.msg_type, wsr88d_ray.ray_status);
		*/
		n = wsr88d_read_ray(wf, &wsr88d_ray);
	}
		
	if (n <= 0) return n; /* Read failure. */
	end_of_volume = 0;
/* Step 2. */
	while ( ! end_of_volume ) {
		if ((wsr88d_ray.msg_type & 15) != 1) {
			/*
			fprintf(stderr,"SKIPPING (amid a sweep) packet: type %d, "
						"radial status %d\n",
						 wsr88d_ray.msg_type, wsr88d_ray.ray_status);
			 */

		} else {
			/* Load this ray into the sweep. */
			ray_num = wsr88d_ray.ray_num;
			/* Double check against #  records we've seen. */
			/* It is possible that a reset occurs and we begin to overwrite
			 * previously loaded rays.  I've seen this occur, rarely, in the
			 * WSR88D data.  I must trust 'ray_num'.
			 */
			/*
			if (nrec+1 != ray_num) {
				fprintf(stderr, "Data says %d is ray_num, but, I've seen %d "
								"records.\n", ray_num, nrec+1);
			}
			*/
			if (wsr88d_sweep->ray[ray_num] == NULL) {
				wsr88d_sweep->ray[ray_num] = (Wsr88d_ray *) malloc (sizeof(Wsr88d_ray));
			}
			memcpy(wsr88d_sweep->ray[ray_num], &wsr88d_ray, sizeof(Wsr88d_ray));
		}
		n = wsr88d_read_ray(wf, &wsr88d_ray);
		if (n > 0) nrec++;
		end_of_volume = wsr88d_ray.ray_status == 2 ||
										wsr88d_ray.ray_status == 4 ||
										n <= 0;
	}

	/* Process the last packet of the input data. */
	if ((wsr88d_ray.ray_status == 2 || wsr88d_ray.ray_status == 4) &&
		(wsr88d_ray.msg_type & 15) == 1) {
		/* Load this ray into the sweep. */
		ray_num = wsr88d_ray.ray_num;
		if (wsr88d_sweep->ray[ray_num] == NULL) {
			wsr88d_sweep->ray[ray_num] = (Wsr88d_ray *) malloc (sizeof(Wsr88d_ray));
		}
		memcpy(wsr88d_sweep->ray[ray_num], &wsr88d_ray, sizeof(Wsr88d_ray));
	}

	/* Just to be safe, clear all ray pointers left in this sweep to
	 * the maximum MAX_RAYS_IN_SWEEP.  This is required when the 
	 * wsr88d_sweep is reused and not cleared.
	 */
	free_and_clear_sweep(wsr88d_sweep, ray_num+1, MAX_RAYS_IN_SWEEP);
	
/*
	fprintf(stderr,"Processed %d records for elevation number %d\n",
				 nrec+1, wsr88d_ray.elev_num);
	wsr88d_print_sweep_info(wsr88d_sweep);
*/
	return nrec;
}

/**********************************************************************/
/*                                                                    */
/*                      wsr88d_swap_ray                               */
/*                                                                    */
/**********************************************************************/
void wsr88d_swap_ray(Wsr88d_ray *wsr88d_ray)
{
	short *half_word;
	half_word = (short *)wsr88d_ray;
	for (; half_word<(short *)&wsr88d_ray->msg_time; half_word++)
		swap_2_bytes(half_word);

	swap_4_bytes(&wsr88d_ray->msg_time);
	swap_2_bytes(&wsr88d_ray->num_seg);
	swap_2_bytes(&wsr88d_ray->seg_num);
	swap_4_bytes(&wsr88d_ray->ray_time);
	
	half_word = (short *) &wsr88d_ray->ray_date;
	for (; half_word<(short *)&wsr88d_ray->sys_cal; half_word++)
		swap_2_bytes(half_word);

	swap_4_bytes(&wsr88d_ray->sys_cal);

	half_word = (short *) &wsr88d_ray->refl_ptr;
	for (; half_word<(short *)&wsr88d_ray->data[0]; half_word++)
		swap_2_bytes(half_word);

}

/**********************************************************************/
/*                                                                    */
/*  done 2/28           wsr88d_read_ray                               */
/*                                                                    */
/**********************************************************************/
int wsr88d_read_ray(Wsr88d_file *wf, Wsr88d_ray *wsr88d_ray)
{
	int n;
	n = fread(wsr88d_ray, sizeof(Wsr88d_ray), 1, wf->fptr);
/*  if (n > 0) print_packet_info(wsr88d_ray); */
	if (little_endian())
		wsr88d_swap_ray(wsr88d_ray);

	return n;
}

/**********************************************************************/
/*                                                                    */
/*  not done N/A     wsr88d_read_ray_header                           */
/*                                                                    */
/**********************************************************************/
int wsr88d_read_ray_header(Wsr88d_file *wf,
													 Wsr88d_ray_header *wsr88d_ray_header)
{
	fprintf(stderr,"Stub routine: wsr88d_read_ray_header.\n");
	return 0;
}

/**********************************************************************/
/*                                                                    */
/*  done 3/3         wsr88d_ray_to_float                              */
/*                                                                    */
/**********************************************************************/
int wsr88d_ray_to_float(Wsr88d_ray *ray,
												int THE_DATA_WANTED, float v[], int *n)
{
/*
 *  Input: *ray             -  WSR-88D packet
 * Output: THE_DATA_WANTED  -  Indicates which field to convert.  Fields:
 *                             WSR88D_DZ, WSR88D_VR, WSR88D_SW
 *         v[]              -  The output vector of float values.         
 *         n                -  Length of the output vector.  0 = no data.
 *
 * Returns n.
 *
 * No allocation of space for the output vector performed here.
 */

/* Code from Dan Austin (cvt_pckt_data.c) was the template for this. */

	/* declarations   */
	int num_ref_gates,num_vel_gates,num_spec_gates;
	int refl_ptr, vel_ptr,spec_ptr,res_flag;
	int ival;
	int i;
	
	*n = 0;
	num_ref_gates  = ray->num_refl;
	num_vel_gates  = ray->num_dop;
	num_spec_gates = ray->num_dop;  /* 'num_dop', this is not a typo. */

/* The data pointers are specified from the begining of the 
 * 'Digital Radar Data (Message) Header'.  Since we have a structure
 * that defines all the header variables and a member called 'data'.
 * we must subtract the length of the 'message header' from the data
 * pointer.  Hopefully, the reflecivity pointer will be 0 meaning the
 * first element of the 'data' member; ray->data[0];
 */
#define LENGTH_OF_MESSAGE 100
	if (num_ref_gates > 0) refl_ptr = ray->refl_ptr - LENGTH_OF_MESSAGE;
	else refl_ptr = 0;
	
	vel_ptr  = ray->vel_ptr - LENGTH_OF_MESSAGE;
	spec_ptr = ray->spc_ptr - LENGTH_OF_MESSAGE;
	
	res_flag = ray->vel_res;


/*
	fprintf(stderr,"refl_ptr = %d  #g = %d, ", refl_ptr, num_ref_gates);
	fprintf(stderr," vel_ptr = %d  #g = %d, ", vel_ptr, num_vel_gates);
	fprintf(stderr,"spec_ptr = %d  #g = %d, ", spec_ptr, num_spec_gates);
	fprintf(stderr,"res_flag = %d\n", res_flag);
*/

	if (THE_DATA_WANTED == WSR88D_DZ) {
		/* do the reflectivity data  (dbZ)*/
		if (refl_ptr+num_ref_gates > 2300) 
			fprintf(stderr, "WARNING: # refl index (%d) exceeds maximum (2300)\n",
							refl_ptr+num_ref_gates);
		else {
		for(i=0; i<num_ref_gates; i++) {
			ival = ray->data[refl_ptr+i];
			if(ival > 1)
					v[i] = (((ival-2.0)/2.0)-32.0);
			else if (ival == 1) 
				v[i] = WSR88D_RFVAL;
			else /* ival = 0 */
				v[i] = WSR88D_BADVAL;
		}
		*n = num_ref_gates;
		}

	} else if (THE_DATA_WANTED == WSR88D_VR) {
		/* do the velocity data  (M/S) */
		if (vel_ptr+num_vel_gates > 2300) 
			fprintf(stderr, "WARNING: # vel index (%d) exceeds maximum (2300)\n",
							vel_ptr+num_vel_gates);
		else {
		for(i=0; i<num_vel_gates;i++)   {
			ival = ray->data[vel_ptr+i];
			if(ival > 1)
				if (res_flag == 2) /* High resolution: 0.5 m/s */
					v[i] = (((ival-2.0)/2.0)-63.5);
				else
					v[i] = ((ival-2.0)-127.0);
			else if (ival == 1) 
				v[i] = WSR88D_RFVAL;
			else /* ival = 0 */
				v[i] = WSR88D_BADVAL;
		}
		*n = num_vel_gates;
		}
		
	} else if (THE_DATA_WANTED == WSR88D_SW) {
		/* now do the spectrum width data (M/S)*/
		if (spec_ptr+num_spec_gates > 2300) 
			fprintf(stderr, "WARNING: # spec index (%d) exceeds maximum (2300)\n",
							spec_ptr+num_spec_gates);
		else {
		for(i=0;i<num_spec_gates;i++) {
			ival = ray->data[spec_ptr+i];
				if(ival > 1)
					v[i] = (((ival-2)/2.0)-63.5);
				else if (ival == 1) 
					v[i] = WSR88D_RFVAL;
				else /* ival = 0 */
					v[i] = WSR88D_BADVAL;
		}
		*n = num_spec_gates;
		}
	}
	
	return *n;
}



/**********************************************************************/
/*        Functions that convert some message header values.          */
/**********************************************************************/
/**********************************************************************/
/*                                                                    */
/*  done 3/3   float wsr88d_get_nyquist                               */
/*  done 3/3   float wsr88d_get_atmos_atten_factor                    */
/*  done 3/3   float wsr88d_get_velocity_resolution                   */
/*  done 3/3   int   wsr88d_get_volume_coverage                       */
/*  done 3/3   float wsr88d_get_elevation_angle                       */
/*  done 3/3   float wsr88d_get_azimuth                               */
/*  done 3/3   float wsr88d_get_range                                 */
/*  done 3/3   void  wsr88d_get_date                                  */
/*  done 3/3   void  wsr88d_get_time                                  */
/*  done 5/20  int  *wsr88d_get_vcp_info                              */
/*  done 5/20  float wsr88d_get_fix_angle                             */
/*  done 5/20  int   wsr88d_get_pulse_count                           */
/*  done 5/20  float wsr88d_get_azimuth_rate                          */
/*  done 5/20  float wsr88d_get_pulse_width                           */
/*  done 5/20  float wsr88d_get_prf                                   */
/*  done 5/20  float wsr88d_get_prt                                   */
/*  done 5/20  float wsr88d_get_wavelength                            */
/*  done 5/20  float wsr88d_get_frequency                             */
/*                                                                    */
/**********************************************************************/
float wsr88d_get_nyquist(Wsr88d_ray *ray)
{
	return ray->nyq_vel/100.0;
}

float wsr88d_get_atmos_atten_factor(Wsr88d_ray *ray)
{
	return ray->atm_att/1000.0;
}

float wsr88d_get_velocity_resolution(Wsr88d_ray *ray)
{
	if (ray->vel_res == 2) return 0.5;
	return 0.0;
}

int   wsr88d_get_volume_coverage(Wsr88d_ray *ray)
{
	if (ray->vol_cpat == 11) return 11;
	if (ray->vol_cpat == 12) return 12;
	if (ray->vol_cpat == 21) return 21;
	if (ray->vol_cpat == 31) return 31;
	if (ray->vol_cpat == 32) return 32;
	if (ray->vol_cpat == 121) return 121;
	if (ray->vol_cpat == 211) return 211;
	if (ray->vol_cpat == 212) return 212;
	if (ray->vol_cpat == 221) return 221;
	return 0;
}

float wsr88d_get_elevation_angle(Wsr88d_ray *ray)
{
	return ray->elev/8.0*(180.0/4096.0);
}

float wsr88d_get_azimuth(Wsr88d_ray *ray)
{
	return ray->azm/8.0*(180.0/4096.0);
}

float wsr88d_get_range(Wsr88d_ray *ray)
{
	return ray->unam_rng/10.0;
}

#include <time.h>
void wsr88d_get_date(Wsr88d_ray *ray, int *mm, int *dd, int *yy)
{
/*
 * mm (1-12)
 * dd (1-31)
 * yy (ex. 93)
 */
	time_t itime;
	struct tm *tm_time;
	if (ray == NULL) {
		*mm = *dd = *yy = 0;
		return;
	}

	itime = ray->ray_date - 1;
	itime *= 24*60*60; /* Seconds/day * days. */

	tm_time = gmtime(&itime);
	*mm = tm_time->tm_mon+1;
	*dd = tm_time->tm_mday;
	*yy = tm_time->tm_year;
}

void wsr88d_get_time(Wsr88d_ray *ray, int *hh, int *mm, int *ss, float *fsec)
{
	/*
	 * hh (0-23)
	 * mm (0-59)
	 * ss (0-59)
	 * fsec (fraction of second)
	 */
	double t;
	
	if (ray == NULL) {
		*hh = *mm = *ss = *fsec = 0;
		return;
	}
	t = ray->ray_time;
	t /= 1000.0;
	*hh = t/3600;
	t -= *hh*3600;
	*mm = t/60;
	t -= *mm*60;
	*ss = (int)t;
	*fsec = t - *ss;
}



/*
 * Get_vcp_info - gets info about the volume coverage pattern for this scan
 * parameters:
 * int vcp_num - volume coverage pattern number 
 * int el_num - elevation number w/in vcp
 * returns: int *vcp_info - ptr to array w/vcp info 
 * calls from: Nexrad2uf
 * calls to: none
 */

/* this database contains volume coverage patterns & associated info:   */
/* (0)= vcp # (1)=pulse width for vcp  "Id$"                            */
/* line[1-n]: (n,0)= elev. # for vcp (n,1)= (fixed angle)*8*(4096/180)  */
/* (n,2)= pulse count (n,3)= (azimuthal sweep rate)*8*(4096/45)         */

static int vcp11[68] ={11,514,88,17,13600,88,0,14000,264,16,12664,264,0,14000,440,6,11736,608,6,24760,784,6,24760,952,10,12712,1128,10,12720,1368,0,18328,1584,0,18496,1824,0,18512,2184,0,18544,2552,0,18576,3040,0,18640,3552,0,18712};

static int vcp12[53]={12,514,91,15,15401,91,0,18204,164,15,15401,164,0,18204,237,15,15401,237,0,18204,328,3,19297,437,3,20393,564,3,20393,728,3,20393,928,3,20393,1165,0,20680,1456,0,20680,1820,0,21033,2276,0,20929,2840,0,20929,3550,0,20929};

static int vcp21[48]={21,514,88,28,8256,88,0,8272,264,28,8256,264,0,8272,440,8,7888,608,8,7888,784,8,8160,1096,12,8160,1800,0,10640,2656,0,10432,3552,0,10496};

static int vcp31[36]={31,516,88,63,3672,88,0,3688,272,63,3672,272,0,3688,456,63,3672,456,0,3688,640,0,3688,816,0,3688};

static int vcp32[32]={32,514,88,64,3616,88,0,3312,272,64,3616,272,0,3312,456,11,2960,640,11,2960,816,11,2960};

static int vcp121[62]={121,514,91,11,21336,91,0,21696,91,0,19952,91,0,15584,264,11,21336,264,0,21696,264,0,19952,264,0,15584,437,6,13985,437,0,19952,437,0,15584,610,6,15729,610,0,19952,610,0,15584,783,6,11872,783,0,21481,1092,6,14712,1802,0,21481,2658,0,21696,3550,0,21696};

static int vcp300[20]={300,514,88,28,8256,88,0,8272,440,8,8160,1800,0,10384};

int *wsr88d_get_vcp_info(int vcp_num,int el_num)
{
/*
 * This routine from Dan Austin.  Program component of nex2uf.
 */
		static int vcp_info[4];
		int fix_angle;
		int pulse_cnt;
		int az_rate;
		int pulse_width;

		/* case statement to get vcp info */
		switch(vcp_num) {
		case 11:
			fix_angle =   vcp11[(3*el_num)-1];
			pulse_cnt =   vcp11[(3*el_num)];
			az_rate =     vcp11[(3*el_num)+1];
			pulse_width = vcp11[1];
			break;
		case 12:
			fix_angle =   vcp12[(3*el_num)-1];
			pulse_cnt =   vcp12[(3*el_num)];
			az_rate =     vcp12[(3*el_num)+1];
			pulse_width = vcp12[1];
			break;
		case 21:
			fix_angle =   vcp21[(3*el_num)-1];
			pulse_cnt =   vcp21[(3*el_num)];
			az_rate =     vcp21[(3*el_num)+1];
			pulse_width = vcp21[1];
			break;
		case 31:
			fix_angle =   vcp31[(3*el_num)-1];
			pulse_cnt =   vcp31[(3*el_num)];
			az_rate =     vcp31[(3*el_num)+1];
			pulse_width = vcp31[1];
			break;
		case 32:
			fix_angle =   vcp32[(3*el_num)-1];
			pulse_cnt =   vcp32[(3*el_num)];
			az_rate =     vcp32[(3*el_num)+1];
			pulse_width = vcp32[1];
			break;
		case 300:
			fix_angle =   vcp300[(3*el_num)-1];
			pulse_cnt =   vcp300[(3*el_num)];
			az_rate =     vcp300[(3*el_num)+1];
			pulse_width = vcp300[1];
			break;
		case 121:
			fix_angle =   vcp121[(3*el_num)-1];
			pulse_cnt =   vcp121[(3*el_num)];
			az_rate =     vcp121[(3*el_num)+1];
			pulse_width = vcp121[1];
			break;
		case 211:
			fix_angle =   vcp11[(3*el_num)-1];
			pulse_cnt =   vcp11[(3*el_num)];
			az_rate =     vcp11[(3*el_num)+1];
			pulse_width = vcp11[1];
			break;
		case 212:
			fix_angle =   vcp12[(3*el_num)-1];
			pulse_cnt =   vcp12[(3*el_num)];
			az_rate =     vcp12[(3*el_num)+1];
			pulse_width = vcp12[1];
			break;
		case 221:
			fix_angle =   vcp21[(3*el_num)-1];
			pulse_cnt =   vcp21[(3*el_num)];
			az_rate =     vcp21[(3*el_num)+1];
			pulse_width = vcp21[1];
			break;
		default:
			fix_angle  = 0;
			pulse_cnt  = 0;
			az_rate    = 0;
			pulse_width= 0;
			break;
		}
		
		/* get array for output */
		vcp_info[0]=fix_angle;
		vcp_info[1]=pulse_cnt;
		vcp_info[2]=az_rate;
		vcp_info[3]=pulse_width;
		
		
		/* return the value array   */
		return(vcp_info);
}


float wsr88d_get_fix_angle(Wsr88d_ray *ray)
{
	int *vcp_info;
	vcp_info = wsr88d_get_vcp_info(ray->vol_cpat, ray->elev_num);
	return vcp_info[0]/8.0*180./4096.0;
}
int wsr88d_get_pulse_count(Wsr88d_ray *ray)
{
	int *vcp_info;
	vcp_info = wsr88d_get_vcp_info(ray->vol_cpat, ray->elev_num);
	return vcp_info[1];
}
float wsr88d_get_azimuth_rate(Wsr88d_ray *ray)
{
	int *vcp_info;
	vcp_info = wsr88d_get_vcp_info(ray->vol_cpat, ray->elev_num);
	return vcp_info[2]/8.0*45./4096.0;
}
float wsr88d_get_pulse_width(Wsr88d_ray *ray)
{
	int *vcp_info;
	vcp_info = wsr88d_get_vcp_info(ray->vol_cpat, ray->elev_num);
	return vcp_info[3]/299.792458;
}

float wsr88d_get_prf(Wsr88d_ray *ray)
{
	float prf;
	float c = 299792458.0;
	float range;

	range = wsr88d_get_range(ray)*1000.0;
	if (range != 0) prf = c/(2*range);
	else prf = 0.0;

	return prf;
}

float wsr88d_get_prt(Wsr88d_ray *ray)
{
	float prf;
	float prt;

	prf = wsr88d_get_prf(ray);
	if (prf != 0) prt = 1.0/prf;
	else prt = 0;
	return prt;
}

/* Note: wsr88d_get_wavelength() below is no longer used because of differences
 * in wavelength for velocity and reflectivity.  The function computes
 * wavelength when Nyquist is present, but returns a constant wavelength
 * otherwise.  Nyquist is present for velocity, but not for reflectivity.  The
 * fact is that WSR-88D radars use a constant wavelength, 10.7 cm., which is
 * the value now used where this function was formerly called in
 * wsr88d_load_sweep_into_volume().
 */

float wsr88d_get_wavelength(Wsr88d_ray *ray)
{
	float wavelength;
	float prf;
	float nyquist;

	prf = wsr88d_get_prf(ray);
	nyquist = wsr88d_get_nyquist(ray);
		/* If required info to determine wavelength does not exist,
				 just use 10 cm. All wsr88d radars are 10cm. MJK */
	if ((prf == 0) || (nyquist == 0.0)) wavelength = 0.10;
	else wavelength = 4*nyquist/prf;
	return wavelength;
}

float wsr88d_get_frequency(Wsr88d_ray *ray)
{
	float freq;
	float c = 299792458.0;

		/* Carrier freq (GHz). Revised 12 Jun 97. MJK */
		freq = (c / wsr88d_get_wavelength(ray)) * 1.0e-9;
	return freq;
}
