#include <stdio.h>
#include <math.h>

#include <stdint.h>

float custom_sqrtf(float x);
float custom_rsqrtf(float x);

typedef union {
    float f;
    uint32_t u;
} fint;


int main(void)
{
    printf("testing custom_sqrt\n");
    {
        fint a;
        a.u= 0 | 1u<<15;    
        printf("Testing sqrt for correctness \n");
        printf("Please wait, this may take up to 5 min\n");
        uint64_t iterations = 0;
        uint32_t errors=0;
        for (fint mantissa ={.u= ( (0<<23)| 0 ) }; mantissa.u< ((255<<23) | 2) ; mantissa.u+=1)
        {
            iterations++;
            float sw = sqrt((double)mantissa.f );
            float hw = custom_sqrtf(mantissa.f);

            fint swf;
            fint hwf;
            swf.f=sw;
            hwf.f=hw;
            int32_t diff=(swf.u-hwf.u);
            if (diff<0) diff=-diff;

            if (diff!=0)errors++;

            if (diff>=1)
            {
                printf("sqrt(%f)= %f\n", mantissa.f, sqrt(mantissa.f ));
                printf("  got : %u\n", hwf.u);
                printf("  expected: %u\n", swf.u);
            }
        }
        printf("Tested %llu iterations\n", iterations);
        printf("%f percent correct", (1.0-(double)errors/iterations )*100.0);
        printf("\n");
    }

    printf("testing custom_rsqrt\n");
    {
        printf("Testing rsqrt for correctness \n");
        printf("Please wait, this may take up to 5 min\n");
        uint64_t iterations = 0;
        uint32_t errors=0;
        for (fint mantissa ={.u= ( (0<<23)| 0 ) }; mantissa.u< ((255<<23) | 2) ; mantissa.u+=1)
        {
            iterations++;
            float sw = 1.0/sqrt((double)mantissa.f );
            float hw = custom_rsqrtf(mantissa.f);

            fint swf;
            fint hwf;
            swf.f=sw;
            hwf.f=hw;
            int32_t diff=(swf.u-hwf.u);
            if (diff<0) diff=-diff;

            if (diff!=0)errors++;

            if (diff>=1)
            {
                printf("rsqrt(%f)= %f\n", mantissa.f, sqrt(mantissa.f ));
                printf("  input : %u\n", mantissa.u);
                printf("  got : %u, %f\n", hwf.u, (double)hwf.f*(1<<23));
                printf("  expected: %u, %f\n", swf.u, 1.0/sqrt((double)mantissa.f)*(1<<23));
            }
        }
        printf("Tested %llu iterations\n", iterations);
        printf("%f percent correct", (1.0-(double)errors/iterations )*100.0);
        printf("\n");
     }

    return 0;
}
