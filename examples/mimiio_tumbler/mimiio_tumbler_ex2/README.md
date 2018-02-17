# mimiio_tumbler_ex2

## はじめに

### 概要

Fairy I/O Tumbler 上で、libmimixfe と組み合わせ（機能制限有）、録音及び信号処理された音声を入力として、リアルタイムでリモートホストに送信し、リアルタイムでリモートホストから認識結果を受信する最もシンプルなサンプルプログラムです。サンプルプログラムの単純化のために、機能制限として、libmimixfe の設定上、同時検出する音源数が 1 の場合のみに対応し、同時複数音源を抽出する設定になっている場合には利用することができません。

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) と異なり、このサンプルプログラムでは、libmimixfe の VAD の結果を利用します。すなわち、プログラムの開始時点では、libmimixfe による音声録音及び信号処理が開始されるのみで、リモートホストへの接続は開かれませず、VAD によって発話開始が検出されたときに初めて libmimiio を利用してリモートホストへの接続を開き、音声送信及び結果受信を開始します。VAD によって発話終了が検出された時点でリモートホストに `recog-break` 命令が送信され、最終結果が得られます。VAD が検出したひとつの発話区間ごとに `recog-in-progress` ステートでの途中結果と `recog-finished` ステートでの最終結果が得られる実用的なサンプルプログラムです。

このサンプルプログラムと同一の機能を実現するために、他の設計を取ることも可能です。この設計は、デモンストレーションのためにシンプル化した設計となっていることに留意してください。

### 全体動作フロー

1. プログラムが開始される
2. コマンドライン引数を解析する
3. libmimixfe により、XFERecorder を構築し、録音及び信号処理を開始する
4. libmimixfe の VAD により発話開始が検出された時点で、libmimiio によりリモートホストへの接続を開き、WebSocket 通信を開始する
5. 各種コールバック関数によって、録音及び信号処理された音声が適切にリモートホストに送信され、結果が受信される
6. libmimixfe の VAD により発話終了が検出された時点で、libmimiio によりリモートホストに最終結果を得るための命令（`recog-break` 命令）が送信される。
7. 最終結果が受信される
8. 4 から 7 を任意回数繰り返す
9. Ctrl+C の入力により、全体が終了する

### コマンドライン引数

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) と同様です

## 主要部解説

### 接続情報の準備

