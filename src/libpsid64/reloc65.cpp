// SPDX-License-Identifier: GPL-2.0-or-later
/*
    xa65 - 6502 cross assembler and utility suite
    reloc65 - relocates 'o65' files
    Copyright (C) 1997 André Fachat (a.fachat@physik.tu-chemnitz.de)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
    Modified by Dag Lem <resid@nimrod.no>
    Relocate and extract text segment from memory buffer instead of file.
    For use with VICE VSID.
*/

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "reloc65.h"


#define BUF     (9*2+8)         /* 16 bit header */

struct file65 {
        char            *fname;
        size_t          fsize;
        unsigned char   *buf;
        int             tbase, tlen, dbase, dlen, bbase, blen, zbase, zlen;
        int             tdiff, ddiff, bdiff, zdiff;
        int             nundef;
        char            **ud;
        unsigned char   *segt;
        unsigned char   *segd;
        unsigned char   *utab;
        unsigned char   *rttab;
        unsigned char   *rdtab;
        unsigned char   *extab;
        globals_t       *globals;
};


int read_options(const unsigned char *f);
int read_undef(const unsigned char *f, file65 *fp);
unsigned char *reloc_seg(unsigned char *f, int len, unsigned char *rtab, file65 *fp);
unsigned char *reloc_globals(unsigned char *, file65 *fp);

file65 file;
unsigned char cmp[] = { 1, 0, 'o', '6', '5' };

int reloc65(char** buf, int* fsize, int addr, globals_t* globals)
{
        int mode, hlen;

        int tflag=0, dflag=0, bflag=0, zflag=0;
        int tbase=0, dbase=0, bbase=0, zbase=0;
        int extract = 0;

        file.globals = globals;

        file.buf = (unsigned char *) *buf;
        file.fsize = *fsize;
        tflag= 1;
        tbase = addr;
        extract = 1;

        if (memcmp(file.buf, cmp, 5) != 0) {
                return 0;
        }

        mode=file.buf[7]*256+file.buf[6];
        if(mode & 0x2000) {
                return 0;
        } else if(mode & 0x4000) {
                return 0;
        }

        hlen = BUF+read_options(file.buf+BUF);

        file.tbase = file.buf[ 9]*256+file.buf[ 8];
        file.tlen  = file.buf[11]*256+file.buf[10];
        file.tdiff = tflag? tbase - file.tbase : 0;
        file.dbase = file.buf[13]*256+file.buf[12];
        file.dlen  = file.buf[15]*256+file.buf[14];
        file.ddiff = dflag? dbase - file.dbase : 0;
        file.bbase = file.buf[17]*256+file.buf[16];
        file.blen  = file.buf[19]*256+file.buf[18];
        file.bdiff = bflag? bbase - file.bbase : 0;
        file.zbase = file.buf[21]*256+file.buf[20];
        file.zlen  = file.buf[23]*256+file.buf[21];
        file.zdiff = zflag? zbase - file.zbase : 0;

        file.segt  = file.buf + hlen;
        file.segd  = file.segt + file.tlen;

        file.utab  = file.segd + file.dlen;

        file.rttab = file.utab + read_undef(file.utab, &file);

        file.rdtab = reloc_seg(file.segt, file.tlen, file.rttab, &file);
        file.extab = reloc_seg(file.segd, file.dlen, file.rdtab, &file);

        reloc_globals(file.extab, &file);

        if(tflag) {
                file.buf[ 9]= (tbase>>8)&255;
                file.buf[ 8]= tbase & 255;
        }
        if(dflag) {
                file.buf[13]= (dbase>>8)&255;
                file.buf[12]= dbase & 255;
        }
        if(bflag) {
                file.buf[17]= (bbase>>8)&255;
                file.buf[16]= bbase & 255;
        }
        if(zflag) {
                file.buf[21]= (zbase>>8)&255;
                file.buf[20]= zbase & 255;
        }

        /* free array with names of undefined labels */
        free(file.ud);

        switch(extract) {
        case 0: /* whole file */
                return 1;
        case 1: /* text segment */
                *buf = (char *) file.segt;
                *fsize = file.tlen;
                return 1;
        case 2:
                *buf = (char *) file.segd;
                *fsize = file.dlen;
                return 1;
        default:
                return 0;
        }
}


