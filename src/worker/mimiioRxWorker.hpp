/**
 * @file mimiioRxWorker.hpp
 * @brief Multi-threaded response receiving class
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef LIBMIMIIO_MIMIIOIMPLRXWORKER_HPP__
#define LIBMIMIIO_MIMIIOIMPLRXWORKER_HPP__

#include "typedef.hpp"
#include <Poco/Runnable.h>
#include <memory>

/**
 * @namespace mimiio::worker
 * @brief Namespace for multi-threaded transfer/receiving functionalities.
 */
namespace mimiio{ class mimiioImpl; namespace worker{

/**
 * @class mimiioRxWorker
 * @brief Multi-threaded response receiving class
 */
class mimiioRxWorker : public Poco::Runnable
{
public:

	typedef std::unique_ptr<mimiioRxWorker> Ptr;

	/**
	 * @brief C'tor
	 *
	 * @param [in] impl mimiioImpl class, mimi(R) API implementation encapsulated.
	 * @param [in] logger logger
	 */
	mimiioRxWorker(const mimiioImpl::Ptr& impl, ON_RX_CALLBACK_T func, void* userdata, Poco::Logger& logger);

	/**
	 * @brief D'tor
	 */
	~mimiioRxWorker();

	/**
	 * @brief Set finish flag when response receiving is finished.
	 *
	 * When this function calls, internal finish_ flag is true, then response receiving loop in run() is break.
	 */
	void finish();

	/**
	 * @brief Get finish flag, that is indicated response receiving loop is whether finished or not.
	 *
	 * @return true if response receiving loop is finished, otherwise false.
	 */
	bool finished() const;

	/**
	 * @brief Get errorno in this class
	 *
	 * @return error number
	 */
	int errorno() const;

	/**
	 * @brief Start response receiving loop
	 *
	 * This loop is break when finish() is called.
	 */
	void run();

private:

	const mimiioImpl::Ptr& impl_;
	ON_RX_CALLBACK_T func_;
	void* userdata_;
	int errorno_;
	bool finish_;
	bool finished_;
	Poco::Logger& logger_;
};

}}

#endif
