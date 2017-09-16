#ifndef _COMPONENTS_H
#define _COMPONENTS_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
/* Separate compoents of source 8bit RGB image */

template<typename T>
void rgbToComponents(T d_r, T d_g, T d_b, unsigned char * src, int width, int height);

namespace component{
#include <getopt.h>
static struct option longopts[] = {
    {"level",       1, 0, 'l'}, //level of dwt
    {"97",          0, 0, '9'}, //9/7 transform
    {"53",          0, 0, '5'}, //5/3transform
    {"write-visual",0, 0, 'w'}, //write output (subbands) in visual (tiled) order instead of linear
    {"help",        0, 0, 'h'},
    {0,0,0,0}
};

};
/* Copy a 8bit source image data into a color compoment of type T */
//template<typename T>
//void bwToComponent(T *d_c, unsigned char * src, int width, int height);

#endif
