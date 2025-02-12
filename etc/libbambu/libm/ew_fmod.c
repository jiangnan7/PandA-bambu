/**
 * Porting of the libm library to the PandA framework
 * starting from the original FDLIBM 5.3 (Freely Distributable LIBM) developed by SUN
 * plus the newlib version 1.19 from RedHat and plus uClibc version 0.9.32.1 developed by Erik Andersen.
 * The author of this port is Fabrizio Ferrandi from Politecnico di Milano.
 * The porting fall under the LGPL v2.1, see the files COPYING.LIB and COPYING.LIBM_PANDA in this directory.
 * Date: September, 11 2013.
 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include "math_private.h"

/*
 * __ieee754_fmod(x,y)
 * Return x mod y in exact arithmetic
 * Method: shift and subtract
 */

double __hide_ieee754_fmod(double x, double y)
{
   int n, hx, hy, hz, ix, iy, sx, i;
   unsigned lx, ly, lz;

   hx = GET_HI(x);       /* high word of x */
   lx = GET_LO(x);       /* low  word of x */
   hy = GET_HI(y);       /* high word of y */
   ly = GET_LO(y);       /* low  word of y */
   sx = hx & 0x80000000; /* sign of x */
   hx ^= sx;             /* |x| */
   hy &= 0x7fffffff;     /* |y| */

   /* purge off exception values */
   if((hy | ly) == 0 || (hx >= 0x7ff00000) ||   /* y=0,or x not finite */
      ((hy | ((ly | -ly) >> 31)) > 0x7ff00000)) /* or y is NaN */
      return __builtin_nan("");
   if(hx <= hy)
   {
      if((hx < hy) || (lx < ly))
         return x; /* |x|<|y| return x */
      if(lx == ly)
         return Zero[(unsigned)sx >> 31]; /* |x|=|y| return x*0*/
   }

   /* determine ix = ilogb(x) */
   if(hx < 0x00100000)
   { /* subnormal x */
      if(hx == 0)
      {
         for(ix = -1043, i = lx; i > 0; i <<= 1)
            ix -= 1;
      }
      else
      {
         for(ix = -1022, i = (hx << 11); i > 0; i <<= 1)
            ix -= 1;
      }
   }
   else
      ix = (hx >> 20) - 1023;

   /* determine iy = ilogb(y) */
   if(hy < 0x00100000)
   { /* subnormal y */
      if(hy == 0)
      {
         for(iy = -1043, i = ly; i > 0; i <<= 1)
            iy -= 1;
      }
      else
      {
         for(iy = -1022, i = (hy << 11); i > 0; i <<= 1)
            iy -= 1;
      }
   }
   else
      iy = (hy >> 20) - 1023;

   /* set up {hx,lx}, {hy,ly} and align y to x */
   if(ix >= -1022)
      hx = 0x00100000 | (0x000fffff & hx);
   else
   { /* subnormal x, shift x to normal */
      n = -1022 - ix;
      if(n <= 31)
      {
         hx = (hx << n) | (lx >> (32 - n));
         lx <<= n;
      }
      else
      {
         hx = lx << (n - 32);
         lx = 0;
      }
   }
   if(iy >= -1022)
      hy = 0x00100000 | (0x000fffff & hy);
   else
   { /* subnormal y, shift y to normal */
      n = -1022 - iy;
      if(n <= 31)
      {
         hy = (hy << n) | (ly >> (32 - n));
         ly <<= n;
      }
      else
      {
         hy = ly << (n - 32);
         ly = 0;
      }
   }

   /* fix point fmod */
   n = ix - iy;
   while(n--)
   {
      hz = hx - hy;
      lz = lx - ly;
      if(lx < ly)
         hz -= 1;
      if(hz < 0)
      {
         hx = hx + hx + (lx >> 31);
         lx = lx + lx;
      }
      else
      {
         if((hz | lz) == 0) /* return sign(x)*0 */
            return Zero[(unsigned)sx >> 31];
         hx = hz + hz + (lz >> 31);
         lx = lz + lz;
      }
   }
   hz = hx - hy;
   lz = lx - ly;
   if(lx < ly)
      hz -= 1;
   if(hz >= 0)
   {
      hx = hz;
      lx = lz;
   }

   /* convert back to floating value and restore the sign */
   if((hx | lx) == 0) /* return sign(x)*0 */
      return Zero[(unsigned)sx >> 31];
   while(hx < 0x00100000)
   { /* normalize x */
      hx = hx + hx + (lx >> 31);
      lx = lx + lx;
      iy -= 1;
   }
   if(iy >= -1022)
   { /* normalize output */
      hx = ((hx - 0x00100000) | ((iy + 1023) << 20));
      SET_HIGH_WORD(x, hx | sx);
      SET_LOW_WORD(x, lx);
   }
   else
   { /* subnormal output */
      n = -1022 - iy;
      if(n <= 20)
      {
         lx = (lx >> n) | ((unsigned)hx << (32 - n));
         hx >>= n;
      }
      else if(n <= 31)
      {
         lx = (hx << (32 - n)) | (lx >> n);
         hx = sx;
      }
      else
      {
         lx = hx >> (n - 32);
         hx = sx;
      }
      SET_HIGH_WORD(x, hx | sx);
      SET_LOW_WORD(x, lx);
      x *= one; /* create necessary signal */
   }
   return x; /* exact output */
}

/*
 * wrapper fmod(x,y)
 */
double fmod(double x, double y) /* wrapper fmod */
{
#ifdef _IEEE_LIBM
   return __hide_ieee754_fmod(x, y);
#else
   double z;
   z = __hide_ieee754_fmod(x, y);
   if(_LIB_VERSION == _IEEE_ || isnan(y) || isnan(x))
      return z;
   if(y == 0.0)
   {
      return __hide_kernel_standard(x, y, 27); /* fmod(x,0) */
   }
   else
      return z;
#endif
}
