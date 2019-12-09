/**
 * @file mimiio.cpp
 * @brief libmimiio external API definition
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "config.h"
#include "mimiio.h"
#include "typedef.hpp"
#include "strerror.hpp"
#include "mimiioController.hpp"
#include "mimiioSynchronousAPIController.hpp"
#include "mimiioAsynchronousCallbackAPIController.hpp"
#include "mimiioImpl.hpp"
#include "mimiioEncoderFactory.hpp"
#include <Poco/Logger.h>
#include <Poco/AutoPtr.h>
#ifdef _WIN32
#include <Poco/EventLogChannel.h>
#else
#include <Poco/ConsoleChannel.h>
#include <Poco/SyslogChannel.h>
#include <syslog.h>
#endif
#include <Poco/PatternFormatter.h>
#include <Poco/FormattingChannel.h>
#include <Poco/AsyncChannel.h>
#include <memory>
#include <mutex>

static void set_logger_properties(Poco::Logger& logger, int level) {
	// Initialize and prepare log context
#ifdef _WIN32
	Poco::AutoPtr<Poco::EventLogChannel> pChannel(new Poco::EventLogChannel());
#elif __APPLE__
	// Despite what you pass to setlogmask(), in its default configuration, OS X will only write messages to the system log
	// that have a priority of LOG_NOTICE or higher,
	Poco::AutoPtr<Poco::ConsoleChannel> pChannel(new Poco::ConsoleChannel());
#else
	Poco::AutoPtr<Poco::SyslogChannel> pChannel(new Poco::SyslogChannel());
#endif
	Poco::AutoPtr<Poco::PatternFormatter> fmt(new Poco::PatternFormatter);
	fmt->setProperty("pattern","%Y-%m-%d %H:%M:%S (%N) [%s:%P:%I:%p] %t"); // set log base format
	fmt->setProperty(Poco::PatternFormatter::PROP_TIMES, "local"); // set log time local
	Poco::AutoPtr<Poco::FormattingChannel> pfmtChannel(new Poco::FormattingChannel(fmt, pChannel));
	logger.setChannel(pfmtChannel.get());
	// AsyncChannel may not be stable for multi-thread environment.
	//Poco::AutoPtr<Poco::AsyncChannel> pasyncfmtChannel(new Poco::AsyncChannel(pfmtChannel));
	//logger.setChannel(pasyncfmtChannel);
	logger.setLevel(level);
}

MIMI_IO* mimi_open(
		const char* mimi_host,
		int mimi_port,
		void (*on_tx_func)(char* buffer, size_t* len, bool* recog_break, int* txfunc_error, void* userdata_for_tx),
		void (*on_rx_func)(const char* result, size_t len, int* rxfunc_error, void* userdata_for_rx),
		void* userdata_for_tx,
		void* userdata_for_rx,
		MIMIIO_AUDIO_FORMAT format,
		int samplingrate,
		int channels,
		const MIMIIO_HTTP_REQUEST_HEADER* request_headers,
		int request_headers_len,
		const char* access_token,
		int loglevel,
		int* errorno)
{
	static std::once_flag flag;
	Poco::Logger& logger = Poco::Logger::get(PACKAGE_NAME);
	try{
		std::call_once(flag, set_logger_properties, logger, loglevel);

		// Create encoder
		mimiio::mimiioEncoderFactory encoderFactory(logger);
		mimiio::encoder::Encoder* encoder = encoderFactory.createEncoder(format, samplingrate, channels);
		// Create request header
		std::vector<MIMIIO_HTTP_REQUEST_HEADER> requestHeaders;
		for(int i=0;i<request_headers_len;++i){
			requestHeaders.push_back(request_headers[i]);
		}
		MIMIIO_HTTP_REQUEST_HEADER contentType;
		std::strcpy(contentType.key, "X-Mimi-Content-Type");
		std::strcpy(contentType.value, encoder->ContentType().c_str());
		requestHeaders.push_back(contentType);
		// delete encoder (re-create in mimiioController)
		delete encoder;

		//with/without authentication
		mimiio::mimiioImpl* impl = nullptr;
		if(access_token == nullptr){
			poco_debug((logger), "lmio: mimi_open without authentication.");
			impl = new mimiio::mimiioImpl(mimi_host, mimi_port, requestHeaders, (logger));
		}else{
			poco_debug((logger), "lmio: mimi_open with authentication.");
			impl = new mimiio::mimiioImpl(mimi_host, mimi_port, requestHeaders, access_token, (logger));
		}

		//synchronous or asynchronous callback API
		mimiio::mimiioController* ctrler = nullptr;
		if(on_tx_func == nullptr || on_rx_func == nullptr){
			// hidden API, comment out in mimiio.h and mimiio.cpp
			poco_debug((logger), "using synchronous API.");
			ctrler = new mimiio::mimiioSynchronousAPIController(impl, encoderFactory.createEncoder(format, samplingrate, channels), (logger));
		}else{
			//poco_debug(logger, "using asynchronous callback API.");
			ctrler = new mimiio::mimiioAsynchronousCallbackAPIController(impl, encoderFactory.createEncoder(format, samplingrate, channels), on_tx_func, on_rx_func, userdata_for_tx, userdata_for_rx, (logger));
		}

		MIMI_IO* mio = new MIMI_IO();
		mio->mt_.reset(ctrler);
		*errorno = 0;
		return mio;
	}catch(const Poco::Net::SSLContextException &e){
		*errorno = 601; // SSL client context error
		(logger).fatal("lmio: mimi_open failed: SSL connection failed, client context error.");
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::InvalidCertificateException &e){
		*errorno = 602; // SSL invalid certificate error
		(logger).fatal("lmio: mimi_open failed: SSL connection failed, invalid certificate: %s",e.displayText());
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::CertificateValidationException &e){
		*errorno = 603; // SSL certificate validation error
		(logger).fatal("lmio: mimi_open failed: SSL connection failed, server certificate validation error: %s", e.displayText());
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::SSLConnectionUnexpectedlyClosedException &e){
		*errorno = 604; // SSL unexpectedly connection closed.
		(logger).fatal("lmio: mimi_open failed: SSL connection failed, ssl connection unexpectedly closed: %s", e.displayText());
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::SSLException &e){
		*errorno = 605; // SSL error, server certificate validation error
		(logger).fatal("lmio: mimi_open failed: SSL connection failed: %s", e.displayText());
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::WebSocketException &e){
		*errorno = 800 + static_cast<int>(e.code()); // 800s' error
		(logger).fatal("lmio: mimi_open failed: WebSocket exception: %s (%d)", std::string(mimiio::strerror(*errorno)), *errorno);
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::HostNotFoundException &e){
		*errorno = 701; // host not found
		(logger).fatal("lmio: mimi_open failed: Host not found.");
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::ConnectionRefusedException &e){
		*errorno = 704; // connection refused by remote host
		(logger).fatal("lmio: mimi_open failed: Connection refused by remote host.");
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::ConnectionResetException &e){
		*errorno = 705; // connection reset by peer, which means exceeded simultaneous processing limit.
		(logger).fatal("lmio: mimi_open failed: Connection reset by peer, which means exceeded simultaneous processing limit.");
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::NoMessageException &e){
		//NoMessageException is often occurred when connection reset by peer.
		*errorno = 705; // connection reset by peer, which means exceeded simultaneous processing limit.
		(logger).fatal("lmio: mimi_open failed: Connection reset by peer(no msg), which means exceeded simultaneous processing limit.");
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::Net::NetException &e){
		*errorno = 799; // undefined network error
		(logger).fatal("lmio: mimi_open failed: %s",e.displayText());
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::TimeoutException &e){
		*errorno = 703; // timed out for establishing connection
		(logger).fatal("lmio: mimi_open failed: timed out. %s",e.displayText());
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const Poco::FileNotFoundException &e){
		*errorno = 601; // SSL client context error
		(logger).fatal("lmio: mimi_open failed: Client context error, client cert file not found.");
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const mimiio::encoder::EncoderInitException &e){
		*errorno = 501;
		(logger).fatal("lmio: mimi_open failed: Encoder initialization error: %s", e.what());
		//(logger).getChannel()->close();
		return nullptr;
	}catch(const std::exception &e){
		*errorno = 101; // unknown error
		(logger).fatal("lmio: mimi_open failed: %s",std::string(e.what()));
		//(logger).getChannel()->close();
		return nullptr;
	}catch(...){
		*errorno = 101; // unknown error
		(logger).fatal("lmio: mimi_open failed with unknown reason.");
		//(logger).getChannel()->close();
		return nullptr;
	}
}

int mimi_start(MIMI_IO* mio)
{
	return mio->mt_->start();
}

bool mimi_is_active(MIMI_IO* mio)
{
	return mio->mt_->isActive();
}

MIMIIO_STREAM_STATE mimi_stream_state(MIMI_IO* mio)
{
	return mio->mt_->streamState();
}


void mimi_close(MIMI_IO* mio)
{
	if(mio != nullptr){
		delete mio;
	}
}

int mimi_error(MIMI_IO *mio)
{
	return mio->mt_->errorno();
}

const char* mimi_strerror(int errorno)
{
	return mimiio::strerror(errorno);
}

const char* mimi_version()
{
	return PACKAGE_VERSION;
}


/*
int mimi_send(MIMI_IO* mio, char* buffer, size_t len)
{
	std::vector<char> b(buffer, buffer+len);
	return mio->mt_->send(b);
}

void mimi_break(MIMI_IO* mio)
{
	mio->mt_->send_break();
}

int mimi_recv(MIMI_IO* mio, char* result, size_t len, bool blocking)
{
	std::vector<char> buffer;
	int n = mio->mt_->receive(buffer, blocking);
	std::memcpy(result, &buffer[0], len);
	return ((n < len)? n : len);
}
*/
