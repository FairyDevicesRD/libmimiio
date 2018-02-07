AUTOMAKE_OPTIONS=subdir-objects

lib_LTLIBRARIES=libmimiio.la
include_HEADERS=mimiio.h

EXTRA_DIST=config.h.in

if DEBUG
AM_CXXFLAGS = -g3 -O0 @POCO_CPPFLAGS@ -fno-inline -std=c++11 -D_DEBUG ${FLAC_CFLAGS}
POCO_CLIBS = -lPocoNetd -lPocoFoundationd -lPocoNetSSLd -lPocoCryptod 
else
AM_CXXFLAGS = -g -O2 @POCO_CPPFLAGS@ -std=c++11 ${FLAC_CFLAGS}
POCO_CLIBS = -lPocoNet -lPocoFoundation -lPocoNetSSL -lPocoCrypto
endif

OS_SPECIFIC_CFLAGS = @OS_SPECIFIC_CFLAGS@
OS_SPECIFIC_LINKS = @OS_SPECIFIC_LINKS@

noinst_HEADERS=config.h \
mimiioAsynchronousCallbackAPIController.hpp \
mimiioSynchronousAPIController.hpp \
mimiioController.hpp \
mimiioImpl.hpp \
mimiioEncoderFactory.hpp \
strerror.hpp \
typedef.hpp \
worker/mimiioTxWorker.hpp \
worker/mimiioRxWorker.hpp \
encoder/encoder.hpp \
encoder/flac.hpp \
encoder/pcm.hpp \
encoder/flacPT.hpp

SRC_SOURCES=mimiio.cpp \
mimiioAsynchronousCallbackAPIController.cpp \
mimiioSynchronousAPIController.cpp \
mimiioController.cpp \
mimiioImpl.cpp \
mimiioEncoderFactory.cpp \
worker/mimiioTxWorker.cpp \
worker/mimiioRxWorker.cpp \
encoder/flac.cpp

libmimiio_la_LDFLAGS=-no-undefined -version-info  @SHARED_VERSION_INFO@ @SHLIB_VERSION_ARG@
libmimiio_la_SOURCES=$(SRC_SOURCES)
libmimiio_la_LIBADD=-lm $(POCO_CLIBS) @POCO_LDFLAGS@ ${FLAC_LIBS}
