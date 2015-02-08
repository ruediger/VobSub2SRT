/*
 * This file is part of MPlayer.
 *
 * MPlayer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * MPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with MPlayer; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPLAYER_SPUDEC_H
#define MPLAYER_SPUDEC_H

//#include "libvo/video_out.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void spudec_heartbeat(void *self, unsigned int pts100);
void spudec_assemble(void *self, unsigned char *packet, unsigned int len, int pts100);
void spudec_draw(void *self, void (*draw_alpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride));
void spudec_draw_scaled(void *self, unsigned int dxs, unsigned int dys, void (*draw_alpha)(int x0,int y0, int w,int h, unsigned char* src, unsigned char *srca, int stride));
int spudec_apply_palette_crop(void *self, uint32_t palette, int sx, int ex, int sy, int ey);
void spudec_update_palette(void *self, unsigned int *palette);
void *spudec_new_scaled(unsigned int *palette, unsigned int frame_width, unsigned int frame_height, uint8_t *extradata, int extradata_len, unsigned int y_threshold);
void *spudec_new(unsigned int *palette, unsigned int y_threshold);
void spudec_free(void *self);
void spudec_reset(void *self);	// called after seek
int spudec_visible(void *self); // check if spu is visible
void spudec_set_font_factor(void * self, double factor); // sets the equivalent to ffactor
//void spudec_set_hw_spu(void *self, const vo_functions_t *hw_spu);
int spudec_changed(void *self);
void spudec_calc_bbox(void *me, unsigned int dxs, unsigned int dys, unsigned int* bbox);
void spudec_set_forced_subs_only(void * const self, const unsigned int flag);
void spudec_set_paletted(void *self, const uint8_t *pal_img, int stride,
                         const void *palette,
                         int x, int y, int w, int h,
                         double pts, double endpts);
/// call this after spudec_assemble and spudec_heartbeat to get the packet data
void spudec_get_data(void *self, const unsigned char **image, size_t *image_size, unsigned *width, unsigned *height,
                     unsigned *stride, unsigned *start_pts, unsigned *end_pts);

#ifdef __cplusplus
}
#endif

#endif /* MPLAYER_SPUDEC_H */
