/*
 * @file user_program.h
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
#ifndef EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_USER_PROGRAM_H_
#define EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_USER_PROGRAM_H_

#include "../../include/BlockingQueue.h"
#include <XFETypedef.h>
#include <atomic>
#include <future>

class UserProgram
{
public:
	static void task(
			BlockingQueue<std::string>& recog_result, std::atomic<mimixfe::SpeechState>& state, std::atomic<bool>& finish);

	UserProgram(BlockingQueue<std::string>& recog_result, std::atomic<mimixfe::SpeechState>& state);
	~UserProgram();
	void start();
	void finish();
private:
	BlockingQueue<std::string>& recog_result_;
	std::atomic<mimixfe::SpeechState>& state_;
	std::atomic<bool> finish_;
	std::future<void> task_f_;
};



#endif /* EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_USER_PROGRAM_H_ */
