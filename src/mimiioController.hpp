/**
 * @file mimiioController.hpp
 * @brief Controller base class for APIs.
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef LIBMIMIIO_MIMIIOIMPLTHREAD_HPP__
#define LIBMIMIIO_MIMIIOIMPLTHREAD_HPP__

#include "typedef.hpp"
#include "mimiioImpl.hpp"
#include "mimiioEncoderFactory.hpp"
#include <Poco/ThreadPool.h>
#include <Poco/Logger.h>

namespace mimiio{

class mimiioController
{
public:

	/**
	 * @brief C'tor with mimiioImpl class, mimi(R) API implementation class
	 *
	 * @param [in] mimiioImpl mimiio implementation class
	 * @param [in] encoderFactory Audio encoder factory
	 * @param [in] logger logger
	 */
	mimiioController(mimiioImpl* impl, encoder::Encoder* encoder, Poco::Logger& logger);

	/**
	 * @brief D'tor and release all subsequent resources
	 */
	virtual ~mimiioController();

	/**
	 * @brief Activate a mimi connection
	 * @attention Subclass must override this function.
	 */
	virtual int start() = 0;

	/**
	 * @brief Determine a mimi connection is active or not.
	 * @attention Subclass must override this function.
	 *
	 * @return Return true if the connection is active.
	 */
	virtual bool isActive() const = 0;


	/**
	 * @brief Get stream state.
	 *
	 * @return Returns current mimiio stream state.
	 */
	virtual MIMIIO_STREAM_STATE streamState() const = 0;

	/**
	 * @brief Send audio data to mimi(R) remote host.
	 * @attention Synchronous API. Asynchronous callback API should be used for maximum performance and stability.
	 * @todo Synchronous API is not supported for using Encoder internally.
	 *
	 * @param [in] buffer audio data
	 * @return size of sent data
	 */
	virtual int send(const std::vector<char>& buffer);

	/**
	 * @brief Send break command to mimi(R) remote host.
	 * @attention Synchronous API. Asynchronous callback API should be used for maximum performance and stability.
	 * @todo Synchronous API is not supported for using Encoder internally.
	 *
	 * One MUST send break command immediately after the audio stream is considerably paused otherwise the recognition result would be not fixed. See <em> mimi(R) WebSocket API service </em> in detail about break command.
	 */
	virtual void send_break();

	/**
	 * @brief Receive result from mimi(R) remote host.
	 * @attention Synchronous API.  Asynchronous callback API should be used for maximum performance and stability.
	 * @todo Synchronous API is not supported for using Encoder internally.
	 *
	 * Note that one should check error state with using mimi_error() after each call of this function.
	 *
	 * @param [in] buffer The buffer for received result
	 * @param [in] blocking Set the mimi_recv() in blocking mode if \e blocking is true, disable blocking mode if \e blocking is false.
	 * @return actual size of received data.
	 */
	virtual int receive(std::vector<char>& buffer, bool blocking);

	/**
	 * @brief Get errorno
	 *
	 * @return error number
	 */
	int errorno() const { return errorno_; }

protected:
	mimiioImpl::Ptr impl_;
	encoder::Encoder::Ptr encoder_;
	Poco::Logger& logger_;
	int errorno_;
	bool started_; // for streamState();

private:

	mimiioController(mimiioController const&) = delete;
	mimiioController(mimiioController &&) = delete;
	mimiioController& operator = (mimiioController const&) = delete;
	mimiioController& operator = (mimiioController&&) = delete;

};

}

#endif
