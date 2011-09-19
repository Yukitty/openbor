/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

///////////////////////////////////////////////////////////////////////////
//         This file defines some commmon methods used by the gamelib
////////////////////////////////////////////////////////////////////////////
#include <assert.h>
#include "globals.h"
#include "types.h"

static transpixelfunc pfp;
static unsigned int fillcolor;
static blend16fp pfp16;
static blend32fp pfp32;
static unsigned char* table;
static int transbg;
int trans_sw, trans_sh, trans_dw, trans_dh;
static void (*drawfp)(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy);

/*transpixelfunc, 8bit*/
static unsigned char remapcolor(unsigned char* table, unsigned char color, unsigned char unused)
{
	return table[color];
}

static unsigned char blendcolor(unsigned char* table, unsigned char color1, unsigned char color2)
{
	if(!table) return color1;
	return table[color1<<8|color2];
}

static unsigned char blendfillcolor(unsigned char* table, unsigned char unused, unsigned char color)
{
	if(!table) return fillcolor;
	return table[fillcolor<<8|color];
}


/**

 draw a pixel from source gfx surface to destination screen
 complex

*/

inline void draw_pixel_dummy(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)  
{
	int pb = pixelbytes[(int)dest->pixelformat];
	unsigned char* pd = ((unsigned char*)(dest->data)) + (dx + dy*dest->width)*pb; 
	memset(pd, 0, pb);
}

inline void draw_pixel_screen(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	unsigned char *ptrd8, *ptrs8, ps8;
	unsigned short *ptrd16, *ptrs16, pd16, ps16;
	unsigned int *ptrd32, *ptrs32, pd32, ps32;
	switch(dest->pixelformat)
	{
		case PIXEL_8:
			ps8 = *(((unsigned char*)src->screen->data) + sx + sy * src->screen->width);
			if(transbg && !ps8) return;
			else if(fillcolor) ps8 = fillcolor;
			ptrd8 = ((unsigned char*)dest->data) + dx + dy * dest->width;
			*ptrd8 =pfp?pfp(table, ps8, *ptrd8):ps8;
			break;
		case PIXEL_16:
			ptrd16 = ((unsigned short*)dest->data) + dx + dy * dest->width;
			pd16 = *ptrd16;
			switch(src->screen->pixelformat)
			{
			case PIXEL_16:
				ptrs16 = ((unsigned short*)src->screen->data) + sx + sy * src->screen->width;
				ps16 = *ptrs16;
				if(transbg && !ps16) return;
				break;
			case PIXEL_x8:
				ptrs8 = ((unsigned char*)src->screen->data) + sx + sy * src->screen->width;
				if(transbg && !*ptrs8) return;
				ps16 = table?((unsigned short*)table)[*ptrs8]:((unsigned short*)src->screen->palette)[*ptrs8];
				break;
			default:
				return;
			}
			if(fillcolor) ps16 = fillcolor;
			if(!pfp16) *ptrd16 = ps16;
			else       *ptrd16 = pfp16(ps16, pd16);
			break;
		case PIXEL_32:
			ptrd32 = ((unsigned int*)dest->data) + dx + dy * dest->width;
			pd32 = *ptrd32;
			switch(src->screen->pixelformat)
			{
			case PIXEL_32:
				ptrs32 = ((unsigned int*)src->screen->data) + sx + sy * src->screen->width;
				ps32 = *ptrs32;
				if(transbg && !ps32) return;
				break;
			case PIXEL_x8:
				ptrs8 = ((unsigned char*)src->screen->data) + sx + sy * src->screen->width;
				if(transbg && !*ptrs8) return;
				ps32 = table?((unsigned int*)table)[*ptrs8]:((unsigned int*)src->screen->palette)[*ptrs8];
				break;
			default:
				return;
			}
			if(fillcolor) ps32 = fillcolor;
			if(!pfp32) *ptrd32 = ps32;
			else       *ptrd32 = pfp32(ps32, pd32);
			break;

	}
}


inline void draw_pixel_bitmap(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	//stub
	// should be OK for now since s_screen and s_bitmap are identical to each other
	assert(sizeof(s_screen)!=sizeof(s_bitmap));
	draw_pixel_screen(dest, src, dx, dy, sx, sy);
}

// get a pixel from specific sprite
// should be fairly slow due to the RLE compression
inline char sprite_get_pixel(s_sprite* sprite, int x, int y){
	int *linetab;
	register int lx = 0, count;
	unsigned char * data;

	//should we check? 
	//if(y<0 || y>=sprite->height || x<0 || x>=sprite->width)
	//	return 0;


	linetab = ((int*)sprite->data) + y;

	data = ((unsigned char*)linetab) + (*linetab);

	while(1) {
		count = *data++;
		if(count == 0xFF) return 0;
		if(lx+count>x) return 0; // transparent pixel
		lx += count;
		count = *data++;
		if(!count) continue;
		if(lx + count > x)
		{
			return data[x-lx]; // not transparent pixel
		}
		lx+=count;
		data+=count;
	}

	return 0;

}

