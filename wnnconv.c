/*
 * Copyright Kyoto University Research Institute for Mathematical Sciences
 *                 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright OMRON Corporation. 1987, 1988, 1989, 1990, 1991, 1992, 1999
 * Copyright ASTEC, Inc. 1987, 1988, 1989, 1990, 1991, 1992
 * Copyright FreeWnn Project 1999, 2000, 2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <wnn/jllib.h>

#define G0      0
#define G1      1
#define G2      2
#define G3      3
#define SS      4
#define GL      1
#define GR      2
#define LS0     0x0f
#define LS1     0x0e
#define LS1R    0x7e
#define LS2     0x6e
#define LS2R    0x7d
#define LS3     0x6f
#define LS3R    0x7c
#define SS2     0x8e
#define SS3     0x8f


#define CS_MASK         0x8080
#define CS0_MASK        0x0000
#define CS1_MASK        0x8080
#define CS2_MASK        0x0080
#define CS3_MASK        0x8000

#define CS1     0
#define CS2     1
#define CS3     2


static int _etc_cs[3] = { 2, 1, 2 };
static int _etc_cs_len[3] = { 2, 1, 2 };
static int cs_mask[3] = { 0x8080, 0x0080, 0x8000 };


int
ieuc_to_eeuc (eeuc, ieuc, iesiz)
     unsigned char *eeuc;
     w_char *ieuc;
     int iesiz;
{
  register int x;
  register w_char *ie;
  register unsigned char *ee;
  register int cs_id, mask, non_limit = 0;
  ie = ieuc;
  ee = eeuc;

  if (iesiz == -1)
    non_limit = 1;
  for (; (non_limit ? (*ie) : (iesiz > 0)); iesiz -= sizeof (w_char))
    {
      x = *ie++;
      mask = x & CS_MASK;
      if (mask == CS0_MASK || x == 0xffff)
        {
          *ee++ = x;
        }
      else
        {
          cs_id = (mask == cs_mask[CS3]) ? CS3 : ((mask == cs_mask[CS2]) ? CS2 : CS1);
          if (_etc_cs[cs_id] <= 0)
            continue;
          if (cs_id == CS2)
            *ee++ = SS2;
          else if (cs_id == CS3)
            *ee++ = SS3;
          if (_etc_cs[cs_id] > 1)
            *ee++ = (x >> 8) | 0x80;
          if (_etc_cs[cs_id] > 0)
            *ee++ = (x & 0xff) | 0x80;
        }
    }
  return ((char *) ee - (char *) eeuc);
}


int
eeuc_to_ieuc (ieuc, eeuc, eesiz)
     w_char *ieuc;
     unsigned char *eeuc;
     register int eesiz;
{
  register unsigned char x;
  register w_char *ie;
  register unsigned char *ee;
  register int cs_id, non_limit = 0;
  ie = ieuc;
  ee = eeuc;

  if (eesiz == -1)
    non_limit = 1;
  for (; (non_limit ? (*ee) : (eesiz > 0));)
    {
      x = *ee++;
      if (x > 0x9f || x == SS2 || x == SS3)
        {
          cs_id = ((x == SS2) ? CS2 : ((x == SS3) ? CS3 : CS1));
          if (cs_id == CS2 || cs_id == CS3)
            x = *ee++;
          if (_etc_cs[cs_id] <= 0)
            continue;
          if (_etc_cs[cs_id] > 1)
            {
              *ie = (w_char) (x & 0x7f) << 8;
              x = *ee++;
            }
          else
            {
              *ie = (w_char) 0;
            }
          *ie |= (x & 0x7f);
          *ie++ |= cs_mask[cs_id];
          eesiz -= _etc_cs[cs_id] + 1;
        }
      else
        {
          *ie++ = x;
          eesiz--;
        }
    }
  return ((char *) ie - (char *) ieuc);
}


int
wnn_sStrcpy (c, w)
     register char *c;
     register w_char *w;
{
  register int ret;

  ret = ieuc_to_eeuc (c, w, -1);
  c[ret] = '\0';
  return (ret);
}


int
wnn_Sstrcpy (w, c)
     w_char *w;
     unsigned char *c;
{
  register int ret;

  ret = eeuc_to_ieuc (w, c, -1) / sizeof (w_char);
  w[ret] = (w_char) 0;
  return (ret);
}


