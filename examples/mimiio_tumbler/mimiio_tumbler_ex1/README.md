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

次に、`start()` 関数によって録音を開始します。

### libmimiio によるリモートホストへの接続オープンと WebSocket 通信の開始