inline void draw_pixel_sprite(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy)
{
	unsigned char *ptrd8, ps8;
	unsigned short *ptrd16, pd16, ps16;
	unsigned int *ptrd32, pd32, ps32;
	switch(dest->pixelformat)
	{
		case PIXEL_8:
			if(!(ps8 = sprite_get_pixel(src->sprite, sx, sy)))
				return;
			ptrd8 = ((unsigned char*)dest->data) + dx + dy * dest->width;
			if(fillcolor) ps8 = fillcolor;
			*ptrd8 =pfp?pfp(table, ps8, *ptrd8):ps8;
			break;
		case PIXEL_16:
			if(!(ps8 = sprite_get_pixel(src->sprite, sx, sy)))
				return;
			ptrd16 = ((unsigned short*)dest->data) + dx + dy * dest->width;
			pd16 = *ptrd16;
			if(fillcolor) ps16 = fillcolor;
			else ps16 = table?((unsigned short*)table)[ps8]:((unsigned short*)src->sprite->palette)[ps8];
			if(!pfp16) *ptrd16 = ps16;
			else       *ptrd16 = pfp16(ps16, pd16);
			break;
		case PIXEL_32:
			if(!(ps8 = sprite_get_pixel(src->sprite, sx, sy)))
				return;
			ptrd32 = ((unsigned int*)dest->data) + dx + dy * dest->width;
			pd32 = *ptrd32;
			if(fillcolor) ps32 = fillcolor;
			else ps32 = table?((unsigned int*)table)[ps8]:((unsigned int*)src->sprite->palette)[ps8];
			if(!pfp32) *ptrd32 = ps32;
			else       *ptrd32 = pfp32(ps32, pd32);
			break;
	}

}

inline void draw_pixel_gfx(s_screen* dest, gfx_entry* src, int dx, int dy, int sx, int sy){
	//drawfp(dest, src, dx, dy, sx, sy);
	switch(src->type){
	case gfx_sprite:
		draw_pixel_sprite(dest, src, dx, dy, sx, sy);
		break;
	case gfx_screen:
		draw_pixel_screen(dest, src, dx, dy, sx, sy);
		break;
	case gfx_bitmap:
		draw_pixel_bitmap(dest, src, dx, dy, sx, sy);
		break;
	default:
		draw_pixel_dummy(dest, src, dx, dy, sx, sy);//debug purpose
		break;
	}
}


void init_gfx_global_draw_stuff(s_screen* dest, gfx_entry* src, s_drawmethod* drawmethod){

	int spf = 0; //source pixel format
	drawfp = draw_pixel_dummy;
	pfp = NULL; fillcolor = 0; pfp16 = NULL;pfp32 = NULL; table = NULL; transbg = 0;

	//nasty checkings due to those different pixel formats
	switch(src->type)
	{
	case gfx_screen:
		//printf("gfx_screen\n");
		drawfp = draw_pixel_screen;
		trans_sw = src->screen->width;
		trans_sh = src->screen->height;
		break;
	case gfx_bitmap:
		//printf("gfx_bitmap\n");
		spf = src->bitmap->pixelformat;
		drawfp = draw_pixel_bitmap;
		trans_sw = src->bitmap->width;
		trans_sh = src->bitmap->height;
		break;
	case gfx_sprite:
		//printf("gfx_sprite\n");
		spf = src->sprite->pixelformat;
		drawfp = draw_pixel_sprite;
		trans_sw = src->sprite->width;
		trans_sh = src->sprite->height;
		break;
	default:
		//printf("gfx_unknown\n");
		return;
	}

	trans_dw = dest->width;
	trans_dh = dest->height;
	switch(dest->pixelformat)
	{
	case PIXEL_8:
		if(drawmethod->fillcolor) fillcolor = drawmethod->fillcolor&0xFF;
		else fillcolor = 0;

		table = NULL;

		if(drawmethod->table)
		{
			table = drawmethod->table;
			pfp = remapcolor;
		}
		else if(drawmethod->alpha>0)
		{
			table = blendtables[drawmethod->alpha-1];
			pfp = (fillcolor==TRANSPARENT_IDX?blendcolor:blendfillcolor);
		}
		else pfp = (fillcolor==TRANSPARENT_IDX?NULL:blendfillcolor);
		break;
	case PIXEL_16:
		fillcolor = drawmethod->fillcolor;
		if(drawmethod->alpha>0) pfp16 = blendfunctions16[drawmethod->alpha-1];
		else pfp16 = NULL;
		table = drawmethod->table;
		break;
	case PIXEL_32:
		fillcolor = drawmethod->fillcolor;
		if(drawmethod->alpha>0) pfp32 = blendfunctions32[drawmethod->alpha-1];
		else pfp32 = NULL;
		table = drawmethod->table;
		break;
	default: 
		return;
	}

	transbg = drawmethod->transbg; // check color key, we'll need this for screen and bitmap
}


