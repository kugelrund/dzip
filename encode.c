#include "dzip.h"

#include <assert.h>

int dem_sound(void)
{
	uchar mask = inptr[1];
	int len = 11;
	uInt chanent, c, entity;

	if (mask & SND_VOLUME) len++;
	if (mask & SND_ATTENUATION) len++;

	inptr[1] = DZ_IDENTIFIER_SOUND+mask;
	chanent = getshort(inptr+len-9) & 0xffff;
	entity = chanent >> 3;
	chanent = (entity << 3) | ((2-(chanent & 0x07)) & 0x07);

	if (entity < MAX_ENT)
	{
		c = getshort(inptr+len-6) - oldent[entity].org0;
		c = cnvlong(c); memcpy(inptr+len-6,&c,2);
		c = getshort(inptr+len-4) - oldent[entity].org1;
		c = cnvlong(c); memcpy(inptr+len-4,&c,2);
		c = getshort(inptr+len-2) - oldent[entity].org2;
		c = cnvlong(c); memcpy(inptr+len-2,&c,2);
	}

	chanent = cnvlong(chanent);
	memcpy(inptr+len-9,&chanent,2);
	discard_msg(1);
	copy_msg(len-1);
	return 0;
}

int dem_time(void)
{
	long tmp = getlong(inptr+1) - dem_gametime;
	dem_gametime = getlong(inptr+1);
	tmp = cnvlong(tmp);
	memcpy(inptr+1,&tmp,4);
	if (inptr[3] || inptr[4]) { *inptr = DZ_longtime; copy_msg(5); }
	else { copy_msg(3); discard_msg(2); }
	return 0;
}

/* used by lots of msgs */
int dem_string(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
	return 0;
}

int dem_serverinfo(void)
{
	uchar *ptr = inptr + 7;
	uchar *start_ptr;

	while (*ptr++);
	do {
		start_ptr = ptr;
		while (*ptr++);
	} while (ptr - start_ptr > 1);
	do {
		start_ptr = ptr;
		while (*ptr++);
	} while (ptr - start_ptr > 1);
	copy_msg(ptr-inptr);
	sble = 0;
	return 0;
}

int dem_lightstyle(void)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
	return 0;
}

int dem_updatename(void)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
	return 0;
}

uchar bdiff(int x, int y)
{
	int d = x - y;
	if (d < 0) d += 256;
	return d & 0xff;
}

