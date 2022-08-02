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
 * get_site - fills structure containing radar site info
 * parameters: 
 * char *in_sitenm - ptr to radar site name
 * returns: struct radar_site *currsite->- ptr to current site structure
 * Returns NULL on failure of 'in_sitenm' lookup.
 * calls from: Proc_file
*/

#include <stdio.h>
#include <string.h>
int strcasecmp(const char *s1, const char *s2);
#include <stdlib.h>
#include "wsr88d.h"

Wsr88d_site_info *wsr88d_get_site(char *in_sitenm)
{
	/* variable declarations	 */
	char line[100];
	int scan_count,i;
	int in_number,in_latd,in_latm,in_lats,in_lond,in_lonm,in_lons;
	int in_height;
	char in_site[5],in_city[16],in_state[4]; /* One extra for \0 */
	struct radar_site *currsite=NULL;
	FILE *in_file;

	if((in_file=fopen(WSR88D_SITE_INFO_FILE, "r")) !=NULL)
	{
    	/* read each line */
		while (fgets(line,sizeof(line),in_file) != NULL)
		{
 			scan_count=sscanf(line,"%d %s %s %s %d %d %d %d %d %d %d",&in_number,in_site,in_city,in_state,&in_latd,&in_latm,&in_lats,&in_lond,&in_lonm,&in_lons,&in_height);
			if(scan_count != 11 && scan_count != 0)
				fprintf(stderr,"get_site: sitedb read");
			else
			{
		    	/* get the radar site info */

				/* check for the matching site name*/
				if(strcasecmp(in_sitenm,in_site)== 0)
				  {
					if((currsite=(struct radar_site *)malloc(sizeof(struct radar_site)))==NULL) {
					  perror("wsr88d_get_site");
					  return NULL;
					}
				    currsite->number = in_number;
				    for(i=0;i<4;i++)
					currsite->name[i]   = in_site[i];
					for(i=0;i<15;i++)
						currsite->city[i]   = in_city[i];
					for(i=0;i<2;i++)
						currsite->state[i]  = in_state[i];
					currsite->latd   = in_latd;
					currsite->latm   = in_latm;
					currsite->lats   = in_lats;
					currsite->lond   = in_lond;
					currsite->lonm   = in_lonm;
					currsite->lons   = in_lons;
					currsite->height = in_height;
					currsite->bwidth = -999;
					currsite->spulse = 1530;
					currsite->lpulse = 4630;
					break;
				}
			}
		}
	/* close the file */
	(void)fclose(in_file);
	}
	else
	  perror(WSR88D_SITE_INFO_FILE);
	

	return(currsite);
}