//sin cos tables
//sin cos tables
const double sin_table[] = //360
{
0, 0.01745240643728351, 0.03489949670250097, 0.05233595624294383, 0.0697564737441253, 0.08715574274765816, 0.10452846326765346, 0.12186934340514747, 0.13917310096006544, 0.15643446504023087, 0.17364817766693033, 0.1908089953765448, 0.20791169081775931, 0.224951054343865, 0.24192189559966773, 0.25881904510252074, 0.27563735581699916, 0.29237170472273677, 0.3090169943749474, 0.32556815445715664, 0.3420201433256687, 0.35836794954530027, 0.374606593415912, 0.3907311284892737, 0.40673664307580015, 0.42261826174069944, 0.4383711467890774, 0.45399049973954675, 0.4694715627858908, 0.48480962024633706, 0.49999999999999994, 0.5150380749100542, 0.5299192642332049, 0.5446390350150271, 0.5591929034707469, 0.573576436351046, 0.5877852522924731, 0.6018150231520483, 0.6156614753256582, 0.6293203910498374, 0.6427876096865392, 0.6560590289905072, 0.6691306063588582, 0.6819983600624985, 0.6946583704589972, 0.7071067811865475, 0.7193398003386511, 0.7313537016191705, 0.7431448254773941, 0.754709580222772, 0.766044443118978, 0.7771459614569708, 0.788010753606722, 0.7986355100472928, 0.8090169943749474, 0.8191520442889918, 0.8290375725550417, 0.8386705679454239, 0.848048096156426, 0.8571673007021122, 0.8660254037844386, 0.8746197071393957, 0.8829475928589269, 0.8910065241883678, 0.898794046299167, 0.9063077870366499, 0.9135454576426009, 0.9205048534524403, 0.9271838545667874, 0.9335804264972017, 0.9396926207859083, 0.9455185755993167, 0.9510565162951535, 0.9563047559630354, 0.9612616959383189, 0.9659258262890683, 0.9702957262759965, 0.9743700647852352, 0.9781476007338056, 0.981627183447664, 0.984807753012208, 0.9876883405951378, 0.9902680687415702, 0.992546151641322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598242, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913, 1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682734, 0.9925461516413221, 0.9902680687415704, 0.9876883405951377, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630355, 0.9510565162951536, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.913545457642601, 0.90630778703665, 0.8987940462991669, 0.8910065241883679, 0.8829475928589271, 0.8746197071393958, 0.8660254037844387, 0.8571673007021123, 0.8480480961564261, 0.8386705679454239, 0.8290375725550417, 0.819152044288992, 0.8090169943749474, 0.7986355100472927, 0.788010753606722, 0.777145961456971, 0.766044443118978, 0.7547095802227718, 0.7431448254773942, 0.7313537016191706, 0.7193398003386514, 0.7071067811865476, 0.6946583704589971, 0.6819983600624986, 0.6691306063588583, 0.6560590289905073, 0.6427876096865395, 0.6293203910498377, 0.6156614753256584, 0.6018150231520482, 0.5877852522924732, 0.5735764363510464, 0.5591929034707469, 0.544639035015027, 0.5299192642332049, 0.5150380749100544, 0.49999999999999994, 0.48480962024633717, 0.4694715627858911, 0.45399049973954686, 0.4383711467890773, 0.4226182617406995, 0.40673664307580043, 0.39073112848927416, 0.37460659341591223, 0.3583679495453002, 0.3420201433256689, 0.32556815445715703, 0.3090169943749475, 0.29237170472273704, 0.27563735581699966, 0.258819045102521, 0.24192189559966773, 0.22495105434386478, 0.20791169081775931, 0.19080899537654497, 0.17364817766693027, 0.15643446504023098, 0.13917310096006574, 0.12186934340514754, 0.10452846326765373, 0.08715574274765864, 0.06975647374412552, 0.05233595624294381, 0.0348994967025007, 0.01745240643728344, 1.2246063538223772e-16, -0.017452406437283192, -0.0348994967025009, -0.052335956242943564, -0.06975647374412483, -0.08715574274765794, -0.10452846326765305, -0.12186934340514774, -0.13917310096006552, -0.15643446504023073, -0.17364817766693047, -0.19080899537654472, -0.20791169081775906, -0.22495105434386497, -0.2419218955996675, -0.25881904510252035, -0.275637355816999, -0.2923717047227364, -0.30901699437494773, -0.32556815445715675, -0.34202014332566865, -0.35836794954530043, -0.374606593415912, -0.39073112848927355, -0.4067366430757998, -0.4226182617406993, -0.43837114678907707, -0.45399049973954625, -0.46947156278589086, -0.48480962024633694, -0.5000000000000001, -0.5150380749100542, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.601815023152048, -0.6156614753256578, -0.6293203910498376, -0.6427876096865392, -0.6560590289905074, -0.6691306063588582, -0.6819983600624984, -0.6946583704589974, -0.7071067811865475, -0.7193398003386509, -0.7313537016191701, -0.743144825477394, -0.7547095802227717, -0.7660444431189779, -0.7771459614569711, -0.7880107536067221, -0.7986355100472928, -0.8090169943749473, -0.8191520442889916, -0.8290375725550414, -0.838670567945424, -0.848048096156426, -0.8571673007021121, -0.8660254037844384, -0.874619707139396, -0.882947592858927, -0.8910065241883678, -0.8987940462991668, -0.9063077870366497, -0.913545457642601, -0.9205048534524403, -0.9271838545667873, -0.9335804264972016, -0.9396926207859082, -0.9455185755993168, -0.9510565162951535, -0.9563047559630353, -0.961261695938319, -0.9659258262890683, -0.9702957262759965, -0.9743700647852351, -0.9781476007338056, -0.9816271834476639, -0.984807753012208, -0.9876883405951377, -0.9902680687415704, -0.9925461516413221, -0.9945218953682734, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190956, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546151641322, -0.9902680687415704, -0.9876883405951378, -0.9848077530122081, -0.9816271834476641, -0.9781476007338058, -0.9743700647852352, -0.9702957262759966, -0.9659258262890682, -0.9612616959383188, -0.9563047559630354, -0.9510565162951536, -0.945518575599317, -0.9396926207859085, -0.9335804264972021, -0.9271838545667874, -0.9205048534524405, -0.9135454576426008, -0.9063077870366499, -0.898794046299167, -0.8910065241883679, -0.8829475928589271, -0.8746197071393961, -0.8660254037844386, -0.8571673007021123, -0.8480480961564262, -0.8386705679454243, -0.8290375725550421, -0.8191520442889918, -0.8090169943749476, -0.798635510047293, -0.7880107536067218, -0.7771459614569708, -0.7660444431189781, -0.7547095802227722, -0.7431448254773946, -0.731353701619171, -0.7193398003386517, -0.7071067811865477, -0.6946583704589976, -0.6819983600624982, -0.6691306063588581, -0.6560590289905074, -0.6427876096865396, -0.6293203910498378, -0.6156614753256588, -0.6018150231520483, -0.5877852522924734, -0.5735764363510465, -0.5591929034707473, -0.544639035015027, -0.5299192642332058, -0.5150380749100545, -0.5000000000000004, -0.4848096202463369, -0.4694715627858908, -0.45399049973954697, -0.438371146789077, -0.4226182617407, -0.40673664307580015, -0.3907311284892747, -0.37460659341591235, -0.35836794954530077, -0.3420201433256686, -0.32556815445715753, -0.3090169943749476, -0.29237170472273627, -0.2756373558169998, -0.2588190451025207, -0.24192189559966787, -0.22495105434386534, -0.20791169081775987, -0.19080899537654466, -0.17364817766693127, -0.1564344650402311, -0.13917310096006588, -0.12186934340514811, -0.10452846326765341, -0.08715574274765832, -0.06975647374412476, -0.05233595624294437, -0.034899496702500823, -0.01745240643728445
};
const double cos_table[] = //360
{
1, 0.9998476951563913, 0.9993908270190958, 0.9986295347545738, 0.9975640502598242, 0.9961946980917455, 0.9945218953682733, 0.992546151641322, 0.9902680687415704, 0.9876883405951378, 0.984807753012208, 0.981627183447664, 0.9781476007338057, 0.9743700647852352, 0.9702957262759965, 0.9659258262890683, 0.9612616959383189, 0.9563047559630354, 0.9510565162951535, 0.9455185755993168, 0.9396926207859084, 0.9335804264972017, 0.9271838545667874, 0.9205048534524404, 0.9135454576426009, 0.9063077870366499, 0.898794046299167, 0.8910065241883679, 0.882947592858927, 0.8746197071393957, 0.8660254037844387, 0.8571673007021123, 0.848048096156426, 0.838670567945424, 0.8290375725550416, 0.8191520442889918, 0.8090169943749474, 0.7986355100472928, 0.788010753606722, 0.7771459614569709, 0.766044443118978, 0.7547095802227721, 0.7431448254773942, 0.7313537016191706, 0.7193398003386512, 0.7071067811865476, 0.6946583704589974, 0.6819983600624985, 0.6691306063588582, 0.6560590289905073, 0.6427876096865394, 0.6293203910498375, 0.6156614753256583, 0.6018150231520484, 0.5877852522924731, 0.5735764363510462, 0.5591929034707468, 0.5446390350150272, 0.5299192642332049, 0.5150380749100544, 0.5000000000000001, 0.4848096202463371, 0.46947156278589086, 0.4539904997395468, 0.43837114678907746, 0.42261826174069944, 0.4067366430758002, 0.39073112848927394, 0.37460659341591196, 0.3583679495453004, 0.3420201433256688, 0.32556815445715675, 0.30901699437494745, 0.29237170472273677, 0.27563735581699916, 0.25881904510252074, 0.2419218955996679, 0.22495105434386492, 0.20791169081775945, 0.19080899537654491, 0.17364817766693041, 0.15643446504023092, 0.1391731009600657, 0.12186934340514749, 0.10452846326765346, 0.08715574274765814, 0.06975647374412545, 0.052335956242943966, 0.03489949670250108, 0.017452406437283376, 6.123031769111886e-17, -0.017452406437283477, -0.03489949670250073, -0.05233595624294362, -0.06975647374412533, -0.08715574274765823, -0.10452846326765333, -0.12186934340514736, -0.13917310096006535, -0.15643446504023103, -0.1736481776669303, -0.1908089953765448, -0.20791169081775912, -0.2249510543438648, -0.24192189559966778, -0.25881904510252085, -0.27563735581699905, -0.29237170472273666, -0.30901699437494734, -0.3255681544571564, -0.3420201433256687, -0.35836794954530027, -0.37460659341591207, -0.3907311284892736, -0.40673664307580004, -0.42261826174069933, -0.4383711467890775, -0.4539904997395467, -0.46947156278589053, -0.484809620246337, -0.4999999999999998, -0.5150380749100543, -0.5299192642332048, -0.5446390350150271, -0.5591929034707467, -0.5735764363510458, -0.587785252292473, -0.6018150231520484, -0.6156614753256583, -0.6293203910498373, -0.6427876096865394, -0.6560590289905075, -0.6691306063588582, -0.6819983600624984, -0.694658370458997, -0.7071067811865475, -0.7193398003386512, -0.7313537016191705, -0.743144825477394, -0.754709580222772, -0.7660444431189779, -0.7771459614569707, -0.7880107536067219, -0.7986355100472929, -0.8090169943749473, -0.8191520442889916, -0.8290375725550416, -0.8386705679454242, -0.848048096156426, -0.8571673007021122, -0.8660254037844387, -0.8746197071393957, -0.8829475928589268, -0.8910065241883678, -0.898794046299167, -0.9063077870366499, -0.9135454576426008, -0.9205048534524401, -0.9271838545667873, -0.9335804264972017, -0.9396926207859083, -0.9455185755993167, -0.9510565162951535, -0.9563047559630354, -0.9612616959383187, -0.9659258262890682, -0.9702957262759965, -0.9743700647852352, -0.9781476007338057, -0.981627183447664, -0.984807753012208, -0.9876883405951377, -0.9902680687415702, -0.992546151641322, -0.9945218953682733, -0.9961946980917455, -0.9975640502598242, -0.9986295347545738, -0.9993908270190958, -0.9998476951563913, -1, -0.9998476951563913, -0.9993908270190958, -0.9986295347545738, -0.9975640502598243, -0.9961946980917455, -0.9945218953682734, -0.992546151641322, -0.9902680687415702, -0.9876883405951378, -0.984807753012208, -0.981627183447664, -0.9781476007338057, -0.9743700647852352, -0.9702957262759965, -0.9659258262890684, -0.9612616959383189, -0.9563047559630355, -0.9510565162951535, -0.9455185755993167, -0.9396926207859084, -0.9335804264972017, -0.9271838545667874, -0.9205048534524404, -0.9135454576426011, -0.90630778703665, -0.8987940462991671, -0.8910065241883681, -0.8829475928589269, -0.8746197071393958, -0.8660254037844386, -0.8571673007021123, -0.8480480961564261, -0.838670567945424, -0.8290375725550418, -0.819152044288992, -0.8090169943749476, -0.798635510047293, -0.7880107536067222, -0.7771459614569708, -0.766044443118978, -0.7547095802227719, -0.7431448254773942, -0.7313537016191706, -0.7193398003386511, -0.7071067811865477, -0.6946583704589976, -0.6819983600624989, -0.6691306063588585, -0.6560590289905076, -0.6427876096865395, -0.6293203910498372, -0.6156614753256581, -0.6018150231520483, -0.5877852522924732, -0.5735764363510464, -0.5591929034707472, -0.544639035015027, -0.529919264233205, -0.5150380749100545, -0.5000000000000004, -0.48480962024633683, -0.46947156278589075, -0.4539904997395469, -0.43837114678907773, -0.42261826174069994, -0.4067366430758001, -0.3907311284892738, -0.3746065934159123, -0.3583679495453007, -0.3420201433256694, -0.32556815445715664, -0.30901699437494756, -0.2923717047227371, -0.2756373558169989, -0.25881904510252063, -0.24192189559966778, -0.22495105434386525, -0.2079116908177598, -0.19080899537654547, -0.17364817766693033, -0.15643446504023103, -0.13917310096006494, -0.12186934340514717, -0.10452846326765336, -0.08715574274765825, -0.06975647374412558, -0.052335956242944306, -0.03489949670250165, -0.017452406437283498, -1.836909530733566e-16, 0.01745240643728313, 0.03489949670250128, 0.052335956242943946, 0.06975647374412522, 0.08715574274765789, 0.10452846326765298, 0.12186934340514768, 0.13917310096006546, 0.15643446504023067, 0.17364817766692997, 0.19080899537654425, 0.20791169081775856, 0.22495105434386492, 0.24192189559966745, 0.25881904510252113, 0.2756373558169994, 0.2923717047227367, 0.30901699437494723, 0.3255681544571563, 0.34202014332566816, 0.35836794954529954, 0.37460659341591196, 0.3907311284892735, 0.40673664307580054, 0.4226182617406996, 0.4383711467890774, 0.45399049973954664, 0.4694715627858904, 0.4848096202463365, 0.5000000000000001, 0.5150380749100542, 0.5299192642332047, 0.5446390350150266, 0.5591929034707462, 0.573576436351046, 0.5877852522924729, 0.6018150231520479, 0.6156614753256585, 0.6293203910498375, 0.6427876096865392, 0.656059028990507, 0.6691306063588578, 0.681998360062498, 0.6946583704589966, 0.7071067811865473, 0.7193398003386509, 0.7313537016191707, 0.7431448254773942, 0.7547095802227719, 0.7660444431189778, 0.7771459614569706, 0.7880107536067216, 0.7986355100472928, 0.8090169943749473, 0.8191520442889916, 0.8290375725550414, 0.838670567945424, 0.8480480961564254, 0.8571673007021121, 0.8660254037844384, 0.8746197071393958, 0.8829475928589269, 0.8910065241883678, 0.8987940462991671, 0.9063077870366497, 0.913545457642601, 0.9205048534524399, 0.9271838545667873, 0.9335804264972015, 0.9396926207859084, 0.9455185755993165, 0.9510565162951535, 0.9563047559630357, 0.9612616959383187, 0.9659258262890683, 0.9702957262759965, 0.9743700647852351, 0.9781476007338056, 0.981627183447664, 0.9848077530122079, 0.9876883405951377, 0.9902680687415702, 0.992546151641322, 0.9945218953682733, 0.9961946980917455, 0.9975640502598243, 0.9986295347545738, 0.9993908270190958, 0.9998476951563913
};