int dem_clientdata(void)
{
	uchar buf[32];
	uchar *ptr = inptr+3;
	uInt mask = getshort(inptr+1);
	long tmp;

	memset(&newcd,0,sizeof(cdata_t));
	newcd.voz = 22;
	newcd.items = 0x4001;

	#define CFDIFF(x,def,bit) if (newcd.x == def) newcd.force |= bit;

	if (mask & SU_VIEWHEIGHT)
		{ newcd.voz = *ptr++; CFDIFF(voz,22,DZ_CD_VIEWHEIGHT_FORCE); }
	if (mask & SU_IDEALPITCH)
		{ newcd.pax = *ptr++; CFDIFF(pax,0,DZ_CD_IDEALPITCH_FORCE); }
	if (mask & SU_PUNCH0)
		{ newcd.ang0 = *ptr++; CFDIFF(ang0,0,DZ_CD_PUNCH0_FORCE); }
	if (mask & SU_VELOCITY0)
		{ newcd.vel0 = *ptr++; CFDIFF(vel0,0,DZ_CD_VELOCITY0_FORCE); }
	if (mask & SU_PUNCH1)
		{ newcd.ang1 = *ptr++; CFDIFF(ang1,0,DZ_CD_PUNCH1_FORCE); }
	if (mask & SU_VELOCITY1)
		{ newcd.vel1 = *ptr++; CFDIFF(vel1,0,DZ_CD_VELOCITY1_FORCE); }
	if (mask & SU_PUNCH2)
		{ newcd.ang2 = *ptr++; CFDIFF(ang2,0,DZ_CD_PUNCH2_FORCE); }
	if (mask & SU_VELOCITY2)
		{ newcd.vel2 = *ptr++; CFDIFF(vel2,0,DZ_CD_VELOCITY2_FORCE); }
	newcd.items = getlong(ptr); ptr += 4;
	newcd.invbit = !(mask & SU_ITEMS);
	newcd.uk10 = !!(mask & SU_ONGROUND);
	newcd.uk11 = !!(mask & SU_INWATER);
	if (mask & SU_WEAPONFRAME)
		{ newcd.wpf = *ptr++; CFDIFF(wpf,0,DZ_CD_WEAPONFRAME_FORCE); }
	if (mask & SU_ARMOR)
		{ newcd.av = *ptr++; CFDIFF(av,0,DZ_CD_ARMOR_FORCE); }
	if (mask & SU_WEAPON)
		{ newcd.wpm = *ptr++; CFDIFF(wpm,0,DZ_CD_WEAPON_FORCE); }
	newcd.health = getshort(ptr); ptr += 2;
	newcd.am = *ptr++;
	newcd.sh = *ptr++;
	newcd.nl = *ptr++;
	newcd.rk = *ptr++;
	newcd.ce = *ptr++;
	newcd.wp = *ptr++;
	discard_msg(ptr-inptr);

	mask = DZ_IDENTIFIER_CLIENTDATA_DIFF;
	ptr = buf+4;

	#define CDIFF(x,b) \
		if (oldcd.x != newcd.x) \
		{ mask |= b; *ptr++ = bdiff(newcd.x,oldcd.x); }

	CDIFF(vel2,DZ_CD_VELOCITY2_DIFF);
	CDIFF(vel0,DZ_CD_VELOCITY0_DIFF);
	CDIFF(vel1,DZ_CD_VELOCITY1_DIFF);
	CDIFF(wpf,DZ_CD_WEAPONFRAME_DIFF);
	if (oldcd.uk10 != newcd.uk10) mask |= DZ_CD_ONGROUND_DIFF;
	CDIFF(ang0,DZ_CD_PUNCH0_DIFF);
	CDIFF(am,DZ_CD_AMMO_DIFF);
	if (oldcd.health != newcd.health)
	{
		mask |= DZ_CD_HEALTH_DIFF;
		tmp = newcd.health - oldcd.health;
		tmp = cnvlong(tmp);
		memcpy(ptr,&tmp,2); ptr += 2;
	}
	if (oldcd.items != newcd.items)
	{
		mask |= DZ_CD_ITEMS_DIFF;
		tmp = newcd.items ^ oldcd.items;
		tmp = cnvlong(tmp);
		memcpy(ptr,&tmp,4); ptr += 4;		
	}
	CDIFF(av,DZ_CD_ARMOR_DIFF);
	CDIFF(pax,DZ_CD_IDEALPITCH_DIFF);
	CDIFF(sh,DZ_CD_SHELLS_DIFF);
	CDIFF(nl,DZ_CD_NAILS_DIFF);
	CDIFF(rk,DZ_CD_ROCKETS_DIFF);
	CDIFF(wpm,DZ_CD_WEAPON_DIFF);
	CDIFF(wp,DZ_CD_WEAPONINDEX_DIFF);
	if (oldcd.uk11 != newcd.uk11) mask |= DZ_CD_INWATER_DIFF;
	CDIFF(voz,DZ_CD_VIEWHEIGHT_DIFF);
	CDIFF(ce,DZ_CD_CELLS_DIFF);
	CDIFF(ang1,DZ_CD_PUNCH1_DIFF);
	CDIFF(ang2,DZ_CD_PUNCH2_DIFF);
	if (oldcd.invbit != newcd.invbit) mask |= DZ_CD_INVBIT_DIFF;

	if (mask & 0xffffff00) mask |= DZ_CD_MOREBITS_DIFF;
	if (mask & 0xffff0000) mask |= DZ_CD_MOREBITS1_DIFF;
	if (mask & 0xff000000) mask |= DZ_CD_MOREBITS2_DIFF;

	tmp = cnvlong(mask);
	memcpy(buf,&tmp,4);
	if (!(mask & DZ_CD_MOREBITS_DIFF))
		{ memmove(buf+1,buf+4,ptr-buf-4); ptr -= 3; }
	else if (!(mask & DZ_CD_MOREBITS1_DIFF))
		{ memmove(buf+2,buf+4,ptr-buf-4); ptr -= 2; }
	else if (!(mask & DZ_CD_MOREBITS2_DIFF))
		{ memmove(buf+3,buf+4,ptr-buf-4); ptr -= 1; }
	insert_msg(buf,ptr-buf);

	if (newcd.force != oldcd.force)
	{
		mask = DZ_IDENTIFIER_CLIENTDATA_FORCE | (newcd.force ^ oldcd.force);
		if (mask & 0xff00) mask |= DZ_CD_MOREBITS_FORCE;
		buf[0] = mask & 0xff;
		buf[1] = (mask >> 8) & 0xff;
		insert_msg(buf,1+!!buf[1]);
	}

	oldcd = newcd;
	return 0;
}

