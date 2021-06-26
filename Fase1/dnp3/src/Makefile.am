lib_LTLIBRARIES = libdnp3.la

libdnp3_la_SOURCES = \
	dnp3.c
#	../../../utils/t2buf.c  # if you uncomment this line, do not
#	                        # forget to append a backslash above!

libdnp3_la_CFLAGS = \
	-I$(top_srcdir)/../../utils \
	-I$(top_srcdir)/../../tranalyzer2/src
#	-I$(top_srcdir)/../tcpFlags/src         # tell the compiler where to find header
#	                                        # files from dependent plugins
#	                                        # !!! if you uncomment this line, do not
#	                                        # !!! forget to append a backslash above

if APPLE
libdnp3_la_CFLAGS += -D_DARWIN_C_SOURCE  # OSX specific flags
else
libdnp3_la_CFLAGS += -D_GNU_SOURCE
endif

libdnp3_la_LDFLAGS = -shrext .so  # extension for shared library
                                     # (without this flag, would be '.dylib' on OSX)
