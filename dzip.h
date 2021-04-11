#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "external/zlib/zlib.h"

typedef unsigned char uchar;

#define MAX_ENT 1024
#define MAJOR_VERSION 2
#define MINOR_VERSION 10
#define INITCRC 0xffffffff

enum { TYPE_NORMAL, TYPE_DEMV1, TYPE_TXT, TYPE_PAK, TYPE_DZ, TYPE_DEM,
	TYPE_NEHAHRA, TYPE_DIR, TYPE_STORE, TYPE_LAST };

enum {
	DEM_bad, DEM_nop, DEM_disconnect, DEM_updatestat, DEM_version,
	DEM_setview, DEM_sound, DEM_time, DEM_print, DEM_stufftext,
	DEM_setangle, DEM_serverinfo, DEM_lightstyle, DEM_updatename,
	DEM_updatefrags, DEM_clientdata, DEM_stopsound, DEM_updatecolors,
	DEM_particle, DEM_damage, DEM_spawnstatic, DEM_spawnbinary,
	DEM_spawnbaseline, DEM_temp_entity, DEM_setpause, DEM_signonnum,
	DEM_centerprint, DEM_killedmonster, DEM_foundsecret,
	DEM_spawnstaticsound, DEM_intermission, DEM_finale,
	DEM_cdtrack, DEM_sellscreen, DEM_cutscene, DZ_longtime,
/* nehahra */
	DEM_showlmp = 35, DEM_hidelmp, DEM_skybox, DZ_showlmp
};

/**
 * Identifiers for encoded messages in ".dz" encoding
 */
#define DZ_IDENTIFIER_CLIENTDATA_FORCE    (0x40|0x10)
#define DZ_IDENTIFIER_CLIENTDATA_DIFF     (0x40)
#define DZ_IDENTIFIER_UPDATEENTITY_FORCE  (0x20|0x10|0x01)
#define DZ_IDENTIFIER_UPDATEENTITY_DIFF   (0x20|0x10)
#define DZ_IDENTIFIER_SOUND               (0x20|0x10|0x08)

/**
 * Sound bit flags in ".dem" files. See protocol.h in Quake source code.
 */
#define SND_VOLUME      0x01
#define SND_ATTENUATION 0x02
#define SND_LOOPING     0x04

/**
 * Clientdata bit flags in ".dem" files. See protocol.h in Quake source code.
 */
#define SU_VIEWHEIGHT   0x0001
#define SU_IDEALPITCH   0x0002
#define SU_PUNCH0       0x0004
#define SU_PUNCH1       0x0008
#define SU_PUNCH2       0x0010
#define SU_VELOCITY0    0x0020
#define SU_VELOCITY1    0x0040
#define SU_VELOCITY2    0x0080
#define SU_AIMENT       0x0100
#define SU_ITEMS        0x0200
#define SU_ONGROUND     0x0400
#define SU_INWATER      0x0800
#define SU_WEAPONFRAME  0x1000
#define SU_ARMOR        0x2000
#define SU_WEAPON       0x4000

/* Flags for "clientdata" to force update of a property when there is no
   difference */
#define DZ_CD_VELOCITY0_FORCE   0x0001
#define DZ_CD_VELOCITY1_FORCE   0x0002
#define DZ_CD_VELOCITY2_FORCE   0x0004
#define DZ_CD_MOREBITS_FORCE    0x0008
/* Identifier for clientdata is encoded within the mask at bits 4 to 7. So these
   are reserved for the identifier. */
#define DZ_CD_PUNCH0_FORCE      0x0100
#define DZ_CD_PUNCH1_FORCE      0x0200
#define DZ_CD_PUNCH2_FORCE      0x0400
#define DZ_CD_VIEWHEIGHT_FORCE  0x0800
#define DZ_CD_IDEALPITCH_FORCE  0x1000
#define DZ_CD_WEAPONFRAME_FORCE 0x2000
#define DZ_CD_ARMOR_FORCE       0x4000
#define DZ_CD_WEAPON_FORCE      0x8000

/* Flags for "clientdata" that signal difference in properties in ".dz" files */
#define DZ_CD_VELOCITY2_DIFF    0x00000001
#define DZ_CD_VELOCITY0_DIFF    0x00000002
#define DZ_CD_VELOCITY1_DIFF    0x00000004
#define DZ_CD_MOREBITS_DIFF     0x00000008
/* Identifier for clientdata is encoded within the mask at bits 4 to 7. So these
   are reserved for the identifier. */
