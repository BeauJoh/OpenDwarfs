#
# Copyright 2017 Beau Johnston and The Australian National University. All
# rights reserved.
#
bin_PROGRAMS += dwt2d

dwt2d_SOURCES = spectral-methods/dwt2d/components.cpp spectral-methods/dwt2d/ppm.h spectral-methods/dwt2d/dwt.cpp spectral-methods/dwt2d/main.cpp
dwt2d_CPPFLAGS = -DOUTPUT -I$(top_srcdir)/spectral-methods/dwt2d -I$(top_srcdir)/spectral-methods/dwt2d/dwt_cl

all_local += dwt2d-all-local
exec_local += dwt2d-exec-local

dwt2d-all-local:
	cp $(top_srcdir)/spectral-methods/dwt2d/dwt2d_kernel.cl .

dwt2d-exec-local:
	cp $(top_srcdir)/spectral-methods/dwt2d/dwt2d_kernel.cl ${DESTDIR}${bindir}
