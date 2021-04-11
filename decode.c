#include "dzip.h"

int dem_decode_type;

void demx_nop(void)
{
	copy_msg(1);
}

void demx_disconnect(void)
{
	copy_msg(1);
}

void demx_updatestat(void)
{
	copy_msg(6);
}

void demx_version(void)
{
	copy_msg(5);
}

void demx_setview(void)
{
	copy_msg(3);
}

void demx_sound(void)
{
	int c, len;
	uInt entity;
	uchar mask = inptr[1];
	uchar channel;

	if (*inptr > DEM_sound)
	{
		len = 10;
		mask = *inptr & 3;
	}
	else
		len = 11;
	if (mask & SND_VOLUME) len++;
	if (mask & SND_ATTENUATION) len++;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { copy_msg(len); return; }
#endif
	*inptr = DEM_sound;
	insert_msg(inptr,1);

	*inptr = mask;
	channel = inptr[len-9] & 7;
	inptr[len-9] = (inptr[len-9] & 0xf8) + ((2 - channel) & 7);

	if ((entity = getshort(inptr+len-9) >> 3) < MAX_ENT)
	{
		c = getshort(inptr+len-6); c += newent[entity].org0;
		c = cnvlong(c); memcpy(inptr+len-6,&c,2);
		c = getshort(inptr+len-4); c += newent[entity].org1;
		c = cnvlong(c); memcpy(inptr+len-4,&c,2);
		c = getshort(inptr+len-2); c += newent[entity].org2;
		c = cnvlong(c); memcpy(inptr+len-2,&c,2);
	}

	copy_msg(len);
}

void demx_longtime(void)
{
	long tmp = getlong(inptr+1);
	dem_gametime += tmp;
	tmp = cnvlong(dem_gametime);
	*inptr = DEM_time;
	memcpy(inptr+1,&tmp,4);
	copy_msg(5);
}

void demx_time(void)
{
	uchar buf[5];
	long tmp = getshort(inptr+1) & 0xffff;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { demx_longtime(); return; }
#endif
	dem_gametime += tmp;
	tmp = cnvlong(dem_gametime);
	buf[0] = DEM_time;
	memcpy(buf+1,&tmp,4);
	insert_msg(buf,5);
	discard_msg(3);
}

/* used by lots of msgs */
void demx_string(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void demx_setangle(void)
{
	copy_msg(4);
}

void demx_serverinfo(void)
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
}

void demx_lightstyle(void)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void demx_updatename(void)
{
	uchar *ptr = inptr + 2;
	while (*ptr++);
	copy_msg(ptr-inptr);
}

void demx_updatefrags(void)
{
	copy_msg(4);
}

int bplus(int x, int y)
{
	if (x >= 128) x -= 256;
	return y + x;
}

void create_clientdata_msg(void)
{
	uchar buf[32];
	uchar *ptr = buf+3;
	int mask = newcd.invbit? 0 : SU_ITEMS;
	long tmp;

	buf[0] = DEM_clientdata;

	#define CMADD(x,def,bit,bit2) if (newcd.x != def || newcd.force & bit2)\
		{ mask |= bit; *ptr++ = newcd.x; }

	CMADD(voz,22,SU_VIEWHEIGHT,DZ_CD_VIEWHEIGHT_FORCE);
	CMADD(pax,0,SU_IDEALPITCH,DZ_CD_IDEALPITCH_FORCE);
	CMADD(ang0,0,SU_PUNCH0,DZ_CD_PUNCH0_FORCE);
	CMADD(vel0,0,SU_VELOCITY0,DZ_CD_VELOCITY0_FORCE);
	CMADD(ang1,0,SU_PUNCH1,DZ_CD_PUNCH1_FORCE);
	CMADD(vel1,0,SU_VELOCITY1,DZ_CD_VELOCITY1_FORCE);
	CMADD(ang2,0,SU_PUNCH2,DZ_CD_PUNCH2_FORCE);
	CMADD(vel2,0,SU_VELOCITY2,DZ_CD_VELOCITY2_FORCE);
	tmp = cnvlong(newcd.items); memcpy(ptr,&tmp,4); ptr += 4;
	if (newcd.uk10) mask |= SU_ONGROUND;
	if (newcd.uk11) mask |= SU_INWATER;
	CMADD(wpf,0,SU_WEAPONFRAME,DZ_CD_WEAPONFRAME_FORCE);
	CMADD(av,0,SU_ARMOR,DZ_CD_ARMOR_FORCE);
	CMADD(wpm,0,SU_WEAPON,DZ_CD_WEAPON_FORCE);
	tmp = cnvlong(newcd.health); memcpy(ptr,&tmp,2); ptr += 2;
	*ptr++ = newcd.am;
	*ptr++ = newcd.sh;
	*ptr++ = newcd.nl;
	*ptr++ = newcd.rk;
	*ptr++ = newcd.ce;
	*ptr++ = newcd.wp;
	mask = cnvlong(mask);
	memcpy(buf+1,&mask,2);
	insert_msg(buf,ptr-buf);

	oldcd = newcd;
}

