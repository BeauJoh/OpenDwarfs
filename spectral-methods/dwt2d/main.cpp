#include <unistd.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "common.h"
#include "components.h"
#include "dwt.h"

#include "../../include/rdtsc.h"
#include "../../include/common_args.h"
#include "../../include/lsb.h"

//using namespace std;

#define THREADS 256 

struct dwt {
    char * srcFilename;
    char * outFilename;
    unsigned char *srcImg;
    int pixWidth;
    int pixHeight;
    int components;
    int dwtLvls;
};

cl_context context = 0;
cl_command_queue commandQueue = 0;
cl_program program = 0;
cl_device_id cldevice = 0;
cl_kernel kernel = 0;
cl_kernel c_CopySrcToComponents = 0;
cl_kernel c_CopySrcToComponent = 0;
cl_kernel kl_fdwt53Kernel;
cl_int errNum = 0;

bool loadPPM(char* filename,int& width, int& height, int&channels, unsigned char** data);
bool loadPGM(char* filename,int& width, int& height, int&channels, unsigned char** data);
bool savePGM(char* filename,int width,int height, unsigned char* data);
bool savePGM(char* filename, int width, int height, int* data);
bool savePGM(char* filename,int width,int height, unsigned char* data, int channel_to_save, int number_of_channels);

///
// Load the input image.
//
int getImg(char * srcFilename, unsigned char *srcImg, int inputSize)
{
    // printf("Loading ipnput: %s\n", srcFilename);
    char path[] = "";
    char *newSrc = NULL;
    
    if((newSrc = (char *)malloc(strlen(srcFilename)+strlen(path)+1)) != NULL)
    {
        newSrc[0] = '\0';
        strcat(newSrc, path);
        strcat(newSrc, srcFilename);
        srcFilename= newSrc;
    }
    printf("Loading input: %s\n", srcFilename);

    //read image
    int i = open(srcFilename, O_RDONLY, 0644);
    if (i == -1) 
	{ 
        err(errno,"cannot access %s", srcFilename);
        return -1;
    }
    int ret = read(i, srcImg, inputSize);
    printf("precteno %d, inputsize %d\n", ret, inputSize);
    close(i);

    return 0;
}

///
//Show user how to use this program
//
void usage() {
    printf("dwt [options] src_img.rgb <out_img.dwt>\n\
  -l, --level\t\t\tDWT level, default 3\n\
  -f, --forward\t\t\tforward transform\n\
  -r, --reverse\t\t\treverse transform\n\
  -9, --97\t\t\t9/7 transform\n\
  -5, --53\t\t\t5/3 transform\n\
  -w  --write-visual\t\twrite output in visual (tiled) fashion instead of the linear\n");
}

