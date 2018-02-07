/**
 * @file mimiioAsynchronousCallbackAPIController.cpp
 * @brief Controller class for asynchronous callback API
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "mimiioAsynchronousCallbackAPIController.hpp"

namespace mimiio{

mimiioAsynchronousCallbackAPIController::mimiioAsynchronousCallbackAPIController(
		mimiioImpl* impl,
		encoder::Encoder* encoder,
		ON_TX_CALLBACK_T txfunc,
		ON_RX_CALLBACK_T rxfunc,
		void* userdata_for_tx,
		void* userdata_for_rx,
		Poco::Logger& logger) :
		mimiioController(impl, encoder, logger),
		rxWorker_(new worker::mimiioRxWorker(impl_, rxfunc, userdata_for_rx, logger)),
		txWorker_(new worker::mimiioTxWorker(impl_, encoder_, txfunc, userdata_for_tx, logger)),
		monitor_(new mimiioAsynchronousCallbackAPIMonitor(rxWorker_, txWorker_, &errorno_, logger))
{
	poco_debug(logger_, "AsynchronousCallbackAPIController: initialized.");
}

mimiioAsynchronousCallbackAPIController::~mimiioAsynchronousCallbackAPIController()
{
	txWorker_->finish(); // if isActive() == true, following 2 lines mean force termination, otherwise they have no effect because both tx and rxWorker have already finished.
	rxWorker_->finish();
	monitor_->finish();
	tpool_.joinAll();
	//poco_debug(logger_,"AsynchronousCallbackAPIController: Asynchronous callback API closed.");
}

bool mimiioAsynchronousCallbackAPIController::isActive() const
{
	if(txWorker_->finished() && rxWorker_->finished()){
		return false;
	}else{
		return true;
	}
}

MIMIIO_STREAM_STATE mimiioAsynchronousCallbackAPIController::streamState() const
{
	if(!started_){
		return MIMIIO_STREAM_WAIT;
	}

	if(txWorker_->finished() && rxWorker_->finished()){
		return MIMIIO_STREAM_CLOSED;
	}else if(txWorker_->finished()){
		return MIMIIO_STREAM_RECV;
	}else if(rxWorker_->finished()){
		return MIMIIO_STREAM_SEND;
	}else{
		return MIMIIO_STREAM_BOTH;
	}
}


int mimiioAsynchronousCallbackAPIController::start()
{
	try{
		poco_debug(logger_, "AsynchronousCallbackAPIController: Asynchronous callback API starts");
		tpool_.start(*(monitor_.get()));
		tpool_.start(*(txWorker_.get()));
		tpool_.start(*(rxWorker_.get()));
		started_ = true;
		return 0;
	}catch(std::exception &e){
		logger_.fatal("AsynchronousCallbackAPIController: Could not start API (905); %s", std::string(e.what()));
		return 905;
	}
}


}