#define CPLUS(x,bit) if (mask & bit) { newcd.x = bplus(*ptr++,oldcd.x); }

void demx_clientdata(void)
{
	uchar *ptr = inptr;
	int mask = *ptr++;

	newcd = oldcd;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { demv1_clientdata(); return; }
#endif
	if (mask & DZ_CD_MOREBITS_DIFF) mask += *ptr++ << 8;
	if (mask & DZ_CD_MOREBITS1_DIFF) mask += *ptr++ << 16;
	if (mask & DZ_CD_MOREBITS2_DIFF) mask += *ptr++ << 24;

	CPLUS(vel2,DZ_CD_VELOCITY2_DIFF);
	CPLUS(vel0,DZ_CD_VELOCITY0_DIFF);
	CPLUS(vel1,DZ_CD_VELOCITY1_DIFF);

	CPLUS(wpf,DZ_CD_WEAPONFRAME_DIFF);
	if (mask & DZ_CD_ONGROUND_DIFF) newcd.uk10 = !oldcd.uk10;
	CPLUS(ang0,DZ_CD_PUNCH0_DIFF);
	CPLUS(am,DZ_CD_AMMO_DIFF);
	if (mask & DZ_CD_HEALTH_DIFF) { newcd.health += getshort(ptr); ptr += 2; }
	if (mask & DZ_CD_ITEMS_DIFF) { newcd.items ^= getlong(ptr); ptr += 4; }
	CPLUS(av,DZ_CD_ARMOR_DIFF);

	CPLUS(pax,DZ_CD_IDEALPITCH_DIFF);
	CPLUS(sh,DZ_CD_SHELLS_DIFF);
	CPLUS(nl,DZ_CD_NAILS_DIFF);
	CPLUS(rk,DZ_CD_ROCKETS_DIFF);
	CPLUS(wpm,DZ_CD_WEAPON_DIFF);
	CPLUS(wp,DZ_CD_WEAPONINDEX_DIFF);
	if (mask & DZ_CD_INWATER_DIFF) newcd.uk11 = !oldcd.uk11;

	CPLUS(voz,DZ_CD_VIEWHEIGHT_DIFF);
	CPLUS(ce,DZ_CD_CELLS_DIFF);
	CPLUS(ang1,DZ_CD_PUNCH1_DIFF);
	CPLUS(ang2,DZ_CD_PUNCH2_DIFF);
	if (mask & DZ_CD_INVBIT_DIFF) newcd.invbit = !oldcd.invbit;

	discard_msg(ptr-inptr);

	if ((*ptr & 0xf0) == DZ_IDENTIFIER_CLIENTDATA_FORCE)
	{
		mask = *ptr++;
		if (mask & DZ_CD_MOREBITS_FORCE) mask |= *ptr++ << 8;
		newcd.force ^= mask & 0xff07;
		discard_msg(ptr-inptr);
	}

	create_clientdata_msg();
}

