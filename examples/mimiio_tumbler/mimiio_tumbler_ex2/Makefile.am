AUTOMAKE_OPTIONS=subdir-objects
MIMIIODIR = ../../../src
OS_SPECIFIC_LINKS = @OS_SPECIFIC_LINKS@

if MIMIXFE_DEP_BUILD

bin_PROGRAMS = mimiio_tumbler_ex2

if DEBUG

AM_CFLAGS = -g3	-O0 -fno-inline -D_DEBUG $(MIMIXFE_CPPFLAGS) 
AM_CXXFLAGS = -g3 -O0 -fno-inline -D_DEBUG $(MIMIXFE_CPPFLAGS) -std=c++11

mimiio_tumbler_ex2_SOURCES = mimiio_tumbler_ex2.cpp
mimiio_tumbler_ex2_LDADD = $(MIMIIODIR)/.libs/libmimiio.a $(POCO_S_LIBS) $(MIMIXFE_LDFLAGS) $(OS_SPECIFIC_LINKS) -lPocoNetd -lPocoNetSSLd -lPocoFoundationd -lPocoJSONd $(FLAC_LIBS) -lmimixfe

else 

AM_CFLAGS = -g -O2 $(MIMIXFE_CPPFLAGS)
AM_CXXFLAGS = -g -O2 $(MIMIXFE_CPPFLAGS) -std=c++11

mimiio_tumbler_ex2_SOURCES = mimiio_tumbler_ex2.cpp
mimiio_tumbler_ex2_LDADD = $(MIMIIODIR)/.libs/libmimiio.a $(OS_SPECIFIC_LINKS) $(MIMIXFE_LDFLAGS) $(FLAC_LIBS) -lPocoNet -lPocoNetSSL -lPocoFoundation -lPocoJSON -lmimixfe

endif
endif
