#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "../../include/rdtsc.h"
#include "../../include/common_args.h"
#include "../../include/lsb.h"

size_t working_kernel_memory = 0;
size_t cluster_invokations = 0;

extern "C"
int setup(int argc, char** argv);									/* function prototype */

size_t localWorkSize;
size_t globalWorkSize;
cl_program clProgram;
cl_kernel clKernel_invert_mapping;
cl_kernel clKernel_kmeansPoint;

/* _d denotes it resides on the device */
int    *membership_new;												/* newly assignment membership */
cl_mem feature_d;													/* inverted data array */
cl_mem feature_flipped_d;											/* original (not inverted) data array */
cl_mem membership_d;												/* membership on the device */
float  *block_new_centers;											/* sum of points in a cluster (per block) */
cl_mem clusters_d;													/* cluster centers on the device */
cl_mem block_clusters_d;											/* per block calculation of cluster centers */
cl_mem block_deltas_d;												/* per block calculation of deltas */

/* image memory */
/*cl_mem t_features;
cl_mem t_features_flipped;
cl_mem t_clusters;*/


/* -------------- initCL() -------------- */
extern "C"
void initCL()
{
    LSB_Set_Rparam_string("region", "kernel_creation");
    LSB_Res();

    cl_int errcode = 0;

    ocd_initCL();
    localWorkSize = ocd_get_options().workgroup_1d;

    if (localWorkSize == 0){
        localWorkSize = 16;
    }

    int num_kernels = 50;
    char* kernel_files = (char*) malloc(sizeof(char*)*num_kernels);
    strcpy(kernel_files,"kmeans_opencl_kernel");

    clProgram = ocdBuildProgramFromFile(context,device_id,kernel_files,NULL);
    CHKERR(errcode, "Failed to build program!");

    clKernel_invert_mapping = clCreateKernel(clProgram, "invert_mapping", &errcode);
    CHKERR(errcode, "Failed to create kernel!");
    clKernel_kmeansPoint = clCreateKernel(clProgram, "kmeansPoint", &errcode);
    CHKERR(errcode, "Failed to create kernel!");
    LSB_Rec(0);
}
/* -------------- initCL() end -------------- */

/* -------------- allocateMemory() ------------------- */
/* allocate device memory, calculate number of blocks and threads, and invert the data array */
extern "C"
void allocateMemory(int npoints, int nfeatures, int nclusters, float **features)
{
    LSB_Set_Rparam_string("region", "device_side_buffer_setup");
    LSB_Res();
    cl_int errcode;
    if(npoints % localWorkSize != 0){
        printf("The local workgroup (%i items) doesn't even divide the global problem space (%i items)\n",localWorkSize,npoints);
        exit(EXIT_FAILURE);
    }
    globalWorkSize = npoints;

	/* allocate memory for memory_new[] and initialize to -1 (host) */
	membership_new = (int*) malloc(npoints * sizeof(int));
	for(int i=0;i<npoints;i++) {
		membership_new[i] = -1;
	}

	/* allocate memory for block_new_centers[] (host) */
	block_new_centers = (float *) malloc(nclusters*nfeatures*sizeof(float));
	
	/* allocate memory for feature_flipped_d[][], feature_d[][] (device) */
    feature_flipped_d = clCreateBuffer(context, CL_MEM_READ_ONLY, npoints*nfeatures*sizeof(float), NULL, &errcode);
    CHKERR(errcode, "Failed to create buffer!");
	 
    errcode = clEnqueueWriteBuffer(commands, feature_flipped_d, CL_TRUE, 0, npoints*nfeatures*sizeof(float), features[0], 0, NULL, &ocdTempEvent);
    
    clFinish(commands);
	START_TIMER(ocdTempEvent, OCD_TIMER_H2D, "Point/Feature Copy", ocdTempTimer)
	END_TIMER(ocdTempTimer)
	CHKERR(errcode, "Failed to create buffer!");
    feature_d = clCreateBuffer(context, CL_MEM_READ_WRITE, npoints*nfeatures*sizeof(float), NULL, &errcode);
    working_kernel_memory += sizeof(float)*npoints*nfeatures;
    CHKERR(errcode, "Failed to create buffer!");
		
	/* invert the data array (kernel execution) */	
    unsigned int arg = 0;
    errcode = clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(cl_mem), (void *) &feature_flipped_d);
    errcode |= clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(cl_mem), (void *) &feature_d);
    errcode |= clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(int), (void *) &npoints);
    errcode |= clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(int), (void *) &nfeatures);
    CHKERR(errcode, "Failed to set kernel arg!");

    errcode = clEnqueueNDRangeKernel(commands, clKernel_invert_mapping, 1, NULL, &globalWorkSize, &localWorkSize, 0, NULL, &ocdTempEvent);
    clFinish(commands);
    	START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Invert Mapping Kernel", ocdTempTimer)
	END_TIMER(ocdTempTimer)
	CHKERR(errcode, "Failed to enqueue kernel!");
		
	/* allocate memory for membership_d[] and clusters_d[][] (device) */
    membership_d = clCreateBuffer(context, CL_MEM_READ_WRITE, npoints*sizeof(int), NULL, &errcode);
    working_kernel_memory += sizeof(int)*npoints;
    CHKERR(errcode, "Failed to create buffer!");
    clusters_d = clCreateBuffer(context, CL_MEM_READ_ONLY, nclusters*nfeatures*sizeof(float), NULL, &errcode);
    working_kernel_memory += nclusters*nfeatures*sizeof(float);
    CHKERR(errcode, "Failed to create buffer!");

    LSB_Rec(0);
}
/* -------------- allocateMemory() end ------------------- */