void demx_stopsound(void)
{
	copy_msg(3);
}

void demx_updatecolors(void)
{
	copy_msg(3);
}

void demx_particle(void)
{
	copy_msg(12);
}

void demx_damage(void)
{
	copy_msg(9);
}

void demx_spawnstatic(void)
{
	copy_msg(14);
}

void demx_spawnbinary(void)
{
	copy_msg(1);
}

void demx_spawnbaseline(void)
{
	uchar buf[16], *ptr = inptr+3;
	ent_t ent;
	int mask = getshort(inptr+1);
	
	sble = (sble + (mask & 0x03ff)) % 0x400;
	mask = mask >> 8;
	memset(&ent,0,sizeof(ent_t));
	ent.modelindex = *ptr++;
	if (mask & DZ_SB_FRAME) ent.frame = *ptr++;
	if (mask & DZ_SB_COLORMAP) ent.colormap = *ptr++;
	if (mask & DZ_SB_SKIN) ent.skin = *ptr++;
	if (mask & DZ_SB_ORIGIN)
	{
		ent.org0 = getshort(ptr); ptr += 2;
		ent.org1 = getshort(ptr); ptr += 2;
		ent.org2 = getshort(ptr); ptr += 2;
	}
	if (mask & DZ_SB_ANGLE1) ent.ang1 = *ptr++;
	if (mask & DZ_SB_ANGLE0AND2) { ent.ang0 = *ptr++; ent.ang2 = *ptr++; }
	discard_msg(ptr-inptr);

	buf[0] = DEM_spawnbaseline;
	mask = cnvlong(sble); memcpy(buf+1,&mask,2);
	buf[3] = ent.modelindex;
	buf[4] = ent.frame;
	buf[5] = ent.colormap;
	buf[6] = ent.skin;
	mask = cnvlong(ent.org0); memcpy(buf+7,&mask,2);
	buf[9] = ent.ang0;
	mask = cnvlong(ent.org1); memcpy(buf+10,&mask,2);
	buf[12] = ent.ang1;
	mask = cnvlong(ent.org2); memcpy(buf+13,&mask,2);
	buf[15] = ent.ang2;
	insert_msg(buf,16);

	base[sble] = ent;
	copybaseline = 1;
}

extern uchar te_size[];

void demx_temp_entity(void)
{
	if (inptr[1] == 17)
		copy_msg(strlen(inptr + 2) + 17);
	else
		copy_msg(te_size[inptr[1]]);
}

void demx_setpause(void)
{
	copy_msg(2);
}

void demx_signonnum(void)
{
	copy_msg(2);
}

void demx_killedmonster(void)
{
	copy_msg(1);
}

void demx_foundsecret(void)
{
	copy_msg(1);
}

void demx_spawnstaticsound(void)
{
	copy_msg(10);
}

void demx_intermission(void)
{
	copy_msg(1);
}

void demx_cdtrack(void)
{
	copy_msg(3);
}

void demx_sellscreen(void)
{
	copy_msg(1);
}

/* nehahra */
void demx_showlmp(void)
{
	uchar *ptr = inptr + 1;
	while (*ptr++);
	while (*ptr++);
	ptr += 2;
	*inptr = DEM_showlmp;
	copy_msg(ptr-inptr);
}