#define DZ_CD_WEAPONFRAME_DIFF  0x00000100
#define DZ_CD_ONGROUND_DIFF     0x00000200
#define DZ_CD_PUNCH0_DIFF       0x00000400
#define DZ_CD_AMMO_DIFF         0x00000800
#define DZ_CD_HEALTH_DIFF       0x00001000
#define DZ_CD_ITEMS_DIFF        0x00002000
#define DZ_CD_ARMOR_DIFF        0x00004000
#define DZ_CD_MOREBITS1_DIFF    0x00008000
#define DZ_CD_IDEALPITCH_DIFF   0x00010000
#define DZ_CD_SHELLS_DIFF       0x00020000
#define DZ_CD_NAILS_DIFF        0x00040000
#define DZ_CD_ROCKETS_DIFF      0x00080000
#define DZ_CD_WEAPON_DIFF       0x00100000
#define DZ_CD_WEAPONINDEX_DIFF  0x00200000
#define DZ_CD_INWATER_DIFF      0x00400000
#define DZ_CD_MOREBITS2_DIFF    0x00800000
#define DZ_CD_VIEWHEIGHT_DIFF   0x01000000
#define DZ_CD_CELLS_DIFF        0x02000000
#define DZ_CD_PUNCH1_DIFF       0x04000000
#define DZ_CD_PUNCH2_DIFF       0x08000000
#define DZ_CD_INVBIT_DIFF       0x10000000

/* Flags for spawnbaseline properties in ".dz" encoding.
   Bits 1 and 2 are used to encode a part of the entity index. */
#define DZ_SB_FRAME       0x04
#define DZ_SB_COLORMAP    0x08
#define DZ_SB_SKIN        0x10
#define DZ_SB_ORIGIN      0x20
#define DZ_SB_ANGLE1      0x40
#define DZ_SB_ANGLE0AND2  0x80

/**
 * "updateentity" bit flags in ".dem" files. See protocol.h in Quake source
 */
#define U_MOREBITS   0x0001
#define U_ORIGIN0    0x0002
#define U_ORIGIN1    0x0004
#define U_ORIGIN2    0x0008
#define U_ANGLE1     0x0010
#define U_NOLERP     0x0020
#define U_FRAME      0x0040
#define U_SIGNAL     0x0080
#define U_ANGLE0     0x0100
#define U_ANGLE2     0x0200
#define U_MODEL      0x0400
#define U_COLORMAP   0x0800
#define U_SKIN       0x1000
#define U_EFFECTS    0x2000
#define U_LONGENTITY 0x4000
#define U_TRANS      0x8000  /* nehahra */

/* Flags for "updateentity" to force update of a property when there is no
   difference. First 10 bits are used to encode entity index. */
#define DZ_UE_ORIGIN1_FORCE    0x000400
#define DZ_UE_ANGLE0_FORCE     0x000800
#define DZ_UE_ANGLE1_FORCE     0x001000
#define DZ_UE_ANGLE2_FORCE     0x002000
#define DZ_UE_FRAME_FORCE      0x004000
#define DZ_UE_MOREBITS_FORCE   0x008000
#define DZ_UE_ORIGIN0_FORCE    0x010000
#define DZ_UE_ORIGIN2_FORCE    0x020000
#define DZ_UE_MODEL_FORCE      0x040000
#define DZ_UE_COLORMAP_FORCE   0x080000
#define DZ_UE_SKIN_FORCE       0x100000
#define DZ_UE_EFFECTS_FORCE    0x200000
#define DZ_UE_LONGENTITY_FORCE 0x400000
#define DZ_UE_TRANS_FORCE      0x800000  /* nehahra */

/* Flags for "updateentity" that signal difference in properties in ".dz"
   encoding */
#define DZ_UE_ORIGIN2_DIFF            0x000001
#define DZ_UE_ORIGIN1_DIFF            0x000002
#define DZ_UE_ORIGIN0_DIFF            0x000004
#define DZ_UE_ANGLE0_DIFF             0x000008
#define DZ_UE_ANGLE1_DIFF             0x000010
#define DZ_UE_ANGLE2_DIFF             0x000020
#define DZ_UE_FRAME_SINGLE_DIFF       0x000040
#define DZ_UE_MOREBITS_DIFF           0x000080
#define DZ_UE_FRAME_NORMAL_DIFF       0x000100
#define DZ_UE_ORIGIN0_MOREBITS_DIFF   0x000200
#define DZ_UE_ORIGIN1_MOREBITS_DIFF   0x000400
#define DZ_UE_ORIGIN2_MOREBITS_DIFF   0x000800
#define DZ_UE_EFFECTS_DIFF            0x001000
#define DZ_UE_MODEL_DIFF              0x002000
#define DZ_UE_NOLERP_DIFF             0x004000
#define DZ_UE_MOREBITS2_DIFF          0x008000
#define DZ_UE_COLORMAP_DIFF           0x010000
#define DZ_UE_SKIN_DIFF               0x020000
#define DZ_UE_NEHAHRA_ALPHA_DIFF      0x040000
#define DZ_UE_NEHAHRA_FULLBRIGHT_DIFF 0x080000


typedef struct {
	uchar voz, pax;
	uchar ang0, ang1, ang2;
	uchar vel0, vel1, vel2;
	long items;
	uchar uk10, uk11, invbit;
	uchar wpf, av, wpm;
	int health;
	uchar am, sh, nl, rk, ce, wp;
	int force;
} cdata_t;

