/*
 * @file user_program.cpp
 * \~english
 * @brief User program example
 * \~japanese
 * @brief ユーザープログラムの例
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/02/18
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

#include "user_program.h"
#include <unistd.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>

#include <tumbler/ledring.h>

// For JSON parsing
#include <Poco/JSON/JSON.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/ParseHandler.h>
#include <Poco/Dynamic/Var.h>
#include <sstream>

void UserProgram::task(
		BlockingQueue<std::string>& recog_result, std::atomic<mimixfe::SpeechState>& state, std::atomic<bool>& finish)
{
	// Initial values for current state
	bool uttr_end_ring = false;
	std::future<void> led_f;
	std::atomic<bool> led_rot_cont(true);

	while(!finish.load()){
		std::string result;
		recog_result.pop(result);
		if(result == "__finish__") break;

		Poco::JSON::Parser parser;
		Poco::DynamicStruct dsresult = *(parser.parse(result).extract<Poco::JSON::Object::Ptr>());
		auto response_size = dsresult["response"].size();
		std::stringstream s;
		for(auto i=0;i<response_size;++i){
			s << dsresult["response"][i]["result"].toString();
		}
		if(s.str() == "") continue;

		if(state.load() == mimixfe::SpeechState::SpeechEnd && !uttr_end_ring){
			uttr_end_ring = true;
			// LED リングの点灯変更
			std::cout << "user program: led ring change (async)" << std::endl;
			led_f = std::async(std::launch::async, [](std::atomic<bool>& led_rot_cont){
					tumbler::LEDRing& ring = tumbler::LEDRing::getInstance();
					tumbler::LED background(0,0,100);
					for(int i=0;i<18;++i){
						tumbler::Frame frame(background);
						frame.setLED(i, tumbler::LED(0,255,0));
						ring.addFrame(frame);
					}
					ring.setFPS(30);
					while(led_rot_cont.load()){
						ring.show(false);
					}
			}, std::ref(led_rot_cont));
		}

		if(dsresult["status"] == "recog-finished"){
			std::cout << "user program: fixed result(" << response_size << ") = " << s.str() << std::endl;

			/// DO SOME VALUABLE TASK HERE ///
			sleep(2); // replace this!
			std::cout << "user program: ok" << std::endl;
			//////////////////////////////////

			led_rot_cont.store(false);
			led_f.get();
			tumbler::LEDRing& ring = tumbler::LEDRing::getInstance();
			ring.reset(true);
			uttr_end_ring = false;
			led_rot_cont.store(true);
		}
	}
	std::cerr << "User program task is finished." << std::endl;
}

UserProgram::UserProgram(BlockingQueue<std::string>& recog_result,  std::atomic<mimixfe::SpeechState>& state) :
		recog_result_(recog_result), state_(state), finish_(false)
{}

UserProgram::~UserProgram()
{
	finish();
	task_f_.get();
}

void UserProgram::start()
{
	std::cerr << "User program started." << std::endl;
	task_f_ = std::async(std::launch::async, UserProgram::task, std::ref(recog_result_), std::ref(state_), std::ref(finish_));
}

void UserProgram::finish()
{
	recog_result_.push("__finish__");
}