void demx_updateentity(void)
{
	uchar buf[32], *ptr;
	int mask, i, entity;
	int baseval = 0, prev;
	ent_t n, o;
	int32_t tmp;

#ifndef GUI
	if (dem_decode_type == TYPE_DEMV1) { demv1_updateentity(); return; }
#endif
	lastent = 0;
	for (ptr = inptr+1; *ptr; ptr++)
	{
		if (*ptr == 0xff) { baseval += 0xfe; continue; }
		entity = baseval + *ptr;
		newent[entity].active = 1;
		while (entlink[lastent] <= entity) lastent = entlink[lastent];
		if (lastent < entity)
		{
			entlink[entity] = entlink[lastent];
			entlink[lastent] = entity;
		}
	}

	for (prev = 0, i = entlink[0], ptr++; i < MAX_ENT; i = entlink[i])
	{
		newent[i].org0 += newent[i].od0;
		newent[i].org1 += newent[i].od1;
		newent[i].org2 += newent[i].od2;

		if (!newent[i].active) { prev = i; continue; }

		mask = *ptr++;

		if (mask == DZ_UE_MOREBITS_DIFF)
		{
			oldent[i] = newent[i] = base[i];
			entlink[prev] = entlink[i];
			continue;
		}

		prev = i;

		if (mask == 0x00) { newent[i].active = 0; continue; }

		if (mask & DZ_UE_MOREBITS_DIFF) mask += (*ptr++) << 8;
		if (mask & DZ_UE_MOREBITS2_DIFF) mask += (*ptr++) << 16;

		n = newent[i];
		o = oldent[i];

		if (mask & DZ_UE_ORIGIN2_DIFF)
			{ n.od2 = bplus(*ptr++,o.od2); n.org2 = o.org2 + n.od2; }
		if (mask & DZ_UE_ORIGIN2_MOREBITS_DIFF)
			{ n.org2 = getshort(ptr); ptr += 2; n.od2 = n.org2 - o.org2; }
		if (mask & DZ_UE_ORIGIN1_DIFF)
			{ n.od1 = bplus(*ptr++,o.od1); n.org1 = o.org1 + n.od1; }
		if (mask & DZ_UE_ORIGIN1_MOREBITS_DIFF)
			{ n.org1 = getshort(ptr); ptr += 2; n.od1 = n.org1 - o.org1; }
		if (mask & DZ_UE_ORIGIN0_DIFF)
			{ n.od0 = bplus(*ptr++,o.od0); n.org0 = o.org0 + n.od0; }
		if (mask & DZ_UE_ORIGIN0_MOREBITS_DIFF)
			{ n.org0 = getshort(ptr); ptr += 2; n.od0 = n.org0 - o.org0; }

		if (mask & DZ_UE_ANGLE0_DIFF) n.ang0 = bplus(*ptr++,o.ang0);
		if (mask & DZ_UE_ANGLE1_DIFF) n.ang1 = bplus(*ptr++,o.ang1);
		if (mask & DZ_UE_ANGLE2_DIFF) n.ang2 = bplus(*ptr++,o.ang2);
		if (mask & DZ_UE_FRAME_SINGLE_DIFF) n.frame = o.frame+1;
		if (mask & DZ_UE_FRAME_NORMAL_DIFF) n.frame = bplus(*ptr++,o.frame);

		if (mask & DZ_UE_EFFECTS_DIFF) n.effects = *ptr++;
		if (mask & DZ_UE_MODEL_DIFF) n.modelindex = *ptr++;
		if (mask & DZ_UE_NOLERP_DIFF) n.newbit = !o.newbit;
		if (mask & DZ_UE_COLORMAP_DIFF) n.colormap = *ptr++;
		if (mask & DZ_UE_SKIN_DIFF) n.skin = *ptr++;
	/* nehahra */
		if (mask & DZ_UE_NEHAHRA_ALPHA_DIFF) { n.alpha = getfloat(ptr); ptr += 4; }
		if (mask & DZ_UE_NEHAHRA_FULLBRIGHT_DIFF) n.fullbright = *ptr++;

		newent[i] = n;
	}

	if (*ptr == DZ_IDENTIFIER_UPDATEENTITY_FORCE)
	{
		ptr++;
		while ((mask = getshort(ptr)))
		{
			ptr += 2;
			mask &= 0xffff;
			if (mask & DZ_UE_MOREBITS_FORCE) mask |= *ptr++ << 16;
			entity = mask & 0x3ff;
			newent[entity].force ^= mask & 0xfffc00;
		}
		ptr += 2;
	}

	discard_msg(ptr-inptr);

	for (i = entlink[0]; i < MAX_ENT; i = entlink[i])
	{
		ent_t n = newent[i], b = base[i];

		ptr = buf+2;
		mask = U_SIGNAL;

		if (i > 0xff || (n.force & DZ_UE_LONGENTITY_FORCE))
		{
			tmp = cnvlong(i);
			memcpy(ptr,&tmp,2);
			ptr += 2;
			mask |= U_LONGENTITY;
		}
		else
			*ptr++ = i;

		#define BDIFF(x,bit,bit2) \
			if (n.x != b.x || n.force & bit2) \
				{ *ptr++ = n.x; mask |= bit; }

		BDIFF(modelindex,U_MODEL,DZ_UE_MODEL_FORCE);
		BDIFF(frame,U_FRAME,DZ_UE_FRAME_FORCE);
		BDIFF(colormap,U_COLORMAP,DZ_UE_COLORMAP_FORCE);
		BDIFF(skin,U_SKIN,DZ_UE_SKIN_FORCE);
		BDIFF(effects,U_EFFECTS,DZ_UE_EFFECTS_FORCE);
		if (n.org0 != b.org0 || n.force & DZ_UE_ORIGIN0_FORCE)
		    { mask |= U_ORIGIN0; tmp = cnvlong(n.org0);
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang0,U_ANGLE0,DZ_UE_ANGLE0_FORCE);
		if (n.org1 != b.org1 || n.force & DZ_UE_ORIGIN1_FORCE)
		    { mask |= U_ORIGIN1; tmp = cnvlong(n.org1);
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang1,U_ANGLE1,DZ_UE_ANGLE1_FORCE);
		if (n.org2 != b.org2 || n.force & DZ_UE_ORIGIN2_FORCE)
		    { mask |= U_ORIGIN2; tmp = cnvlong(n.org2);
		      memcpy(ptr,&tmp,2); ptr += 2; }
		BDIFF(ang2,U_ANGLE2,DZ_UE_ANGLE2_FORCE);
/* nehahra */
		if (n.force & DZ_UE_TRANS_FORCE)
		{
			float f = 1;

			if (n.fullbright)
				f = 2;
			memcpy(&tmp, &f, sizeof(tmp));
			tmp = cnvlong(tmp);
			memcpy(ptr, &tmp, 4);
			memcpy(&tmp, &n.alpha, sizeof(tmp));
			tmp = cnvlong(tmp);
			memcpy(ptr + 4, &tmp, 4);
			ptr += 8;
			if (f == 2)
			{
				f = (char)(n.fullbright - 1);
				memcpy(&tmp, &f, sizeof(tmp));
				tmp = cnvlong(tmp);
				memcpy(ptr, &tmp, 4);
				ptr += 4;
			}
			mask |= U_TRANS;
		}
		
		if (n.newbit) mask |= U_NOLERP;
		if (mask & 0xff00) mask |= U_MOREBITS;
		buf[0] = mask & 0xff;
		buf[1] = (mask & 0xff00) >> 8;
		if (!(mask & U_MOREBITS)) { memmove(buf+1,buf+2,ptr-buf-2); ptr--; }
		insert_msg(buf,ptr-buf);

		oldent[i] = newent[i];
	}

}

