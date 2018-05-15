/*
 * @file SpeechEvent.cpp
 * \~english
 * @brief 
 * \~japanese
 * @brief 発話（アタランス）イベントの処理の実装
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/03/14
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

#include "SpeechEvent.h"

SpeechEvent::SpeechEvent() : stream_(nullptr), prev_(nullptr), eventLoopContinue_(true), eventId_(0)
{
}

SpeechEvent::~SpeechEvent()
{
	finish();
	eventLoop_.get();
}

void SpeechEvent::speechStartHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo){}

void SpeechEvent::inSpeechHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo){}

void SpeechEvent::speechEndHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo){}

void SpeechEvent::responseHandler(int sourceId, const std::string& response){}

void SpeechEvent::errorHandler(int errorno){}

void SpeechEvent::start()
{
	eventLoop_ = std::async(std::launch::async, [&]{
		while(eventLoopContinue_.load()){
			Stream::SpeechEventData data;
			stream_->notified(data); // block until notification received from stream.
			if(data.finish_){
				break;
			}
			if(data.response_.size() != 0){
				responseHandler(data.sourceId_, data.response_);
			}else{
				if(data.status_ == mimixfe::SpeechState::SpeechStart){
					speechStartHandler(data.sourceId_, data.streamInfo_);
				}else if(data.status_ == mimixfe::SpeechState::InSpeech && data.streamInfo_.size() != 0){
					inSpeechHandler(data.sourceId_, data.streamInfo_);
				}else if(data.status_ == mimixfe::SpeechState::SpeechEnd){
					speechEndHandler(data.sourceId_, data.streamInfo_);
				}
			}
		}
	});
}

void SpeechEvent::finish()
{
	eventLoopContinue_ = false;
	Stream::SpeechEventData data;
	data.finish_ = true;
	stream_->notify(data); // イベントループ内のブロックを解除する
}
