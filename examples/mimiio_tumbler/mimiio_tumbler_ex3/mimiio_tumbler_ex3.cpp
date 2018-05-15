/*
 * @file mimiio_tumbler_ex3.cpp
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
#include "class/Stream.h"
#include "class/SpeechEvent.h"
#include "class/SpeechEventStack.h"

#include <tumbler/speaker.h>
#include <tumbler/ledring.h>
#include <mimiio.h>
#include <XFERecorder.h>

#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/ParseHandler.h>
#include <Poco/Dynamic/Var.h>

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <stdexcept>
#include <sstream>
#include <atomic>
#include <fstream>

volatile sig_atomic_t interrupt_flag_ = 0;
void sig_handler_(int signum){ interrupt_flag_ = 1; }

/**
 * @class MySpeechEvent
 * @brief SpeechEvent クラスをオーバーライドして実装し、発話に関する各種のイベント発生時の処理を定義する。別発話発生時は別のインスタンスが並列に呼ばれ、
 * インスタンス間はリンクトリスト構造によりつながっている。
 */
class MySpeechEvent : public SpeechEvent
{
public:
	MySpeechEvent()
	{
		if(se1_.size() == 0){
			// 効果音のロード
			std::ifstream inputfs("data/se1.raw", std::ios::in|std::ios::binary|std::ios::ate);
			if(!inputfs.is_open()){
				throw std::runtime_error("MySpeechEvent: Could not open file data/se1.raw");
			}
			size_t inputsize = inputfs.tellg();
			se1_.resize(inputsize/2);
			inputfs.seekg(0, std::ios::beg);
			inputfs.read(reinterpret_cast<char*>(&se1_[0]),inputsize);
			inputfs.close();
		}

		// LED リングの点灯パターンの初期化
		led_control_ = true; // LED リングの制御権を与える
		if(pt1_.size() == 0){
			tumbler::LED background(0,0,100);
			pt1_.resize(18);
			for(int i=0;i<18;++i){
				tumbler::Frame frame(background);
				frame.setLED(i, tumbler::LED(0,255,0));
				pt1_[i] = frame;
			}
		}
	}

	/**
	 * @brief 発話開始検出された時点で呼び出される関数
	 * @param [in] sourceId libmimixfe で定義される sourceId
	 * @param [in] streamInfo libmimixfe で定義される音声ストリーム情報
	 * @details 発話開始検出された時点で 1 回のみ呼び出される
	 */
	virtual void speechStartHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo) override
	{
		std::cout << "SPEECH START" << std::endl;

		if(prev_ != nullptr){
			// 新たに発話検出された場合は、１つ前の発話イベントに対して LED 制御権を放棄させる
			// LED リングの制御クラスを別途設けることで、そちらに情報を集約しても良いが、ここでは１つ前の発話イベントを利用する例として示した。
			static_cast<MySpeechEvent*>(prev_)->releaseLEDRingControl();
		}
	}

	/**
	 * @brief 発話中に一定時間ごとに呼び出される関数
	 * @param [in] sourceId libmimixfe で定義される sourceId
	 * @param [in] streamInfo libmimixfe で定義される音声ストリーム情報
	 * @details libmimixfe の録音コールバック関数が発話中ステートで呼び出される都度、この関数も呼び出される
	 */
	virtual void inSpeechHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo) override
	{
		std::cout << "IN SPEECH" << std::endl;
	}

	/**
	 * @brief 発話終了検出された時点で呼び出される関数
	 * @param [in] sourceId libmimixfe で定義される sourceId
	 * @param [in] streamInfo libmimixfe で定義される音声ストリーム情報
	 * @details 発話終了検出された時点で 1 回のみ呼び出される
	 */
	virtual void speechEndHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo) override
	{
		std::cout << "SPEECH END" << std::endl;

		// 発話終了検出時に音声サインを再生し、LED リングの点灯状態を変える例
		// 1. 音声再生（非同期）
		tumbler::Speaker& spk = tumbler::Speaker::getInstance();
		spk.batchPlay(se1_, 44100, 0.05, tumbler::Speaker::PlayBackMode::normal_);

		// 2. LED リングの点灯状態の変更
		led_s_ = true;
		led_f_ = std::async(std::launch::async, [&]{
				tumbler::LEDRing& ring = tumbler::LEDRing::getInstance();
				ring.clearFrames();
				ring.setFPS(30);
				ring.setFrames(pt1_);
				while(led_s_.load() && led_control_.load()){
					ring.show(false);
				}
		});
	}

	/**
	 * @brief libmimiio によってリモートホスト側から何らかの応答を受信する都度呼び出される関数
	 * @param [in] sourceId libmimixfe で定義される sourceId
	 * @param [in] response リモートホストからの応答内容文字列、典型的には JSON 文字列となる
	 * @details libmimiio の受信コールバック関数が呼び出される都度、この関数が呼び出される
	 */
	virtual void responseHandler(int sourceId, const std::string& response) override
	{
		//応答結果の JSON を解析し、最終認識結果を受信した時点から、何らかのユーザー処理を想定して 3 秒待ち、LED リングの点灯状態を元に戻す例
		Poco::JSON::Parser parser;
		Poco::DynamicStruct dsresult = *(parser.parse(response).extract<Poco::JSON::Object::Ptr>());
		auto response_size = dsresult["response"].size();
		std::stringstream s; // JSON の result フィールドの情報を抜き出したもの
		for(auto i=0;i<response_size;++i){
			s << dsresult["response"][i]["result"].toString();
		}
		if(dsresult["status"] == "recog-finished"){
			std::cout << "RESPONSE(FINAL): " << s.str() << std::endl;

			//// ここで最終確定結果を利用して何かをする想定 ////
			sleep(3); // 何かをするのに掛かった時間（実際に 3 秒は UX 上時間が掛かりすぎでありダメだが、サンプルの動きの分かりやすさのために長くしたもの）
			///////////////////////////////////////////////

			led_s_ = false;
			led_f_.get();
			if(led_control_.load()){ // 上記の 3 秒の間に、次の発話が行われている場合があり、その場合、次の発話イベント側で LED リングが制御される
				tumbler::LEDRing& ring = tumbler::LEDRing::getInstance();
				ring.reset(true); // LED リングの点灯状態をデフォルト状態に戻す
			}

			finish(); // ユーザー定義処理が終了したことを伝える
		}else{
			std::cout << "RESPONSE: " << s.str() << std::endl;
		}
	}

	/**
	 * @brief libmimixfe, libmimiio, 本サンプルプログラム内部のどちらかで何らかの継続不可能なエラーが起こった場合に呼び出される関数
	 * @param [in[ errorno エラー番号
	 */
	virtual void errorHandler(int errorno) override{}

	/**
	 * @brief このイベントクラスに LED 制御権を放棄させる
	 */
	void releaseLEDRingControl(){ led_control_ = false; }

