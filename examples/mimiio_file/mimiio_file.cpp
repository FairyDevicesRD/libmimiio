/*
 * @file mimiio_file.cp
 * @ingroup examples_src
 * \~english
 * @brief Simple example of sending audio from file. This is just for API demonstration,
 * if you actually want to send audio to mimi from file, you should consider using mimi
 * HTTP API Service instead of WebSocket Service.
 *
 * \~japanese
 * @brief 音声ファイルから音声データをサーバーに送信する例. この例は API デモンストレーションのための例であり、
 * もし実際に音声ファイルを送信したい場合は、mimi HTTP API Service の利用を検討してください。
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
#include <iostream>
#include <mimiio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __APPLE__
#include <syslog.h>
#endif

#include <stdlib.h>

FILE *inputfile_; //!< audio file to be sent

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
void txfunc(char *buffer, size_t *len, bool *recog_break, int *txfunc_error, void *userdata)
{
    size_t chunk_size = 2048;
    *len = fread(buffer, 1, chunk_size, inputfile_); // 音声ファイルを chunk_size ずつサーバーに送信します
    if (*len < chunk_size) { // 音声ファイルを読み終わった時点で、送信の終了を宣言します
        *recog_break = true;
    }

    //If you want to emulate speed-limited communication line, see below;
    //リアルタイム送信をエミュレートする場合は以下のような実装が利用可能です
    //
    //float send_rate_ = 256; // 送信速度制限[kbps]
    //float byte_per_sec = send_rate_*1024. / 8.;
    //float wait_sec = 1.0 / (byte_per_sec / chunk_size);
    //usleep(wait_sec*1000000.);
    //*len = fread(buffer, 1, chunk_size, inputfile_);
    //if (*len < chunk_size) {
    //    *recog_break = true;
    //}
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
void rxfunc(const char *result, size_t len, int *rxfunc_error, void *userdata)
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
 * Simple example of sending audio from file.
 * @return exit code
 */
int main(int argc, char **argv) {

	// Parsing command-line arguments
    cmdline::parser p;
    {
        // mandatory
        p.add<std::string>("host", 'h', "Host name", true);
        p.add<int>("port", 'p', "Port", true);
        p.add<std::string>("input", 'i', "Input file", true);
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
        std::cerr << "Invalid audio format: " << p.get<std::string>("format")
                  << std::endl;
        return 1;
    }

    // Open input file
    inputfile_ = fopen(p.get<std::string>("input").c_str(), "rb");
    if (inputfile_ == nullptr) {
        std::cerr << "Could not open file: " << p.get<std::string>("input")
                  << std::endl;
        return 1;
    }

    // Prepare mimi runtime configuration
    int errorno = 0;
    size_t header_size = 1;
    MIMIIO_HTTP_REQUEST_HEADER h[header_size];
    strcpy(h[0].key, "x-mimi-process");
    strcpy(h[0].value, "asr");

    // Open mimi stream
    MIMI_IO *mio = mimi_open(
        p.get<std::string>("host").c_str(), p.get<int>("port"), txfunc, rxfunc,
        nullptr, nullptr, af, p.get<int>("rate"), p.get<int>("channel"), h,
        header_size, access_token, MIMIIO_LOG_DEBUG, &errorno);

    if (mio == nullptr) {
        fprintf(stderr, "Could not initialize mimi(R) service. mimi_open() "
                        "failed: %s (%d)\n",
                mimi_strerror(errorno), errorno);
        fclose(inputfile_);
        return 1;
    }
    if (p.exist("verbose")) {
        fprintf(stderr, "Connection is successfully opened.\n");
    }

    // Start mimi stream
    errorno = mimi_start(mio);
    if (errorno != 0) {
        fprintf(stderr, "Could not start mimi(R) service. mimi_start() filed. See syslog in detail.\n");
        mimi_close(mio);
        fclose(inputfile_);
        return 1;
    }
    if (p.exist("verbose")) {
        fprintf(stderr, "Full duplex stream is started.\n");
    }
    while (mimi_is_active(mio)) {
        // Wait 0.1 sec to avoid busy loop
        usleep(100000);
    }
    if (p.exist("verbose")) {
        fprintf(stderr, "mimi_is_active returns false now.\n");
    }
    errorno = mimi_error(mio);
    if (errorno != 0) {
        fprintf(
            stderr,
            "An error occurred while communicating mimi(R) service: %s (%d)\n",
            mimi_strerror(errorno), errorno);
        mimi_close(mio);
        fclose(inputfile_);
        return 1;
    }

    // Close mimi connection
    mimi_close(mio);
    if (p.exist("verbose")) {
        fprintf(stderr, "Connection is closed.\n");
    }
    fclose(inputfile_);
    return 0;
}
