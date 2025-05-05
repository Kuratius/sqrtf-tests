//Copyright (C) 2025 Dominik Kurz

//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 3 of the License, or (at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.

//You should have received a copy of the GNU Lesser General Public License
//along with this program; if not, write to the Free Software Foundation,
//Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#define QUIET_NAN ((255 << 23) | ((1 << 22) | 1))
#define INF (0xFF << 23)
#define NEGATIVE_INF (1u<<31)|(0xFF << 23)

# define likely(x) __builtin_expect (!!(x), 1)
# define unlikely(x) __builtin_expect ((x), 0)

#define LUCKY 0

extern float sqrt_core(float x);



static uint32_t mantissa_rsqrt(uint32_t x)
{
    //printf("Fixed point Input: %f \n", (float)x/(1<<23));
    union
    {
        float f;
        uint32_t i;
    } xu;
    xu.i=x;
    if (x<(1<<24))
    {
        xu.i &= ((1<<23) -1 );
        xu.i |=(127<<23);
    } else 
    {
        xu.i>>=1;
        xu.i &= ((1<<23) -1 );        
        xu.i |=(128<<23);
    }
    //printf("Input float: %f \n", xu.f);
    double correct_result=1.0d/sqrt((double)xu.f);
    double fres=sqrt_core(xu.f);
    double nr=0.5d*fres*(3.0d-xu.f*fres*fres);
    nr=0.5d*nr*(3.0d-xu.f*nr*nr);
    union
    {
        double d;
        uint64_t l;
    } xd;
    xd.d=nr;
    uint32_t retval=((xd.l<<11) | (1ull<<63) )>>39;
    uint32_t Y=x;
    uint32_t U=retval;
    //we could return (U+1)>>1 and be 90 % accurate 
    //but we want infinitely precise rounding
#if LUCKY

    if (x==((9039928)*2  )) U |=1;
    return (U+1)>>1;
#else
    U |=1;
    uint32_t A_hi= ((uint64_t) U*U)>>32;
    uint32_t A_lo= U*U;
    uint32_t B_hi = ((uint64_t) Y*A_lo)>>32;
    uint64_t C= (uint64_t) Y*A_hi;
    uint64_t S=B_hi+C;
    uint32_t P=S>>32;
    uint32_t Q=1u<<(9);
    if (P >= Q)
    { 
       return  U>>1;
    } else 
    {
       return (U+1)>>1;
    }
#endif
}

float custom_rsqrtf(float x)
{
    union
    {
        float f;
        uint32_t i;
    } xu;
    xu.f = x;
    int32_t exp = (int32_t)xu.i >> 23; //must compile to arithmetic shift
                                       //otherwise undef. behavior
    if(likely(exp > 0 ))
    {
        if(unlikely(exp == 255))
        {
            // 1 / sqrt(+Inf) = +0
            // 1 / sqrt(NaN) = qNaN
            xu.i = (xu.i == INF) ? 0 : QUIET_NAN;
            return xu.f;
        } else //normal case 
        {
            uint32_t mantissa = xu.i &~ ((uint32_t)exp << 23);
            mantissa += 1u << 23;
            mantissa<<= !(exp & 1) ; 
            uint32_t newMantissa=mantissa_rsqrt(mantissa);
            int32_t newExp = -((exp - 127) >> 1) + 127 -2;
            xu.i = newMantissa + ((uint32_t)newExp << 23);
            return xu.f;
        }
    } else //exp<=0
    {
        if(likely(exp == 0 ))
        {
            if (likely (xu.i!=0 )) //subnormal
            {
                //printf("subnormal branch!\n");
                uint32_t mantissa = xu.i;
                int32_t shift=__builtin_clz(mantissa)- (31-23);
                mantissa<<=shift; // normalize subnormal
                shift=- 126-shift;
                mantissa <<= shift & 1 ;
                int32_t newExp = (-((shift)>>1)) +127-2;
                uint32_t new_mantissa=mantissa_rsqrt(mantissa); 
                xu.i = new_mantissa + ((uint32_t)newExp << 23);
                return xu.f;
            } else //rsqrt(+0)=+INF
            {
                xu.i = INF; 
                return xu.f;
            }
        } else if (xu.i == (1u<<31))
        {
            xu.i= NEGATIVE_INF; //rsqrt(-0)=-INF
            return xu.f;             
        } else {
            xu.i=QUIET_NAN;  //rsqrt(negative)=qNaN
            return xu.f;        
        }
    }
}
