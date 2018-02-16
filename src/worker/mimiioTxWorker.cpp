/**
 * @file mimiioTxWorker.cpp
 * @brief Implementation for audio transfer thread
 * @author Copyright (c) 2014-2015 Fairy Devices Inc. http://www.fairydevices.jp/
 * @author Masato Fujino
 */

#include "strerror.hpp"
#include "mimiioImpl.hpp"
#include "worker/mimiioTxWorker.hpp"
#include <Poco/Thread.h>
#include <Poco/Format.h>
#include <Poco/Net/NetException.h>

namespace mimiio{ namespace worker{

const int maximum_send_buffer_size_ = 262144; //!< maximum send buffer size for WebSocket payload for a data frame.

mimiioTxWorker::mimiioTxWorker(
		const mimiioImpl::Ptr& impl,
		const encoder::Encoder::Ptr& encoder,
		ON_TX_CALLBACK_T func,
		void* userdata,
		Poco::Logger& logger) :
		impl_(impl),
		encoder_(encoder),
		func_(func),
		userdata_(userdata),
		errorno_(0),
		finish_(false),
		finished_(false),
		logger_(logger)
{
	poco_debug(logger_, "lmio: txWorker: initialized.");
}


mimiioTxWorker::~mimiioTxWorker()
{
	//poco_debug(logger_, "txWorker: finished.");
}

void mimiioTxWorker::finish()
{
	finish_ = true;
}

bool mimiioTxWorker::finished() const
{
	return finished_;
}

int mimiioTxWorker::errorno() const
{
	return errorno_;
}

void mimiioTxWorker::run()
{
	std::vector<char> buffer(mimiio::worker::maximum_send_buffer_size_);
	while(!finish_){
		try{
			if(impl_->closed()){
				break; // break tx loop
			}
			size_t len = 0;
			bool recog_break = false;
			int tx_error = 0;
			func_(&buffer[0], &len, &recog_break, &tx_error, userdata_); //user defined callback for tx audio (txfunc)
			if(tx_error != 0){
				errorno_ = tx_error;
				logger_.fatal("lmio: txWorker: user defined error occurred in txfunc callback (%d), terminate txWorker and sendBreak to remote host.",errorno_);
				// When user defined error occurred in txfunc, WebSocket connection may be OK so that rxWorker waits for timeout at impl_->receive().
				// It is better to close immediately, but client-initiated WebSocket closing is not good way according to WebSocket protocol,
				// We just send to server 'break' command so that the server will close the connection.
				impl_->send_break();
				break; // break tx loop
			}
			if(mimiio::worker::maximum_send_buffer_size_ < len){
				//Buffer overrun has occurred!
				//This might have caused destructive memory error, when you're enough happy to be nothing happened. libmimiio is shutdown immediately.
				errorno_ = 903;
				logger_.fatal("lmio: txWorker: %s (%d), terminate txWorker.", mimiio::strerror(errorno_), errorno_);
				break; // break tx loop
			}
			if(len == 0){
				//set just recog-break only
				if(recog_break){
					encoder_->Flush();
					std::vector<char> encodedData;
					encoder_->GetEncodedData(encodedData);
					if(encodedData.size() != 0){
						poco_debug_f1(logger_, "lmio: flush encoder and send data length = %d bytes (1).", static_cast<int>(encodedData.size()));
						impl_->send_frame(encodedData, encodedData.size()); //First, send audio data
					}
					impl_->send_break();
					poco_debug(logger_, "lmio: txWorker: sent recog-break, finish txWorker normally.");
					break;
				}
				Poco::Thread::sleep(100); // avoid busy loop with short time pause only when length is 0
				continue; //do nothing
			}

			//Audio encoding
			std::vector<char> slice;
			for(size_t i=0;i<len;++i){
				slice.push_back(buffer[i]);
			}
			encoder_->Encode(slice);
			std::vector<char> encodedData;
			encoder_->GetEncodedData(encodedData);
			if(encodedData.size() == 0 && !recog_break){
				poco_debug_f2(logger_, "lmio: encoder in=%d, out=%d", static_cast<int>(len), static_cast<int>(encodedData.size()));
				Poco::Thread::sleep(1); // avoid busy loop with short time pause
				continue;
			}
			poco_debug_f2(logger_, "lmio: encoder in=%d, out=%d", static_cast<int>(len), static_cast<int>(encodedData.size()));

			//Transmit audio data
			if(encodedData.size() != 0){
				impl_->send_frame(encodedData, encodedData.size()); //First, send audio data
			}
			if(recog_break){
				encodedData.clear();
				encoder_->Flush();
				encoder_->GetEncodedData(encodedData);
				if(encodedData.size() != 0){
					poco_debug_f1(logger_, "lmio: flush encoder and send data length = %d bytes (2).", static_cast<int>(encodedData.size()));
					impl_->send_frame(encodedData, encodedData.size()); //First, send audio data
				}
				impl_->send_break(); //Next, set recog-break
				poco_debug(logger_, "lmio: txWorker: sent recog-break (with audio), finish txWorker normally.");
				break;
			}
		}catch(const Poco::Net::WebSocketException &e){
			errorno_ = 800 + static_cast<int>(e.code());
			logger_.fatal("lmio: txWorker: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
			break;
		}catch(const Poco::TimeoutException &e){
			errorno_ = 830; //timeout;
			logger_.fatal("lmio: txWorker: WebSocket exception: %s (%d)", mimiio::strerror(errorno_), errorno_);
			break;
		}catch(const Poco::Net::NetException &e){
			errorno_ = 790; // network error
			logger_.fatal("lmio: txWorker: Network exception: %s (%d)", e.displayText(), errorno_);
			break;
		}catch(const Poco::IOException &e){
			errorno_ = 799; // undefined network error
			logger_.fatal("lmio: txWorker: I/O error, %s, terminate txWorker.", std::string(e.displayText()));
			break;
		}catch(const std::exception &e){
			errorno_ = 799; // undefined network error
			logger_.fatal("lmio: txWorker: Unknown error, std exception %s, terminate txWorker.", std::string(e.what()));
			break;
		}catch(...){
			errorno_ = 799; // undefined network error
			logger_.fatal("lmio: txWorker: Unknown error, terminate txWorker.");
			break;
		}
	}//while
	logger_.information("lmio: txWorker: tx loop finished with code %d", errorno_);
	finished_ = true;
}

}}