void (* const demx_message[])(void) = {
	demx_nop, demx_disconnect, demx_updatestat, demx_version,
	demx_setview, demx_sound, demx_time, demx_string, demx_string,
	demx_setangle, demx_serverinfo, demx_lightstyle, demx_updatename,
	demx_updatefrags, demx_clientdata, demx_stopsound, demx_updatecolors,
	demx_particle, demx_damage, demx_spawnstatic, demx_spawnbinary,
	demx_spawnbaseline, demx_temp_entity, demx_setpause, demx_signonnum,
	demx_string, demx_killedmonster, demx_foundsecret,
	demx_spawnstaticsound, demx_intermission, demx_string,
	demx_cdtrack, demx_sellscreen, demx_string, demx_longtime,
	demx_string, demx_string, demx_showlmp	/* nehahra */
};

unsigned long cam0, cam1, cam2;

void dem_uncompress_init (int type)
{
	dem_decode_type = -type;
	memset(&base,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldent,0,sizeof(ent_t)*MAX_ENT);
	memset(&oldcd,0,sizeof(cdata_t));
	oldcd.voz = 22;
	oldcd.items = 0x4001;
	entlink[0] = MAX_ENT;
	cam0 = cam1 = cam2 = 0;
	copybaseline = 0;
	dem_gametime = 0;
	maxent = 0;
	sble = 0;
}
	
