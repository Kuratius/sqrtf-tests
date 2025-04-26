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
#define QUIET_NAN ((255 << 23) | ((1 << 22) | 1))
#define INF (0xFF << 23)
#define NEGATIVE_INF (1u<<31)|(0xFF << 23)

# define likely(x) __builtin_expect (!!(x), 1)
# define unlikely(x) __builtin_expect ((x), 0)

static uint32_t mantissa_sqrt_asm(uint32_t x);
static uint32_t mantissa_rsqrt_asm(uint32_t x);
static uint32_t sqrt_core_asm(uint32_t x, uint32_t y);

//#define __ARM_ARCH_5TE__
#if  !defined( __ARM_ARCH_5TE__) || defined(__OPTIMIZE_SIZE__)
static uint32_t sqrt_core(uint32_t x, uint32_t y)
{
    x<<=6;
    uint32_t t=x+(x>>1);
    if (t < (1u<<31)){ //first iteration is special cased
        t+=t>>1;
        if (t < (1u<<31)) {
            x=t;
            y+=y>>1;
        }
    }
    #ifndef __OPTIMIZE_SIZE__
    #pragma GCC unroll 12 
    #endif
    for (uint32_t i =2; i<14; i+=1){
        uint32_t t=x+(x>>i);
        t+=t>>i;
        if (t < (1u<<31)) {
            x=t;
            y+=y>>i;
        }
    }
    uint32_t hi = ((uint64_t)(x)*(y)  )>>32;
    y+=y>>1; //this can overflow
    return ((y)-(hi) ); //but then this underflows so they cancel
}

static uint32_t mantissa_sqrt(uint32_t x)
{
    uint32_t new_mantissa=sqrt_core(x,x<<7 )>>8;
    //we could stop here but we want infinitely precise rounding
    int64_t msquared=(int64_t)new_mantissa*new_mantissa;
    int64_t x0=x;
    x0<<=23;
    int64_t residual=x0-msquared;
    if (residual>new_mantissa)
    {
            new_mantissa+=1;
    }  
    return new_mantissa; 
}

static uint32_t mantissa_rsqrt(uint32_t x)
{
    uint32_t Y=x;
    uint32_t U=sqrt_core(x,1u<<30 )>>6;
    //we could return (U+1)>>1 and be 90 % accurate 
    //but we want infinitely precise rounding
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
}
#else
static inline uint32_t mantissa_sqrt(uint32_t x)
{
    return mantissa_sqrt_asm(x);
}

static inline uint32_t mantissa_rsqrt(uint32_t x)
{
    return mantissa_rsqrt_asm(x);
}

static inline uint32_t sqrt_core(uint32_t x, uint32_t y)
{
    return sqrt_core_asm(x,y);
}

#endif

float custom_sqrtf(float x)
{
    union
    {
        float f;
        uint32_t i;
    } xu;
    xu.f = x;
    int32_t exponent= (int32_t )xu.i >>23;
    // check if exponent is 0
    if (likely(exponent>0) )
    {
        if (unlikely( exponent==255 ) ) //check if negative or NaN
        {
            // Expected behavior:
            // sqrt(-f)=+qNaN, sqrt(-NaN)=+qNaN,sqrt(-Inf)=+qNaN
            xu.i = (xu.i == INF) ? INF : QUIET_NAN;
            return xu.f;
        } else 
        {
            uint32_t mantissa = xu.i &~ ((uint32_t)exponent << 23);
            exponent = exponent - (127);
            mantissa += 1 << 23; // adds implicit bit to mantissa.
            mantissa <<= (exponent & 1);

            exponent >>= 1;
            // This is meant to be a floor division
            // meaning -1/2= -0.5 should map to -1
            exponent = (exponent + (126 ));

            uint32_t new_mantissa=mantissa_sqrt(mantissa);
            xu.i = (exponent<<23) + new_mantissa;
            return xu.f;
        }
    }
    else
    {
        if (likely(exponent == 0))  
        {   
            if (likely (xu.i !=0) )
            {

                uint32_t mantissa = xu.i &~ ((uint32_t)exponent << 23);
                int32_t shift=__builtin_clz(mantissa)- (31-23);
                mantissa<<=shift; // normalize subnormal
                int32_t exponent =  - (126)-shift;
                mantissa <<= (exponent & 1);
                uint32_t new_mantissa=mantissa_sqrt(mantissa); 

                exponent >>= 1;
                exponent = (exponent + (126));
                xu.i = (exponent<<23) + new_mantissa;
                return xu.f;            
            } else //sqrt(+0)=+0
            {
                xu.i = 0; 
                return xu.f;
            }

        } else if (xu.i == (1u<<31))
        {
            xu.i= (1u<<31); //sqrt(-0)=-0
            return xu.f;             
        } else {
            xu.i=QUIET_NAN;  //sqrt(negative)=qNaN
            return xu.f;        
        }

    }
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
