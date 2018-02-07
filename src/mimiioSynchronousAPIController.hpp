/**
 * @file mimiioSynchronousAPIController.hpp
 * @brief Controller class for synchronous API
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef LIBMIMIIO_MIMIIOSYNCHRONOUSAPICONTROLLER_HPP_
#define LIBMIMIIO_MIMIIOSYNCHRONOUSAPICONTROLLER_HPP_

#include "mimiioController.hpp"

namespace mimiio{
class mimiioImpl;


/**
 * @class mimiioSynchronousAPIController
 * @brief Control class for synchronous API.
 * @attention Synchronous API. Asynchronous callback API should be used for maximum performance and stability.
 */
class mimiioSynchronousAPIController : public mimiioController
{
public:

	/**
	 * @brief C'tor with mimiioImpl class, mimi(R) API implementation class
	 *
	 * @param [in] mimiioImpl mimiio implementation class
	 * @param [in] logger logger
	 */
	mimiioSynchronousAPIController(mimiioImpl* impl, encoder::Encoder* encoder, Poco::Logger& logger);

	/**
	 * @brief D'tor and release all subsequent resources
	 */
	virtual ~mimiioSynchronousAPIController();

	/**
	 * @brief Determine a mimi connection is active or not.
	 * @attention Subclass must override this function.
	 *
	 * @return Return true if the connection is active.
	 */
	virtual bool isActive() const;

	/**
	 * @brief Get current stream state.
	 *
	 * @return Return current stream state.
	 */
	virtual MIMIIO_STREAM_STATE streamState() const;

	/**
	 * @brief Start communication threads
	 */
	virtual int start();

private:

	mimiioSynchronousAPIController(mimiioSynchronousAPIController const&) = delete;
	mimiioSynchronousAPIController(mimiioSynchronousAPIController &&) = delete;
	mimiioSynchronousAPIController& operator = (mimiioSynchronousAPIController const&) = delete;
	mimiioSynchronousAPIController& operator = (mimiioSynchronousAPIController&&) = delete;
};

}

#endif