///
// Separate compoents of 8bit RGB source image
//in file components.cu
void rgbToComponents(cl_mem d_r, cl_mem d_g, cl_mem d_b, unsigned char * h_src, int width, int height)
{
    LSB_Set_Rparam_string("region", "device_side_buffer_setup");
    LSB_Res();
    int pixels      = width * height;
    int alignedSize =  DIVANDRND(width*height, THREADS) * THREADS; //aligned to thread block size -- THREADS
    LSB_Set_Rparam_string("region", "c_CopySrcToComponents_kernel");
    cl_mem cl_d_src;
    cl_d_src = clCreateBuffer(context, CL_MEM_READ_ONLY, pixels*3*sizeof(unsigned char), NULL, &errNum);
    CHKERR(errNum, "Failed to create buffer [cl_d_src]!");
    LSB_Rec(0);

    LSB_Set_Rparam_string("region", "device_side_h2d_copy");
    LSB_Res();
    errNum = clEnqueueWriteBuffer(commandQueue, cl_d_src, CL_TRUE, 0, pixels*3*sizeof(unsigned char), (void*)h_src, 0, NULL, NULL);
    clFinish(commandQueue);
    CHKERR(errNum, "Failed to write to buffer [cl_d_src]!");
    LSB_Rec(0);

    size_t globalWorkSize[1] = {alignedSize};
    size_t localWorkSize[1] = { THREADS };

    LSB_Set_Rparam_string("region", "setting_c_CopySrcToComponents_kernel_arguments");
    LSB_Res();
    errNum  = clSetKernelArg(c_CopySrcToComponents, 0, sizeof(cl_mem), &d_r);
    errNum |= clSetKernelArg(c_CopySrcToComponents, 1, sizeof(cl_mem), &d_g);
    errNum |= clSetKernelArg(c_CopySrcToComponents, 2, sizeof(cl_mem), &d_b);
    errNum |= clSetKernelArg(c_CopySrcToComponents, 3, sizeof(cl_mem), &cl_d_src);
    errNum |= clSetKernelArg(c_CopySrcToComponents, 4, sizeof(int), &pixels);
    CHKERR(errNum, "Failed to set arguments [c_CopySrcToComponents]!");
    LSB_Rec(0);

    LSB_Set_Rparam_string("region", "c_CopySrcToComponents_kernel");
    LSB_Res();
    errNum = clEnqueueNDRangeKernel(commandQueue, c_CopySrcToComponents, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    clFinish(commandQueue);
    CHKERR(errNum, "Failed to run kernel [c_CopySrcToComponents]!");
    LSB_Rec(0);

    // Free Memory 
    errNum = clReleaseMemObject(cl_d_src);  
    CHKERR(errNum, "Failed to free memory [c_d_src]!");
}



///
// Copy a 8bit source image data into a color compoment
//in file components.cu
void bwToComponent(cl_mem d_c, unsigned char * h_src, int width, int height)
{
    LSB_Set_Rparam_string("region", "device_side_buffer_setup_and_transfer");
    LSB_Res();
    cl_mem cl_d_src;
    int pixels      = width*height;
    int alignedSize =  DIVANDRND(pixels, THREADS) * THREADS;

    cl_d_src = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, pixels, h_src, NULL);
    CHKERR(errNum, "Failed to create buffer [cl_d_src]!");
    LSB_Rec(0);

    size_t globalWorkSize[1] = { alignedSize };
    size_t localWorkSize[1] = { THREADS };
    assert(alignedSize%(THREADS) == 0);

    LSB_Set_Rparam_string("region", "setting_c_CopySrcToComponent_kernel_arguments");
    LSB_Res();
    errNum  = clSetKernelArg(c_CopySrcToComponent, 0, sizeof(cl_mem), &d_c);
    errNum |= clSetKernelArg(c_CopySrcToComponent, 1, sizeof(cl_mem), &cl_d_src);
    errNum |= clSetKernelArg(c_CopySrcToComponent, 2, sizeof(int), &pixels);
    CHKERR(errNum, "Failed to set kernel arguments [c_CopySrcToComponent]!");
    LSB_Rec(0);

    LSB_Set_Rparam_string("region", "c_CopySrcToComponent_kernel");
    LSB_Res();
    cl_event event;	
    errNum = clEnqueueNDRangeKernel(commandQueue, c_CopySrcToComponent, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    clFinish(commandQueue);
    CHKERR(errNum, "Failed to enqueue kernel [c_CopySrcToComponent]!");
    LSB_Rec(0);

    // Free Memory 
    errNum = clReleaseMemObject(cl_d_src);  
    CHKERR(errNum, "Failed to free memory [c_d_src]!");
}



/// Only computes optimal number of sliding window steps, number of threadblocks and then lanches the 5/3 FDWT kernel.
/// @tparam WIN_SX  width of sliding window
/// @tparam WIN_SY  height of sliding window
/// @param in       input image
/// @param out      output buffer
/// @param sx       width of the input image 
/// @param sy       height of the input image
///launchFDWT53Kerneld is in file 
void launchFDWT53Kernel (int WIN_SX, int WIN_SY, cl_mem in, cl_mem out, int sx, int sy)
{
    // compute optimal number of steps of each sliding window
	// cuda_dwt called a function divRndUp from namespace cuda_gwt. this function takes n and d, "return (n / d) + ((n % d) ? 1 : 0);"
	//
	
    const int steps = ( sy/ (15 * WIN_SY)) + ((sy % (15 * WIN_SY)) ? 1 : 0);	
	
	int gx = ( sx/ WIN_SX) + ((sx %  WIN_SX) ? 1 : 0);  //use function divRndUp(n, d){return (n / d) + ((n % d) ? 1 : 0);}
	int gy = ( sy/ (WIN_SY*steps)) + ((sy %  (WIN_SY*steps)) ? 1 : 0);
	
	printf("sliding steps = %d , gx = %d , gy = %d \n", steps, gx, gy);
	
    // prepare grid size
	size_t globalWorkSize[2] = { gx*WIN_SX, gy*1};
    size_t localWorkSize[2]  = { WIN_SX , 1};

    // printf("\n globalx=%d, globaly=%d, blocksize=%d\n", gx, gy, WIN_SX);
    LSB_Set_Rparam_string("region", "setting_kl_fdwt53Kernel_kernel_arguments");
    LSB_Res();
	errNum  = clSetKernelArg(kl_fdwt53Kernel, 0, sizeof(cl_mem), &in);
	errNum |= clSetKernelArg(kl_fdwt53Kernel, 1, sizeof(cl_mem), &out);
	errNum |= clSetKernelArg(kl_fdwt53Kernel, 2, sizeof(int), &sx);
	errNum |= clSetKernelArg(kl_fdwt53Kernel, 3, sizeof(int), &sy);
	errNum |= clSetKernelArg(kl_fdwt53Kernel, 4, sizeof(int), &steps);
	errNum |= clSetKernelArg(kl_fdwt53Kernel, 5, sizeof(int), &WIN_SX);
	errNum |= clSetKernelArg(kl_fdwt53Kernel, 6, sizeof(int), &WIN_SY);
    LSB_Rec(0);
    CHKERR(errNum, "Failed to set kernel arguments [kl_fdwt53Kernel]!");

    LSB_Set_Rparam_string("region", "kl_fdwt53Kernel_kernel");
    LSB_Res();
	errNum = clEnqueueNDRangeKernel(commandQueue, kl_fdwt53Kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
    clFinish(commandQueue);
    LSB_Rec(0);

    CHKERR(errNum, "Failed to execute kernel [kl_fdwt53Kernel]!");

}


/// Simple cudaMemcpy wrapped in performance tester.
/// param dest  destination bufer
/// param src   source buffer
/// param sx    width of copied image
/// param sy    height of copied image
///from /cuda_gwt/common.h/namespace
void memCopy (cl_mem dest,  cl_mem src, const size_t sx, const size_t sy){
    LSB_Set_Rparam_string("region", "device_side_d2d_copy");
    LSB_Res();
	errNum = clEnqueueCopyBuffer (commandQueue, src, dest, 0, 0, sx*sy*sizeof(int), 0, NULL, NULL);
    CHKERR(errNum, "Failed to copy memory buffer!");
    LSB_Rec(0);
}



/// Forward 5/3 2D DWT. See common rules (above) for more details.
/// @param in      Expected to be normalized into range [-128, 127].
///                Will not be preserved (will be overwritten).
/// @param out     output buffer on GPU
/// @param sizeX   width of input image (in pixels)
/// @param sizeY   height of input image (in pixels)
/// @param levels  number of recursive DWT levels
/// @backup use to test time
//at the end of namespace dwt_cuda (line338)
void fdwt53(cl_mem in, cl_mem out, int sizeX, int sizeY, int levels)
{
    LSB_Set_Rparam_int("dwt_level",levels);
    // select right width of kernel for the size of the image
    if(sizeX >= 960) 
	{
      launchFDWT53Kernel(192, 8, in, out, sizeX, sizeY);
    } 
	else if (sizeX >= 480) 
	{
      launchFDWT53Kernel(128, 8, in, out, sizeX, sizeY);
    } else if (sizeX >= 240) 
	{
      launchFDWT53Kernel(64, 8, in, out, sizeX, sizeY);
	}
    else
    {
      launchFDWT53Kernel(8, 8, in, out, sizeX, sizeY);
	}

		
	// if this was not the last level, continue recursively with other levels
	if (levels > 1)
	{
		// copy output's LL band back into input buffer 
		const int llSizeX = (sizeX / 2) + ((sizeX % 2) ? 1 :0);
		const int llSizeY = (sizeY / 2) + ((sizeY % 2) ? 1 :0);
		memCopy(in, out, llSizeX, llSizeY);
		
		// run remaining levels of FDWT
		fdwt53(in, out, llSizeX, llSizeY, levels - 1);
	}	
}
	

///
// in dwt.cu
int nStage2dDWT(cl_mem in, cl_mem out, cl_mem backup, int pixWidth, int pixHeight, int stages, bool forward)
{
    printf("\n*** %d stages of 2D forward DWT:\n", stages);

    // create backup of input, because each test iteration overwrites it 
    const int size = pixHeight * pixWidth * sizeof(int);
	
	// Measure time of individual levels. 
	if (forward)
		fdwt53(in, out, pixWidth, pixHeight, stages );
	//else
	//	rdwt(in, out, pixWidth, pixHeight, stages);
	// rdwt means rdwt53(can be found in file rdwt53.cu) which has not been defined 
	
	
	return 0;
}



///
//in file dwt.cu
void samplesToChar(unsigned char * dst, int * src, int samplesNum)
{
    int i;

    for(i = 0; i < samplesNum; i++)
	{
        int r = src[i]+128;
        if (r > 255) r = 255;
        if (r < 0)   r = 0; 
        dst[i] = (unsigned char)r;
    }
}



///
//in file dwt.cu
/// Write output linear orderd
int writeLinear(cl_mem component, int pixWidth, int pixHeight, const char * filename, const char * suffix)
{
	unsigned char * result;
    int *gpu_output;
    int i;
    int size;
    int samplesNum = pixWidth*pixHeight;
	
	size = samplesNum*sizeof(int);
	gpu_output = (int *)malloc(size);
    memset(gpu_output, 0, size);
	result = (unsigned char *)malloc(samplesNum);
    LSB_Set_Rparam_string("region", "device_side_d2h_copy");
    LSB_Res();
	errNum = clEnqueueReadBuffer(commandQueue, component, CL_TRUE, 0, size, gpu_output, 0, NULL, NULL);
    LSB_Rec(0);
    CHKERR(errNum, "Failed to read buffer!");

	// T to char 
	samplesToChar(result, gpu_output, samplesNum);
	
	// Write component 
	char outfile[strlen(filename)+strlen(suffix)];
    strcpy(outfile, filename);
    strcpy(outfile+strlen(filename), suffix);
    i = open(outfile, O_CREAT|O_WRONLY, 0644);
	if (i == -1) 
	{ 
        err(errno,"cannot access %s", outfile);
        return -1;
    }
	printf("\nWriting to %s (%d x %d)\n", outfile, pixWidth, pixHeight);
    write(i, result, samplesNum);
    close(i);
	
	// Clean up 
    free(gpu_output);
    free(result);

    return 0;
}

///
// Write output visual ordered 
//in file dwt.cu
int writeNStage2DDWT(cl_mem component, int pixWidth, int pixHeight, int stages, const char * filename, const char * suffix)
{
    struct band {
        int dimX; 
        int dimY;
    };
    struct dimensions {
        struct band LL;
        struct band HL;
        struct band LH;
        struct band HH;
    };

    unsigned char * result;
    int *src;
    int *dst;
    int i,s;
    int size;
    int offset;
    int yOffset;
    int samplesNum = pixWidth*pixHeight;
    struct dimensions * bandDims;

    bandDims = (struct dimensions *)malloc(stages * sizeof(struct dimensions));

    bandDims[0].LL.dimX = DIVANDRND(pixWidth,2);
    bandDims[0].LL.dimY = DIVANDRND(pixHeight,2);
    bandDims[0].HL.dimX = pixWidth - bandDims[0].LL.dimX;
    bandDims[0].HL.dimY = bandDims[0].LL.dimY;
    bandDims[0].LH.dimX = bandDims[0].LL.dimX;
    bandDims[0].LH.dimY = pixHeight - bandDims[0].LL.dimY;
    bandDims[0].HH.dimX = bandDims[0].HL.dimX;
    bandDims[0].HH.dimY = bandDims[0].LH.dimY;

    for (i = 1; i < stages; i++) 
    {
        bandDims[i].LL.dimX = DIVANDRND(bandDims[i-1].LL.dimX,2);
        bandDims[i].LL.dimY = DIVANDRND(bandDims[i-1].LL.dimY,2);
        bandDims[i].HL.dimX = bandDims[i-1].LL.dimX - bandDims[i].LL.dimX;
        bandDims[i].HL.dimY = bandDims[i].LL.dimY;
        bandDims[i].LH.dimX = bandDims[i].LL.dimX;
        bandDims[i].LH.dimY = bandDims[i-1].LL.dimY - bandDims[i].LL.dimY;
        bandDims[i].HH.dimX = bandDims[i].HL.dimX;
        bandDims[i].HH.dimY = bandDims[i].LH.dimY;
    }

#if 0
    printf("Original image pixWidth x pixHeight: %d x %d\n", pixWidth, pixHeight);
    for (i = 0; i < stages; i++) 
    {
        printf("Stage %d: LL: pixWidth x pixHeight: %d x %d\n", i, bandDims[i].LL.dimX, bandDims[i].LL.dimY);
        printf("Stage %d: HL: pixWidth x pixHeight: %d x %d\n", i, bandDims[i].HL.dimX, bandDims[i].HL.dimY);
        printf("Stage %d: LH: pixWidth x pixHeight: %d x %d\n", i, bandDims[i].LH.dimX, bandDims[i].LH.dimY);
        printf("Stage %d: HH: pixWidth x pixHeight: %d x %d\n", i, bandDims[i].HH.dimX, bandDims[i].HH.dimY);
    }
#endif

    size = samplesNum*sizeof(int);

    src = (int *)malloc(size);
    memset(src, 0, size);
    dst = (int *)malloc(size);
    memset(dst, 0, size);
    //result = (unsigned char *)malloc(samplesNum);
    LSB_Set_Rparam_string("region", "device_side_d2h_copy");
    LSB_Res();
    errNum = clEnqueueReadBuffer(commandQueue, component, CL_TRUE, 0, size, src, 0, NULL, NULL);
    LSB_Rec(0);
    CHKERR(errNum, "Failed to copy buffer [d2h]!");

    // LL Band 	
    size = bandDims[stages-1].LL.dimX * sizeof(int);
    for (i = 0; i < bandDims[stages-1].LL.dimY; i++) 
    {
        memcpy(dst+i*pixWidth, src+i*bandDims[stages-1].LL.dimX, size);
    }

    for (s = stages - 1; s >= 0; s--) {
        // HL Band
        size = bandDims[s].HL.dimX * sizeof(int);
        offset = bandDims[s].LL.dimX * bandDims[s].LL.dimY;
        for (i = 0; i < bandDims[s].HL.dimY; i++) 
        {
            memcpy(dst+i*pixWidth+bandDims[s].LL.dimX,
                src+offset+i*bandDims[s].HL.dimX, 
                size);
        }

        // LH band
        size = bandDims[s].LH.dimX * sizeof(int);
        offset += bandDims[s].HL.dimX * bandDims[s].HL.dimY;
        yOffset = bandDims[s].LL.dimY;
        for (i = 0; i < bandDims[s].HL.dimY; i++) 
        {
            memcpy(dst+(yOffset+i)*pixWidth,
                src+offset+i*bandDims[s].LH.dimX, 
                size);
        }

        //HH band
        size = bandDims[s].HH.dimX * sizeof(int);
        offset += bandDims[s].LH.dimX * bandDims[s].LH.dimY;
        yOffset = bandDims[s].HL.dimY;
        for (i = 0; i < bandDims[s].HH.dimY; i++) 
        {
            memcpy(dst+(yOffset+i)*pixWidth+bandDims[s].LH.dimX,
                src+offset+i*bandDims[s].HH.dimX, 
                size);
        }
    }

    // Write component
    //samplesToChar(result, dst, samplesNum);	

    char* pgm_suffix = ".pgm";    
    char outfile[strlen(filename)+strlen(suffix)+strlen(pgm_suffix)];
    strcpy(outfile, filename);
    strcpy(outfile+strlen(filename), suffix);
    strcpy(outfile+strlen(filename)+strlen(suffix), pgm_suffix);
    printf("\nWriting to %s (%d x %d)\n", outfile, pixWidth, pixHeight);
    if(!savePGM(outfile,pixWidth,pixHeight,dst)){
        return EXIT_FAILURE;
    }

    //i = open(outfile, O_CREAT|O_WRONLY, 0644);
    //
    //if (i == -1) 
    //{
    //    err(errno,"cannot access %s", outfile);
    //    return -1;
    //}
    //
    //printf("\nWriting to %s (%d x %d)\n", outfile, pixWidth, pixHeight);
    //write(i, result, samplesNum);
    //close(i);

    free(src);
    free(dst);
    //free(result);
    free(bandDims);
	
	return 0;
}




///
// Process of DWT algorithm
//
template <typename T>
void processDWT(struct dwt *d, int forward, int writeVisual)
{
    LSB_Set_Rparam_string("region", "device_side_buffer_setup");
    LSB_Res();

    int componentSize = d->pixWidth * d->pixHeight * sizeof(T);
    T *c_r_out, *c_g_out, *c_b_out, *backup, *c_r, *c_g, *c_b;

    // initialize to zeros
    cl_mem cl_c_r_out;
    cl_c_r_out = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum);
    CHKERR(errNum, "Failed to create buffer [cl_c_r_out]!");

    cl_mem cl_backup;
    cl_backup  = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum);  
    CHKERR(errNum, "Failed to create buffer [cl_backup]!");

    if (d->components == 3) {
        // Alloc two more buffers for G and B 
        cl_mem cl_c_g_out;
        cl_c_g_out = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum);  
        CHKERR(errNum, "Failed to create buffer [cl_c_g_out]!");

        cl_mem cl_c_b_out;
        cl_c_b_out = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum);
        CHKERR(errNum, "Failed to create buffer [cl_c_b_out]!");
        
        // Load components 
        cl_mem cl_c_r, cl_c_g, cl_c_b;
        cl_c_r = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum);
        CHKERR(errNum, "Failed to create buffer [cl_c_r]!");
        cl_c_g = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum);
        CHKERR(errNum, "Failed to create buffer [cl_c_g]!");
        cl_c_b = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum); 
        CHKERR(errNum, "Failed to create buffer [cl_c_b]!");
        LSB_Rec(0);

        rgbToComponents(cl_c_r, cl_c_g, cl_c_b, d->srcImg, d->pixWidth, d->pixHeight);

        printf("Working kernel memory: %fKiB\n",(componentSize*2)/1024.0);
        //Compute DWT and always store int file
        nStage2dDWT(cl_c_r, cl_c_r_out, cl_backup, d->pixWidth, d->pixHeight, d->dwtLvls, forward);
        nStage2dDWT(cl_c_g, cl_c_g_out, cl_backup, d->pixWidth, d->pixHeight, d->dwtLvls, forward);
        nStage2dDWT(cl_c_b, cl_c_b_out, cl_backup, d->pixWidth, d->pixHeight, d->dwtLvls, forward);
        
        
        // ---------test----------
