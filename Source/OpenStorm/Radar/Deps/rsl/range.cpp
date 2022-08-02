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
#include <math.h>
#include "rsl.h"

#  define M_PI		3.14159265358979323846
#  define M_PI_2	1.57079632679489661923
/* Earth radius in km. */
double Re = (6374.0*4.0/3.0);

/********************************************************************/
/*                                                                  */
/*               RSL_set_earth_rad(double new_Re)                   */
/*                                                                  */
/*  By: Dennis Flanigan                                             */
/*      Applied Research Corporation                                */
/*      August 26, 1994                                             */
/********************************************************************/
void RSL_set_earth_radius(double new_Re)
{
  Re = new_Re;
}

/*********************************************************************/
/*                                                                   */
/*         void RSL_get_gr_slantr_h                                  */
/*   *gr     - ground range in KM.                                   */
/*   *slantr - slant range in  KM.                                   */
/*   *h      - height in KM.                                         */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      June  7, 1995                                                */
/*********************************************************************/
void RSL_get_gr_slantr_h(Ray *ray, int i, float *gr, float *slantr, float *h)
{
  *gr = *slantr = *h = 0.0;
  if (ray == NULL) return;
  *slantr = i * ray->h.gate_size + ray->h.range_bin1;
  *slantr /= 1000; /* meters to km */
  RSL_get_groundr_and_h(*slantr, ray->h.elev, gr, h);
  return;
}

/*********************************************************************/
/*                                                                   */
/*         RSL_get_groundr_and_h(sr, elev, &gr, &h)                  */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      Space Applications Corporation                               */
/*      July 23, 1994                                                */
/*********************************************************************/
void RSL_get_groundr_and_h(float slant_r, float elev, float *gr, float *h)
{
/* Input:
 *   slant_r - slant range, along the beam, in km.
 *   elev    - elevation angle, in degrees.
 * 
 * Output:
 *   gr      - Ground range in km.
 *   h       - Height of data point above earth, in km.
 */
  double GR;
  double H;

  if (slant_r == 0.0) {*h = 0; *gr = 0; return;}
  elev += 90;
  H = sqrt( pow(Re,2.0) + pow(slant_r,2.0) - 2*Re*slant_r*cos(elev*M_PI/180.0));
  if (H != 0.0) {
	GR = Re * acos( ( pow(Re,2.0) + pow(H,2.0) - pow(slant_r,2.0)) / (2 * Re * H));
  } else {
	GR = slant_r;
	H = Re;
  }
  H -= Re;
  *h = H;
  *gr = GR;

}


/*********************************************************************/
/*                                                                   */
/*          RSL_get_slantr_and_elev(gr, h, &r, &e)                   */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      Space Applications Corporation                               */
/*      July 23, 1994                                                */
/*                                                                   */
/*********************************************************************/
void RSL_get_slantr_and_elev(float gr, float h, float *slant_r, float *elev)
{
/* Input:
 *   gr      - Ground range in km.
 *   h       - Height of data point above earth, in km.
 * 
 * Output:
 *   slant_r - slant range, along the beam, in km.
 *   elev    - elevation angle, in degrees.
 */

/* This equation lifted from Dennis Flanigan's rsph.c code. */

  double slant_r_2; /* Slant range squared. */
  double ELEV;
  double SLANTR;

  if (gr == 0) {*slant_r = 0; *elev = 0; return;}

  h += Re;
  
  slant_r_2 = pow(Re,2.0) + pow(h,2.0) - (2*Re*h*cos(gr/Re));
  SLANTR = sqrt(slant_r_2);

  ELEV = acos( (pow(Re,2.0) + slant_r_2 - pow(h,2.0)) / (2*Re*(SLANTR)));
  ELEV *= 180.0/M_PI;
  ELEV -= 90.0;

  *slant_r = SLANTR;
  *elev = ELEV;
}


/*********************************************************************/
/*                                                                   */
/*          RSL_get_slantr_and_h(gr, elev, &sr, &h)                  */
/*                                                                   */
/*  By: John Merritt                                                 */
/*      Space Applications Corporation                               */
/*      July 23, 1994                                                */
/*                                                                   */
/*********************************************************************/
void RSL_get_slantr_and_h(float gr, float elev, float *slant_r, float *h)
{
/* Input:
 *   gr      - Ground range in km.
 *   elev    - elevation angle, in degrees.
 * 
 * Output:
 *   slant_r - slant range, along the beam, in km.
 *   h       - Height of data point above earth, in km.
 */

  double ELEV;
  double SLANTR;
  double ALPHA;
  double BETA;
  double GAMMA;
  double A;

  if (gr == 0) {*slant_r = 0; *h = 0; return;}

  ELEV = elev*M_PI/180.0;
  ALPHA = ELEV + M_PI_2;    /* Elev angle + 90 */
  GAMMA = gr/Re;
  BETA = M_PI - (ALPHA + GAMMA);  /* Angle made by Re+h and slant */
  SLANTR = Re*(sin(GAMMA)/sin(BETA));
  A = Re*sin(ALPHA)/sin(BETA);

  *h = (float) (A - Re);
  *slant_r = (float)SLANTR;  
}

