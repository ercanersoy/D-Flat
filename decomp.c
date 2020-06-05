/* ------------------- decomp.c -------------------- */

/*
 * Decompress the application.HLP file
 * or load the application.TXT file if the .HLP file
 * does not exist
 */

#include <assert.h>
#include "dflat.h"
#include "htree.h"

static int in8;
static int ct8 = 8;
static FILE *fi;
static BYTECOUNTER bytectr;
struct htr *HelpTree;
static int root;

/* ------- open the help database file -------- */
FILE *OpenHelpFile(const char *fn, const char *md)
{
    char *cp;
    int treect, i;
    char helpname[65];

    /* -------- get the name of the help file ---------- */
    BuildFileName(helpname, fn, ".hlp");
    if ((fi = fopen(helpname, md)) == NULL)
        return NULL;
	if (HelpTree == NULL)	{
    	/* ----- read the byte count ------ */
    	fread(&bytectr, sizeof bytectr, 1, fi);
    	/* ----- read the frequency count ------ */
    	fread(&treect, sizeof treect, 1, fi);
    	/* ----- read the root offset ------ */
    	fread(&root, sizeof root, 1, fi);
    	HelpTree = calloc(treect-256, sizeof(struct htr));
		if (HelpTree != NULL)	{
    		/* ---- read in the tree --- */
    		for (i = 0; i < treect-256; i++)    {
        		fread(&HelpTree[i].left,  sizeof(int), 1, fi);
        		fread(&HelpTree[i].right, sizeof(int), 1, fi);
    		}
		}
	}
    return fi;
}

/* ----- read a line of text from the help database ----- */
void *GetHelpLine(char *line)
{
    int h;
    *line = '\0';
    while (TRUE)    {
        /* ----- decompress a line from the file ------ */
        h = root;
        /* ----- walk the Huffman tree ----- */
        while (h > 255)    {
            /* --- h is a node pointer --- */
            if (ct8 == 8)   {
                /* --- read 8 bits of compressed data --- */
                if ((in8 = fgetc(fi)) == EOF)    {
                    *line = '\0';
                    return NULL;
                }
                ct8 = 0;
            }
            /* -- point to left or right node based on msb -- */
            if (in8 & 0x80)
                h = HelpTree[h-256].left;
            else
                h = HelpTree[h-256].right;
            /* --- shift the next bit in --- */
            in8 <<= 1;
            ct8++;
        }
        /* --- h < 255 = decompressed character --- */
        if (h == '\r')
            continue;    /* skip the '\r' character */
        /* --- put the character in the buffer --- */
        *line++ = h;
        /* --- if '\n', end of line --- */
        if (h == '\n')
            break;
    }
    *line = '\0';    /* null-terminate the line */
    return line;
}

/* --- compute the database file byte and bit position --- */
void HelpFilePosition(long *offset, int *bit)
{
    *offset = ftell(fi);
    if (ct8 < 8)
        --*offset;
    *bit = ct8;
}

/* -- position the database to the specified byte and bit -- */
void SeekHelpLine(long offset, int bit)
{
    int fs = fseek(fi, offset, 0);
	assert(fs == 0);
    ct8 = bit;
    if (ct8 < 8)    {
        in8 = fgetc(fi);
		in8 <<= bit;
    }
}
