//#define __global
//#define __kernel

__kernel void vadd(__global uint *a, __global uint *b, __global uint *c) {
    int i = get_global_id(0);

    c[i] = a[i];

}
