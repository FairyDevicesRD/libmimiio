/*
 * @file mimiio_tumbler_ex2.cpp
 * \~english
 * @brief Simple example of progressive sending audio with libmimixfe signal processing. The output of libmimixfe is limited to 1ch.
 * \~japanese
 * @brief libmimixfe を利用した連続マイク入力から信号処理済音声データをサーバーにリアルタイムで送信する最もシンプルな例のひとつ
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/02/17
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

#include "../../include/cmdline/cmdline.h"
#include "../../include/BlockingQueue.h"
#include <mimiio.h>
#include <XFERecorder.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <stdexcept>
#include <sstream>
#include <atomic>

volatile sig_atomic_t interrupt_flag_ = 0;
void sig_handler_(int signum){ interrupt_flag_ = 1; }

/**
 * @class StreamData
 * @brief User defined data for txfunc
 */
class StreamData{
public:
	StreamData() : recog_break_(false) {}
	SampleQueue queue_;
	std::atomic<bool> recog_break_;
};

/**
 * \~english
 * @brief Callback function for sending audio
 * @attention The maximum size of \e len is limited to 32kib according to mimi(R) WebSocket API service specification.
 * @note A new thread created by libmimiio processes this callback function. Note that this function is NOT processed on main thread.
 * @attention \e txfunc_error must be negative value to avoid overlapping of libmimio's internal error codes.
 * @param [out] buffer Audio data to be sent
 * @param [out] len The size of audio data stored in \e buffer, which is limited to 32kib.
 * @param [out] recog_break If true, <em> break command </em> to be sent.
 * @param [out] txfunc_error User defined error code. Set negative value when an error occurred in this callback function.
 * @param [in,out] userdata User defined data.
 *
 * \~japanese
 * @brief 音声データを送信するためのユーザー定義コールバック関数. libmimiio から定期的に呼ばれる。
 * @attention 仕様書記載の通り、音声データの最大チャンクサイズは 32 KB
 * @note このコールバック関数は、libmimiio が準備するスレッドから呼び出され、そのスレッドは libmimiio が初期化されるスレッドとは異なることに留意する。
 * @attention  \e txfunc_error に入れるユーザー定義エラーは、libmimiio の内部エラーコードと重複しないようにマイナスの値を利用することを推奨
 * @param [out] buffer libmimiio によってサーバーに送信される音声データ
 * @param [out] len 上記バッファの長さ. 最大 32 KB
 * @param [out] recog_break 音声送信を終了し、サーバーから最終結果を得るときに true を渡す。true が渡されると、二度と txfunc は呼ばれない。
 * @param [out] txfunc_error ユーザー定義エラーコード。このコールバック関数内でエラーが発生し、かつ、libmimiio に対してサーバーへの通信路を切断したいときに 0 以外の数値を書き込む。
 * libmimiio の内部エラーコードを重複しないように、マイナスの値を利用することを推奨する。
 * @param [in,out] userdata 任意のユーザー定義データ
 */
void txfunc(char *buffer, size_t *len, bool *recog_break, int* txfunc_error, void* userdata)
{
	std::chrono::milliseconds timeout(1000); // queue timeout
	StreamData* sdata = static_cast<StreamData*>(userdata);
	*recog_break = sdata->recog_break_.load();
	auto current_queue_size = sdata->queue_.size();
	int length = 0;
	std::vector<short> tmp;
	for(auto i=0;i<current_queue_size;i+=2){
		short sample = 0;
		if(sdata->queue_.pop(sample, timeout)){
			tmp.push_back(sample);
		    length += 2;
		}else{
			*txfunc_error = -100; // Timeout for pop from queue.
			break;
		}
	}
	*len = length;
	std::memcpy(buffer, &tmp[0], tmp.size()*2);
}

/**
 * \~english
 * @brief Callback function for receiving results
 * @note A new thread created by libmimiio processes this callback function. Note that this function is NOT processed on main thread.
 * @attention \e rxfunc_error must be negative value to avoid overlapping of libmimio's internal error codes.
 * @param [in] result The result received from mimi(R) remote host. The result is NULL terminated when the result is text, otherwise not NULL terminated.
 * @param [in] len The size of \e result.
 * @param [out] rxfunc_error User defined error code Set negative value when an error occurred in this callback function.
 * @param [in,out] userdata User defined data
 *
 * \~japanese
 * @brief サーバーからデータを受信する都度 libmimiio から呼ばれるユーザー定義コールバック関数
 * @note このコールバック関数は、libmimiio が準備するスレッドから呼び出され、そのスレッドは libmimiio が初期化されるスレッドとは異なることに留意する。
 * @attention  \e rxfunc_error に入れるユーザー定義エラーは、libmimiio の内部エラーコードと重複しないようにマイナスの値を利用することを推奨
 * @param [in] result サーバーから受信した内容。文字列の場合は C 言語形式の NULL 終端文字列。
 * @param [in] size_t len 上記の長さ
 * @param [out] rxfunc_error ユーザー定義エラーコード。このコールバック関数内でエラーが発生し、かつ、libmimiio に対してサーバーへの通信路を切断したいときに 0 以外の数値を書き込む。
 * libmimiio の内部エラーコードを重複しないように、マイナスの値を利用することを推奨する。
 * @param [in,out] userdata 任意のユーザー定義データ
 */