private:
	static std::vector<short> se1_; // 効果音 #1
	static std::vector<tumbler::Frame> pt1_; // LEDリングの点灯パターン #1

	std::future<void> led_f_;
	std::atomic<bool> led_s_;
	std::atomic<bool> led_control_;
};

std::vector<short> MySpeechEvent::se1_;
std::vector<tumbler::Frame> MySpeechEvent::pt1_;

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
)
{
	static_cast<SpeechEventStack<MySpeechEvent>*>(userdata)->push(buffer, buflen, state, sourceId, info, infolen);
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
        p.add("enable-spcn", '\0', "Enable speculative connection");
        p.add<std::string>("lang",'\0',"Language code", false, "ja");
        p.add<std::string>("services",'\0',"mimi services", false, "asr");
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

		// Set connection params with using command line params.
	    Stream::ConnectionParam param(p.get<std::string>("host"), p.get<int>("port"), access_token,
	    		p.get<int>("rate"), p.get<int>("channel"), af, p.exist("verbose"), p.get<std::string>("lang"), p.get<std::string>("services"), p.exist("enable-spcn"));
	    // Create event stack
	    SpeechEventStack<MySpeechEvent> eventStack(param);
		// Initialize mimixfe
	    int xfe_errorno = 0;
	    mimixfe::XFESourceConfig s;
	    mimixfe::XFEECConfig e;
	    mimixfe::XFEVADConfig v;
	    mimixfe::XFEBeamformerConfig b;
	    mimixfe::XFEStaticLocalizerConfig c({mimixfe::Direction(270, 90)});
	    mimixfe::XFERecorder rec(s,e,v,b,c,recorderCallback,reinterpret_cast<void*>(&eventStack));
	    rec.setLogLevel(LOG_UPTO(LOG_DEBUG));
	    if(signal(SIGINT, sig_handler_) == SIG_ERR){ // For backward compatibility
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
	    // Loop for monitoring
	    int usec = 100000; // 0.1sec, you should choose appropriate value
	    while(rec.isActive()){
			 usleep(usec);
			 if(interrupt_flag_ != 0){
				 if (p.exist("verbose")) {
					 std::cerr << "Stream is to be finished..." << std::endl;
				 }
				 rec.stop(); // Ensure new sessions will never be created
				 eventStack.abort(); // Abort if any sessions exist in the queue, no sessions might be in the queue in typical cases.
			 }
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