#define _max(a,b) (((a)>(b))?(a):(b))
#define _min(a,b) (((a)<(b))?(a):(b))


void gfx_draw_rotate(s_screen* dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod)
{
	double zoomx, zoomy, rzoomx, rzoomy, sina, cosa, ax, ay, bx, by, rx0, ry0, cx, cy, srcx0_f, srcx_f, srcy0_f, srcy_f, angle;
	int i, j, srcx, srcy;
	int xbound[4], ybound[4], xmin, xmax, ymin, ymax;
	double xboundf[4], yboundf[4];
	zoomx = drawmethod->scalex / 256.0;
	zoomy = drawmethod->scaley / 256.0;
	angle = drawmethod->rotate;
    sina = sin_table[(int)angle];
    cosa = cos_table[(int)angle];

	init_gfx_global_draw_stuff(dest, src, drawmethod);

	centerx += drawmethod->centerx;
	centery += drawmethod->centery;
	
	/////////////////begin clipping////////////////////////////
	xboundf[0] = drawmethod->flipx? (centerx-trans_sw)*zoomx : -centerx*zoomx;
	yboundf[0] = drawmethod->flipy? (centery-trans_sh)*zoomy : -centery*zoomy;
	xboundf[1] = xboundf[0] + trans_sw*zoomx;
	yboundf[1] = yboundf[0];
	xboundf[2] = xboundf[0];
	yboundf[2] = yboundf[0] + trans_sh*zoomy;
	xboundf[3] = xboundf[1];
	yboundf[3] = yboundf[2];

	for(i=0; i<4; i++){
		xbound[i] =  (int)(x + xboundf[i]*cosa - yboundf[i]*sina);
        ybound[i] =  (int)(y + xboundf[i]*sina + yboundf[i]*cosa);
	}

	xmin = _max(_min(_min(xbound[0],xbound[1]), _min(xbound[2],xbound[3])), 0);
	xmax = _min(_max(_max(xbound[0],xbound[1]), _max(xbound[2],xbound[3])), trans_dw);
	ymin = _max(_min(_min(ybound[0],ybound[1]), _min(ybound[2],ybound[3])), 0);
	ymax = _min(_max(_max(ybound[0],ybound[1]), _max(ybound[2],ybound[3])), trans_dh);
	/////////////////end clipping////////////////////////////

	// tricks to keep rotate not affected by flip
	if(drawmethod->flipx) {zoomx = -zoomx;}
	else  {angle = -angle;}
	if(drawmethod->flipy) {	zoomy = -zoomy; angle = -angle;}

	angle = (((int)angle)%360+360)%360;
	//if(!zoomx || !zoomy) return; //should be checked already
    rzoomx = 1.0 / zoomx;
    rzoomy = 1.0 / zoomy;
    sina = sin_table[(int)angle];
    cosa = cos_table[(int)angle];
    ax = rzoomx*cosa; 
    ay = rzoomx*sina; 
    bx = -rzoomy*sina; 
    by = rzoomy*cosa; 
    rx0 = centerx;
    ry0 = centery;
	x -= rx0;
	y -= ry0;
    cx = -(rx0+x)*rzoomx*cosa+(ry0+y)*rzoomy*sina+rx0;
    cy = -(rx0+x)*rzoomx*sina-(ry0+y)*rzoomy*cosa+ry0; 
	srcx0_f= cx+ymin*bx+xmin*ax, srcx_f;
    srcy0_f= cy+ymin*by+xmin*ay, srcy_f;

    for (j=ymin; j<ymax; j++)
    {
        srcx_f = srcx0_f;
        srcy_f = srcy0_f;
        for (i=xmin;i<xmax;i++)
        {
            srcx=(int)(srcx_f);
            srcy=(int)(srcy_f);
			if(srcx>=0 && srcx<trans_sw && srcy>=0 && srcy<trans_sh)
				draw_pixel_gfx(dest, src, i, j, srcx, srcy);
            srcx_f+=ax;
            srcy_f+=ay;
        }
        srcx0_f+=bx;
        srcy0_f+=by;
    }
}

