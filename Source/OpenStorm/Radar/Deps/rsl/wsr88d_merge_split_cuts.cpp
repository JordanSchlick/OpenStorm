#include <math.h>
#include "rsl.h"

#ifdef _WIN32
#define strdup _strdup
#endif


static int merge_split_cuts = 1;

void RSL_wsr88d_merge_split_cuts_on()
{
    merge_split_cuts = 1;
}

void RSL_wsr88d_merge_split_cuts_off()
{
    merge_split_cuts = 0;
}

void RSL_wsr88d_keep_short_refl()
{
    RSL_wsr88d_merge_split_cuts_off();
}

int wsr88d_merge_split_cuts_is_set()
{
    return merge_split_cuts;
}

void wsr88d_remove_extra_refl(Radar *radar)
{
    /* This function removes reflectivity from all velocity sweeps in the split
     * cut, keeping it in the surveillance sweep only.
     */

    int i;
    float prev_elev = 0.0;

    if (radar->v[DZ_INDEX] == NULL) return;

    if (radar->v[DZ_INDEX]->sweep[0] != NULL)
      prev_elev = radar->v[DZ_INDEX]->sweep[0]->h.elev;

    for (i=1; i < radar->v[DZ_INDEX]->h.nsweeps; i++) {
        if (radar->v[DZ_INDEX]->sweep[i]) {
            if (fabsf(radar->v[DZ_INDEX]->sweep[i]->h.elev - prev_elev) < .2) {
                RSL_free_sweep(radar->v[DZ_INDEX]->sweep[i]);
                radar->v[DZ_INDEX]->sweep[i] = NULL;
            }
            else prev_elev = radar->v[DZ_INDEX]->sweep[i]->h.elev;
        }
    }
}

void wsr88d_move_vcp121_extra_velsweeps(Radar *radar)
{
    /* Move the extra velocity and spectrum-width sweeps in VCP 121 split cuts
     * to other volume indexes.  We do this in order to allow all data moments
     * at a particular split cut elevation to later be moved to the same sweep
     * index in the radar structure.
     */

    int iswp;

    if (radar->v[VR_INDEX] != NULL) {
        if (radar->v[V2_INDEX] == NULL) {
            radar->v[V2_INDEX] = RSL_new_volume(radar->v[VR_INDEX]->h.nsweeps);
            radar->v[V2_INDEX]->h.type_str = strdup("Velocity 2");
            radar->v[V2_INDEX]->h.f = VR_F;
            radar->v[V2_INDEX]->h.invf = VR_INVF;
        }
        if (radar->v[V3_INDEX] == NULL) {
            radar->v[V3_INDEX] = RSL_new_volume(radar->v[VR_INDEX]->h.nsweeps);
            radar->v[V3_INDEX]->h.type_str = strdup("Velocity 3");
            radar->v[V3_INDEX]->h.f = VR_F;
            radar->v[V3_INDEX]->h.invf = VR_INVF;
        }
    }
    if (radar->v[SW_INDEX] != NULL) {
        if (radar->v[S2_INDEX] == NULL) {
            radar->v[S2_INDEX] = RSL_new_volume(radar->v[SW_INDEX]->h.nsweeps);
            radar->v[S2_INDEX]->h.type_str = strdup("Spectrum width 2");
            radar->v[S2_INDEX]->h.f = SW_F;
            radar->v[S2_INDEX]->h.invf = SW_INVF;
        }
        if (radar->v[S3_INDEX] == NULL) {
            radar->v[S3_INDEX] = RSL_new_volume(radar->v[SW_INDEX]->h.nsweeps);
            radar->v[S3_INDEX]->h.type_str = strdup("Spectrum width 3");
            radar->v[S3_INDEX]->h.f = SW_F;
            radar->v[S3_INDEX]->h.invf = SW_INVF;
        }
    }

    for (iswp=2; iswp < 16; iswp++) {
        switch (iswp) {
        case 2: case 6: case 9: case 12: case 15:
            if (radar->v[VR_INDEX] == NULL) break;
            if (radar->v[VR_INDEX]->sweep[iswp] == NULL) break;
            radar->v[V2_INDEX]->sweep[iswp] = radar->v[VR_INDEX]->sweep[iswp];
            radar->v[VR_INDEX]->sweep[iswp] = NULL;
            if (radar->v[SW_INDEX] == NULL) break;
            if (radar->v[SW_INDEX]->sweep[iswp] == NULL) break;
            radar->v[S2_INDEX]->sweep[iswp] = radar->v[SW_INDEX]->sweep[iswp];
            radar->v[SW_INDEX]->sweep[iswp] = NULL;
            break;
        case 3: case 7: case 10: case 13:
            if (radar->v[VR_INDEX] == NULL) break;
            if (radar->v[VR_INDEX]->sweep[iswp] == NULL) break;
            radar->v[V3_INDEX]->sweep[iswp] = radar->v[VR_INDEX]->sweep[iswp];
            radar->v[VR_INDEX]->sweep[iswp] = NULL;
            if (radar->v[SW_INDEX] == NULL) break;
            if (radar->v[SW_INDEX]->sweep[iswp] == NULL) break;
            radar->v[S3_INDEX]->sweep[iswp] = radar->v[SW_INDEX]->sweep[iswp];
            radar->v[SW_INDEX]->sweep[iswp] = NULL;
            break;
        }
    }
}


Radar *wsr88d_merge_split_cuts(Radar *radar)
{
    /* Move the split-cut sweeps so that all sweeps of an elevation angle
     * are at the same sweep index in the Radar structure.
     */

    if (!wsr88d_merge_split_cuts_is_set()) RSL_wsr88d_merge_split_cuts_on();

    wsr88d_remove_extra_refl(radar);
    if (radar->h.vcp == 121) wsr88d_move_vcp121_extra_velsweeps(radar);
    radar = RSL_prune_radar(radar);

    return radar;
}
