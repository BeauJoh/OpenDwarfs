#ifndef DWT_CL_H
#define	DWT_CL_H

namespace dwt_cl {
  void fdwt53(int * in, int * out, int sizeX, int sizeY, int levels);
  void rdwt53(int * in, int * out, int sizeX, int sizeY, int levels);
  void fdwt97(float * in, float * out, int sizeX, int sizeY, int levels);
  void rdwt97(float * in, float * out, int sizeX, int sizeY, int levels);  
}//  namespace dwt_cuda
#endif	// DWT_CL_H
