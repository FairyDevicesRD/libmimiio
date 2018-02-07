/**
 * @file mimiioTxWorker.hpp
 * @brief Implementation for audio transfer thread
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef LIBMIMIIO_MIMIIOIMPLTXWORKER_HPP__
#define LIBMIMIIO_MIMIIOIMPLTXWORKER_HPP__

#include "typedef.hpp"
#include "encoder/encoder.hpp"
#include <Poco/Runnable.h>
#include <memory>

namespace mimiio{ class mimiioImpl; namespace worker{

class mimiioTxWorker : public Poco::Runnable
{
public:

	typedef std::unique_ptr<mimiioTxWorker> Ptr;

	/**
	 * @brief C'tor
	 *
	 * @param [in] impl mimiioImpl class, mimi(R) API implementation encapsulated.
	 * @param [in] encoder Audio encoder
	 * @param [in] func txfunc
	 * @param [in] userdata User defined data for txfunc
	 * @param [in] logger logger
	 */
	mimiioTxWorker(
			const mimiioImpl::Ptr& impl,
			const encoder::Encoder::Ptr& encoder,
			ON_TX_CALLBACK_T func,
			void* userdata,
			Poco::Logger& logger);

	/**
	 * @brief D'tor
	 */
	~mimiioTxWorker();

	/**
	 * @brief Set finish flag when audio sending is finished.
	 *
	 * When this function calls, internal finish_ flag is true, then audio sending loop in run() is break.
	 */
	void finish();

	/**
	 * @brief Get finish flag, that is indicated audio sending loop is whether finished or not.
	 *
	 * @return true if audio sending loop is finished, otherwise false.
	 */
	bool finished() const;

	/**
	 * @brief Get errorno in this class
	 *
	 * @return error number
	 */
	int errorno() const;

	/**
	 * @brief Start audio-sending loop
	 *
	 * This loop is break when finish() is called.
	 */
	void run();

private:

	const mimiioImpl::Ptr& impl_;
	const encoder::Encoder::Ptr& encoder_;
	ON_TX_CALLBACK_T func_;
	void* userdata_;
	int errorno_;
	bool finish_;
	bool finished_;
	Poco::Logger& logger_;
};

}}

#endif