int read_options(const unsigned char *buf) {
        int c, l=0;

        c=buf[0];
        while(c && c!=EOF) {
          c&=255;
          l+=c;
          c=buf[l];
        }
        return ++l;
}

int read_undef(const unsigned char *buf, file65 *fp) {
        int i, n, l = 2;

        n = buf[0] + 256*buf[1];

        fp->nundef = n;
        fp->ud = (char **) calloc(n, sizeof(char *));

/*printf("number of undefined labels = %d\n", fp->nundef);*/
        i=0;
        while(i<n){
          fp->ud[i] = (char*) buf+l;
/*printf("undefined label %d = '%s'\n", i, fp->ud[i]);*/
          while(buf[l++]);
          i++;
        }
        return l;
}

static int find_global(const unsigned char *bp, const file65 *fp) {
        const char *name;
        int nl = bp[0]+256*bp[1];

        name = fp->ud[nl];
        globals_t::iterator iter = fp->globals->find(name);
        if (iter != fp->globals->end())
        {
                return iter->second;
        }
        fprintf(stderr,"Warning: undefined label '%s'\n", name);
        return 0;
}

#define reldiff(s)      (((s)==2)?fp->tdiff:(((s)==3)?fp->ddiff:(((s)==4)?fp->bdiff:(((s)==5)?fp->zdiff:0))))

unsigned char *reloc_seg(unsigned char *buf, int len, unsigned char *rtab, file65 *fp) {
        int adr = -1;
        int type, seg, old, n_new;
/*printf("tdiff=%04x, ddiff=%04x, bdiff=%04x, zdiff=%04x\n",
                fp->tdiff, fp->ddiff, fp->bdiff, fp->zdiff);*/
        while(*rtab) {
          if((*rtab & 255) == 255) {
            adr += 254;
            rtab++;
          } else {
            adr += *rtab & 255;
            rtab++;
            type = *rtab & 0xe0;
            seg = *rtab & 0x07;
/*printf("reloc entry @ rtab=%p (offset=%d), adr=%04x, type=%02x, seg=%d\n",rtab-1, *(rtab-1), adr, type, seg);*/
            rtab++;
            switch(type) {
            case 0x80:
                // two byte address
                old = buf[adr] + 256*buf[adr+1];
                if (seg) n_new = old + reldiff(seg);
                else n_new = old + find_global(rtab, fp);
                buf[adr] = n_new & 255;
                buf[adr+1] = (n_new>>8)&255;
                break;
            case 0x40:
                // high byte of an address
                old = buf[adr]*256 + *rtab;
                if (seg) n_new = old + reldiff(seg);
                else n_new = old + find_global(rtab, fp);
                buf[adr] = (n_new>>8)&255;
// FIXME: I don't understand the line below. Why should we write data do the
// relocation table?
//              *rtab = n_new & 255;
                rtab++;
                break;
            case 0x20:
                // low byte of an address
                old = buf[adr];
                if (seg) n_new = old + reldiff(seg);
                else n_new = old + find_global(rtab, fp);
                buf[adr] = n_new & 255;
                break;
            }
            if(seg==0) rtab+=2;
          }
        }
        if(adr > len) {
/*
          fprintf(stderr,"reloc65: %s: Warning: relocation table entries past segment end!\n",
                fp->fname);
*/
        }
        return ++rtab;
}

unsigned char *reloc_globals(unsigned char *buf, file65 *fp) {
        int n, n_new;

        n = buf[0] + 256*buf[1];
        buf +=2;

        while(n) {
/*printf("relocating %s, ", buf);*/
          while(*(buf++));
          int seg = *buf;
          int old = buf[1] + 256*buf[2];

          if (seg) n_new = old + reldiff(seg);
          else n_new = old + find_global(buf+1, fp);
/*printf("old=%04x, seg=%d, rel=%04x, n_new=%04x\n", old, seg, reldiff(seg), n_new);*/
          buf[1] = n_new & 255;
          buf[2] = (n_new>>8) & 255;
          buf +=3;
          n--;
        }
        return buf;
}