/*      T *h_r_out=(T*)malloc(componentSize);
		errNum = clEnqueueReadBuffer(commandQueue, cl_c_g_out, CL_TRUE, 0, componentSize, h_r_out, 0, NULL, NULL); 
		fatal_CL(errNum, __LINE__);
        int ii;
		for(ii=0;ii<componentSize/sizeof(T);ii++) {
			fprintf(stderr, "%d ", (int)h_r_out[ii]);
			if((ii+1) % (d->pixWidth) == 0) fprintf(stderr, "\n");
        }
*/        // ---------test----------

#ifdef OUTPUT        
        // Store DWT to file
        if(writeVisual){
                writeNStage2DDWT(cl_c_r_out, d->pixWidth, d->pixHeight, d->dwtLvls, d->outFilename, ".r");
                writeNStage2DDWT(cl_c_g_out, d->pixWidth, d->pixHeight, d->dwtLvls, d->outFilename, ".g");
                writeNStage2DDWT(cl_c_b_out, d->pixWidth, d->pixHeight, d->dwtLvls, d->outFilename, ".b");
        } else {
            writeLinear(cl_c_r_out, d->pixWidth, d->pixHeight, d->outFilename, ".r");
            writeLinear(cl_c_g_out, d->pixWidth, d->pixHeight, d->outFilename, ".g");
            writeLinear(cl_c_b_out, d->pixWidth, d->pixHeight, d->outFilename, ".b");
        }