typedef struct { 
	uInt ptr;	/* start of file inside dz */
	uInt size;	/* v1: intermediate size. v2: compressed size */
	uInt real;	/* uncompressed size */
	unsigned short len;	/* length of name */
	unsigned short pak;
	uInt crc;
	uInt type;
	uInt date;
	uInt inter;	/* v2: intermediate size */
	char *name;
} direntry_t;
#define DE_DISK_SIZE 32

typedef struct {
	uchar modelindex, frame;
	uchar colormap, skin;
	uchar effects;
	uchar ang0, ang1, ang2;
	uchar newbit, present, active;
	uchar fullbright;	/* nehahra */
	int org0, org1, org2;
	int od0, od1, od2;
	int force;
	float alpha;		/* nehahra */
} ent_t;

typedef struct {
	char name[56];
	uInt ptr;
	uInt len;
} pakentry_t;

int bplus (int, int);
void copy_msg (uInt);
void create_clientdata_msg (void);
void crc_init (void);
void dem_compress (uInt, uInt);
int dem_copy_ue (void);
uInt dem_uncompress (uInt);
void dem_uncompress_init (int);
void demv1_clientdata (void);
void demv1_updateentity (void);
void demv1_dxentities (void);
void dzAddFolder (char *);
void dzCompressFile (char *, uInt, uInt);
void dzDeleteFiles  (uInt *, uInt, void (*)(uInt, uInt));
void dzExtractFile (uInt, int);
int dzRead (int);
int dzReadDirectory (char *);
void dzFile_Read (void *, uInt);
void dzFile_Write (void *, uInt);
uInt dzFile_Size (void);
void dzFile_Seek (uInt);
void dzFile_Truncate (void);
void dzWrite (void *, int);
void dzWriteDirectory (void);
void *Dzip_malloc (uInt);
void *Dzip_realloc (void *, uInt);
char *Dzip_strdup (const char *);
void end_zlib_compression (void);
void error (const char *, ...);
char *FileExtension (char *);
int get_filetype (char *);
char *GetFileFromPath (char *);
void Infile_Read (void *, uInt);
void Infile_Seek (uInt);
void Infile_Store (uInt);
void insert_msg (void *, uInt);
void make_crc (uchar *, int);
void normal_compress (uInt);
void Outfile_Write (void *, uInt);

#define pakid *(int *)"PACK"
#define discard_msg(x) inptr += x

#ifndef SFXVAR
#define SFXVAR extern
#endif

extern uchar dem_updateframe;
SFXVAR uchar copybaseline;
SFXVAR int maxent, lastent, sble;
extern int maj_ver, min_ver;	/* of the current dz file */
#define p_blocksize 32768
extern int numfiles;
extern uInt totalsize;
SFXVAR int entlink[MAX_ENT];
SFXVAR long dem_gametime;
SFXVAR long outlen;
SFXVAR unsigned long cam0, cam1, cam2;
SFXVAR uchar *inblk, *outblk, *inptr;
extern uchar *tmpblk;
SFXVAR cdata_t oldcd, newcd;
SFXVAR ent_t base[MAX_ENT], oldent[MAX_ENT], newent[MAX_ENT];
extern direntry_t *directory;

short getshort (uchar *);
int32_t getlong (uchar *);
float getfloat (uchar *);
#ifndef DZIP_BIG_ENDIAN
#define cnvlong(x) (x)
#else
#define cnvlong(x) getlong((uchar*)(&x))
#endif

#define Z_BUFFER_SIZE 16384
extern z_stream zs;
extern uchar *zbuf;
extern uInt ztotal;
extern int zlevel;

#ifdef GUI

#define printf
#define fprintf
#define fflush

#include "gui\gui_export.h"

#define AbortOp *ge.AbortOp
#define crcval *ge.crcval
#define dzFile_Read ge.ArchiveFile_Read
#define dzFile_Write ge.ArchiveFile_Write
#define dzFile_Size ge.ArchiveFile_Size
#define dzFile_Seek ge.ArchiveFile_Seek
#define dzFile_Truncate ge.ArchiveFile_Truncate
#define Dzip_malloc ge.malloc
#define Dzip_realloc ge.realloc
#define Dzip_strdup ge.strdup
#define GuiProgressMsg ge.GuiProgressMsg
#define Infile_Read ge.Infile_Read
#define Infile_Seek ge.Infile_Seek
#define Infile_Store ge.Infile_Store
#define Outfile_Write ge.Outfile_Write
#define error ge.error
#define deflate ge.deflate
#define deflateEnd ge.deflateEnd
#define deflateInit ge.deflateInit
#define inflate ge.inflate
#define inflateEnd ge.inflateEnd
#define inflateInit ge.inflateInit

#else

extern char AbortOp;
extern unsigned long crcval;

#endif

#ifdef WIN32
#define DIRCHAR '\\'
#define WRONGCHAR '/'
#define strcasecmp stricmp
#else
#define DIRCHAR '/'
#define WRONGCHAR '\\'
#endif