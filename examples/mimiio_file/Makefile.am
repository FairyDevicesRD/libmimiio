bin_PROGRAMS = mimiio_file

AUTOMAKE_OPTIONS=subdir-objects
MIMIIODIR = ../../src
OS_SPECIFIC_LINKS = @OS_SPECIFIC_LINKS@

if DEBUG

AM_CFLAGS = -g	-O0 -fno-inline -D_DEBUG 
AM_CXXFLAGS = -g -O0 -fno-inline -D_DEBUG @POCO_CPPFLAGS@ -std=c++11
AM_LDFLAGS = @POCO_LDFLAGS@

mimiio_file_SOURCES = mimiio_file.cpp
mimiio_file_LDADD = $(MIMIIODIR)/.libs/libmimiio.a $(OS_SPECIFIC_LINKS) @POCO_LDFLAGS@ -lPocoNetSSLd -lPocoNetd -lPocoUtild -lPocoXMLd -lPocoJSONd -lPocoFoundationd -lPocoCryptod $(FLAC_LIBS)

else

AM_CFLAGS = -g -O3 
AM_CXXFLAGS = -g -O3 @POCO_CPPFLAGS@ -std=c++11

mimiio_file_SOURCES = mimiio_file.cpp
mimiio_file_LDADD = $(MIMIIODIR)/libmimiio.la $(OS_SPECIFIC_LINKS) $(FLAC_LIBS) @POCO_LDFLAGS@ -lPocoNet -lPocoNetSSL -lPocoFoundation -lPocoJSON -lPocoCrypto -lPocoUtil -lPocoXML

endif