uInt dem_uncompress_block(void)
{
	long a1;
	uchar cfields;
#ifdef GUI
	int uemask = DZ_IDENTIFIER_UPDATEENTITY_DIFF;
	int cdmask = DZ_IDENTIFIER_CLIENTDATA_DIFF;
#else
	int uemask = (dem_decode_type == TYPE_DEMV1)? 0x80 : DZ_IDENTIFIER_UPDATEENTITY_DIFF;
	int cdmask = (dem_decode_type == TYPE_DEMV1)? 0xf0 : DZ_IDENTIFIER_CLIENTDATA_DIFF;
#endif
	cfields = *inptr++;

	if (cfields & 1) { cam0 += getlong(inptr); inptr += 4; }
	if (cfields & 2) { cam1 += getlong(inptr); inptr += 4; }
	if (cfields & 4) { cam2 += getlong(inptr); inptr += 4; }

	outlen = 0;
	insert_msg(&a1,4);
	a1 = cnvlong(cam0); insert_msg(&a1,4);
	a1 = cnvlong(cam1); insert_msg(&a1,4);
	a1 = cnvlong(cam2); insert_msg(&a1,4);

	dem_updateframe = 0;
	while (*inptr)
	{		
		if ((*inptr & 0xf8) == uemask)
			demx_updateentity();
		else
		{
#ifndef GUI
			if (dem_updateframe)
				{ demv1_dxentities(); dem_updateframe = 0; }
#endif
			if (*inptr && *inptr <= DZ_showlmp)
				demx_message[*inptr - 1]();
			else if ((*inptr & 0xf0) == cdmask) 
				demx_clientdata();
			else if ((*inptr & 0xf8) == DZ_IDENTIFIER_SOUND)
				demx_sound();
			else if (*inptr >= 0x80)
			{
				if (!dem_copy_ue())
					return 0;
			}
			else
				return 0;
		}
	}
#ifndef GUI
	if (dem_updateframe) demv1_dxentities();
#endif
	outlen -= 16;
	outlen = cnvlong(outlen);
	memcpy(outblk,&outlen,4);
	Outfile_Write(outblk,cnvlong(outlen)+16);

	if (copybaseline)
	{
		copybaseline = 0;
		memcpy(oldent,base,sizeof(ent_t)*MAX_ENT);
		memcpy(newent,base,sizeof(ent_t)*MAX_ENT);
	}

	return inptr-inblk+1;
}

uInt dem_uncompress (uInt maxsize)
{
	uInt blocksize = 0;
	inptr = inblk;
	if (dem_decode_type < 0) 
	{
		dem_decode_type = -dem_decode_type;
		while (inptr[blocksize] != '\n' && blocksize < 12)
			blocksize++;

		if (blocksize == 12)	/* seriously corrupt! */
			return 0;

		Outfile_Write(inblk, ++blocksize);
		inptr += blocksize;
	}
	while (blocksize < 16000 && blocksize < maxsize)
	{
		if (AbortOp)
			return 0;
		if (*inptr == 0xff)
		{
			uInt len = getlong(inptr+1);
			if (p_blocksize - blocksize - 5 < len)
				return blocksize;
			Outfile_Write(inptr + 5,len);
			blocksize = inptr - inblk + len + 5;
		}
		else
			blocksize = dem_uncompress_block();
		if (!blocksize)
			return 0;	/* corrupt encoding */
		inptr++;
	}
	return blocksize;
}
