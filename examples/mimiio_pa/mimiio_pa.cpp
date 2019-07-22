/*
 * @file mimiio_pa.cp
 * @ingroup examples_src
 * \~english
 * @brief Simple example of progressive sending audio from microphone with portaudio
 * @note According to the portaudio document, you must not use blocking queue (lock queue) in portaudio callback function.
 * In the strict sense, lock-free queue is suitable for the callback function. For this example program, we use blocking queue to simplify demonstration.
 * \~japanese
 * @brief portaudio を利用した連続マイク入力から音声データをサーバーにリアルタイムで送信する例
 * @note portaudio のドキュメントに従うと、portaudio のコールバック関数内でブロッキングキュー（ロックキュー）を使うべきではなく、
 * ロックフリーキューを使わなければならないが、本サンプルプログラムでは簡単のためブロッキングキューで代替する。
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/02/01
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

#include "../include/cmdline/cmdline.h"
#include "../include/BlockingQueue.h"

#include <mimiio.h>
#include <portaudio.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t interrupt_flag_ = 0;
void sig_handler_(int signum){ interrupt_flag_ = 1; }

/**
 * \~english
 * @brief portaudio record callback, see portaudio documents in detail such as meaning of each arguments.
 * \~japanese
 * @brief portaudio の録音コールバック関数、引数の意味など詳細は portaudio の公式ドキュメントを参照してください
 */