`````````.cpp
Session::ConnectionParam param(p.get<std::string>("host"), p.get<int>("port"), access_token,
			 p.get<int>("rate"), p.get<int>("channel"), af, p.exist("verbose"));
Session session(param);
``````````

`ConnectionParam` クラスはリモートホストへの接続情報等をまとめたデータクラスです。`Session` クラスは後述しますが、発話単位の音声データと通信ストリームを保持するクラスです。`XFERecorder` のコンストラクタのユーザー定義データとして `Session` クラスのインスタンスのポインタを与えます。

### libmimixfe による XFERecorder の初期化と録音の開始

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) とほぼ同様です。一部 XFE のパラメータを高速寄りにチューニングしている場合があります。ユーザー定義データとして、上記の通り、`Session` クラスのインスタンスのポインタを与えています。

### メインスレッドの監視待機

``````````.cpp
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
``````````

メインスレッドは、`XFERecorder` の状態のみを定期的に確認しながら待機するためのループに入ります。このループがビジーループにならないように、適切な時間を `sleep()` する必要があります。このスリープ時間は応用アプリケーションによって適切に決定してください。

ユーザーによって Ctrl+C が入力されると、interrupt_flag_ が 0 以外の値となります。録音を終了し、待機ループを抜け全体が終了します。

### libmimixfe 録音コールバック関数の実装

``````````.cpp
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
``````````

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) との差に着目してください。`userdata*` からは後述する `Session` クラスを取り出しています。`Session` クラスには、ex1 でも利用したブロッキングキューがあり、`addAudioData()` 関数により、ブロッキングキューにデータを push しているところは同じです。

libmimixfe の VAD の判定結果は、このコールバック関数の第三引数 `state` に与えられます。発話の開始（`mimixfe::SpeechState::SpeechStart`）が検出された時点で、セッションを開き（`open()`）、通信を開始（`start()`）します。

発話の終了（`mimixfe::SpeechState::SpeechEnd`）が検出された時点で、最終結果を得るための命令(`recog-break`)を送信しています。

##### 留意点

この実装では、`session->open()` において、直列に `mimi_open()` が呼び出されているため、接続を開くための待ち時間が発生し、ブロックされます。典型的には 100 ミリ秒以下程度の待ち時間ですが、この実装のように、コールバック関数内で、本質的には待ち時間が不明である処理を直列で実行しコールバック関数の動作をブロックすることは、好ましい実装ではありません。より適切には、`session->open()` の呼び出しはブロックしないようにし、別のスレッドで接続のオープンが試行される形とするべきですが、このサンプルプログラムでは全体のシンプル化のために、このような実装としています。プロダクションにおいては、そのような実装を推奨します。

### Session クラスの実装

#### コンストラクタとデストラクタ

``````````.cpp
class Session
{
public:	
	Session(const ConnectionParam& param) : param_(param){}
	~Session(){ if(mio_ != nullptr) mimi_close(mio_); }
``````````

コンストラクタでは、リモートホストへの接続情報を受け取ります。このサンプルプログラムの設計では、デストラクタは不要ですが、形式的にリソースの解放を行っています。

#### リモートホストへの接続を開く

``````````.cpp
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
``````````

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) と同様ですが、`mimi_open()` が何らかのテンポラリな要因で失敗した場合のリトライを行う例を示しています。

このサンプルプログラムでは、簡単のため、`Session` クラスのひとつのインスタンスを使い回すので、冒頭で、前回の接続が残っている場合には、その接続が終了するまでやはり直列に待つ簡易的な実装としています。

#### リモートホストとの WebSocket 通信を開始する

``````````.cpp

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

``````````

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) と同様です。


#### リモートホストとの接続を終了する

``````````.cpp

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

``````````

このサンプルプログラムでは、リモートホストとの WebScoket 通信が健全であるかどうかを通信中に別スレッドから監視していません。`close()` が呼ばれるタイミングは、`open()` 関数の冒頭のみであり、つまり、「次回の」発話検出時点で、「今回の」発話のために使っていた通信ストリームに対する `close()` が呼ばれるということになります。 mimi(R) WebSocket API Service では、正常終了した場合には、RFC6455 に従って、リモートホスト側から WebSocket 接続を切断します。従って、発話と発話の間隔が十分大きい場合、`close()` が呼ばれた時点では、既にその対象とする通信ストリームは終了していることになります。このため、`mimi_is_active()` は false を返し、冒頭のループは実行されません。つまり、`close()` 関数は、`mimi_close()` により、関連するメモリが解放されるのみの動作となります。

発話と発話の間隔が短い場合、つまり、前回の発話終了検出後、最終結果を受信する前に、今回の発話が開始されてしまった場合は、冒頭のループで、前回の発話の最終結果が受信されるまで（すなわち、リモートホスト側から WebSocket 接続が切断されるまで）、待機されます。その後、`mimi_close()` が呼ばれることになります。

実際的には、libmimixfe の VAD の設定に依存しますが、発話と発話の間隔は一定以上確保されます。これは、発話を取りこぼすということではなく、短い無音時間は発話継続であると判定されるということです。別発話に分かれるためには、典型的には 1 秒程度の発話間隔が必要です。そして、リモートホストは、この発話間隔に対しては十分高速であり、典型的には、冒頭のループが実行されることはありません。

このサンプルプログラムでは、簡単のため、`Session` クラスのひとつのインスタンスを使い回すので、次の接続のために、`recog_break` フラグを元に戻しておきます。

#### `recog-break` 命令の送信

``````````.cpp
	/**
	 * @brief Send recog-break
	 */
	void recogBreak(){
		sdata_.recog_break_.store(true);
	}
``````````

libmimiio の音声送信コールバック関数から、データ送信の終了を宣言する `recog-break` 命令を送信するために、共有の bool 型変数 `recog_break` に true を与えます。この変数は、libmimixfe の録音コールバック関数が実行されているスレッドから write され、libmimiio の音声送信スレッドから read される共有変数であり、`std::atomic<>` によりデータ競合を防いでいます。

#### リモートホストへ送信するべき音声データの取扱い

``````````.cpp

	/**
	 * @brief Add single audio sample to queue
	 * @param [in] sample Single audio sample
	 */
	void addAudioData(const short& sample) { sdata_.queue_.push(sample); }
``````````

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) でも利用した `SampleQueue` に音声データを push するだけのメンバー関数を用意しています。単に形式的にカプセル化したものであり、`queue_` を public にして直接 push すればこの関数は不要です。

### libmimiio 音声送信コールバック関数の実装

``````````.cpp
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
``````````

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) とほぼ同様です。ユーザー定義データから、`recog_break` フラグも取り出していることに留意してください。

また、キューからデータを取り出す**前**に `recog_break` を取り出していることに留意してください。データプロデューサー側（録音コールバック関数）では、先にデータを詰めてから後で `recog_break` フラグをセットしていることを思い出してください。つまり、データコンシューマー側であるこのコールバック関数が呼ばれた時に、もし `recog_break` フラグがセットされていれば、その時点でのキューを読み切れば、少なくとも、その発話についての音声データは取り切れるということになります。これとは逆に、データコンシューマー側でキューからデータを取り出した**後**に `recog_break` を評価すると、その時間差で、その発話についての音声データの末尾を取りこぼすことになりますので注意してください。

##### 留意点

この実装は、簡単のために、簡易的な設計に基づいており、不適切な動作の原因となり得ます。上記動作のために、`recog_break` フラグを評価した次の行で、その時点でのキューサイズを確定していますが、この 1 行の間に、データプロデューサー側が、recog_break フラグをオフにして、さらに新しいデータを追加してしまっていることが**ない**と保証することはできません（実際には、この実装においては、そのようなことが起こるとは想定しにくいですが…）。

より堅牢な設計として、recog_break フラグを、この実装のようにキューと別に持つのではなく、キューのデータ要素自体がデータの終了等を示すことができるように、要素自体を short 型ではなく、short 型のサンプル値を含む、何らかの構造体とするという設計を推奨します。

### libmimiio 結果受信コールバック関数の実装

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) と同様です。
