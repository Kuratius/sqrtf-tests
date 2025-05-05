	.text
    .global sqrt_core
	.type	sqrt_core, @function
sqrt_core:
        movaps  %xmm0, %xmm0
        rsqrtss %xmm0, %xmm0
        //movd    %eax, %xmm1
	    ret