void rxfunc(const char* result, size_t len, int* rxfunc_error, void *userdata)
{
	std::string s(result, len);
	std::cout << s << std::endl;
}

/**
 * @class Session
 * @brief ひとつの発話に対応する音声データ、リモートホストへの接続等の一式を管理する
 */
class Session
{
public:
	class ConnectionParam
	{
	public:
		ConnectionParam(
				const std::string& host, int port, const char* token,
				int rate, int channel, MIMIIO_AUDIO_FORMAT af, bool verbose) :
					host_(host), port_(port), token_(token), rate_(rate), channel_(channel), af_(af), verbose_(verbose) {}
		const std::string host_;
		const int port_;
		const char* token_;
		int rate_;
		int channel_;
		bool verbose_;
		MIMIIO_AUDIO_FORMAT af_;
		std::vector<MIMIIO_HTTP_REQUEST_HEADER> header_;
	};

	Session(const ConnectionParam& param) : param_(param){}
	~Session(){ if(mio_ != nullptr) mimi_close(mio_); }

	/**
	 * @brief Open stream to remote host.
	 * @return true if successfully opened the stream.
	 */
	bool open()
	{
		if(mio_ != nullptr){
			close();
		}
		int retry = 0;
		while(retry++ < mimi_open_retry_){
			int errorno = 0;
			mio_ = mimi_open(
					param_.host_.c_str(), param_.port_, txfunc, rxfunc,
					static_cast<void*>(&sdata_), static_cast<void*>(nullptr), param_.af_, param_.rate_, param_.channel_,
					param_.header_.data(), param_.header_.size(), param_.token_, MIMIIO_LOG_DEBUG, &errorno);
			if(mio_ == nullptr){
				std::cerr << "Could not open mimi(R) API service. mimi_open() failed: "
						<< mimi_strerror(errorno) << " (" << errorno << "), retry = " << retry << std::endl;
			}else{
				if(param_.verbose_){
					 std::cerr << "mimi connection is successfully opened." << std::endl;
				}
				return true;
			}
		}
		// 接続が複数回失敗した場合は失敗とする
		return false;
	}

	/**
	 * @brief Start the stream to remote host.
	 * @return true if successfully start the stream.
	 */
	bool start()
	{
		 int errorno = mimi_start(mio_);
		 if(errorno != 0){
			 std::cerr << "Could not start mimi(R) service. mimi_start() filed. See syslog in detail.\n";
			 mimi_close(mio_);
			 return false;
		 }
		 if (param_.verbose_) {
			 std::cerr << "mimi connection is successfully started, decoding starts..." << std::endl;
		 }
		 return true;
	}

	void close()
	{
		while(mimi_is_active(mio_)){
			usleep(1000000);
		}
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
		sdata_.recog_break_.store(false);
	}

	/**
	 * @brief Send recog-break
	 */
	void recogBreak(){
		sdata_.recog_break_.store(true);
	}

	/**
	 * @brief Add single audio sample to queue
	 * @param [in] sample Single audio sample
	 */
	void addAudioData(const short& sample) { sdata_.queue_.push(sample); }

private:
	const ConnectionParam& param_;
	MIMI_IO *mio_ = nullptr;
	StreamData sdata_;
	const int mimi_open_retry_ = 3;
};

/**
 * \~english
 * @brief libmimixfe record callback, see libmimixfe documents in detail such as meaning of each arguments.
 * \~japanese
 * @brief libmimixfe 録音コールバック関数、引数の意味など詳細は libmimixfe ドキュメントを参照してください
 */
void recorderCallback(
		short* buffer,
		size_t buflen,
		mimixfe::SpeechState state,
		int sourceId,
		mimixfe::StreamInfo* info,
		size_t infolen,
		void* userdata
){
	Session* session = reinterpret_cast<Session*>(userdata);
	for(size_t i=0;i<buflen;++i){
		session->addAudioData(buffer[i]);
	}
	if(state == mimixfe::SpeechState::SpeechStart){
		session->open();
		session->start();
	}else if(state == mimixfe::SpeechState::SpeechEnd){
		session->recogBreak();
	}
}

// 送信音声形式をコマンドライン引数から指定するための実装例
struct AFENTRY
{
    char const *name;
    MIMIIO_AUDIO_FORMAT af;
};

