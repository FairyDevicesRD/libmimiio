# mimiio_tumbler_ex1

## 概要

Fairy I/O Tumbler T-01 上で、libmimixfe と組み合わせ（機能制限有）、録音及び信号処理された音声を入力として、リアルタイムでリモートホストに送信し、リアルタイムでリモートホストから認識結果を受信する最もシンプルなサンプルプログラムです。サンプルプログラムの単純化のために、機能制限として、libmimixfe の設定上、同時検出する音源数が 1 の場合のみに対応し、同時複数音源を抽出する設定になっている場合には利用することができません。

本サンプルプログラムでは、最もシンプルな実装とするために、libmimixfe の VAD の結果を無視します。プログラムの開始時点で、リモートホストへの接続を開き、発話区間検出状況に関わらず、リモートホストへのコネクションを維持し、Ctrl+C によってプログラムの終了が指示された際に、最終認識結果を得ます。このため、最終認識結果を除き、途中得られる認識結果は、発話区間検出状況によらず、全て `recog-in-progress` ステートになります。

本サンプルプログラムと同一の機能を実現するために、他の設計を取ることも可能です。この設計は、デモンストレーションのためにシンプル化した設計となっていることに留意してください。

## 動作フロー

- プログラムが開始される
- コマンドライン引数を解析する
- libmimixfe により、XFERecorder を構築し、録音を開始する
- libmimiio により、リモートホストへの接続を開き、Websocket 通信を開始する
- 各種コールバック関数によって、録音及び信号処理された音声が、適切にリモートホストに送信され、結果が受信される。
- Ctrl+C の入力により、最終結果を得るための命令がリモートホストに送信される
- 最終結果が受信され、全体が終了する

## 主要部解説

### libmimixfe による XFERecorder の初期化と録音の開始

``````````.cpp
int xfe_errorno = 0;
mimixfe::XFESourceConfig s;
mimixfe::XFEECConfig e;
mimixfe::XFEVADConfig v;
mimixfe::XFEBeamformerConfig b;
mimixfe::XFEStaticLocalizerConfig c({mimixfe::Direction(270, 90)});
mimixfe::XFERecorder rec(s,e,v,b,c,recorderCallback,reinterpret_cast<void*>(&queue));
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
}
``````````

前半部分で、libmimixfe に対して、Tumbler 正面固定方向の音源のみを収録することを指定します。エコーキャンセラ、VAD 等が全てデフォルト設定で有効になっています。次に、ログレベルを LOG_DEBUG に設定しています。この辺りの実装は、libmimixfe のサンプルプログラムも参考にしてください。

次に、本サンプルプログラムのためのシグナルハンドラを設定します。シグナルハンドラの設定位置は、本来は `main()` 関数の冒頭となりますが、libmimixfe の後方互換性のために、この位置に書いています。新しい libmimixfe をご利用の場合は、通常通り `main()` 関数の冒頭で問題ありません。

次に、`start()` 関数によって録音を開始します。`p.exist("verbose")` は、コマンドライン引数に `--verbose` が含まれているときに true となります。録音が開始されると、`/var/log/syslog` の tumbler エントリで libmimixfe の録音・信号処理ログが出力されます。ログレベルによって、ログ内容がフィルターされることに留意してください。

### libmimiio によるリモートホストへの接続オープンと WebSocket 通信の開始

#### 実行時設定の準備

``````````.cpp
// Prepare mimi runtime configuration
size_t header_size = 0;
MIMIIO_HTTP_REQUEST_HEADER *h = nullptr;
``````````

本サンプルプログラムでは、実行時設定を行っていません。 実際は mimi API Service のドキュメントに従って、適切な実行時設定を与えるようにしてください。

#### リモートホストへの接続を開く

``````````.cpp
// Open mimi stream
int errorno = 0;
MIMI_IO *mio = mimi_open(
	p.get<std::string>("host").c_str(), p.get<int>("port"), txfunc, rxfunc,
	static_cast<void*>(&queue), static_cast<void*>(&queue), af, p.get<int>("rate"), p.get<int>("channel"), h,
	header_size, access_token, MIMIIO_LOG_DEBUG, &errorno);