int dem_spawnbaseline(void)
{
	uchar buf[16], *ptr;
	ent_t ent;
	int index = getshort(inptr+1);
	int diff;

	memset(&ent,0,sizeof(ent_t));
	ent.modelindex = inptr[3];
	ent.frame = inptr[4];
	ent.colormap = inptr[5];
	ent.skin = inptr[6];
	ent.org0 = getshort(inptr+7);
	ent.ang0 = inptr[9];
	ent.org1 = getshort(inptr+10);
	ent.ang1 = inptr[12];
	ent.org2 = getshort(inptr+13);
	ent.ang2 = inptr[15];
	discard_msg(16);

	buf[0] = DEM_spawnbaseline;
	diff = (index - sble + 0x400) % 0x400;
	buf[1] = diff & 0xff;
	buf[2] = diff >> 8;
	ptr = buf+3;
	*ptr++ = ent.modelindex;
	if (ent.frame) { buf[2] |= DZ_SB_FRAME; *ptr++ = ent.frame; }
	if (ent.colormap) { buf[2] |= DZ_SB_COLORMAP; *ptr++ = ent.colormap; }
	if (ent.skin) { buf[2] |= DZ_SB_SKIN; *ptr++ = ent.skin; }
	if (ent.org0 || ent.org1 || ent.org2)
	{
		int tmp;
		buf[2] |= DZ_SB_ORIGIN;
		tmp = ent.org0; tmp = cnvlong(tmp); memcpy(ptr,&tmp,2);
		tmp = ent.org1; tmp = cnvlong(tmp); memcpy(ptr+2,&tmp,2);
		tmp = ent.org2; tmp = cnvlong(tmp); memcpy(ptr+4,&tmp,2);
		ptr += 6;
	}
	if (ent.ang1) { buf[2] |= DZ_SB_ANGLE1; *ptr++ = ent.ang1; }
	if (ent.ang0 || ent.ang2)
	{	buf[2] |= DZ_SB_ANGLE0AND2; *ptr++ = ent.ang0; *ptr++ = ent.ang2; }
	insert_msg(buf,ptr-buf);
	base[index] = ent;
	sble = index;
	copybaseline = 1;
	return 0;
}

const uchar te_size[] = {8, 8, 8, 8, 8, 16, 16, 8, 8, 16,
				  8, 8, 10, 16, 8, 8, 14};

int dem_temp_entity(void)
{
	uchar entitytype = inptr[1];
	if (entitytype == 17)
	{
		directory[numfiles].type = TYPE_NEHAHRA;
		copy_msg(strlen(inptr + 2) + 17);
		return 0;
	}
	if (entitytype > 17)
		return 1;
	copy_msg(te_size[entitytype]);
	return 0;
}

/* nehahra */
int dem_showlmp(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	while (*ptr++);
	ptr += 2;
	*inptr = DZ_showlmp;	/* DEM_showlmp (35) is used by DZ_longtime */
	copy_msg(ptr-inptr);
	directory[numfiles].type = TYPE_NEHAHRA;
	return 0;
}

int dem_hidelmp(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
	directory[numfiles].type = TYPE_NEHAHRA;
	return 0;
}

int dem_skybox(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
	directory[numfiles].type = TYPE_NEHAHRA;
	return 0;
}
/* end nehahra */

