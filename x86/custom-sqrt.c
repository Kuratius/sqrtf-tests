#include <stdio.h>
#include <math.h>

#include <stdint.h>


#include <time.h>
# define unlikely(x) __builtin_expect ((x), 0)
float custom_rsqrtf(float x);

typedef union {
    float f;
    uint32_t u;
} fint;


int main(void)
{


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
                printf("rsqrt(%f)= %f\n", mantissa.f, 1.0/sqrt(mantissa.f ));
                printf("  input : %u, %f\n", mantissa.u, mantissa.f);
                printf("  got : %u, %f\n", hwf.u, (double)hwf.f*(1<<23));
                printf("  expected: %u, %f\n", swf.u, 1.0/sqrt((double)mantissa.f)*(1<<23));
            }
        }
        printf("Tested %llu iterations\n", iterations);
        printf("%f percent correct", (1.0-(double)errors/iterations )*100.0);
        printf("\n");
     }


    printf("Testing speed...\n");
    {
        uint64_t t=clock();
        for (fint mantissa ={.u= ( (0<<23)| 0 ) }; mantissa.u< ((255<<23) | 2) ; mantissa.u+=1)
        {
            //volatile float sw = 1.0/sqrt((double)mantissa.f );
            volatile float sw = 1.0/mantissa.f*sqrt((double)mantissa.f );
        }

        printf("Naive: Diff %llu \n", clock()-t);
     }

    {
        uint64_t t=clock();
        for (fint mantissa ={.u= ( (0<<23)| 0 ) }; mantissa.u< ((255<<23) | 2) ; mantissa.u+=1)
        {
            volatile float sw = custom_rsqrtf(mantissa.f );
        }

        printf("Rsqrtss: Diff %llu \n", clock()-t);
     }



    {
        uint64_t t=clock();
        for (fint mantissa ={.u= ( (0<<23)| 0 ) }; mantissa.u< ((255<<23) | 2) ; mantissa.u+=1)
        {
            double fres=sqrt_core(mantissa.f);

            double x=mantissa.f*0.5f;
            float threehalfs=3.0f/2.0f;
            double nr=fres*(threehalfs-x*fres*fres);
            nr=nr*(threehalfs-x*nr*nr);
            union
            {
                double d;
                uint64_t l;
            } xd;
            xd.d=nr;
            uint32_t retval=((xd.l<<11) | (1ull<<63) )>>39;
            uint32_t U=retval;
            if (unlikely((mantissa.u & ((1<<23)-1)== ((9039928)& ((1<<23)-1) )  ))) 
                U |=1;
            xd.l= (((xd.l>>53)-1)<<53) | ((U+1)>>1);

            volatile float sw = xd.d;



        }

        printf("Rsqrtss: Diff %llu \n", clock()-t);
     }


    return 0;
}
