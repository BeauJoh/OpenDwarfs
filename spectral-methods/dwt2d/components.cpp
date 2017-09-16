#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "components.h"
#include "common.h"

/* the old "components.cu" has been separate into two parts 
   "components.cpp",contains functions
   "components.cl", contains all kernel functions
*/

/* Separate compoents of 8bit RGB source image */

//need add some segments
