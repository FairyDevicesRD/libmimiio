/**
 * @file mimiioAsynchronousCallbackAPIController.hpp
 * @brief Controller class for asynchronous callback API
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#ifndef LIBMIMIIO_MIMIIOASYNCHRONOUSCALLBACKAPICONTROLLER_HPP_
#define LIBMIMIIO_MIMIIOASYNCHRONOUSCALLBACKAPICONTROLLER_HPP_

#include "mimiioController.hpp"
#include "worker/mimiioRxWorker.hpp"
#include "worker/mimiioTxWorker.hpp"
#include <Poco/Runnable.h>

namespace mimiio{
class mimiioImpl;

class mimiioAsynchronousCallbackAPIMonitor : public Poco::Runnable
{
public:

	typedef std::unique_ptr<mimiioAsynchronousCallbackAPIMonitor> Ptr;

	mimiioAsynchronousCallbackAPIMonitor(const worker::mimiioRxWorker::Ptr& rxWorker,
										 const worker::mimiioTxWorker::Ptr& txWorker,
										 int* errorno,
										 Poco::Logger& logger) :
										 rxWorker_(rxWorker),
										 txWorker_(txWorker),
										 errorno_(errorno),
										 finish_(false),
										 finished_(false),
										 logger_(logger)
	{
		poco_debug(logger_,"AsynchronousCallbackAPIMonitor: initialized.");
	}

	~mimiioAsynchronousCallbackAPIMonitor()
	{
		//poco_debug(logger_,"AsynchronousCallbackAPIMonitor: closed.");
	}

	/**
	 * @brief Set finish flag
	 *
	 * When this function calls, internal finish_ flag is true, then audio sending loop in run() is break.
	 */
	void finish(){ finish_ = true; }

	/**
	 * @brief Get finish flag, that is indicated audio sending loop is whether finished or not.
	 *
	 * @return true if audio sending loop is finished, otherwise false.
	 */
	bool finished() const { return finished_; }

	/**
	 * @brief Start audio-sending loop
	 *
	 * This loop is break when finish() is called.
	 */
	void run()
	{
		poco_debug(logger_,"AsynchronousCallbackAPIMonitor: loop starts.");
		while(!finish_){
			if(txWorker_->errorno() != 0 || rxWorker_->errorno() != 0){
				if(txWorker_->errorno() != 0){
					*errorno_ = txWorker_->errorno();
					poco_debug_f1(logger_, "AsynchronousCallbackAPIMonitor: txWorker error detected, errorno = %d", *errorno_);
				}else{
					*errorno_ = rxWorker_->errorno();
					poco_debug_f1(logger_, "AsynchronousCallbackAPIMonitor: rxWorker error detected, errorno = %d", *errorno_);
				}
				txWorker_->finish();
				rxWorker_->finish();
				break; // finish monitor
			}
			Poco::Thread::sleep(10);
		}
		poco_debug(logger_,"AsynchronousCallbackAPIMonitor: loop ends.");
		finished_ = true;
	}

private:
	const worker::mimiioRxWorker::Ptr &rxWorker_;
	const worker::mimiioTxWorker::Ptr &txWorker_;
	int* errorno_;
	bool finish_;
	bool finished_;
	Poco::Logger& logger_;
};

/**
 * @class mimiioAsynchronousCallbackAPIController
 * @brief Control class for asynchronous callback API
 * @see worker::mimiioRxWorker
 * @see worker::mimiioTxWorker
 */
class mimiioAsynchronousCallbackAPIController : public mimiioController
{
public:

	/**
	 * @brief C'tor with mimiioImpl class, mimi(R) API implementation class
	 *
	 * @param [in] mimiioImpl mimiio implementation class
	 * @param [in] txfunc user defined callback function which is called periodically by libmimiio txWorker thread for transfer audio
	 * @param [in] rxfunc user defined callback function which is called periodically by libmimiio rxWorker thread for receiving response from remote host.
	 * @param [in,out] userdata_for_tx user defined data for \e txfunc
	 * @param [in,out] userdata_for_rx user defined data for \e rxfunc
	 * @param [in] logger logger
	 */
	mimiioAsynchronousCallbackAPIController(
			mimiioImpl* impl,
			encoder::Encoder* encoder,
			ON_TX_CALLBACK_T txfunc,
			ON_RX_CALLBACK_T rxfunc,
			void* userdata_for_tx,
			void* userdata_for_rx,
			Poco::Logger& logger);

	/**
	 * @brief D'tor and release all subsequent resources
	 */
	virtual ~mimiioAsynchronousCallbackAPIController();

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

	mimiioAsynchronousCallbackAPIController(mimiioAsynchronousCallbackAPIController const&) = delete;
	mimiioAsynchronousCallbackAPIController(mimiioAsynchronousCallbackAPIController &&) = delete;
	mimiioAsynchronousCallbackAPIController& operator = (mimiioAsynchronousCallbackAPIController const&) = delete;
	mimiioAsynchronousCallbackAPIController& operator = (mimiioAsynchronousCallbackAPIController&&) = delete;

	Poco::ThreadPool tpool_;
	worker::mimiioRxWorker::Ptr rxWorker_;
	worker::mimiioTxWorker::Ptr txWorker_;
	mimiioAsynchronousCallbackAPIMonitor::Ptr monitor_;
};

}

#endif
