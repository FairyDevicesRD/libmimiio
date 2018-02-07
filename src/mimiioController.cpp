/**
 * @file mimiioController.cpp
 * @brief Controller base class for APIs.
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "mimiioController.hpp"
#include "strerror.hpp"

namespace mimiio{

mimiioController::mimiioController(mimiioImpl* impl, encoder::Encoder* encoder, Poco::Logger& logger)  :
		impl_(impl),
		encoder_(encoder),
		logger_(logger),
		errorno_(0),
		started_(false)
{
	poco_debug(logger_, "mimiioController: initialized.");
}

mimiioController::~mimiioController()
{
	poco_debug(logger_, "mimiioController: closed.");
	//logger_.getChannel()->close();
}

int mimiioController::send(const std::vector<char>& buffer)
{
	try{
		if(impl_->closed()){
			logger_.warning("mimiioController: mimi connection is already closed.");
			return 0;
		}
		poco_debug_f1(logger_, "mimiioController: send (%d bytes)", buffer.size());
		return impl_->send_frame(buffer, buffer.size());
	}catch(const Poco::Net::WebSocketException &e){
		errorno_ = 800 + static_cast<int>(e.code());
		logger_.error("mimiioController: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
		return 0;
	}catch(const Poco::TimeoutException &e){
		errorno_ = 830; //timeout;
		logger_.error("mimiioController: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
		return 0;
	}catch(const Poco::Net::NetException &e){
		errorno_ = 790; // network error
		logger_.error("mimiioController: Network exception: %s (%d)", e.displayText(), errorno_);
		return 0;
	}catch(const std::exception &e){
		errorno_ = 799; // undefined network error
		logger_.error("mimiioController: Unknown error, std exception %s", e.what());
		return 0;
	}catch(...){
		errorno_ = 799; // undefined network error
		logger_.error("mimiioController: Unknown error.");
		return 0;
	}
}

void mimiioController::send_break()
{
	try{
		if(impl_->closed()){
			logger_.warning("mimiioController: mimi connection is already closed.");
			return;
		}
		poco_debug(logger_, "mimiioController: send break");
		impl_->send_break();
	}catch(const Poco::Net::WebSocketException &e){
		errorno_ = 800 + static_cast<int>(e.code());
		logger_.error("mimiioController: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
	}catch(const Poco::TimeoutException &e){
		errorno_ = 830; //timeout;
		logger_.error("mimiioController: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
	}catch(const Poco::Net::NetException &e){
		errorno_ = 790; // network error
		logger_.error("mimiioController: Network exception: %s (%d)", e.displayText(), errorno_);
	}catch(const std::exception &e){
		errorno_ = 799; // undefined network error
		logger_.error("mimiioController: Unknown error, std exception %s", e.what());
	}catch(...){
		errorno_ = 799; // undefined network error
		logger_.error("mimiioController: Unknown error.");
	}
}

int mimiioController::receive(std::vector<char>& buffer, bool blocking)
{
	try{
		impl_->set_blocking(blocking);
		if(impl_->closed()){
			logger_.warning("mimiioController: mimi connection is already closed.");
			return 0;
		}
		mimiioImpl::OPF_TYPE opc;
		short closeStatus = 0;
		int n = impl_->receive_frame(buffer, opc, closeStatus);
		poco_debug_f1(logger_, "mimiioController: receive (%d bytes)", n);
		if(opc == mimiioImpl::PING_FRAME){
			return 0; // libmimiio user don't have to care about ping/pong response
		}else if(opc == mimiioImpl::CLOSE_FRAME){
			if(n == 0){
				errorno_ = 904;
				logger_.warning("mimiioController: %s (%d)", mimiio::strerror(errorno_), errorno_);
			}else{
				//pass through server side's error code.
				errorno_ = static_cast<int>(closeStatus);
				logger_.error("mimiioController: %s (%d)", mimiio::strerror(errorno_), errorno_);
			}
			return 0; // libmimiio user don't have to care about close status, because it's copied to errorno_ in this class.
		}else{ // for text frame or binary frame
			return n;
		}
	}catch(const Poco::Net::WebSocketException &e){
		errorno_ = 800 + static_cast<int>(e.code());
		logger_.error("mimiioController: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
		return 0;
	}catch(const Poco::TimeoutException &e){
		errorno_ = 830; // timeout
		logger_.error("mimiioController: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
		return 0;
	}catch(const Poco::Net::NetException &e){
		errorno_ = 790; // network error
		logger_.error("mimiioController: Network exception: %s (%d)", e.displayText(), errorno_);
		return 0;
	}catch(const UnknownFrameReceived &e){
		errorno_ = 890; // unknown flag received
		logger_.error("mimiioController: WebSocket exception: %s (%d), terminate rxWorker.", mimiio::strerror(errorno_), errorno_);
		return 0;
	}catch(const UnexpectedNetworkDisconnection &e){
		errorno_ = 791; // unexpected network disconnection
		logger_.error("mimiioController: Network exception: %s (%d), terminate rxWorker.", mimiio::strerror(errorno_), errorno_);
		return 0;
	}catch(const std::exception &e){
		errorno_ = 799; // undefined network error
		logger_.error("mimiioController: Unknown error, std exception: %s, terminate rxWorker.", e.what());
		return 0;
	}catch(...){
		errorno_ = 799; // undefined network error
		logger_.error("mimiioController: Unknown error, terminate rxWorker.");
		return 0;
	}
}

}


