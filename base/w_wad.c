
//**************************************************************************
//**
//** w_wad.c : Heretic 2 : Raven Software, Corp.
//**
//** $Revision$
//** $Date$
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "h2stdinc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "h2def.h"

// MACROS ------------------------------------------------------------------

/* PC Shareware : */
#define	PCDEMO_LUMPS	2856
#define	PCDEMO_WADSIZE	10644136
/* Mac Shareware: */
#define	MACDEMO_LUMPS	3500
#define	MACDEMO_WADSIZE	13596228
/* PC Retail, original: */
#define	RETAIL10_LUMPS	4249
#define	RETAIL10_SIZE	20128392
/* PC Retail, patched : */
#define	RETAIL11_LUMPS	4270
#define	RETAIL11_SIZE	20083672
/* DeathKings, original: */
#define	DKINGS10_LUMPS	325
#define	DKINGS10_SIZE	4429700
/* DeathKings, patched : */
#define	DKINGS11_LUMPS	326
#define	DKINGS11_SIZE	4440584

// TYPES -------------------------------------------------------------------

typedef struct
{
	char identification[4];
	int numlumps;
	int infotableofs;
} wadinfo_t;

typedef struct
{
	int filepos;
	int size;
	char name[8];
} filelump_t;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void W_MergeLumps(const char *start, const char *end);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

lumpinfo_t *lumpinfo;
int numlumps;
void **lumpcache;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static lumpinfo_t *PrimaryLumpInfo;
static int PrimaryNumLumps;
static void **PrimaryLumpCache;
static lumpinfo_t *AuxiliaryLumpInfo;
static int AuxiliaryNumLumps;
static void **AuxiliaryLumpCache;
static int AuxiliaryHandle = 0;
boolean AuxiliaryOpened = false;

// CODE --------------------------------------------------------------------

