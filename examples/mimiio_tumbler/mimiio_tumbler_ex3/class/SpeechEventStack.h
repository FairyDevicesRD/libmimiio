/*
 * @file ApplicatonStack.h
 * \~english
 * @brief Application stack for execution
 * \~japanese
 * @brief アプリケーションの実行スタック
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/03/21
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
#ifndef EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_APPLICATIONSTACK_H_
#define EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_APPLICATIONSTACK_H_

#include <XFETypedef.h>
#include "Stream.h"
#include "../../../include/BlockingDeque.h"

#include <memory>

#include "SpeechEvent.h"

template<class SE>
class SpeechEventStack {
public:
    SpeechEventStack(const Stream::ConnectionParam &param);

    ~SpeechEventStack();

    /**
     * @brief Push audio samples and all XFE output
     */
    void push(
            const short *buf, size_t buflen, mimixfe::SpeechState state, int sourceId,
            const mimixfe::StreamInfo *info, size_t infolen);

    void abort();

private:
    void start();

    BlockingDeque<SE *> deque_;
    const Stream::ConnectionParam &param_;
    int serialNumber_;
};

template<class SE>
SpeechEventStack<SE>::SpeechEventStack(const Stream::ConnectionParam &param) : param_(param) {
    serialNumber_ = 0;
    SE *ua = new SE();
    ua->stream_.reset(new Stream(param_));
    ua->eventId_ = ++serialNumber_;
    ua->start();
    deque_.push_front(ua);
    start();
    if (param_.verbose_) {
        std::cerr << "SpeechEventStack is started." << std::endl;
    }
}

template<class SE>
SpeechEventStack<SE>::~SpeechEventStack() {
    //TODO
}


template<class SE>
void SpeechEventStack<SE>::push(
        const short *buf, size_t buflen, mimixfe::SpeechState state, int sourceId,
        const mimixfe::StreamInfo *info, size_t infolen) {
    deque_.front()->stream_->push(buf, buflen, sourceId, state, info, infolen);
    if (state == mimixfe::SpeechState::SpeechStart) {
        deque_.front()->stream_->start();
    } else if (state == mimixfe::SpeechState::SpeechEnd) {
        deque_.front()->stream_->recogBreak();
        SE *ua = new SE();
        ua->stream_.reset(new Stream(param_));
        ua->prev_ = deque_.front();
        ua->eventId_ = ++serialNumber_;
        ua->start();
        deque_.push_front(ua);
        if (param_.verbose_) {
            std::cerr << "New stream is set, current stack size = " << deque_.size() << std::endl;
        }
    }
}


template<class SE>
void SpeechEventStack<SE>::abort() {
    //TODO
    //イベントスタックに含まれるイベント全体を終了させる
}

template<class SE>
void SpeechEventStack<SE>::start() {
    //TODO
    //終了したイベントを検知し、イベントスタックの長さを一定に保つ
}

#endif /* EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_APPLICATIONSTACK_H_ */