int dem_copy_ue(void)
{	/* 2.10: changed this to look more like uwe's demospecs */
	unsigned short mask = inptr[0] & 0x7f;
	int len = 1;

	if (mask & U_MOREBITS)
	{
		mask |= inptr[len++] << 8;
		if (mask & U_TRANS)
			return 0;
	}
	if (mask & U_LONGENTITY) len++;	/* entity number */
	if (mask & U_MODEL) len++;	/* model index */
	if (mask & U_FRAME) len++;	/* frame */
	if (mask & U_COLORMAP) len++;	/* colormap */
	if (mask & U_SKIN) len++;	/* skin */
	if (mask & U_EFFECTS) len++;	/* effects */
	if (mask & U_ORIGIN0) len += 2;/* origin[0] */
	if (mask & U_ANGLE0) len++;	/* angles[0] */
	if (mask & U_ORIGIN1) len += 2;/* origin[1] */
	if (mask & U_ANGLE1) len++;	/* angles[1] */
	if (mask & U_ORIGIN2) len += 2;/* origin[2] */
	if (mask & U_ANGLE2) len++;	/* angles[2] */
	copy_msg(len + 1);	// add one more for entity number
	return 1;
}

void diff_entities(void);

int dem_updateentity(void)
{
	uint16_t mask = *inptr & 0x7f;
	short entity;
	static short lastentity;
	int len = 1;

	if (!dem_updateframe)
	{
		dem_updateframe = 1;
		lastentity = 0;
	}

	if (dem_updateframe == 2)
	{
		dem_copy_ue();
		return 0;
	}

	if (mask & U_MOREBITS) mask |= (inptr[len++] << 8);
	if (mask & U_LONGENTITY) {
		entity = getshort(inptr+len);
		len += 2;
	} else {
		entity = inptr[len++];
	}

	if (entity <= lastentity)
	{
		diff_entities();
		dem_updateframe = 2;
		dem_copy_ue();
		return 0;
	}
	lastentity = entity;

	memcpy(newent + entity, base + entity, sizeof(ent_t));
	if (mask & U_LONGENTITY && entity <= 0xff)
		newent[entity].force |= DZ_UE_LONGENTITY_FORCE;

	#define FDIFF(x,bit) if (newent[entity].x == base[entity].x) \
					newent[entity].force |= bit;
	if (mask & U_MODEL)
	    { newent[entity].modelindex = inptr[len++];
	      FDIFF(modelindex,DZ_UE_MODEL_FORCE); }
	if (mask & U_FRAME)
	    { newent[entity].frame = inptr[len++]; FDIFF(frame,DZ_UE_FRAME_FORCE); }
	if (mask & U_COLORMAP)
	    { newent[entity].colormap=inptr[len++]; FDIFF(colormap,DZ_UE_COLORMAP_FORCE); }
	if (mask & U_SKIN)
	    { newent[entity].skin = inptr[len++]; FDIFF(skin,DZ_UE_SKIN_FORCE); }
	if (mask & U_EFFECTS)
	    { newent[entity].effects = inptr[len++]; FDIFF(effects,DZ_UE_EFFECTS_FORCE); }
	if (mask & U_ORIGIN0)
	    { newent[entity].org0 = getshort(inptr+len);
	      len += 2; FDIFF(org0,DZ_UE_ORIGIN0_FORCE); }
	if (mask & U_ANGLE0)
	    { newent[entity].ang0 = inptr[len++]; FDIFF(ang0,DZ_UE_ANGLE0_FORCE); }
	if (mask & U_ORIGIN1)
	    { newent[entity].org1 = getshort(inptr+len);
	      len += 2; FDIFF(org1,DZ_UE_ORIGIN1_FORCE); }
	if (mask & U_ANGLE1)
	    { newent[entity].ang1 = inptr[len++]; FDIFF(ang1,DZ_UE_ANGLE1_FORCE); }
	if (mask & U_ORIGIN2)
	    { newent[entity].org2 = getshort(inptr+len);
	      len += 2; FDIFF(org2,DZ_UE_ORIGIN2_FORCE); }
	if (mask & U_ANGLE2)
	    { newent[entity].ang2 = inptr[len++]; FDIFF(ang2,DZ_UE_ANGLE2_FORCE); }
/* nehahra */
	if (mask & U_TRANS)
	{
		float tmp = getfloat(inptr + len);
		directory[numfiles].type = TYPE_NEHAHRA;
		if (tmp != 2 && tmp != 1)
			return 1;
		newent[entity].alpha = getfloat(inptr + len + 4);
		len += 8;
		if (tmp == 2)
		{
			newent[entity].fullbright = 1 + (int)getfloat(inptr + len);
			if (newent[entity].fullbright != 2 && newent[entity].fullbright != 1)
				return 1;
			len += 4;
		}
		else newent[entity].fullbright = 0;
		newent[entity].force |= DZ_UE_TRANS_FORCE;
	}
	newent[entity].newbit = mask & U_NOLERP;
	newent[entity].present = 1;

	newent[entity].od0 = newent[entity].org0 - oldent[entity].org0;
	newent[entity].od1 = newent[entity].org1 - oldent[entity].org1;
	newent[entity].od2 = newent[entity].org2 - oldent[entity].org2;

	while (entlink[lastent] <= entity) lastent = entlink[lastent];
	if (lastent < entity)
	{
		entlink[entity] = entlink[lastent];
		entlink[lastent] = entity;
	}

	discard_msg(len);
	return 0;
}

