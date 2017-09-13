#
# Copyright 2017 by The Australian National University. All rights reserved.
#
bin_PROGRAMS += openclfft

openclfft_SOURCES = spectral-methods/OpenCLfft2/src/CLFFTErrors.cpp spectral-methods/OpenCLfft2/src/CLFFTContext.cpp spectral-methods/OpenCLfft2/test/TestFunctions.cpp spectral-methods/OpenCLfft2/test/t_benchmarks.cpp
openclfft_CPPFLAGS = -I$(top_srcdir)/spectral-methods/OpenCLfft2/src -I$(top_srcdir)/spectral-methods/OpenCLfft2/test

all_local += openclfft-all-local
exec_local += openclfft-exec-local

openclfft-all-local:
	cp $(top_srcdir)/spectral-methods/OpenCLfft2/src/FFTKernels.cl .

openclfft-exec-local:
	cp $(top_srcdir)/spectral-methods/OpenCLfft2/src/FFTKernels.cl ${DESTDIR}${bindir}
