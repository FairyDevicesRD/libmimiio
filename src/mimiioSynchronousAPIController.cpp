/**
 * @file mimiioSynchronousAPIController.cpp
 * @brief Controller class for synchronous API
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "mimiioSynchronousAPIController.hpp"

namespace mimiio{

mimiioSynchronousAPIController::mimiioSynchronousAPIController(mimiioImpl* impl, encoder::Encoder* encoder, Poco::Logger& logger) :
		mimiioController(impl, encoder, logger)
{
	poco_debug(logger_, "SynchronousAPIController: initialized.");
}

mimiioSynchronousAPIController::~mimiioSynchronousAPIController()
{
	poco_debug(logger_, "SynchronousAPIController: closed.");
}

bool mimiioSynchronousAPIController::isActive() const
{
	if(impl_->closed()){
		return false;
	}else{
		return true;
	}
}

MIMIIO_STREAM_STATE mimiioSynchronousAPIController::streamState() const
{
	if(!started_){
		return MIMIIO_STREAM_WAIT;
	}

	if(impl_->closed()){
		return MIMIIO_STREAM_CLOSED;
	}else{
		return MIMIIO_STREAM_BOTH;
	}
}

int mimiioSynchronousAPIController::start()
{
	poco_debug(logger_, "SynchronousAPIController: started.");
	started_ = true;
	return 0;
}

}