int (* const dem_message[])(void) = {
	NULL, NULL, NULL, NULL,
	NULL, dem_sound, dem_time, dem_string,
	dem_string,	NULL, dem_serverinfo, dem_lightstyle,
	dem_updatename,	NULL, dem_clientdata, NULL,
	NULL, NULL, NULL, NULL,
	NULL, dem_spawnbaseline, dem_temp_entity, NULL,
	NULL, dem_string, NULL, NULL,
	NULL, NULL, dem_string, NULL,
	NULL, dem_string,
	dem_showlmp, dem_hidelmp, dem_skybox	/* nehahra */
};

/* 2.10: removed all the dem_* functions that just did
	copy_msg(#) and replaced with this */
const uchar constCopy[] = {
	1, 1, 6, 5, 3, 0, 0, 0, 0, 4, 0, 0, 0, 4, 0, 3,
	3,12, 9,14, 1, 0, 0, 2, 2, 0, 1, 1,10, 1, 0, 3,
	1, 0, 0, 0, 0
};

void update_activate(int i, int *baseval, uchar *buf)
{
	uchar *ptr = buf;
	while (i - *baseval >= 0xff) { *ptr++ = 0xff; *baseval += 0xfe; }
	*ptr++ = i - *baseval;
	insert_msg(buf,ptr-buf);
}