#endif		
	    clReleaseMemObject(cl_backup);	
		clReleaseMemObject(cl_c_r);
		clReleaseMemObject(cl_c_g);
		clReleaseMemObject(cl_c_b);
		clReleaseMemObject(cl_c_g_out);
		clReleaseMemObject(cl_c_b_out);

	} else if(d->components == 1) { 
        // Load components 
        cl_mem cl_c_r;
        cl_c_r = clCreateBuffer(context, CL_MEM_READ_WRITE, componentSize, NULL, &errNum);
        CHKERR(errNum, "Failed to create buffer [cl_c_r]!");
        LSB_Rec(0);

        bwToComponent(cl_c_r, d->srcImg, d->pixWidth, d->pixHeight);

        printf("Working kernel memory: %fKiB\n",(componentSize*2)/1024.0);
        // Compute DWT
        nStage2dDWT(cl_c_r, cl_c_r_out, cl_backup, d->pixWidth, d->pixHeight, d->dwtLvls, forward);
    
        //Store DWT to file
        if(writeVisual){
            writeNStage2DDWT(cl_c_r_out, d->pixWidth, d->pixHeight, d->dwtLvls, d->outFilename, ".r");
        } else {
            writeLinear(cl_c_r_out, d->pixWidth, d->pixHeight, d->outFilename, ".r");
        }

        clReleaseMemObject(cl_c_r);

    } 

    clReleaseMemObject(cl_c_r_out);
}