boolean W_IsWadPresent(const char *filename)
{
	char path[MAX_OSPATH], *waddir;
	int handle = -1;

	/* try the directory specified by the
	 * shared data environment variable first.
	 */
	waddir = getenv(DATA_ENVVAR);
	if (waddir && *waddir)
	{
		snprintf (path, sizeof(path), "%s/%s", waddir, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Try UserDIR */
	{
		snprintf (path, sizeof(path), "%s%s", basePath, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Now try CWD */
	{
		handle = open(filename, O_RDONLY|O_BINARY);
	}
	if (handle == -1)
		return false;	/* Didn't find the file. */
	close(handle);
	return true;
}

//==========================================================================
//
// W_AddFile
//
// Files with a .wad extension are wadlink files with multiple lumps,
// other files are single lumps with the base filename for the lump name.
//
//==========================================================================

void W_AddFile(const char *filename)
{
	wadinfo_t header;
	lumpinfo_t *lump_p;
	char path[MAX_OSPATH], *waddir;
	int handle, length, flength;
	int startlump;
	filelump_t *fileinfo, singleinfo;
	filelump_t *freeFileInfo;
	int	i, j;
	byte	*c;

	handle = -1;
	/* try the directory specified by the
	 * shared data environment variable first.
	 */
	waddir = getenv(DATA_ENVVAR);
	if (waddir && *waddir)
	{
		snprintf (path, sizeof(path), "%s/%s", waddir, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Try UserDIR */
	{
		snprintf (path, sizeof(path), "%s%s", basePath, filename);
		handle = open(path, O_RDONLY|O_BINARY);
	}
	if (handle == -1)	/* Now try CWD */
	{
		handle = open(filename, O_RDONLY|O_BINARY);
	}
	if (handle == -1)
		return;		/* Didn't find the file. */

	flength = filelength(handle);
	startlump = numlumps;
	if (strcasecmp(filename + strlen(filename) - 3, "wad") != 0)
	{ // Single lump file
		fileinfo = &singleinfo;
		freeFileInfo = NULL;
		singleinfo.filepos = 0;
		singleinfo.size = LONG(flength);
		M_ExtractFileBase(filename, singleinfo.name);
		numlumps++;
	}
	else
	{ // WAD file
		read(handle, &header, sizeof(header));
		if (strncmp(header.identification, "IWAD", 4) != 0)
		{
			if (strncmp(header.identification, "PWAD", 4) != 0)
			{ // Bad file id
				I_Error("Wad file %s doesn't have IWAD or PWAD id\n", filename);
			}
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps * sizeof(filelump_t);
		if (strncmp(header.identification, "IWAD", 4) == 0 &&
		    header.numlumps == PCDEMO_LUMPS && flength == PCDEMO_WADSIZE)
		{
			shareware = true;
			ST_Message("Shareware WAD detected (4 level 1.0 PC version).\n");
		}
		else if (strncmp(header.identification, "IWAD", 4) == 0 &&
			 header.numlumps == MACDEMO_LUMPS && flength == MACDEMO_WADSIZE)
		{
			shareware = true;
			ST_Message("Shareware WAD detected (4 level 1.1 Mac version).\n");
		}
		fileinfo = (filelump_t *) malloc(length);
		if (!fileinfo)
		{
			I_Error("W_AddFile: fileinfo malloc failed\n");
		}
		freeFileInfo = fileinfo;
		lseek(handle, header.infotableofs, SEEK_SET);
		read(handle, fileinfo, length);
		numlumps += header.numlumps;
	}

	// Fill in lumpinfo
	lumpinfo = (lumpinfo_t *) realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));
	if (!lumpinfo)
	{
		I_Error("Couldn't realloc lumpinfo");
	}
	lump_p = &lumpinfo[startlump];
	for (i = startlump; i < numlumps; i++, lump_p++, fileinfo++)
	{
		memset(lump_p->name, 0, 8);
		lump_p->handle = handle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		strncpy(lump_p->name, fileinfo->name, 8);
		/* In the Mac demo wad, many (1784) of the lump names
		 * have their first character with the high bit (0x80)
		 * set.  I don't know the reason for that..  We must
		 * clear the high bits for such Mac wad files to work
		 * in this engine. This shouldn't break other wads. */
		c = (byte *)lump_p->name;
		for (j = 0; j < 8; c++, j++)
			*c &= 0x7f;
	}
	if (freeFileInfo)
	{
		free(freeFileInfo);
	}
}

//==========================================================================
//
// W_InitMultipleFiles
//
// Pass a null terminated list of files to use.  All files are optional,
// but at least one file must be found.  Lump names can appear multiple
// times.  The name searcher looks backwards, so a later file can
// override an earlier one.
//
//==========================================================================

void W_InitMultipleFiles(const char **filenames)
{
	int size;

	// Open all the files, load headers, and count lumps
	numlumps = 0;
	lumpinfo = (lumpinfo_t *) malloc(1); // Will be realloced as lumps are added

	for ( ; *filenames; filenames++)
	{
		W_AddFile(*filenames);
	}
	if (!numlumps)
	{
		I_Error("W_InitMultipleFiles: no files found");
	}

	// Merge lumps for flats and sprites
	W_MergeLumps("S_START","S_END");
	W_MergeLumps("F_START","F_END");

	// Set up caching
	size = numlumps * sizeof(*lumpcache);
	lumpcache = (void **) malloc(size);
	if (!lumpcache)
	{
		I_Error("Couldn't allocate lumpcache");
	}
	memset(lumpcache, 0, size);

	PrimaryLumpInfo = lumpinfo;
	PrimaryLumpCache = lumpcache;
	PrimaryNumLumps = numlumps;
}

//==========================================================================
//
// IsMarker
//
// From BOOM/xdoom.  Finds an S_START or SS_START marker
//
//==========================================================================

static int IsMarker(const char *marker, const char *name)
{
	return !strncasecmp(name, marker, 8) ||
		(*name == *marker && !strncasecmp(name + 1, marker, 7));
}

//==========================================================================
//
// W_MergeLumps
//
// From xdoom/BOOM again.  Merges all sprite lumps into one big 'ol block
//
//==========================================================================

static void W_MergeLumps(const char *start, const char *end)
{
	lumpinfo_t *newlumpinfo;
	int newlumps, oldlumps;
	int in_block = 0;
	int i;

	oldlumps = newlumps = 0;
	newlumpinfo = (lumpinfo_t *) malloc(numlumps * sizeof(lumpinfo_t));
	if (!newlumpinfo)
	{
		I_Error("W_MergeLumps: newlumpinfo malloc failed");
	}

	for (i = 0; i < numlumps; i++)
	{
		//process lumps in global namespace
		if (!in_block)
		{
			//check for start of block
			if (IsMarker(start, lumpinfo[i].name))
			{
				in_block = 1;
				if (!newlumps)
				{
					newlumps++;
					memset(newlumpinfo[0].name, 0, 8);
					strcpy(newlumpinfo[0].name, start);
					newlumpinfo[0].handle = -1;
					newlumpinfo[0].position = newlumpinfo[0].size = 0;
				}
			}
			// else copy it
			else
			{
				lumpinfo[oldlumps++] = lumpinfo[i];
			}
		}
		// process lumps in sprites or flats namespace
		else
		{
			// check for end of block
			if (IsMarker(end, lumpinfo[i].name))
			{
				in_block = 0;
			}
			else if (i && lumpinfo[i].handle != lumpinfo[i-1].handle)
			{
				in_block = 0;
				lumpinfo[oldlumps++] = lumpinfo[i];
			}
			else
			{
				newlumpinfo[newlumps++] = lumpinfo[i];
			}
		}
	}

	// now copy the merged lumps to the end of the old list
	if (newlumps)
	{
		if (oldlumps + newlumps > numlumps)
			lumpinfo = (lumpinfo_t *) realloc(lumpinfo, (oldlumps + newlumps) * sizeof(lumpinfo_t));
		memcpy(lumpinfo + oldlumps, newlumpinfo, sizeof(lumpinfo_t) * newlumps);

		numlumps = oldlumps + newlumps;

		memset(lumpinfo[numlumps].name, 0, 8);
		strcpy(lumpinfo[numlumps].name, end);
		lumpinfo[numlumps].handle = -1;
		lumpinfo[numlumps].position = lumpinfo[numlumps].size = 0;
		numlumps++;
	}
	free (newlumpinfo);
}

//==========================================================================
//
// W_InitFile
//
// Initialize the primary from a single file.
//
//==========================================================================

void W_InitFile(const char *filename)
{
	const char *names[2];

	names[0] = filename;
	names[1] = NULL;
	W_InitMultipleFiles(names);
}

//==========================================================================
//
// W_OpenAuxiliary
//
//==========================================================================

void W_OpenAuxiliary(const char *filename)
{
	int i;
	int size;
	wadinfo_t header;
	int handle;
	int length;
	filelump_t *fileinfo;
	filelump_t *sourceLump;
	lumpinfo_t *destLump;

	if (AuxiliaryOpened)
	{
		W_CloseAuxiliary();
	}
	if ((handle = open(filename, O_RDONLY|O_BINARY)) == -1)
	{
		I_Error("W_OpenAuxiliary: %s not found.", filename);
		return;
	}
	AuxiliaryHandle = handle;
	read(handle, &header, sizeof(header));
	if (strncmp(header.identification, "IWAD", 4))
	{
		if (strncmp(header.identification, "PWAD", 4))
		{ // Bad file id
			I_Error("Wad file %s doesn't have IWAD or PWAD id\n",
				filename);
		}
	}
	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);
	length = header.numlumps*sizeof(filelump_t);
	fileinfo = (filelump_t *) Z_Malloc(length, PU_STATIC, NULL);
	lseek(handle, header.infotableofs, SEEK_SET);
	read(handle, fileinfo, length);
	numlumps = header.numlumps;

	// Init the auxiliary lumpinfo array
	lumpinfo = (lumpinfo_t *) Z_Malloc(numlumps*sizeof(lumpinfo_t), PU_STATIC, NULL);
	sourceLump = fileinfo;
	destLump = lumpinfo;
	for (i = 0; i < numlumps; i++, destLump++, sourceLump++)
	{
		destLump->handle = handle;
		destLump->position = LONG(sourceLump->filepos);
		destLump->size = LONG(sourceLump->size);
		strncpy(destLump->name, sourceLump->name, 8);
	}
	Z_Free(fileinfo);

	// Allocate the auxiliary lumpcache array
	size = numlumps*sizeof(*lumpcache);
	lumpcache = (void **) Z_Malloc(size, PU_STATIC, NULL);
	memset(lumpcache, 0, size);

	AuxiliaryLumpInfo = lumpinfo;
	AuxiliaryLumpCache = lumpcache;
	AuxiliaryNumLumps = numlumps;
	AuxiliaryOpened = true;
}

//==========================================================================
//
// W_CloseAuxiliary
//
//==========================================================================

void W_CloseAuxiliary(void)
{
	int i;

	if (AuxiliaryOpened)
	{
		W_UseAuxiliary();
		for (i = 0; i < numlumps; i++)
		{
			if (lumpcache[i])
			{
				Z_Free(lumpcache[i]);
			}
		}
		Z_Free(AuxiliaryLumpInfo);
		Z_Free(AuxiliaryLumpCache);
		W_CloseAuxiliaryFile();
		AuxiliaryOpened = false;
	}
	W_UsePrimary();
}

//==========================================================================
//
// W_CloseAuxiliaryFile
//
// WARNING: W_CloseAuxiliary() must be called before any further
// auxiliary lump processing.
//
//==========================================================================

void W_CloseAuxiliaryFile(void)
{
	if (AuxiliaryHandle)
	{
		close(AuxiliaryHandle);
		AuxiliaryHandle = 0;
	}
}

//==========================================================================
//
// W_UsePrimary
//
//==========================================================================

void W_UsePrimary(void)
{
	lumpinfo = PrimaryLumpInfo;
	numlumps = PrimaryNumLumps;
	lumpcache = PrimaryLumpCache;
}

//==========================================================================
//
// W_UseAuxiliary
//
//==========================================================================

void W_UseAuxiliary(void)
{
	if (AuxiliaryOpened == false)
	{
		I_Error("W_UseAuxiliary: WAD not opened.");
	}
	lumpinfo = AuxiliaryLumpInfo;
	numlumps = AuxiliaryNumLumps;
	lumpcache = AuxiliaryLumpCache;
}

//==========================================================================
//
// W_NumLumps
//
//==========================================================================

int	W_NumLumps(void)
{
	return numlumps;
}

//==========================================================================
//
// W_CheckNumForName
//
// Returns -1 if name not found.
//
//==========================================================================

int W_CheckNumForName(const char *name)
{
	char name8[9];
	lumpinfo_t *lump_p;

	// Make the name into two integers for easy compares
	memset(name8, 0, sizeof(name8));
	strncpy(name8, name, 8);
	strupr(name8); // case insensitive

	// Scan backwards so patch lump files take precedence
	lump_p = lumpinfo + numlumps;
	while (lump_p-- != lumpinfo)
	{
		if (memcmp(lump_p->name, name8, 8) == 0)
		{
			return (int)(lump_p - lumpinfo);
		}
	}
	return -1;
}

//==========================================================================
//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
//==========================================================================

int	W_GetNumForName (const char *name)
{
	int	i;

	i = W_CheckNumForName(name);
	if (i != -1)
	{
		return i;
	}
	I_Error("W_GetNumForName: %s not found!", name);
	return -1;
}

//==========================================================================
//
// W_LumpLength
//
// Returns the buffer size needed to load the given lump.
//
//==========================================================================

int W_LumpLength(int lump)
{
	if (lump >= numlumps)
	{
		I_Error("W_LumpLength: %i >= numlumps", lump);
	}
	return lumpinfo[lump].size;
}

//==========================================================================
//
// W_ReadLump
//
// Loads the lump into the given buffer, which must be >= W_LumpLength().
//
//==========================================================================

void W_ReadLump(int lump, void *dest)
{
	int c;
	lumpinfo_t *l;

	if (lump >= numlumps)
	{
		I_Error("W_ReadLump: %i >= numlumps", lump);
	}
	l = lumpinfo+lump;
	//I_BeginRead();
	lseek(l->handle, l->position, SEEK_SET);
	c = read(l->handle, dest, l->size);
	if (c < l->size)
	{
		I_Error("W_ReadLump: only read %i of %i on lump %i",
			c, l->size, lump);
	}
	//I_EndRead();
}

//==========================================================================
//
// W_CacheLumpNum
//
//==========================================================================

void *W_CacheLumpNum(int lump, int tag)
{
	byte *ptr;

	if ((unsigned)lump >= numlumps)
	{
		I_Error("W_CacheLumpNum: %i >= numlumps", lump);
	}
	if (!lumpcache[lump])
	{ // Need to read the lump in
		ptr = (byte *) Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);
		W_ReadLump(lump, lumpcache[lump]);
	}
	else
	{
		Z_ChangeTag(lumpcache[lump], tag);
	}
	return lumpcache[lump];
}

//==========================================================================
//
// W_CacheLumpName
//
//==========================================================================

void *W_CacheLumpName(const char *name, int tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

void W_CheckWADFiles (void)
{
	char	lumpmsg[10];
	int		i;

	for (i = 1; i <= 4 && oldwad_10 != true; i++)
	{
		sprintf (lumpmsg, "CLUS%iMSG", i);
		if (W_CheckNumForName(lumpmsg) == -1)
			oldwad_10 = true;
	}
	for (i = 1; i <= 3 && oldwad_10 != true; i++)
	{
		sprintf (lumpmsg, "WIN%iMSG", i);
		if (W_CheckNumForName(lumpmsg) == -1)
			oldwad_10 = true;
	}

#if 0
	if (shareware)
	{
		ST_Message ("\n========================================================================\n");
		ST_Message ("                      Hexen:  Beyond Heretic\n\n");
		ST_Message ("                       4 Level Demo Version\n");
		ST_Message ("                    Press any key to continue.\n");
		ST_Message ("========================================================================\n\n");
		getchar();
	}
#endif

#if defined(DEMO_VERSION)
	if (shareware != true)
		I_Error ("\nShareware WAD not detected.\nThis exe is configured only for the DEMO version of Hexen!\n");
#endif	/* Shareware-only */

#if defined(VERSION10_WAD)
	if (oldwad_10 != true)
		I_Error ("\nThis exe is configured only for Hexen 1.0 (4-player-only) wad files!\n");
	ST_Message ("Running with 4-player-only Version 1.0 hexen.wad files.\n");
#else
	if (oldwad_10 == true)
	{
		ST_Message ("\nIt appears that you are using a 4-player-only Version 1.0 hexen.wad.\n");
		ST_Message ("Running HHexen without a Version 1.1 wadfile can cause many problems.\n");
		ST_Message ("\nPress <ENTER> to continue.\n");
		getchar();
	}
#endif
}

//==========================================================================
//
// W_Profile
//
//==========================================================================

// Ripped out for Heretic
/*
static int	info[2500][10];
static int	profilecount;

void W_Profile (void)
{
	int		i;
	memblock_t	*block;
	void	*ptr;
	char	ch;
	FILE	*f;
	int		j;
	char	name[9];

	for (i = 0; i < numlumps; i++)
	{
		ptr = lumpcache[i];
		if (!ptr)
		{
			ch = ' ';
			continue;
		}
		else
		{
			block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
			if (block->tag < PU_PURGELEVEL)
				ch = 'S';
			else
				ch = 'P';
		}
		info[i][profilecount] = ch;
	}
	profilecount++;

	f = fopen ("waddump.txt","w");
	name[8] = 0;
	for (i = 0; i < numlumps; i++)
	{
		memcpy (name, lumpinfo[i].name, 8);
		for (j = 0; j < 8; j++)
			if (!name[j])
				break;
		for ( ; j < 8; j++)
			name[j] = ' ';
		fprintf (f,"%s ",name);
		for (j = 0; j < profilecount; j++)
			fprintf (f,"    %c",info[i][j]);
		fprintf (f,"\n");
	}
	fclose (f);
}
*/