constexpr AFENTRY afmap[] =
{
    {"MIMIIO_RAW_PCM", MIMIIO_RAW_PCM},
    {"MIMIIO_FLAC_0", MIMIIO_FLAC_0},
    {"MIMIIO_FLAC_1", MIMIIO_FLAC_1},
    {"MIMIIO_FLAC_2", MIMIIO_FLAC_2},
    {"MIMIIO_FLAC_3", MIMIIO_FLAC_3},
    {"MIMIIO_FLAC_4", MIMIIO_FLAC_4},
    {"MIMIIO_FLAC_5", MIMIIO_FLAC_5},
    {"MIMIIO_FLAC_6", MIMIIO_FLAC_6},
    {"MIMIIO_FLAC_7", MIMIIO_FLAC_7},
    {"MIMIIO_FLAC_8", MIMIIO_FLAC_8},
    {"MIMIIO_FLAC_PASS_THROUGH", MIMIIO_FLAC_PASS_THROUGH}
};

constexpr auto afmap_size = sizeof(afmap) / sizeof(afmap[0]);

bool parse_afstring(const std::string &afstring, MIMIIO_AUDIO_FORMAT *af) {
    for (auto i = 0; i < afmap_size; ++i) {
        if (afstring == afmap[i].name) {
            *af = afmap[i].af;
            return true;
        }
    };
    return false;
};

/**
 * @brief main function
 * @return exit code
 */
int main(int argc, char** argv)
{
	// Parsing command-line arguments
    cmdline::parser p;
    {
        // mandatory
        p.add<std::string>("host", 'h', "Host name", true);
        p.add<int>("port", 'p', "Port", true);
        // optional
        p.add<std::string>("token", 't', "Access token", false);
        p.add<int>("rate", '\0', "Sampling rate", false, 16000);
        p.add<int>("channel", '\0', "Number of channels", false, 1);
        p.add<std::string>("format", '\0', "Audio format", false, "MIMIIO_RAW_PCM");
        p.add("verbose", '\0', "Verbose mode");
        p.add("help", '\0', "Show help");
        if (!p.parse(argc, argv)) {
            std::cout << p.error_full() << std::endl;
            std::cout << p.usage() << std::endl;
            std::cout << "Acceptable audio formats:" << std::endl;
            for (auto i = 0; i < afmap_size; ++i) {
                std::cout << "    " << afmap[i].name << std::endl;
            };
            return 0;
        }
    }

    const char *access_token = nullptr;
    if (p.exist("token")) {
        access_token = p.get<std::string>("token").c_str();
    }

    MIMIIO_AUDIO_FORMAT af;
    const bool ok = parse_afstring(p.get<std::string>("format"), &af);
    if (!ok) {
        std::cerr << "Invalid audio format: " << p.get<std::string>("format") << std::endl;
        return 1;
    }

	try{

		// Initialize mimixfe
	    Session::ConnectionParam param(p.get<std::string>("host"), p.get<int>("port"), access_token,
	    		p.get<int>("rate"), p.get<int>("channel"), af, p.exist("verbose"));
	    Session session(param);
	    int xfe_errorno = 0;
	    mimixfe::XFESourceConfig s;
	    mimixfe::XFEECConfig e;
	    mimixfe::XFEVADConfig v;
	    v.timeToInactive_  = 600;
	    v.tailPaddingTime_ = 400;
	    mimixfe::XFEBeamformerConfig b;
	    mimixfe::XFEStaticLocalizerConfig c({mimixfe::Direction(270, 90)});
	    mimixfe::XFERecorder rec(s,e,v,b,c,recorderCallback,reinterpret_cast<void*>(&session));
	    rec.setLogLevel(LOG_UPTO(LOG_DEBUG));
	    if(signal(SIGINT, sig_handler_) == SIG_ERR){
	    	return 1;
		}
	    if(p.exist("verbose")){
	    	std::cerr << "XFE recording stream is successfully initialized." << std::endl;
		}
	    // Recording start
	    rec.start();
	    if (p.exist("verbose")) {
	    	std::cerr << "XFE recording stream is successfully started." << std::endl;
	    	std::cerr << "[[ YOU CAN SPEAK NOW ]]" << std::endl;
		}

	    int usec = 100000; // 0.1sec, you should choose appropriate value
	    while(rec.isActive()){
			 if(interrupt_flag_ != 0){
				 if (p.exist("verbose")) {
					 std::cerr << "Stream is to be finished..." << std::endl;
				 }
				 rec.stop();
				 session.close();
			 }
			 usleep(usec);
		 }
		 if(xfe_errorno != 0){
			 std::cerr << "An error occurred in mimixfe (" << xfe_errorno << ")" << std::endl;
		 }

		 if (p.exist("verbose")) {
			 std::cerr << "All resources are cleaned up." << std::endl;
		 }
		 return 0;

    }catch(const mimixfe::XFERecorderError& e){
    	std::cerr << "XFE Recorder Exception: " << e.what() << "(" << e.errorno() << ")" << std::endl;
		return 2;
	 }catch(const std::exception& e){
		std::cerr << "Exception: " << e.what() << std::endl;
		return 2;
	 }
}



