/*
 * @file Stream.h
 * \~english
 * @brief libmimiio stream management class
 * \~japanese
 * @brief libmimiio stream 管理クラス
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
#ifndef EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_STREAM_H_
#define EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_STREAM_H_

#include "../../include/BlockingQueue.h"
#include <XFERecorder.h>
#include <mimiio.h>

#include <string>
#include <cstring>
#include <future>
#include <vector>
#include <future>

/**
 * @class Stream
 * @brief ひとつの発話に対応する音声データのキュー、リモートホストへの接続等の一式を管理する
 */
class Stream
{
public:
	using Ptr = std::unique_ptr<Stream>;

	/**
	 * @class Status
	 * @brief Stream current status
	 */
	enum class Status
	{
		initialized, //!< Just after constructed
		connecting,  //!< Under connection to mimi cloud (open() is called, but connection is not established yet.)
		connected,   //!< connected to mimi cloud (open() is called, and connection has been established.)
		connectionFailed, //!< connection failed to mimi cloud (open() is called, but connection could not established.)
		connectionTimedout, //!< connection was once established, but timed out because of long mimi_start() absence.
		streamStarted,     //!< start() was called and stream to mimi cloud was successfully started.
		streamFailed, //!< The stream has been stopped because an error occurred on the established connection to mimi cloud.
		recogBreaked, //!< recogBreak() has been called.
		streamEnded, //!< The stream has been normally ended.
	};

	/**
	 * @class ConnectionParam
	 * @brief mimi cloud 接続情報
	 */
	class ConnectionParam
	{
	public:
		ConnectionParam(
				const std::string& host, int port, const char* token,
				int rate, int channel, MIMIIO_AUDIO_FORMAT af, bool verbose,
				const std::string& lang, const std::string& services, bool enableSpeculativeConnection) :
					host_(host), port_(port), token_(token), rate_(rate), channel_(channel), af_(af), verbose_(verbose), lang_(lang), services_(services),
					enableSpeculativeConnection_(enableSpeculativeConnection)
		{
			header_.resize(2);
			strcpy(header_[0].key,"x-mimi-input-language");
			strcpy(header_[0].value, lang_.c_str());
			strcpy(header_[1].key,"x-mimi-process");
			strcpy(header_[1].value, services_.c_str());
		}
		const std::string host_;
		const int port_;
		const char* token_;
		int rate_;
		int channel_;
		bool verbose_;
		const std::string lang_;
		const std::string services_;
		bool enableSpeculativeConnection_;
		MIMIIO_AUDIO_FORMAT af_;
		std::vector<MIMIIO_HTTP_REQUEST_HEADER> header_;
	};

	/**
	 * @class SpeechEventData
	 * @brief Data class for event handler
	 */
	class SpeechEventData
	{
	public:
		SpeechEventData() : finish_(false){}
		std::vector<mimixfe::StreamInfo> streamInfo_;
		std::string response_;
		int sourceId_;
		mimixfe::SpeechState status_;
		bool finish_;
	};

	/**
	 * @brief C'tor
	 * @param [in] param 接続情報
	 */
	Stream(const ConnectionParam& param);

	/**
	 * \~english
	 * @brief D'tor close the stream immediately by calling mimi_close() and wait for some shared states.
	 * @details Note that mimi_close() set finish-flag on threads for data-send and data-receive in libmimiio to be finished immediately,
	 * and called join() function to wait for them to be actually finished.
	 * \~japanese
	 * @brief D'tor は mimi_close() を呼ぶことで接続をすぐに閉じようとし、いくつかの共有状態の解除を待つ
	 * @details mimi_close() は libmimiio において送信スレッド、受信スレッドに終了命令を送出し、それらが終了するまで待つことに留意する
	 * \~
	 */
	~Stream();

	/**
	 * @brief Asynchronously starting the stream to remote host.
	 */
	void start();

	/**
	 * @brief Close the stream to remote host. Wait for the connection is normally closed by remote host.
	 */
	void close();

	/**
	 * @brief Push audio sample and stream information
	 * @param [in] buf Audio sample buffer
	 * @param [in] buflen The length of the buffer
	 * @param [in] state mimixfe speech state
	 * @param [in] info Stream information array
	 * @param [in] infolen The length of the array
	 */
	void push(
			const short* buf, size_t buflen,
			int sourceId, mimixfe::SpeechState state,
			const mimixfe::StreamInfo* info, size_t infolen);

	/**
	 * @brief Return error code
	 * @return error code
	 */
	int errorno() const { return errorno_.load(); }

	void notified(SpeechEventData& data) { notifier_.pop(data); }

	void notify(const SpeechEventData& data) { notifier_.push(data); }

	void recogBreak(){ recogBreak_ = true; state_ = Status::recogBreaked; }

	// data for libmimiio
	BlockingQueue<std::vector<short>> inputQueue_; //!< Blocking queue holds data chunks from microphone
	std::atomic<bool> recogBreak_;
	std::atomic<mimixfe::SpeechState> currentSpeechState_;
	bool verbose_;

private:
	/**
	 * @brief Asynchronously opening stream to remote host.
	 */
	void open();

	// for libmimiio
	const ConnectionParam& param_;

	MIMI_IO *mio_ = nullptr; // TODO unique_ptr
	const int mimi_open_retry_ = 3; // 1 to 3 is suitable for retry.
	std::atomic<Stream::Status> state_; // !< Current state of this Stream
	std::future<void> open_future_; // async thread for mimi_open()
	std::future<void> connection_monitor_; // async thread for speculative connection shutdown in 60 seconds
	std::future<void> start_future_; // async thread for mimi_start() and mimi_is_acitve()
	std::atomic<bool> stream_monitor_continue_;
	std::atomic<int> errorno_;
	std::mutex mutex_;

	BlockingQueue<SpeechEventData> notifier_;
};

#endif /* EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_STREAM_H_ */
