/*
 * @file Stream.cpp
 * \~english
 * @brief Implementation of libmimimio stream management class
 * \~japanese
 * @brief libmimiio stream 管理クラスの実装
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/02/20
 *
 * Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Stream.h"
#include <tumbler/ledring.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

void txfunc(char *buffer, size_t *len, bool *recog_break, int* txfunc_error, void* userdata)
{
	Stream* stream = static_cast<Stream*>(userdata);
	*recog_break = stream->recogBreak_.load();
	if(stream->verbose_ && *recog_break){
		std::cerr << "txfunc: send recog-break" << std::endl;
	}
	std::vector<short> cat;
	auto current_queue_size = stream->inputQueue_.size();
	for(auto i=0;i<current_queue_size;++i){
		std::vector<short> tmp;
		stream->inputQueue_.pop(tmp);
		cat.insert(cat.end(), tmp.begin(), tmp.end());
	}
	*len = cat.size()*sizeof(short);
	memcpy(buffer, cat.data(), *len);
}

void rxfunc(const char* result, size_t len, int* rxfunc_error, void *userdata)
{
	Stream* stream = static_cast<Stream*>(userdata);
	// 通知
	Stream::SpeechEventData data;
	data.response_ = std::string(result, len);
	data.status_ = stream->currentSpeechState_;
	stream->notify(data);
}

Stream::Stream(const Stream::ConnectionParam& param) :
	param_(param),
	state_(Stream::Status::initialized),
	stream_monitor_continue_(true),
	errorno_(0)
{
	recogBreak_ = false;
	currentSpeechState_ = mimixfe::SpeechState::NonSpeech;
	verbose_ = param_.verbose_;
	if(param_.enableSpeculativeConnection_){
		open(); // 投機的接続を実行
	}
}

Stream::~Stream()
{
	stream_monitor_continue_ = false; // finish monitor thread
	if(open_future_.valid()){
		open_future_.get();
	}
	if(start_future_.valid()){
		start_future_.get();
	}
	if(mio_ != nullptr){
		mimi_close(mio_); // TODO unique_ptr
	}
}

void Stream::open()
{
	state_ = Stream::Status::connecting;
	open_future_ = std::async(std::launch::async, [&]{
		int retry = 0;
		int errorno = 0;
		while(retry++ < mimi_open_retry_){
			if(param_.verbose_){
				 std::cerr << "Opening new mimi connection... (" << retry <<  ")" << std::endl;
			}
			mio_ = mimi_open(
					param_.host_.c_str(), param_.port_, txfunc, rxfunc,
					static_cast<void*>(this), static_cast<void*>(this), param_.af_, param_.rate_, param_.channel_,
					param_.header_.data(), param_.header_.size(), param_.token_, MIMIIO_LOG_DEBUG, &errorno);
			if(mio_ == nullptr){
				std::cerr << "Could not open mimi(R) API service. mimi_open() failed: "
						<< mimi_strerror(errorno) << " (" << errorno << "), retry = " << retry << std::endl;
				errorno_ = errorno;
			}else{
				if(param_.verbose_){
					 std::cerr << "mimi connection is successfully opened." << std::endl;
				}
				state_ = Stream::Status::connected; // Connection is established successfully
				return;
			}
		}
		state_ = Stream::Status::connectionFailed; // Connection could not be established in mimi_open_retry_ times.
	});

	connection_monitor_ = std::async(std::launch::async, [&]{
		sleep(10);
		{
			std::unique_lock<std::mutex>(mutex_);
			if(state_.load() == Stream::Status::connected){
				mimi_close(mio_); // If no mimi_start() request in 60 seconds, close the speculative connection.
				state_ = Stream::Status::connectionTimedout;
				if(param_.verbose_){
					std::cerr << "mimi speculative connection is closed because no speech is detected in 60 seconds." << std::endl;
				}
			}
		}
	});
}

void Stream::start()
{
	if(!param_.enableSpeculativeConnection_){
		open(); // 通常接続を実行
	}
	{
		std::unique_lock<std::mutex>(mutex_);
		if(state_.load() == Stream::Status::connecting || state_.load() == Stream::Status::connected || state_.load() == Stream::Status::connectionTimedout){
			if(param_.verbose_){
				if(state_.load() == Stream::Status::connecting){
					std::cerr << "mimi stream is connecting now..." << std::endl;
				}else if(state_.load() == Stream::Status::connected){
					std::cerr << "mimi stream is starting on existing connection..." << std::endl;
				}else{
					std::cerr << "mimi stream is starting..." << std::endl;
				}
			}
		}else{
			// mimi_open() が失敗していたときに mimi_start() でのリコネクションを許容しない
			if(param_.verbose_){
				std::cerr << "Could not start the stream under the error condition " << std::endl;
			}
			return;
		}
	}
	start_future_ = std::async(std::launch::async, [&]{
		open_future_.get(); // Wait for async open() thread is to be finished.
		if(state_.load() == Stream::Status::connectionFailed){
			// Could not open connection to remote host. errorno_ is set by open()
			if(param_.verbose_){
				std::cerr << "Could not open the stream. " << std::endl;
			}
			return;
		}else if(state_.load() == Stream::Status::connectionTimedout){
			// Previous speculative connection has been already timed out,
			// because we cann't know how long ago the previous connection was established.
			if (param_.verbose_) {
				std::cerr << "Previous speculative connection has been already expired. Try re-connecting to remote host..." << std::endl;
			}
			open(); // Re-open connection
			open_future_.get(); // Wait for async open() thread is finished.
			if(state_.load() != Stream::Status::connected){
				// Could not open connection to remote host. errorno_ is set by open()
				return;
			}
		}

		int start_errorno = mimi_start(mio_);
		if(start_errorno == 0){
			if (param_.verbose_) {
				std::cerr << "mimi stream is successfully started, decoding starts..." << std::endl;
			}
			state_ = Stream::Status::streamStarted;
		}else{
			// This is unexpected case which means mimi_start() failed just after mimi_open() was called.
			std::cerr << "Could not start mimi(R) service. mimi_start() failed." << std::endl;
			errorno_ = start_errorno;
			return;
		}

		// For monitoring connection
		while(stream_monitor_continue_.load()){
			if(!mimi_is_active(mio_)){
				errorno_ = mimi_error(mio_);
				if(errorno_ != 0){
					if (param_.verbose_) {
						std::cerr << "mimi stream has an error (" << errorno_.load() << ")" << std::endl;
					}
					state_ = Stream::Status::streamFailed;
				}else{
					if (param_.verbose_) {
						std::cerr << "mimi stream is normally ended." << std::endl;
					}
					state_ = Stream::Status::streamEnded;
				}
				stream_monitor_continue_ = false;
				break;
			}
			usleep(100000);
		}
	});
}

void Stream::close()
{
	int errorno = mimi_error(mio_);
	if(errorno == -100){ // user defined.
		std::cerr << "An error occurred in mimii_tumbler_ex2, timed out for pop from queue." << std::endl;
	}else if(errorno > 0){ //libmimiio internal error code
		std::cerr << "An error occurred in mimiio: " << mimi_strerror(errorno) << "(" << errorno << ")" << std::endl;
	}
	mimi_close(mio_);
	if (param_.verbose_) {
		std::cerr << "mimi connection is closed." << std::endl;
	}
	mio_ = nullptr;
}

void Stream::push(
		const short* buf, size_t buflen,
		int sourceId, mimixfe::SpeechState state,
		const mimixfe::StreamInfo* info, size_t infolen)
{
	// 通知
	SpeechEventData data;
	data.sourceId_ = sourceId;
	if(infolen == 0){
		data.streamInfo_ = std::vector<mimixfe::StreamInfo>();
	}else{
		data.streamInfo_ = std::vector<mimixfe::StreamInfo>(info, info+infolen);
	}
	data.status_ = state;
	notify(data);

	// データ
	if(buflen != 0){
		inputQueue_.push(std::vector<short>(buf, buf+buflen));
	}
	currentSpeechState_ = state;
}