char* read_valid(FILE* fp){
    char* buff = new char[255];
    fscanf(fp,"%s",buff);
    if(!buff || strcmp("#",buff)==0){
        fscanf(fp, "%*[^\n]");//it's a comment in the header file so skip to next line
        if(buff == NULL)return NULL;
        strcpy(buff,read_valid(fp));
    }
    return buff;
}

bool loadPPM(char* filename,int& width, int& height, int& channels, unsigned char** data){
    int max_colour;
    channels = 3;
    FILE* fp = fopen(filename,"rb");
    std::stringstream line_stream;
    try{
        if (fp==NULL)
            throw "Image File Not Found\n";
        char* header = read_valid(fp);
        if (strcmp("P6",header)!=0)
            throw "Different Image Format";
        width = atoi(read_valid(fp));
        height = atoi(read_valid(fp));
        if (width == 0 || height == 0) {
            throw "Invalid dimensions...";
        }
        max_colour = atoi(read_valid(fp));
        if(max_colour!=255){
            throw "Unsupported Bit depth";
        }
    //    read_valid(fp);
    }catch(const char* ec){ 
        //std::cout<<"Error:"<<ec<<'\n';
        return false; 
    }
    unsigned int bytes_to_read = sizeof(unsigned char)*width*height*channels;
    *data = (unsigned char*)malloc(bytes_to_read);
    if (fread(*data, sizeof(unsigned char), bytes_to_read, fp) != bytes_to_read){
        //std::cout<<"Error: couldn't load image"<<std::endl;
        return false;
    }
    fclose(fp);
    return true;
}

