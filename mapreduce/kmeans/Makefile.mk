#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += kmeans
bin_PROGRAMS += kmeans_profiling_outer_loop

kmeans_SOURCES = mapreduce/kmeans/kmeans.c \
	mapreduce/kmeans/cluster.c \
	mapreduce/kmeans/getopt.c \
	mapreduce/kmeans/kmeans_clustering.c \
	mapreduce/kmeans/kmeans_opencl.cpp \
	mapreduce/kmeans/rmse.c

kmeans_profiling_outer_loop_SOURCES = mapreduce/kmeans/kmeans.c \
	mapreduce/kmeans/cluster.c \
	mapreduce/kmeans/getopt.c \
	mapreduce/kmeans/kmeans_clustering.c \
	mapreduce/kmeans/kmeans_opencl.cpp \
	mapreduce/kmeans/rmse.c
kmeans_profiling_outer_loop_CPPFLAGS = $(AM_CPPFLAGS) -DPROFILE_OUTER_LOOP

all_local += kmeans-all-local
exec_local += kmeans-exec-local

kmeans-all-local:
	cp $(top_srcdir)/mapreduce/kmeans/kmeans_opencl_kernel.cl .
	cp $(top_srcdir)/mapreduce/kmeans/kmeans_opencl_kernel_opt_gpu.cl .

kmeans-exec-local:
	cp $(top_srcdir)/mapreduce/kmeans/kmeans_opencl_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/mapreduce/kmeans/kmeans_opencl_kernel_out_gpu.cl ${DESTDIR}${bindir}
