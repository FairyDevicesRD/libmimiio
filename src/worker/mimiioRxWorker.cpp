/**
 * @file mimiioRxWorker.cpp
 * @brief Multi-threaded response receiving class implementation
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "strerror.hpp"
#include "mimiioImpl.hpp"
#include "worker/mimiioRxWorker.hpp"
#include <Poco/Thread.h>
#include <Poco/Format.h>
#include <Poco/Net/NetException.h>
#include <cstdlib>
#include <iostream>

namespace mimiio{ namespace worker{

mimiioRxWorker::mimiioRxWorker(const mimiioImpl::Ptr& impl, ON_RX_CALLBACK_T func, void* userdata, Poco::Logger& logger) :
		impl_(impl),
		func_(func),
		userdata_(userdata),
		errorno_(0),
		finish_(false),
		finished_(false),
		logger_(logger)
{
	poco_debug(logger_, "lmio: rxWorker: initialized.");
}

mimiioRxWorker::~mimiioRxWorker()
{
	//poco_debug(logger_, "lmio: rxWorker: finished.");
}

void mimiioRxWorker::finish()
{
	finish_ = true;
}

bool mimiioRxWorker::finished() const
{
	return finished_;
}

int mimiioRxWorker::errorno() const
{
	return errorno_;
}

void mimiioRxWorker::run()
{
	while(!finish_){
	    std::vector<char> buffer;
		try{
			if(impl_->closed()){
				break; //break rx loop
			}

			short closeStatus = 0;
			mimiioImpl::OPF_TYPE opc;
			int n = impl_->receive_frame(buffer, opc, closeStatus);	//Note that this function is Blocking I/O

			if(opc == mimiioImpl::PING_FRAME){
				continue;
			}else if(opc == mimiioImpl::CLOSE_FRAME){
				if(n == 0){
					errorno_ = 904;
					logger_.warning("lmio: rxWorker1: %s (%d)", mimiio::strerror(errorno_), errorno_);
					break; //break rx loop
				}else{
					if(closeStatus == 1000){
						poco_debug(logger_, "lmio: rxWorker2: Close frame received by remote host with code 1000, finish rxWorker normally.");
						break; //break rx loop
					}else{
						//pass through server side's error code.
						errorno_ = static_cast<int>(closeStatus);
						logger_.warning("lmio: rxWorker3: %s (%d), terminate rxWorker.", mimiio::strerror(errorno_), errorno_);
						break; //break rx loop
					}
				}
			}

			//receive callback (only for text or binary frame)
			int rxfunc_error = 0;
			if(opc == mimiioImpl::TEXT_FRAME){
				if(n == 0){
					errorno_ = 906;
					logger_.warning("lmio: rxWorker: %s (%d)", mimiio::strerror(errorno_), errorno_);
					break; //break rx loop
				}else{
					std::string s(buffer.begin(), buffer.end()); //for null termination
					func_(s.c_str(), buffer.size(), &rxfunc_error, userdata_);
				}
			}else{
				if(n == 0){
					errorno_ = 907;
					logger_.warning("lmio: rxWorker: %s (%d)", mimiio::strerror(errorno_), errorno_);
					break; //break rx loop
				}else{
					func_(&buffer[0], buffer.size(), &rxfunc_error, userdata_);
				}
			}
			if(rxfunc_error != 0){
				errorno_ = rxfunc_error;
				logger_.fatal("lmio: rxWorker: User defined error occurred in mimi_rxfunc callback (%d)", errorno_);
				break; //break tx loop
			}
			Poco::Thread::sleep(1); // avoid busy loop, even if Poco's receive_frame() is set non-blocking mode.
		}catch(const Poco::Net::WebSocketException &e){
			errorno_ = 800 + static_cast<int>(e.code());
			logger_.fatal("lmio: rxWorker: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
			break;
		}catch(const Poco::TimeoutException &e){
			errorno_ = 830; // timeout
			logger_.fatal("lmio: rxWorker: Timeout exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
			break;
		}catch(const Poco::Net::NetException &e){
			errorno_ = 790; // network error
			logger_.error("lmio: rxWorker: Network exception: %s (%d)", e.displayText(), errorno_);
			break;
		}catch(const UnknownFrameReceived &e){
			errorno_ = 890; // unknown flag received
			logger_.fatal("lmio: rxWorker: WebSocket exception: %s (%d), terminate rxWorker.", mimiio::strerror(errorno_), errorno_);
			break;
		}catch(const UnexpectedNetworkDisconnection &e){
			errorno_ = 791; // unexpected network disconnection
			logger_.fatal("lmio: rxWorker: Network exception: %s (%d), terminate rxWorker.", mimiio::strerror(errorno_), errorno_);
			break;
		}catch(const std::exception &e){
			errorno_ = 799; // undefined network error
			logger_.fatal("lmio: rxWorker: Unknown error, std exception: %s, terminate rxWorker.", std::string(e.what()));
			break;
		}catch(...){
			errorno_ = 799; // undefined network error
			logger_.fatal("lmio: rxWorker: Unknown error, terminate rxWorker.");
			break;
		}
	}//while
	logger_.information("lmio: rxWorker: rx loop finished with code %d",errorno_);
	finished_ = true;
}

}}
