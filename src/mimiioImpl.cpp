/**
 * @file mimiioImpl.cpp
 * @brief mimi(R) WebSocket API implementation class.
 * @author Copyright (c) 2014-2017 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "mimiioImpl.hpp"
#include "strerror.hpp"
#include "config.h"

#include <Poco/Net/WebSocket.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/HTTPDigestCredentials.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/Context.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/RejectCertificateHandler.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/KeyConsoleHandler.h>
#include <Poco/Net/ConsoleCertificateHandler.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/SecureStreamSocket.h>
#include <Poco/Net/X509Certificate.h>
#include <Poco/Net/VerificationErrorArgs.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/OAuth20Credentials.h>
#include <Poco/Exception.h>
#include <Poco/Format.h>
#include <Poco/DateTime.h>
#include <Poco/Buffer.h>

#include <exception>
#include <cstdio>
#include <sstream>
#include <iostream>

namespace mimiio{

const long socket_connect_timeout_sec_ = 30; //!< Timeout for connecting remote host
const long socket_send_timeout_sec_ = 30;    //!< Timeout for sending in socket
const long socket_recv_timeout_sec_ = 30;    //!< Timeout for receiving in socket

mimiioImpl::mimiioImpl(const std::string& hostname,
					   int port,
					   std::vector<MIMIIO_HTTP_REQUEST_HEADER>& requestHeaders,
					   const std::string& accessToken,
					   Poco::Logger& logger) :
					   hostname_(hostname),
					   port_(port),
					   closed_(false),
					   logger_(logger),
					   ws_(nullptr)
{
	//Initialize SSL
	Poco::Net::initializeSSL();
    Poco::SharedPtr<Poco::Net::PrivateKeyPassphraseHandler> ph1 = new NoopPrivateKeyPassphraseHandler(false);
    Poco::SharedPtr<Poco::Net::InvalidCertificateHandler> ph2 = new NotifyAndRejectCertificateHandler(false);
#ifdef WITH_SSL_DEFAULT_CERT
    Poco::Net::Context::Ptr ptrContext = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", SSL_DEFAULT_CERT, Poco::Net::Context::VERIFY_RELAXED, 9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
#elif __ANDROID__
    Poco::Net::Context::Ptr ptrContext = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "/etc/security/cacerts/b0f3e76e.0", Poco::Net::Context::VERIFY_RELAXED, 9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
#else
    Poco::Net::Context::Ptr ptrContext = new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_RELAXED, 9, true, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
#endif
    ptrContext->enableSessionCache(true);
    Poco::Net::SSLManager::instance().initializeClient(ph1, ph2, ptrContext);

    //Prepare HTTP Session
    Poco::Net::HTTPSClientSession session(hostname_, port_, ptrContext);

    // Proxy Settings
    set_proxysettings(&session);

    Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, "/", Poco::Net::HTTPMessage::HTTP_1_1);
    Poco::Net::OAuth20Credentials oauth(accessToken);
    Poco::Net::HTTPResponse response;
	Poco::Timespan timeout_connect(mimiio::socket_connect_timeout_sec_,0); // set connection timeout
	session.setTimeout(timeout_connect);
	oauth.authenticate(request);
	if(requestHeaders.size() != 0){
		for(size_t i=0;i<requestHeaders.size();++i){
			request.set(std::string(requestHeaders[i].key),  std::string(requestHeaders[i].value));
		}
	}

	logger_.information("mimiio: WebSocket start connecting...");
	Poco::Net::WebSocket *ws = new Poco::Net::WebSocket(session, request, response);
	//Poco::Net::X509Certificate cert = session.serverCertificate(); // Poco bug
	//X509* px509 = reinterpret_cast<X509*>(static_cast<Poco::Net::WebSocketImpl*>(ws->impl())->peerCertificateX509()); // this patch won't be applied
	//Poco::Net::X509Certificate cert(px509);
    //Poco::DateTime e = cert.expiresOn();
    //std::string issuers = cert.issuerName();
    //std::string expiredate = Poco::format("%d-%d-%d",static_cast<int>(e.year()),static_cast<int>(e.month()),static_cast<int>(e.day()));
    //logger_.information("mimiio: SSL connection established. Issuers: %s, Expires on: %s",issuers, expiredate);
    ws_.reset(ws);
	Poco::Timespan timeout_send(mimiio::socket_send_timeout_sec_, 0);
	Poco::Timespan timeout_recv(mimiio::socket_recv_timeout_sec_, 0);
	ws_->setSendTimeout(timeout_send);
	ws_->setReceiveTimeout(timeout_recv);
	logger_.information("mimiio: WebSocket connection established.");
}


mimiioImpl::mimiioImpl(const std::string& hostname,
		       	   	   int port,
					   std::vector<MIMIIO_HTTP_REQUEST_HEADER>& requestHeaders,
		       	   	   Poco::Logger& logger) :
		       	   	   hostname_(hostname),
		       	   	   port_(port),
		       	   	   closed_(false),
		       	   	   logger_(logger)
{
	// Prepare HTTP context
	Poco::Net::HTTPClientSession session(hostname_, port_);

	// Proxy Settings
	set_proxysettings(&session);

	Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, "/", Poco::Net::HTTPMessage::HTTP_1_1);
	if(requestHeaders.size() != 0){
		for(size_t i=0;i<requestHeaders.size();++i){
			request.set(std::string(requestHeaders[i].key),  std::string(requestHeaders[i].value));
		}
	}
	Poco::Net::HTTPResponse response;
	Poco::Timespan timeout_connect(mimiio::socket_connect_timeout_sec_,0); // set connection timeout
	session.setTimeout(timeout_connect);

	// Open WebSocket connection
	logger_.information("mimiio: WebSocket start connecting...");
	ws_.reset(new Poco::Net::WebSocket(session, request, response));
	Poco::Timespan timeout_send(mimiio::socket_send_timeout_sec_, 0);
	Poco::Timespan timeout_recv(mimiio::socket_recv_timeout_sec_, 0);
	ws_->setSendTimeout(timeout_send);
	ws_->setReceiveTimeout(timeout_recv);
	poco_debug(logger_,"mimiio: send initialize command...");
	logger_.information("mimiio: WebSocket connection established.");
}


mimiioImpl::~mimiioImpl(){}

void mimiioImpl::set_blocking(bool blocking)
{
	poco_debug_f1(logger_, "mimiio: socket blocking mode is %b", blocking);
	ws_->setBlocking(blocking);
}

void mimiioImpl::send_command(const std::string& command)
{
	std::string sc = Poco::format("{\"command\":\"%s\"}",command);
	send_frame(sc);
}

void mimiioImpl::send_break()
{
	send_command("recog-break");
}

int mimiioImpl::send_frame(const std::string& data)
{
	//send text frame
	poco_debug_f2(logger_, "mimiio: tx(text) = %s, %z byte", data, data.size());
	return ws_->sendFrame(data.c_str(), data.size(), Poco::Net::WebSocket::FRAME_TEXT);
}

int mimiioImpl::send_frame(const std::vector<char>& buffer, size_t len)
{
	//send binary frame
	//poco_debug_f1(logger_,"mimiio: tx(binary) = %z byte",len*2);
	//return ws_->sendFrame(&buffer[0], len*2, Poco::Net::WebSocket::FRAME_BINARY);
	poco_debug_f1(logger_,"mimiio: tx(binary) = %z byte",len);
	return ws_->sendFrame(&buffer[0], len, Poco::Net::WebSocket::FRAME_BINARY);
}

int mimiioImpl::receive_frame(std::vector<char> &buffer, OPF_TYPE& opframe, short& closeStatus)
{
	//POCO 1.4x,1.5x malfunction, could not handle PING/PONG response appropriately.
	//Must be patched the malfunction otherwise this function cause incomplete frame received exception.
	int flags = 0;
	//int n = ws_->receiveFrame(&buffer[0], buffer.size(), flags);	
	Poco::Buffer<char> tmpbuffer(0);
	tmpbuffer.clear();
	int n = ws_->receiveFrame(tmpbuffer, flags);
	buffer.clear();
	buffer.resize(tmpbuffer.size());
	for(size_t i=0;i<tmpbuffer.size();++i){ // NOT EFFICIENT! it should be modified.
		buffer[i] = tmpbuffer[i];
	}
		
	opframe = mimiioImpl::NA;    // type of received frame
	closeStatus = 0;
	if(n != 0){
		if(flags == (Poco::Net::WebSocket::FRAME_FLAG_FIN|Poco::Net::WebSocket::FRAME_OP_CLOSE)){
			//CLOSE FRAME received. According to RFC 6455 section 5.5.1, the close frame MAY contain a body
			//that indicates a reason for closing which defined at section 7.4.1 and implemented in strerror.hpp of above 1000 error code.
			//mimi(R) service hosts MUST send back a reason code for closing according to the specification.
			short statusCode = 0;
			((char *)&statusCode)[0] = buffer[1]; // The first two bytes of the body MUST be a 2-byte unsigned integer (in network byte order) representing a status code defined in Section 7.4.
			((char *)&statusCode)[1] = buffer[0];
			closeStatus = statusCode;
			poco_debug_f3(logger_, "mimiio: rx(text) : close frame received: status = %hd, %d byte (%d)", statusCode, n, flags);
			//close response
			//according to RFC 6455 client must send back close_frame to the server with/without the status code received from the server.
			char rst[2];
			rst[0] = ((char*)&statusCode)[1];
			rst[1] = ((char*)&statusCode)[0];
			ws_->sendFrame(rst,2,Poco::Net::WebSocket::FRAME_FLAG_FIN|Poco::Net::WebSocket::FRAME_OP_CLOSE);
			poco_debug(logger_, "mimiio: tx(text) : close frame responded.");
			opframe = mimiioImpl::CLOSE_FRAME;
			closed_ = true; // close frame from server.
		}else{
			//normal operation
			std::string fb(buffer.begin(), buffer.end());
			if(flags == 129){
				//text frame received
				opframe = mimiioImpl::TEXT_FRAME;
				poco_debug_f3(logger_, "mimiio: rx(text) = %s, %d byte (%d)", fb, n, flags);
			}else if(flags == 130){
				//binary frame received
				opframe = mimiioImpl::BINARY_FRAME;
				poco_debug_f3(logger_, "mimiio: rx(binary) = %s, %d byte (%d)", fb, n, flags);
			}
		}
	}else{
		//n == 0
		if(flags == (Poco::Net::WebSocket::FRAME_FLAG_FIN|Poco::Net::WebSocket::FRAME_OP_PING)){
			//PING packet received. mimi(R) service host MAY not send a ping packet, but libmimiio SHOULD treat ping packet appropriately.
			poco_debug(logger_,"mimiio: rx(ping)");
			//Pong response
			ws_->sendFrame(NULL,0,Poco::Net::WebSocket::FRAME_FLAG_FIN|Poco::Net::WebSocket::FRAME_OP_PONG);
			poco_debug(logger_,"mimiio: tx(pong)");
			opframe = mimiioImpl::PING_FRAME;
		}else if(flags == (Poco::Net::WebSocket::FRAME_FLAG_FIN|Poco::Net::WebSocket::FRAME_OP_CLOSE)){
			//zero length close frame; according to RFC 6455 client must send back close_frame to the server with/without the status code received from the server.
			//mimi(R) service host MUST NOT send back such zero length close frame, but libmimiio treat appropriately.
			poco_debug(logger_,"mimiio: rx(text) : close frame received with no status.");
			closeStatus = 0;
			//close response
			ws_->sendFrame(NULL,0,Poco::Net::WebSocket::FRAME_FLAG_FIN|Poco::Net::WebSocket::FRAME_OP_CLOSE);
			poco_debug(logger_,"mimiio: tx(text) : close frame responded.");
			opframe = mimiioImpl::CLOSE_FRAME;
			closed_ = true; // close frame from server.
		}else if(flags == 0){
			//above flags value of 0 means receiveFrame() function in Poco's WebSocket class has not update flags, which initialized 0 in this function.
			logger_.warning("mimiio: rx(n/a) WebSocket connection has already been closed.");
			closed_ = true; // ALREADY closed WITHOUT close frame. Unexpected network disconnection cause this state.
			std::string uflags = Poco::format("%d", static_cast<int>(flags));
			throw UnexpectedNetworkDisconnection(uflags);
		}else{
			//unknown flags, may not reach here.
			logger_.warning(Poco::format("mimiio: rx(n/a) unknown flags received : %d", flags));
			std::string uflags = Poco::format("%d", static_cast<int>(flags));
			throw UnknownFrameReceived(uflags);
		}
	}
	return n;
}

/*
	Reads proxy settings from 'https_proxy' environment variable.

        When using bash:
		1) With user authentication
			export https_proxy=http://<username>:<password>@<proxy server>:<port>/
		2) Without user authentication
			export https_proxy=http://<proxy server>:<port>/
*/
void mimiioImpl::set_proxysettings(Poco::Net::HTTPClientSession* pSession)
{
    const char* tmpEnvProxy = std::getenv("https_proxy");
    if (tmpEnvProxy != NULL) {
	    // Proxy Settings
	    std::string envProxy = std::getenv("https_proxy");
		if(envProxy.size() != 0) {
			logger_.information("mimiio: set proxysettings...");
			envProxy = Poco::replace(envProxy, "http://", "");
			envProxy = Poco::replace(envProxy, "https://", "");
			envProxy = Poco::replace(envProxy, "/",  "");

			// Split of user information and host information.
			std::vector<std::string> vSplitAt;
			std::stringstream ssEnvProxy(envProxy);
			std::string buffer;
			while(std::getline(ssEnvProxy, buffer, '@')) {
				vSplitAt.push_back(buffer);
			}

			// Split into individual configuration information.
			std::vector<std::string> vSplitParam;
			while(vSplitAt.size() != 0) {
			    std::string proxyParam = vSplitAt[vSplitAt.size()-1];
				vSplitAt.pop_back();

				std::stringstream ssProxyParam(proxyParam);
				while( std::getline(ssProxyParam, buffer, ':') ) {
					vSplitParam.push_back(buffer);
				}
			}

			// Proxy information generation
			Poco::Net::HTTPSClientSession::ProxyConfig proxy;
			for(int idx=0;idx<vSplitParam.size();++idx){
				switch(idx) {
					case 0:
						proxy.host=vSplitParam[idx];
						break;
					case 1:
						if (std::all_of(vSplitParam[idx].cbegin(), vSplitParam[idx].cend(), isdigit)) {
							proxy.port=stoi(vSplitParam[idx]);
						} else {
							// default: 80
						}
						break;
					case 2:
						proxy.username=vSplitParam[idx];
						break;
					case 3:
						proxy.password=vSplitParam[idx];
						break;
					default:
						break;
				}
			}

		    tmpEnvProxy = std::getenv("no_proxy");
		    if (tmpEnvProxy != NULL) {
			    std::string envProxy = std::getenv("no_proxy");
				if(envProxy.size() != 0){
					proxy.nonProxyHosts = Poco::replace(envProxy, ",", "|");
				}
			}

			// Setting proxy information for session
			pSession->setProxyConfig(proxy);

			logger_.information("Proxy Host: %s\n", proxy.host);
			logger_.information("Proxy Port: %d\n", (int)proxy.port);
			logger_.information("Proxy username: %s\n", proxy.username);
			logger_.information("Proxy password: %s\n", proxy.password);
			logger_.information("Proxy no_proxy: %s\n", proxy.nonProxyHosts);
		}
	}
}

}
