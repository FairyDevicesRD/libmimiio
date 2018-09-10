/**
 * @file mimiioImpl.hpp
 * @brief mimi(R) WebSocket API implementation class.
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef LIBMIMIIO_MIMIIOIMPL_HPP__
#define LIBMIMIIO_MIMIIOIMPL_HPP__

#include "mimiio.h"
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/PrivateKeyPassphraseHandler.h>
#include <Poco/Net/InvalidCertificateHandler.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Logger.h>
#include <string>
#include <vector>
#include <memory>

namespace Poco{ namespace Net{ class WebSocket; } }

namespace mimiio{

/**
 * @class NoopPrivateKeyPassphraseHandler
 */
class NoopPrivateKeyPassphraseHandler : public Poco::Net::PrivateKeyPassphraseHandler
{
public:
	explicit NoopPrivateKeyPassphraseHandler(bool onServerSide): Poco::Net::PrivateKeyPassphraseHandler(onServerSide){}
	void onPrivateKeyRequested(const void* pSender, std::string& privateKey){}
};

/**
 * @class NotifyAndRejectCertificateHandler
 */
class NotifyAndRejectCertificateHandler : public Poco::Net::InvalidCertificateHandler
{
public:
	explicit NotifyAndRejectCertificateHandler(bool handleErrorsOnServerSide) : Poco::Net::InvalidCertificateHandler(handleErrorsOnServerSide){}
	void onInvalidCertificate(const void* pSender, Poco::Net::VerificationErrorArgs& errorCert)
	{
		errorCert.setIgnoreError(false);
		//throw Poco::Net::InvalidCertificateException(errorCert.errorMessage(), static_cast<int>(errorCert.errorNumber()));
	}
};

/**
 * @class UnknownFrameReceived
 * @brief libmimiio only supports text frame, binary frame, ping frame and close frame. This exception is aroused when libmimiio receives other types of frame.
 */
class UnknownFrameReceived : public std::runtime_error
{
public:
	explicit UnknownFrameReceived(const std::string& s) : std::runtime_error(s){}
};

/**
 * @class UnexpectedNetworkDisconnection
 * @brief This exception may be aroused by receive_frame() when Poco's receive_frame function returned 0 and also flags = 0, which indicates WebSocket connection has been already vanished.
 */
class UnexpectedNetworkDisconnection : public std::runtime_error
{
public:
	explicit UnexpectedNetworkDisconnection(const std::string& s) : std::runtime_error(s){}
};

/**
 * @class mimiioImpl
 * @brief mimi(R) WebSocket API implementation class.
 *
 * All mimi-specific communication procedure defined in the API are encapsulated in this class.
 *
 * @see detail::mimiioRxWorker
 * @see detail::mimiioTxWorker
 */
class mimiioImpl{

public:

	typedef std::unique_ptr<mimiioImpl> Ptr;

	/**
	 * @brief WebSocket container type defined in RFC 6445
	 */
	typedef enum {
		TEXT_FRAME,   //!< text frame
		BINARY_FRAME, //!< binary frame
		PING_FRAME,   //!< ping frame
		CLOSE_FRAME,  //!< close frame
		NA            //!< no frame received which means WebSocket connection has already been closed
	}OPF_TYPE;

	/**
	 * @brief C'tor. Connect to mimi(R) WebSocket API service version 2.0 with access token
	 *
	 * @param [in] hostname mimi(R) remote host
	 * @param [in] port mimi(R) remote port
	 * @param [in] requestHeaders HTTP request headers which is sent with WebSocket upgrade request.
	 * @param [in] accessToken access token
	 * @param [in] logger logger
	 */
	mimiioImpl(const std::string& hostname,
			   int port,
			   std::vector<MIMIIO_HTTP_REQUEST_HEADER>& requestHeaders,
			   const std::string& accessToken,
			   Poco::Logger& logger);

	/**
	 * @brief C'tor. Connect to mimi(R) WebSocket API service version 2.0
	 *
	 * @param [in] hostname mimi(R) remote host
	 * @param [in] port mimi(R) remote port
	 * @param [in] requestHeaders HTTP request headers which is sent with WebSocket upgrade request.
	 * @param [in] logger logger
	 */
	mimiioImpl(const std::string& hostname,
			   int port,
			   std::vector<MIMIIO_HTTP_REQUEST_HEADER>& requestHeaders,
			   Poco::Logger& logger);

	/**
	 * @brief D'tor
	 */
	~mimiioImpl();

	/**
	 * @brief connection is closed or not
	 *
	 * @return true if connection is closed, otherwise false.
	 */
	bool closed() const { return closed_; }

	/**
	 * @brief Send break command to mimi(R) service
	 */
	void send_break();

	/**
	 * @brief Send audio data to mimi(R) service
	 *
	 * @param [in] buffer audio buffer
	 * @param [in] len length of buffer
	 * @return Returns the number of bytes sent, which may be less than the number of bytes specified.
	 */
	int send_frame(const std::vector<char>& buffer, size_t len);

	/**
	 * @brief Receive response from mimi(R) service
	 *
	 * @param [out] buffer response from the mimi(R) service
	 * @param [out] WebSocket frame type
	 * @param [out] close frame status code
	 * @return size of received bytes
	 */
	int receive_frame(std::vector<char>& buffer, OPF_TYPE& opc, short& closeStatus);

	/**
	 * @brief Set socket mode in blocking
	 *
	 * This function is for synchronous API. In asynchronous callback API socket mode is always in blocking.
	 *
	 * @param [in] blocking If it's set true, socket mode is in blocking, false is for non-blocking.
	 */
	void set_blocking(bool blocking);

private:

	void send_command(const std::string& command);

	int send_frame(const std::string& data);

	void set_proxysettings(Poco::Net::HTTPClientSession* session);

	//for reconnection
	const std::string hostname_;
	const int port_;
	const std::string accessToken;
	bool closed_;

	Poco::Logger& logger_;
	std::unique_ptr<Poco::Net::WebSocket> ws_;
};

}
#endif
