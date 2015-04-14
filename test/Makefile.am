## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = \
	   -I$(top_srcdir)/include


cctest_CFLAGS =\
	 -DUNIX \
	 -Wall\
	 -g

filetest_CFLAGS =\
	 -DUNIX \
	 -Wall\
	 -g

bin_PROGRAMS = cctest filetest

cctest_SOURCES = \
	     cctest.c 

cctest_LDFLAGS = -L$(top_builddir)/lib

cctest_LDADD = -lsilcc

filetest_SOURCES = \
	     filetest.c 

filetest_LDFLAGS = -L$(top_builddir)/lib

filetest_LDADD = -lsilcc

