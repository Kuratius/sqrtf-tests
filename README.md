# sqrtf-tests
sqrtf and rsqrtf test
Run 
```
arm-linux-gnueabi-gcc -static -O3 -marm custom-sqrt.c Csqrt.c sqrtcore.s -lm  && qemu-arm -L /usr/arm-linux-gnueabi ./a.out 

``` 
to test the routines.
It will fall back to C code if you are using an architecture other than armv5te.
Optionally, run 
```
arm-linux-gnueabi-gcc -static -Os -marm custom-sqrt.c Csqrt.c sqrtcore-small.s -lm  && qemu-arm -L /usr/arm-linux-gnueabi ./a.out 
```
if you would prefer smaller code size at the cost of performance
