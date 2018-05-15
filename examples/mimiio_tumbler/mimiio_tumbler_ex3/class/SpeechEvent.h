/*
 * @file SpeechEvent.h
 * \~english
 * @brief 
 * \~japanese
 * @brief 発話（アタランス）イベントの処理の定義
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/03/13
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
#ifndef EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_APPLICATION_H_
#define EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_APPLICATION_H_

#include <string>
#include <vector>
#include <memory>

#include <XFERecorder.h>
#include "Stream.h"
#include "../../include/BlockingQueue.h"

template <class SE> class SpeechEventStack;

/**
 * @class SpeechEvent
 * @brief 発話イベントとその処理を簡便のためまとめたクラス。開発者は、このクラスのメンバー関数をオーバーラードして独自の応答処理を実装する。
 */
class SpeechEvent
{
	template <class SE> friend class SpeechEventStack;
public:
	using Ptr = std::unique_ptr<SpeechEvent>;

	SpeechEvent();
	virtual ~SpeechEvent();

	/**
	 * @brief 発話開始検出された時点で呼び出される関数
	 * @param [in] streamInfo libmimixfe で定義される音声ストリーム情報
	 * @details 発話開始検出された時点で 1 回のみ呼び出される
	 */
	virtual void speechStartHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo);

	/**
	 * @brief 発話中に一定時間ごとに呼び出される関数
	 * @param [in] streamInfo libmimixfe で定義される音声ストリーム情報
	 * @details libmimixfe の録音コールバック関数が発話中ステートで呼び出される都度、この関数も呼び出される
	 */
	virtual void inSpeechHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo);

	/**
	 * @brief 発話終了検出された時点で呼び出される関数
	 * @param [in] streamInfo libmimixfe で定義される音声ストリーム情報
	 * @details 発話終了検出された時点で 1 回のみ呼び出される
	 */
	virtual void speechEndHandler(int sourceId, const std::vector<mimixfe::StreamInfo>& streamInfo);

	/**
	 * @brief libmimiio によってリモートホスト側から何らかの応答を受信する都度呼び出される関数
	 * @param [in] response リモートホストからの応答内容文字列、典型的には JSON 文字列となる
	 * @details libmimiio の受信コールバックあkン数が呼び出される都度、この関数が呼び出される
	 */
	virtual void responseHandler(int sourceId, const std::string& response);

	/**
	 * @brief libmimixfe, libmimiio, 本サンプルプログラム内部のどちらかで何らかの継続不可能なエラーが起こった場合に呼び出される関数
	 * @param [in[ errorno エラー番号
	 */
	virtual void errorHandler(int errorno);

	/**
	 * @brief 一つ過去のイベントへのポインタを返す。発話イベント群は Linked List 構造で過去を辿ることができる。
	 * @return 一つ過去のイベントへのポインタ
	 */
	SpeechEvent* prev() const { return prev_; }

	/**
	 * @brief イベントID を返す
	 * @return イベントID
	 */
	int eventId() { return eventId_.load(); }

protected:
	SpeechEvent* prev_;
	void finish();

private:
	// SpeechEventStack がフレンドクラスとなっていることに留意
	bool isFinished() { return !eventLoopContinue_.load(); }
	void start();
	std::future<void> eventLoop_;
	std::atomic<bool> eventLoopContinue_;
	Stream::Ptr stream_;
	std::atomic<int> eventId_;
};

#endif /* EXAMPLES_MIMIIO_TUMBLER_MIMIIO_TUMBLER_EX3_APPLICATION_H_ */