// scalex scaley flipy ...
void gfx_draw_scale(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod)
{
	double osx, sx, sy, dx, dy, w, h, cx, cy, stepdx, stepdy;
	int beginy, endy, beginx, endx;
	int i, j;
	double zoomx = drawmethod->scalex / 256.0;
	double zoomy = drawmethod->scaley / 256.0;
	double shiftf = drawmethod->shiftx / 256.0;
	
	//if(!zoomy || !zoomx) return; //should be checked already

	//printf("==%d %d %d %d %d\n", drawmethod->scalex, drawmethod->scaley, drawmethod->flipx, drawmethod->flipy, drawmethod->shiftx);

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	w = trans_sw * zoomx;
	h = trans_sh * zoomy;
	cx = centerx * zoomx;
	cy = centery * zoomy;
	
	if(drawmethod->flipx) dx = cx - w + x;
	else dx = x - cx;

	if(drawmethod->flipy){
		dy = cy - h + y; 
		shiftf = - shiftf;
	}
	else dy = y - cy; 

	dx += (dy + h - y) * shiftf;

	if(drawmethod->flipx) {
		stepdx = 1.0/zoomx;
		osx = 0.0;
	}else{
		stepdx = -1.0/zoomx;
		osx = trans_sw - 1.0;
	}
	if(drawmethod->flipy){
		stepdy = 1.0/zoomy;
		sy = 0.0;
	}else{
		stepdy = -1.0/zoomy;
		sy = trans_sh - 1.0;
	}

	if(_max(dx+w, dx+w-shiftf*h)<=0) return;
	if(_min(dx, dx-shiftf*h)>=trans_dw) return;
	if(dy>=trans_dh) return;
	if(dy+h<=0) return;

	if(dy+h>trans_dh) {
		endy = trans_dh;
		dx -= shiftf*(dy+h-endy);
		sy += stepdy*(dy+h-endy);
	} else endy = dy+h;

	if(dy<0) beginy = 0;
	else beginy = dy;

	//printf("=%d, %d, %lf, %lf, %lf, %lf, %lf, %lf\n ",x, y, w, h, osx, sy, dx, dy);
	// =64, 144, 44.000000, 36.500000, 43.000000, 0.000000, 38.000000, 143.000000

	for(j=endy-1; j>=beginy; j--, sy+=stepdy, dx -= shiftf){
		if(dx>=trans_dw) continue;
		if(dx+w<=0) continue;
		sx = osx;
		beginx = dx;
		endx = dx+w;
		if(dx<0) beginx = 0;
		else beginx = dx;
		if(dx+w>trans_dw) {
			endx = trans_dw;
			sx += stepdx*(dx+w-trans_dw);
		} else endx = dx+w;
		for(i=endx-1; i>=beginx; i--, sx+=stepdx){
			//printf("%d, %d, %d, %d\n", i, j, (int)sx, (int)sy);
			draw_pixel_gfx(dest, src, i, j, (int)sx, (int)sy);
		}
	}

}