void diff_entities(void)
{
	uchar buf[32];
	uchar *ptr;
	long tmp;
	int i, prev, mask;
	int firstent;
	int baseval = 0;
	uchar *tptr = tmpblk;

	buf[0] = DZ_IDENTIFIER_UPDATEENTITY_DIFF; insert_msg(buf,1);

	for (prev = 0, i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		ent_t n = newent[i], o = oldent[i];

		if (!n.present)
		{
			*tptr++ = 0x80;
			if (!o.active) update_activate(i,&baseval,buf);
			oldent[i] = base[i];
			entlink[prev] = entlink[i];
			continue;
		}

		if (!o.present) update_activate(i,&baseval,buf);

		prev = i;
		ptr = tptr+4;
		mask = 0;
		
		if (o.od2 != n.od2)
		{
			int diff = n.od2 - o.od2;
			if (diff >= -128 && diff <= 127)
			{   if (diff < 0) diff += 256;
			    mask |= DZ_UE_ORIGIN2_DIFF; *ptr++ = diff;
			} else {
			    mask |= DZ_UE_ORIGIN2_MOREBITS_DIFF; tmp = cnvlong(n.org2);
			    memcpy(ptr,&tmp,2); ptr += 2;
			}
		}
		if (o.od1 != n.od1)
		{
			int diff = n.od1 - o.od1;
			if (diff >= -128 && diff <= 127)
			{   if (diff < 0) diff += 256;
			    mask |= DZ_UE_ORIGIN1_DIFF; *ptr++ = diff;
			} else {
			    mask |= DZ_UE_ORIGIN1_MOREBITS_DIFF; tmp = cnvlong(n.org1);
			    memcpy(ptr,&tmp,2); ptr += 2;
			}
		}
		if (o.od0 != n.od0)
		{
			int diff = n.od0 - o.od0;
			if (diff >= -128 && diff <= 127)
			{   if (diff < 0) diff += 256;
			    mask |= DZ_UE_ORIGIN0_DIFF; *ptr++ = diff;
			} else {
			    mask |= DZ_UE_ORIGIN0_MOREBITS_DIFF; tmp = cnvlong(n.org0);
			    memcpy(ptr,&tmp,2); ptr += 2;
			}
		}
		if (o.ang0 != n.ang0)
			{ mask |= DZ_UE_ANGLE0_DIFF; *ptr++ = bdiff(n.ang0,o.ang0); }
		if (o.ang1 != n.ang1)
			{ mask |= DZ_UE_ANGLE1_DIFF; *ptr++ = bdiff(n.ang1,o.ang1); }
		if (o.ang2 != n.ang2)
			{ mask |= DZ_UE_ANGLE2_DIFF; *ptr++ = bdiff(n.ang2,o.ang2); }
		if (o.frame != n.frame)
			{ if (n.frame - o.frame == 1) mask |= DZ_UE_FRAME_SINGLE_DIFF;
			  else { mask |= DZ_UE_FRAME_NORMAL_DIFF;
				 *ptr++ = bdiff(n.frame,o.frame); }
			}
		if (o.effects != n.effects)
			{ mask |= DZ_UE_EFFECTS_DIFF; *ptr++ = n.effects; }
		if (o.modelindex != n.modelindex)
			{ mask |= DZ_UE_MODEL_DIFF; *ptr++ = n.modelindex; }
		if (o.newbit != n.newbit) mask |= DZ_UE_NOLERP_DIFF;
		if (o.colormap != n.colormap)
			{ mask |= DZ_UE_COLORMAP_DIFF; *ptr++ = n.colormap; }
		if (o.skin != n.skin)
			{ mask |= DZ_UE_SKIN_DIFF; *ptr++ = n.skin; }

	/* nehahra */
		if (n.alpha != o.alpha)
		{
			float ftmp;
			ftmp = getfloat((uchar *)&n.alpha);
			memcpy(ptr, &ftmp, 4);
			ptr += 4;
			mask |= DZ_UE_NEHAHRA_ALPHA_DIFF;
		}
		if (n.fullbright != o.fullbright)
			{ mask |= DZ_UE_NEHAHRA_FULLBRIGHT_DIFF; *ptr++ = n.fullbright; }

		if ((mask & 0xffff00) && !(mask & 0x0000ff))
		{
			/* sphere: just set something to not have empty first byte? is that
			   what this is for? seems like it. */
			static_assert(DZ_UE_ORIGIN2_DIFF == 0x01,
			              "Need dummy bit to be the first one.");
			mask |= DZ_UE_ORIGIN2_DIFF;
			tptr[3] = 0;
		}
		else
		{
			memmove(tptr+3,tptr+4,ptr-tptr-4);
			ptr--;
		}
		if (mask & 0xffff00) mask |= DZ_UE_MOREBITS_DIFF;
		if (mask & 0xff0000) mask |= DZ_UE_MOREBITS2_DIFF;

		if (!mask)
		{
			if (o.present && !o.active) continue;
			*tptr++ = 0x00;
			continue;
		}

		newent[i].active = 1;
		if (!o.active && o.present) update_activate(i,&baseval,buf);

		tmp = cnvlong(mask);
		memcpy(tptr,&tmp,3);

		if (!(mask & 0xffff00))
			{ memmove(tptr+1,tptr+3,ptr-tptr-3); ptr -= 2; }
		else if (!(mask & 0xff0000))
			{ memmove(tptr+2,tptr+3,ptr-tptr-3); ptr--; }
		tptr = ptr;
	}

	buf[0] = 0; insert_msg(buf,1);
	insert_msg(tmpblk,tptr-tmpblk);

	firstent = -1;
	for (i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		if (oldent[i].force == newent[i].force) continue;
		if (firstent < 0)
		{
			firstent = i;
			buf[0] = DZ_IDENTIFIER_UPDATEENTITY_FORCE;
			insert_msg(buf,1);
		}
		mask = i | (oldent[i].force ^ newent[i].force);
		if (mask & 0xff0000) mask |= DZ_UE_MOREBITS_FORCE;
		buf[0] = mask & 0xff;
		buf[1] = (mask >> 8) & 0xff;
		buf[2] = (mask >> 16) & 0xff;
		insert_msg(buf,2+!!buf[2]);
	}

	if (firstent > 0)
	{
		buf[0] = buf[1] = 0;
		insert_msg(buf,2);
	}

	for (i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		oldent[i] = newent[i];
		newent[i].present = 0;
	}
}