bool loadPGM(char* filename,int& width, int& height, int& channels, unsigned char** data){
    int max_colour;
    channels = 1;
    FILE* fp = fopen(filename,"rb");
    std::stringstream line_stream;
    try{
        if (fp==NULL)
            throw "Image File Not Found\n";
        char* header = read_valid(fp);
        if (strcmp("P5",header)!=0)
            throw "Different Image Format";
        width = atoi(read_valid(fp));
        height = atoi(read_valid(fp));
        if (width == 0 || height == 0) {
            throw "Invalid dimensions...";
        }
        max_colour = atoi(read_valid(fp));
        if(max_colour!=255){
            throw "Unsupported Bit depth";
        }
    //    read_valid(fp);
    }catch(const char* ec){ 
        //std::cout<<"Error:"<<ec<<'\n';
        return false; 
    }
    unsigned int bytes_to_read = sizeof(unsigned char)*width*height*channels;
    *data = (unsigned char*)malloc(bytes_to_read);
    int read_bytes = fread(*data, sizeof(unsigned char), bytes_to_read, fp);
    if (read_bytes != bytes_to_read){
        //std::cout<<"Error: couldn't load image"<<std::endl;
        return false;
    }
    fclose(fp);
    return true;
}

bool savePGM(char* filename,int width,int height,unsigned char* data){
    int max_colour = 255;
    int channels = 1;
    try{
        if(data == NULL) throw "No data!";
        std::ofstream file(filename,std::ios::binary);
        file<<"P5\n"<<width<<' '<<height<<'\n'<<max_colour<<'\n';
        for(int i = 0 ; i < width*height*channels ; i+=channels){
            file<<data[i];
        }
        return true;
    }catch(const char* e){
        std::cout<<"Error:"<<e<<'\n';
        return false;
    }
}