extern float _sinfactors[256];
#define distortion(x, a) ((int)(_sinfactors[x]*a+0.5))

void gfx_draw_water(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod){
	int sw, sh, dw, dh, ch, sy, t, u, sbeginx, sendx, dbeginx, dendx, bytestocopy, amplitude, time;
	float s, wavelength;

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	sw = trans_sw;
	sh = trans_sh;
	dw = trans_dw;
	dh = trans_dh;
	ch = sh;
	x -= centerx;
	y -= centery;

	amplitude = drawmethod->water.amplitude;
	time = drawmethod->water.wavetime;

	s = (float)(time % 256);

	// Clip x
	if(x + amplitude*2 + sw <= 0 || x - amplitude*2  >= dw) return;
	if(y + sh <=0 || y >= dh) return;

	sy = 0;
	if(s<0) s+=256;

	// Clip y
	if(y<0) {sy = -y; ch += y;}
	if(y+sh > dh) ch -= (y+sh) - dh;

	if(y<0) y = 0;

	u = (drawmethod->water.watermode==1)?distortion((int)s, amplitude):amplitude;
	wavelength = 256 / drawmethod->water.wavelength;
	s += sy*wavelength;

	// Copy data
	do{
		s = s - (int)s + (int)s % 256;
		t = distortion((int)s, amplitude) - u;

		dbeginx = x+t;
		dendx = x+t+sw;
		
		if(dbeginx>=dw || dendx<=0) {dbeginx = dendx = sbeginx = sendx = 0;} //Nothing to draw
		//clip both
		else if(dbeginx<0 && dendx>dw){ 
			sbeginx = -dbeginx; sendx = sbeginx + dw;
			dbeginx = 0; dendx = dw;
		}
		//clip left
		else if(dbeginx<0) {
			sbeginx = -dbeginx; sendx = sw;
			dbeginx = 0;
		}
		// clip right
		else if(dendx>dw) {
			sbeginx = 0; sendx = dw - dbeginx;	
			dendx = dw;
		}
		// clip none
		else{
			sbeginx = 0; sendx = sw;
		}
		bytestocopy = dendx-dbeginx;
		
		//TODO: optimize this if necessary
		for(t=0; t<bytestocopy; t++, sbeginx++, dbeginx++){
			draw_pixel_gfx(dest, src, dbeginx, y, sbeginx, sy);
		}

		s += wavelength;
		y++;
		sy++;
	}while(--ch);
}