/* -------------- deallocateMemory() ------------------- */
/* free host and device memory */
extern "C"
void deallocateMemory()
{
	free(membership_new);
	free(block_new_centers);
    clReleaseMemObject(feature_d);
    clReleaseMemObject(feature_flipped_d);
    clReleaseMemObject(membership_d);

    clReleaseMemObject(clusters_d);
    clReleaseKernel(clKernel_invert_mapping);
    clReleaseKernel(clKernel_kmeansPoint);
    clReleaseProgram(clProgram);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
}
/* -------------- deallocateMemory() end ------------------- */



////////////////////////////////////////////////////////////////////////////////
// Program main																  //

int
main( int argc, char** argv) 
{
	// as done in the CUDA start/help document provided
	ocd_init(&argc, &argv, NULL);
	setup(argc, argv);   
	ocd_finalize();
}

//																			  //
////////////////////////////////////////////////////////////////////////////////


/* ------------------- kmeansCuda() ------------------------ */    
extern "C"
int	// delta -- had problems when return value was of float type
kmeansCuda(float  **feature,				/* in: [npoints][nfeatures] */
           int      nfeatures,				/* number of attributes for each point */
           int      npoints,				/* number of data points */
           int      nclusters,				/* number of clusters */
           int     *membership,				/* which cluster the point belongs to */
		   float  **clusters,				/* coordinates of cluster centers */
		   int     *new_centers_len,		/* number of elements in each cluster */
           float  **new_centers				/* sum of elements in each cluster */
		   )
{
    cl_int errcode;

	int delta = 0;			/* if point has moved */
	int i,j;				/* counters */
#ifndef PROFILE_OUTER_LOOP
    LSB_Set_Rparam_string("region", "device_side_h2d_copy");
    LSB_Res();
#endif

    /* copy membership (host to device) */

    errcode = clEnqueueWriteBuffer(commands, membership_d, CL_TRUE, 0, npoints*sizeof(cl_int), (void *) membership_new, 0, NULL, &ocdTempEvent);
    clFinish(commands);
    START_TIMER(ocdTempEvent, OCD_TIMER_H2D, "Membership Copy", ocdTempTimer)
	END_TIMER(ocdTempTimer)
	CHKERR(errcode, "Failed to enqueue write buffer!");

	/* copy clusters (host to device) */

	errcode = clEnqueueWriteBuffer(commands, clusters_d, CL_TRUE, 0, nclusters*nfeatures*sizeof(cl_float), (void *) clusters[0], 0, NULL, &ocdTempEvent);
    clFinish(commands);
    START_TIMER(ocdTempEvent, OCD_TIMER_H2D, "Cluster Copy", ocdTempTimer)
        END_TIMER(ocdTempTimer)
	CHKERR(errcode, "Failed to enqueue write buffer!");
#ifndef PROFILE_OUTER_LOOP
    LSB_Rec(0);
#endif
#ifndef PROFILE_OUTER_LOOP
    LSB_Set_Rparam_string("region", "setting_kernel_arguments");
    LSB_Res();
#endif

    /* setup execution parameters. */
    unsigned int arg = 0;
    errcode = clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_mem), (void *) &feature_d);
    errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_int), (void *) &nfeatures);
    errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_int), (void *) &npoints);
    errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_int), (void *) &nclusters);
    errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_mem), (void *) &membership_d);
    errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_mem), (void *) &clusters_d);
    CHKERR(errcode, "Failed to set kernel arg!");

    if (cluster_invokations == 0){
        printf("Working kernel memory: %fKiB\n",
                (working_kernel_memory)/1024.0);
    }

	/* execute the kernel */
#ifndef PROFILE_OUTER_LOOP
    LSB_Set_Rparam_string("region", "kmeans_kernel");
    LSB_Res();
#endif
    
    errcode = clEnqueueNDRangeKernel(commands, clKernel_kmeansPoint, 1, NULL, &globalWorkSize, &localWorkSize, 0, NULL, &ocdTempEvent);
    CHKERR(errcode, "Failed to enqueue kernel!");
    errcode = clFinish(commands);
#ifndef PROFILE_OUTER_LOOP
    LSB_Rec(0);
#endif
    START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Point Kernel", ocdTempTimer)
    END_TIMER(ocdTempTimer)
    CHKERR(errcode, "Failed to clFinish!");
	/* copy back membership (device to host) */
#ifndef PROFILE_OUTER_LOOP
    LSB_Set_Rparam_string("region", "device_side_d2h_copy");
    LSB_Res();
#endif
    errcode = clEnqueueReadBuffer(commands, membership_d, CL_TRUE, 0, npoints*sizeof(cl_int), (void *) membership_new, 0, NULL, &ocdTempEvent);
    clFinish(commands);
    START_TIMER(ocdTempEvent, OCD_TIMER_D2H, "Membership Copy", ocdTempTimer)
	END_TIMER(ocdTempTimer)
	CHKERR(errcode, "Failed to enqueue read buffer!");

#ifndef PROFILE_OUTER_LOOP
    LSB_Rec(0);
#endif

	/* for each point, sum data points in each cluster
	   and see if membership has changed:
       if so, increase delta and change old membership, and update new_centers;
       otherwise, update new_centers */
	delta = 0;
	for (i = 0; i < npoints; i++)
	{		
		int cluster_id = membership_new[i];
		new_centers_len[cluster_id]++;
		if (membership_new[i] != membership[i])
		{
			delta++;
			membership[i] = membership_new[i];
		}
		for (j = 0; j < nfeatures; j++)
		{			
			new_centers[cluster_id][j] += feature[i][j];
		}
	}
    cluster_invokations++;
    return delta;

}
/* ------------------- kmeansCuda() end ------------------------ */    