bool savePGM(char* filename, int width, int height, int* data){
    unsigned char* buff = (unsigned char*)malloc(sizeof(unsigned char)*width*height);
    //first convert from a signed image data (-128,128) to signed (0,255)
    for(int i = 0 ; i < width*height ; i++){
        buff[i] = (unsigned char)(data[i]+128);
    }
    savePGM(filename,width,height,buff);
    free(buff);
}

bool savePGM(char* filename,int width,int height, unsigned char* data, int channel, int n_channels){
    int max_colour = 255;
    try{
        if(data == NULL) throw "No data!";
        std::ofstream file(filename,std::ios::binary);
        file<<"P5\n"<<width<<' '<<height<<'\n'<<max_colour<<'\n';
        for(int i = channel ; i < width*height*n_channels ; i+=n_channels){
            file<<data[i];
        }
        return true;
    }catch(const char* e){
        std::cout<<"Error:"<<e<<'\n';
        return false;
    }
}

int main(int argc, char **argv) 
{
    int optindex = 0;
    char ch;

    ocd_init(&argc, &argv, NULL);

    int pixWidth; //<real pixWidth
    int pixHeight; //<real pixHeight
    int bitDepth    = 8; 
    int dwtLvls     = 3; //default numuber of DWT levels
    int forward     = 1; //forward transform
    int dwt97       = 0; //1=dwt9/7, 0=dwt5/3 transform
    int writeVisual = 0; //write output (subbands) in visual (tiled) order instead of linear
    char * pos;
 
    while ((ch = getopt_long(argc, argv, "l:D:95wh", component::longopts, &optindex)) != -1) 
	{
        switch (ch) {
        case 'l':
            dwtLvls = atoi(optarg);
            break;
        case '9':
            dwt97 = 1;
            break;
        case '5':
            dwt97 = 0;
            break;
        case 'w':
            writeVisual = 1;
            break;
        case 'h':
            usage();
            return 0;
        case '?':
            return -1;
        default :
            usage();
            return -1;
        }
    }
	argc -= optind;
	argv += optind;

    if (argc == 0) 
	{ // at least one filename is expected
        printf("Please supply src file name\n");
        usage();
        return -1;
    }

    if (forward == 0) 
	{
        writeVisual = 0; //do not write visual when RDWT
    }
	
    ocd_initCL();
    LSB_Init("dwt2d",0);

	//
	// device init
	// Create an OpenCL context on first available platform
    //
    if (context == NULL)
    {
        std::cerr << "Failed to create OpenCL context." << std::endl;
        return EXIT_FAILURE;
    }

    // Create a command-queue on the first device available
    // on the created context
    commandQueue = commands;
    if (commandQueue == NULL)
    {
        return EXIT_FAILURE;
    }

    LSB_Set_Rparam_string("region", "kernel_creation");
    LSB_Res();

    // Create OpenCL program from com_dwt.cl kernel source
    program = ocdBuildProgramFromFile(context,device_id,"dwt2d_kernel","");
    if (program == NULL)
    {
        printf("fail to create program!!\n");
    }

    // Create OpenCL kernel
    c_CopySrcToComponents = clCreateKernel(program, "c_CopySrcToComponents", NULL); 
    if (c_CopySrcToComponents == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
    }

    c_CopySrcToComponent = clCreateKernel(program, "c_CopySrcToComponent", NULL); 
    if (c_CopySrcToComponent == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
    }

    kl_fdwt53Kernel = clCreateKernel(program, "cl_fdwt53Kernel", NULL); 
    if (kl_fdwt53Kernel == NULL)
    {
        std::cerr<<"Failed to create kernel\n";
    }
    LSB_Rec(0);

    LSB_Set_Rparam_string("region", "host_side_setup");
    LSB_Res();
    //initialize struct dwt
    struct dwt *d;
    d = (struct dwt *)malloc(sizeof(struct dwt));
    d->srcImg = NULL;
    d->dwtLvls  = dwtLvls;

    // file names
    d->srcFilename = (char *)malloc(strlen(argv[0]));

    strcpy(d->srcFilename, argv[0]);
    
    if (argc == 1) 
    { // only one filename supplyed
        d->outFilename = (char *)malloc(strlen(d->srcFilename)+4);
        strcpy(d->outFilename, d->srcFilename);
        strcpy(d->outFilename+strlen(d->srcFilename), ".dwt");
    } else {
        d->outFilename = strdup(argv[1]);
    }
   
    //load either ppm or pgm image
    if (!loadPPM(d->srcFilename,d->pixWidth,d->pixHeight,d->components,&d->srcImg) && !loadPGM(d->srcFilename,d->pixWidth,d->pixHeight,d->components,&d->srcImg)){
        printf("Error: Couldn't load image!\n");
        return EXIT_FAILURE;
    }

    //just a test to see if we can read and write the data.
    //if(!savePGM("original_red_component_as.pgm",d->pixWidth,d->pixHeight,(unsigned char*)d->srcImg,0,3)){
    //    exit(EXIT_FAILURE);
    //}

    //Input review
    printf("\nSource file:\t\t%s\n", d->srcFilename);
    printf(" Dimensions:\t\t%dx%d\n", d->pixWidth, d->pixHeight);
    printf(" DWT levels:\t\t%d\n", d->dwtLvls);
    printf(" 9/7 transform:\t\t%d\n", dwt97);
    LSB_Rec(0);

    // DWT
    // Create memory objects, Set arguments for kernel functions, Queue the kernel up for execution across the array, Read the output buffer back to the Host, Output the result buffer
    if(dwt97 == 1 )
        processDWT<float>(d, forward, writeVisual);
    else // 5/3
        processDWT<int>(d, forward, writeVisual);

    free(d->srcImg);
    clReleaseKernel(c_CopySrcToComponents);
    clReleaseKernel(c_CopySrcToComponent);
    clReleaseKernel(kl_fdwt53Kernel);
    
    LSB_Finalize();
    return 0;
}

