#ifndef _KMEANS_CUDA_KERNEL_H_
#define _KMEANS_CUDA_KERNEL_H_

#ifndef FLT_MAX
#define FLT_MAX 3.40282347e+38
#endif

/* ----------------- invert_mapping() --------------------- */
/* inverts data array from row-major to column-major.

   [p0,dim0][p0,dim1][p0,dim2] ... 
   [p1,dim0][p1,dim1][p1,dim2] ... 
   [p2,dim0][p2,dim1][p2,dim2] ... 
   to
   [dim0,p0][dim0,p1][dim0,p2] ...
   [dim1,p0][dim1,p1][dim1,p2] ...
   [dim2,p0][dim2,p1][dim2,p2] ...
 */
__kernel void invert_mapping(__global float *input, /* original */
        __global float *output,     /* inverted */
        int npoints,                /* npoints */
        int nfeatures)              /* nfeatures */
{
    int point_id = get_local_id(0) + get_local_size(0)*get_group_id(0);	/* id of thread */
    int i;

    if(point_id < npoints){
        for(i=0;i<nfeatures;i++)
            output[point_id + npoints*i] = input[point_id*nfeatures + i];
    }
    return;
}
/* ----------------- invert_mapping() end --------------------- */

/* to turn on the GPU delta and center reduction */
//#define GPU_DELTA_REDUCTION
//#define GPU_NEW_CENTER_REDUCTION
//#define THREADS_PER_BLOCK 256

/* ----------------- kmeansPoint() --------------------- */
/* find the index of nearest cluster centers and change membership*/
    __kernel void
kmeansPoint(__global float  *features, /* in: [npoints*nfeatures] */
        int     nfeatures,
        int     npoints,
        int     nclusters,
        __global int    *membership,
        __constant float  *clusters,
        __local float *dist
        ) 
{

    // block ID
    //const unsigned int block_id = get_num_groups(0)*get_group_id(1)+get_group_id(0);
    //// point/thread ID  
    //const unsigned int point_id = block_id*get_local_size(0)*get_local_size(1) + get_local_id(0);
    unsigned int point_id = get_global_id(0)/get_local_size(0);
    unsigned int cluster_id = get_local_id(0);

    int  index = -1;

    if (point_id < npoints)
    {
        int i, j;
        float min_dist = FLT_MAX;
        //float dist[nclusters];        /* distance square between a point to cluster center */

        /* find the cluster center id with min distance to pt, using all local
         * threads in the local workgroup, instead of: */
        //for (i=0; i<nclusters; i++) {
        {
            i=cluster_id;
            int cluster_base_index = i*nfeatures; /* base index of cluster centers for inverted array */
            float ans=0.0; /* Euclidean distance sqaure */

            for (j=0; j < nfeatures; j++)
            {
                int addr = point_id + j*npoints; /* appropriate index of data point */
                float diff = (features[addr] -
                        clusters[cluster_base_index + j]); /* distance between a data point to cluster centers */
                ans += diff*diff; /* sum of squares */
            }
            dist[i] = ans;

            /* using serial reduction see if distance is smaller than previous
             * ones, if so, change minimum distance and save index of cluster
             * center */
            barrier(CLK_LOCAL_MEM_FENCE);
            if(cluster_id == 0){
                for(int i = 0; i < nclusters; i++){
                    if (dist[i] < min_dist) {
                        min_dist = dist[i];
                        index    = i;
                    }
                }
            }
        }
    }

    if (point_id < npoints && cluster_id == 0)
    {
        /* assign the membership to object point_id */
        membership[point_id] = index;
    }
}
#endif // #ifndef _KMEANS_CUDA_KERNEL_H_
