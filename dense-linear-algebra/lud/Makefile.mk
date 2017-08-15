#
# Copyright 2010 by Virginia Polytechnic Institute and State
# University. All rights reserved. Virginia Polytechnic Institute and
# State University (Virginia Tech) owns the software and its
# associated documentation.
#

bin_PROGRAMS += lud
bin_PROGRAMS += lud_profiling_outer_loop

lud_SOURCES = dense-linear-algebra/lud/lud.c dense-linear-algebra/lud/common.c
lud_profiling_outer_loop_SOURCES = dense-linear-algebra/lud/lud.c dense-linear-algebra/lud/common.c

lud_profiling_outer_loop_CFLAGS = $(AM_CFLAGS) -DPROFILE_OUTER_LOOP

all_local += lud-all-local
exec_local += lud-exec-local

lud-all-local:
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel.cl .
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel_opt_gpu.cl .

lud-exec-local:
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel.cl ${DESTDIR}${bindir}
	cp $(top_srcdir)/dense-linear-algebra/lud/lud_kernel_opt_gpu.cl ${DESTDIR}${bindir}
