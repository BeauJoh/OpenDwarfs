#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <assert.h>
#include "../../include/rdtsc.h"
#include "../../include/common_args.h"
#include "../../include/lsb.h"

#include <omp.h>

#define AOCL_ALIGNMENT 64

#define CPU_DELTA_REDUCE
#define CPU_CENTER_REDUCE

extern "C"
int setup(int argc, char** argv);									/* function prototype */

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

unsigned int num_threads;//1d kernel
unsigned int num_blocks;

/* -------------- initCL() -------------- */
	extern "C"
void initCL()
{

	FILE *kernelFile;
	char *kernelSource;
	size_t kernelLength;

	cl_int errcode;

	ocd_initCL();
    num_threads = ocd_get_options().workgroup_1d;

	size_t max_worksize[3];
	errcode = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(size_t)*3, &max_worksize, NULL);
	CHKERR(errcode, "Failed to get device info!");

    LSB_Set_Rparam_string("region", "kernel_creation");
    LSB_Res();

    char* kernel_files;
	int num_kernels = 50;
	kernel_files = (char*) malloc(sizeof(char*)*num_kernels);
	strcpy(kernel_files,"kmeans_opencl_kernel");
               
    clProgram = ocdBuildProgramFromFile(context,device_id,kernel_files,NULL);

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
	size_t globalWorkSize;
	size_t localWorkSize;
    
    num_blocks = npoints / num_threads;
    if (npoints % num_threads > 0)
        num_blocks++;
    unsigned int num_blocks_perdim = sqrt((double) num_blocks);
    while (num_blocks_perdim * num_blocks_perdim < num_blocks)
        num_blocks_perdim++;

    num_blocks = num_blocks_perdim*num_blocks_perdim;

    /* allocate memory for memory_new[] and initialize to -1 (host) */
	//membership_new = (int*) memalign ( AOCL_ALIGNMENT, npoints * sizeof(int));
	membership_new = (int*) malloc(npoints * sizeof(int));
	for(int i=0;i<npoints;i++) {
		membership_new[i] = -1;
	}

	/* allocate memory for block_new_centers[] (host) */
	//block_new_centers = (float *) memalign ( AOCL_ALIGNMENT, nclusters*nfeatures*sizeof(float));
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
	CHKERR(errcode, "Failed to create buffer!");

	/* invert the data array (kernel execution) */	
	unsigned int arg = 0;
	errcode = clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(cl_mem), (void *) &feature_flipped_d);
	errcode |= clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(cl_mem), (void *) &feature_d);
	errcode |= clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(int), (void *) &npoints);
	errcode |= clSetKernelArg(clKernel_invert_mapping, arg++, sizeof(int), (void *) &nfeatures);
	CHKERR(errcode, "Failed to set kernel arg!");

    globalWorkSize = num_blocks*num_threads;
    localWorkSize = num_threads;

	errcode = clEnqueueNDRangeKernel(commands, clKernel_invert_mapping, 1, NULL, &globalWorkSize, &localWorkSize, 0, NULL, &ocdTempEvent);
	clFinish(commands);
	START_TIMER(ocdTempEvent, OCD_TIMER_KERNEL, "Invert Mapping Kernel", ocdTempTimer)
	END_TIMER(ocdTempTimer)
	CHKERR(errcode, "Failed to enqueue kernel! (Invert Mapping Kernel)");

	/* allocate memory for membership_d[] and clusters_d[][] (device) */
	membership_d = clCreateBuffer(context, CL_MEM_READ_WRITE, npoints*sizeof(int), NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");
	clusters_d = clCreateBuffer(context, CL_MEM_READ_ONLY, nclusters*nfeatures*sizeof(float), NULL, &errcode);
	CHKERR(errcode, "Failed to create buffer!");

    LSB_Rec(0);

    
}
/* -------------- allocateMemory() end ------------------- */

/* -------------- deallocateMemory() ------------------- */
/* free host and device memory */
	extern "C"
void deallocateMemory()
{
    if (membership_new != NULL){
        free(membership_new);
        membership_new = NULL;
    }
    if (block_new_centers != NULL){
        free(block_new_centers);
        block_new_centers = NULL;
    }
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
    printf("Setup done\n");
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

	errcode = clEnqueueWriteBuffer(commands, membership_d, CL_TRUE, 0, npoints*sizeof(int), (void *) membership_new, 0, NULL,NULL);// &ocdTempEvent);
	clFinish(commands);
	START_TIMER(ocdTempEvent, OCD_TIMER_H2D, "Membership Copy", ocdTempTimer)
	END_TIMER(ocdTempTimer)
	CHKERR(errcode, "Failed to enqueue write buffer!");

	/* copy clusters (host to device) */

	errcode = clEnqueueWriteBuffer(commands, clusters_d, CL_TRUE, 0, nclusters*nfeatures*sizeof(float), (void *) clusters[0], 0, NULL, &ocdTempEvent);
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
    size_t localWorkSize[2] = {num_threads, 1};
    size_t globalWorkSize[2] = {num_blocks*localWorkSize[0],
                                num_blocks*localWorkSize[1]};
    
    //size_t globalWorkSize[2] = {npoints*nclusters};
    printf("global work = %zi\n", globalWorkSize[0]);
    //size_t localWorkSize[2] = {ocd_get_options().workgroup_1d,1};
    printf("local work = %zi\n", localWorkSize[0]); 

	unsigned int arg = 0;
	errcode = clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_mem), (void *) &feature_d);
	errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(int), (void *) &nfeatures);
	errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(int), (void *) &npoints);
	errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(int), (void *) &nclusters);
	errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_mem), (void *) &membership_d);
	errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, sizeof(cl_mem), (void *) &clusters_d);
    errcode |= clSetKernelArg(clKernel_kmeansPoint, arg++, nclusters*sizeof(float), NULL);
	CHKERR(errcode, "Failed to set kernel arg!");
#ifndef PROFILE_OUTER_LOOP
    LSB_Rec(0);
#endif
	/* execute the kernel */
#ifndef PROFILE_OUTER_LOOP
    LSB_Set_Rparam_string("region", "kmeans_kernel");
    LSB_Res();
#endif
	errcode = clEnqueueNDRangeKernel(commands, clKernel_kmeansPoint, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, &ocdTempEvent);
	CHKERR(errcode, "Failed to enqueue kernel! (kmeansPoint)");
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
    for(int z = 0; z < npoints; z++){
        membership_new[z] = -1;
    }
    errcode = clEnqueueReadBuffer(commands,
                                  membership_d,
                                  CL_TRUE,
                                  0,
                                  npoints*sizeof(int),
                                  (void *) membership_new,
                                  0,
                                  NULL,
                                  &ocdTempEvent);
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
#ifdef CPU_DELTA_REDUCE
			delta++;
#endif
			membership[i] = membership_new[i];
		}
#ifdef CPU_CENTER_REDUCE
		for (j = 0; j < nfeatures; j++)
		{			
			new_centers[cluster_id][j] += feature[i][j];
		}
#endif
	}

	return delta;

}
/* ------------------- kmeansCuda() end ------------------------ */    