static int recordCallback(
		const void *inputBuffer,
		void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData)
{
	(void) outputBuffer;
	(void) timeInfo;
	(void) statusFlags;
	if(inputBuffer == nullptr) return paContinue;
	SampleQueue* queue = reinterpret_cast<SampleQueue*>(userData);
	const short* buffer = reinterpret_cast<const short*>(inputBuffer);
	for(size_t i=0;i<framesPerBuffer;++i){
		queue->push(buffer[i]);
	}
	if(interrupt_flag_ == 0){
		return paContinue;
	}else{
		queue->push(0);    // The simplest implementation to declare queue end.
		return paComplete; // Ctrl+C to stop recording
	}

}

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
	size_t chunk_size = 3200; //bytes. 3.2 kb for 1.6 ksamples which is correspond to 100 msec at 16 kHz sampling rate.
	std::chrono::milliseconds timeout(1000); // queue timeout
	SampleQueue* queue = static_cast<SampleQueue*>(userdata);
	int length = 0;
	std::vector<short> tmp;
	for(size_t i=0;i<chunk_size;i+=2){
		short sample = 0;
		if(queue->pop(sample, timeout)){
			tmp.push_back(sample);
		    length += 2;
		}else{
			*txfunc_error = -100; // Timeout for pop from queue.
			break;
		}
		if(interrupt_flag_ != 0 && sample == 0){
			*recog_break = true;
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
	if(signal(SIGINT, sig_handler_) == SIG_ERR){
		return 1;
	}

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
        p.add<std::string>("input_language", 'l', "input language", false, "ja");
        p.add<std::string>("output_language", 'L', "output language", false, "ja");
        p.add<std::string>("process", 'x', "x-mimi-process", false, "asr");
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
        std::cerr << "Invalid audio format: " << p.get<std::string>("format")
                  << std::endl;
        return 1;
    }

	BlockingQueue<short> queue;

	// Initialize audio device with portaudio
	PaError paerror = Pa_Initialize();
	if(paerror != paNoError){
		Pa_Terminate();
		std::cerr << "Portaudio error: " << Pa_GetErrorText(paerror) << "(" << paerror << ")\n";
		return 1;
	}
	if(p.exist("verbose")){
		std::cerr << "Audio devices is successfully initialized." << std::endl;
	}

	// Prepare portaudio stream parameters
	PaStreamParameters inputParams;
	inputParams.device = Pa_GetDefaultInputDevice();
	if(inputParams.device == paNoDevice){
		Pa_Terminate();
		std::cerr << "No default input device\n";
		return 1;
	}
	inputParams.channelCount = 1;
	inputParams.sampleFormat = paInt16;
	inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
	inputParams.hostApiSpecificStreamInfo = nullptr;

	// Open portaudio stream
	PaStream* stream;
	paerror = Pa_OpenStream(&stream, &inputParams, nullptr, p.get<int>("rate"), 1024, paClipOff, recordCallback, static_cast<void*>(&queue));
	if(paerror != paNoError){
		Pa_Terminate();
		std::cerr << "Portaudio error: " << Pa_GetErrorText(paerror) << "(" << paerror << ")\n";
		return 1;
	}
	if(p.exist("verbose")){
		std::cerr << "Portaudio recording stream is successfully opened." << std::endl;
	}

	// Start recording from microphone with portaudio
	paerror = Pa_StartStream(stream);
	if(paerror != paNoError){
		Pa_Terminate();
		std::cerr << "Portaudio error: " << Pa_GetErrorText(paerror) << "(" << paerror << ")\n";
		return 1;
	}
	if(p.exist("verbose")){
		std::cerr << "Recording starts." << std::endl;
	}

    std::string mimi_process;
    mimi_process = p.get<std::string>("process");

    std::string input_lang;
    input_lang = p.get<std::string>("input_language");

    std::string output_lang;
    output_lang = p.get<std::string>("output_language");

    // Prepare mimi runtime configuration
    size_t header_size = 3;
    MIMIIO_HTTP_REQUEST_HEADER h[header_size];
    strcpy(h[0].key, "x-mimi-process");
    strcpy(h[0].value, mimi_process.c_str());
    strcpy(h[1].key, "x-mimi-input-language");
    strcpy(h[1].value, input_lang.c_str());
    strcpy(h[2].key, "x-mimi-output-language");
    strcpy(h[2].value, output_lang.c_str());


	// Open mimi stream
	int errorno = 0;
	MIMI_IO *mio = mimi_open(
	        p.get<std::string>("host").c_str(), p.get<int>("port"), txfunc, rxfunc,
			static_cast<void*>(&queue), static_cast<void*>(&queue), af, p.get<int>("rate"), p.get<int>("channel"), h,
	        header_size, access_token, MIMIIO_LOG_DEBUG, &errorno);

	if(mio == NULL){
		std::cerr << "Could not initialize mimi(R) service. mimi_open() failed: " << mimi_strerror(errorno) << " (" << errorno << ")"<< "\n";
		paerror = Pa_CloseStream(stream);
		if(paerror != paNoError){
			Pa_Terminate();
			std::cerr << "Portaudio error: " << Pa_GetErrorText(paerror) << "(" << paerror << ")\n";
		}
		return 1;
	}
    if (p.exist("verbose")) {
    	std::cerr << "mimi connection is successfully opened." << std::endl;
    }

	//Start mimi(R) stream
	errorno = mimi_start(mio);
	if(errorno != 0){
		std::cerr << "Could not start mimi(R) service. mimi_start() filed. See syslog in detail.\n";
		mimi_close(mio);
		paerror = Pa_CloseStream(stream);
		if(paerror != paNoError){
			Pa_Terminate();
			std::cerr << "Portaudio error: " << Pa_GetErrorText(paerror) << "(" << paerror << ")" << std::endl;
		}
		return 1;
	}
    if (p.exist("verbose")) {
        std::cerr << "Full duplex stream is started." << std::endl;
    }

	//Wait main thread.
	while((paerror = Pa_IsStreamActive(stream)) == 1 && mimi_is_active(mio)){
		Pa_Sleep(500);
	}

	//Close streams and check error state.
	paerror = Pa_CloseStream(stream);
	errorno = mimi_error(mio);
	if(paerror != paNoError){
		Pa_Terminate();
		std::cerr << "Portaudio error: " << Pa_GetErrorText(paerror) << "(" << paerror << ")\n";
		mimi_close(mio);
		return 1;
	}
	if(errorno == -100){ // user defined.
		std::cerr << "[ERROR] Timed out for popping from queue." << std::endl;
		mimi_close(mio);
		return 2;
	}else if(errorno > 0){ //libmimiio internal error code
		std::cerr << "An error occurred while communicating mimi(R) service: " << mimi_strerror(errorno) << "(" << errorno << ")" << std::endl;
		mimi_close(mio);
		return 2;
	}
	mimi_close(mio);
	return 0;
}