if(mio == nullptr){
	std::cerr << "Could not initialize mimi(R) service. mimi_open() failed: " << mimi_strerror(errorno) << " (" << errorno << ")"<< "\n";		 
	return 3;
}
if (p.exist("verbose")) {
	std::cerr << "mimi connection is successfully opened." << std::endl;
}
``````````

libmimiio の仕様に従ってリモートホストへの接続を開きます。ここで、コールバック関数に与えるユーザー定義データとして `SampleQueue*` を与えていることに留意してください。このリモートホストへの接続には、短い時間が掛かります。その間、 `XFERecorder` によって録音が継続されていますが、その録音データは、後述する形で、`SampleQueue` 型のキューに蓄積され続けていることに留意してください。この時点で、`XFERecorder` クラスのコンストラクタに与えた録音コールバック関数が libmimixfe によって呼び出され始めることに留意してください。

#### リモートホストとの WebSocket 通信を開始する

``````````.cpp

//Start mimi(R) stream
errorno = mimi_start(mio);
if(errorno != 0){
	std::cerr << "Could not start mimi(R) service. mimi_start() filed. See syslog in detail.\n";
	mimi_close(mio);
	// rec.stop(); // you don't have to call rec.stop() because destructor of XFERecorder class will clean up automatically.
	return 3;
}
if (p.exist("verbose")) {
	std::cerr << "mimi connection is successfully started." << std::endl;
	std::cerr << "Ready..." << std::endl;
}
``````````

`mimi_start()` 関数により、リモートホストとの WebSocket 通信を開始します。接続が開始できなかった場合には、`errorno` にゼロ以外の値が返されます。このときも関連リソースをクリーンアップするために、`mimi_close()` 関数を必ず呼ぶようにしてください。また、この時点では、既に録音が開始されています。録音の明示的な終了は `stop()` 関数
を呼ぶことで実現できますが、`XFERecorder` クラスのデストラクタによって録音は自動的に終了されます。従って、この例のように、直後にプログラム全体が終了し、デストラクタが呼ばれることが明らかな場面で `rec.stop()` を明示的に呼ぶ必要はありません。呼んでも構いません。

この時点で、`mimi_open()` 関数の引数に与えた音声送信コールバック関数と結果受信コールバック関数が libmimiio によって呼び出され始めることに留意してください。以上で、録音・信号処理・通信ストリーム確立・データ送信・結果受信について定常的に実行されている実行状態に入ります。

#### メインスレッドの監視待機

``````````.cpp
int usec = 100000; // 0.1sec, you should choose appropriate value
while(rec.isActive() && mimi_is_active(mio)){
	if(interrupt_flag_ != 0){
		if (p.exist("verbose")) {
			std::cerr << "Waiting for fixed result..." << std::endl;
		}
		rec.stop();
		while(mimi_is_active(mio)){
			usleep(usec);
		}
	}
	usleep(usec);
}
if (p.exist("verbose")) {
	std::cerr << "Stream is to be finished." << std::endl;
}

``````````

メインスレッドは、`XFERecorder` の状態と、リモートホストとの通信ストリームの状態をそれぞれ `isActive()` 関数、`mimi_is_active()` 関数により定期的に確認しながら待機するためのループに入ります。このループがビジーループにならないように、適切な時間を `sleep()` する必要があります。このスリープ時間は応用アプリケーションによって適切に決定してください。

ユーザーによって Ctrl+C が入力されると、`interrupt_flag_` が 0 以外の値となります。このとき、後述する音声送信コールバック関数によって同時に最終結果取得命令が送られているので、待機ループ側では、これ以上録音を継続する必要が無いため、この時点で `stop()` 関数によって、明示的に録音終了を要求します。その後、リモートホストから最終結果が送られてくるのを、さらに待ちます。この時点で、音声送信が既に終了していますが、`mimi_is_active()` は音声送信と結果受信の両方が終了した時にはじめて false を返すため、

``````````.cpp
while(mimi_is_active(mio)){
	usleep(usec);
}
``````````

によって、リモートホストから最終結果が戻され、接続が終了するまで待つということが実現されます。以上が `main()` 関数の主要部となります。

### 録音コールバック関数の実装