void gfx_draw_plane(s_screen *dest, gfx_entry* src, int x, int y, int centerx, int centery, s_drawmethod* drawmethod){

	int i, j, dx, dy, sx, sy, factor, width, height;
	int s_pos, s_step, x_pos;
	int center_offset;

	init_gfx_global_draw_stuff(dest, src, drawmethod);
	centerx += drawmethod->centerx;
	centery += drawmethod->centery;

	x -= centerx;
	y -= centery;

	x_pos = x;

	factor = drawmethod->water.wavelength;

	if(factor < 0) return;
	factor++;

	width = trans_sw;
	height = trans_sh;

	// Check dimensions
	if(x >= trans_dw) return;
	if(y >= trans_dh) return;
	if(x<0){
		width += x;
		x = 0;
	}
	if(y<0){
		height += y;
		y=0;
	}
	if(x+width > trans_dw){
		width = trans_dw - x;
	}
	if(y+height > trans_dh){
		height = trans_dh - y;
	}
	if(width<=0) return;
	if(height<=0) return;


	dy = y;
	dx = x;
	sy = 0;
	for(i=0; i<height; i++, dy++)
	{
		sy = i % trans_sh;

		center_offset = trans_dw / 2 - x;

		s_pos = (drawmethod->water.wavetime - x_pos) * 256 + (256 * trans_sw);
		s_step = trans_sw * 256 / (trans_sw + trans_sw * i / factor);
		s_pos -= center_offset * s_step;

		for(j=0; j<width; j++, s_pos += s_step){
			sx = s_pos >> 8;
			if(sx > trans_sw){
				sx %= trans_sw;
				s_pos = (s_pos & 0xFF) | (sx << 8);
			}
			draw_pixel_gfx(dest, src, dx+j, dy, sx, sy);
		}
	}

}