void dem_compress (uInt start, uInt stop)
{
	unsigned int inlen, pos, crc_cheat;
	int cfields, clen;
	char cdstring[12];
	uchar *ptr;
	unsigned long a1,a2,a3,o1,o2,o3;

	#define bail(s) { fprintf(stderr,"\nwarning: %s\n",s); goto bailout; }

	/* 12 is the max length of a real quake-demo's cd string */
	if (stop - start > 13)
	for (clen = 0; clen < 12; clen++)
	{
		Infile_Read(cdstring + clen, 1);
		if (cdstring[clen] == '\n')
		{
			dzWrite(cdstring, clen + 1);
			goto tryit;
		}
		else if (cdstring[clen] == '-')
			/* ok */;
		else if (cdstring[clen] < '0' || cdstring[clen] > '9')
			break;
	}
	/* failed the test */
	directory[numfiles].type = TYPE_NORMAL;
	Infile_Seek(start);
	crcval = INITCRC;
	normal_compress(stop - start);
	return;
	
tryit:
	memset(&base,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldent,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldcd,0,sizeof(cdata_t));
	oldcd.voz = 22;
	oldcd.items = 0x4001;
	entlink[0] = MAX_ENT;
	o1 = o2 = o3 = 0;
	copybaseline = 0;
	dem_gametime = 0;
	sble = 0;

	for (pos = start + clen + 1; pos < stop; pos += 4 + inlen)
	{
		cfields = 0, clen = 0;
		crc_cheat = crcval;
		if (pos + 4 > stop) bail("unexpected EOF");
		Infile_Read(&inlen,4);
		inlen = cnvlong(inlen) + 12;
		if (inlen > p_blocksize / 2) bail("block too long");
		if (pos + inlen + 4 > stop) bail("unexpected EOF");
		Infile_Read(inblk, inlen);
		outlen = 1;
		ptr = inptr = inblk;
		a1 = getlong(inptr);
		a2 = getlong(inptr + 4);
		a3 = getlong(inptr + 8);
		o1 = a1 - o1; o1 = cnvlong(o1);
		o2 = a2 - o2; o2 = cnvlong(o2);
		o3 = a3 - o3; o3 = cnvlong(o3);
		if (o1) { cfields |= 1; memcpy(ptr,&o1,4); ptr += 4; }
		if (o2) { cfields |= 2; memcpy(ptr,&o2,4); ptr += 4; }
		if (o3) { cfields |= 4; memcpy(ptr,&o3,4); ptr += 4; }

		o1 = a1; o2 = a2; o3 = a3; clen = ptr - inptr;
		copy_msg(clen); discard_msg(12-clen);
		
		dem_updateframe = lastent = 0;
		while (inptr < inblk + inlen)
		{
			if (*inptr && *inptr <= DEM_skybox)
			{
				if (dem_updateframe == 1)
				{
					diff_entities();
					dem_updateframe = 2;
				}
				if (constCopy[(int)(*inptr - 1)])
					copy_msg(constCopy[(int)(*inptr - 1)]);
				else if (dem_message[(int)(*inptr - 1)]())
					bail("unexpected message contents encountered");
			}
			else if (*inptr & 0x80)
			{
				if (dem_updateentity())
					bail("unexpected message contents encountered");
			}
			else
				bail("bad message encountered");
		}
		if (dem_updateframe == 1) diff_entities();

		if (inptr > inblk + inlen) bail("parse error");
		if (outlen > p_blocksize / 2) bail("block too large in output");
		*outblk = cfields;
		cfields = 0; insert_msg(&cfields,1);
		dzWrite(outblk,outlen);
		if (copybaseline)
		{
			memcpy(oldent,base,sizeof(ent_t)*MAX_ENT);
			copybaseline = 0;
		}
		if (AbortOp)
			return;
	}

	return;

bailout:
	Infile_Seek(pos);
	inlen = stop - pos;
	crcval = crc_cheat;

	while (inlen && !AbortOp)
	{
		int toread = (inlen > p_blocksize-5)? p_blocksize-5 : inlen;
		toread = cnvlong(toread);
		*inblk = 0xff;
		memcpy(inblk+1,&toread,4);
		toread = cnvlong(toread);
		Infile_Read(inblk+5,toread);
		dzWrite(inblk,toread+5);
		inlen -= toread;
	}
}
