# mimiio_tumbler_ex1

## はじめに

### 概要

Fairy I/O Tumbler T-01 上で、libmimixfe と組み合わせ（機能制限有）、録音及び信号処理された音声を入力として、リアルタイムでリモートホストに送信し、リアルタイムでリモートホストから認識結果を受信する最もシンプルなサンプルプログラムです。サンプルプログラムの単純化のために、機能制限として、libmimixfe の設定上、同時検出する音源数が 1 の場合のみに対応し、同時複数音源を抽出する設定になっている場合には利用することができません。

本サンプルプログラムでは、最もシンプルな実装とするために、libmimixfe の VAD の結果を無視します。プログラムの開始時点で、リモートホストへの接続を開き、発話区間検出状況に関わらず、リモートホストへのコネクションを維持し、Ctrl+C によってプログラムの終了が指示された際に、最終認識結果を得ます。このため、最終認識結果を除き、途中得られる認識結果は、発話区間検出状況によらず、全て `recog-in-progress` ステートになります。

本サンプルプログラムと同一の機能を実現するために、他の設計を取ることも可能です。この設計は、デモンストレーションのためにシンプル化した設計となっていることに留意してください。

### 全体動作フロー

- プログラムが開始される
- コマンドライン引数を解析する
- libmimixfe により、XFERecorder を構築し、録音を開始する
- libmimiio により、リモートホストへの接続を開き、Websocket 通信を開始する
- 各種コールバック関数によって、録音及び信号処理された音声が適切にリモートホストに送信され、結果が受信される
- Ctrl+C の入力により、最終結果を得るための命令がリモートホストに送信される
- 最終結果が受信され、全体が終了する

### コマンドライン引数

``````````.sh
usage: ./mimiio_tumbler_ex1 --host=string --port=int [options] ...
options:
  -h, --host       Host name (string)
  -p, --port       Port (int)
  -t, --token      Access token (string [=])
      --rate       Sampling rate (int [=16000])
      --channel    Number of channels (int [=1])
      --format     Audio format (string [=MIMIIO_RAW_PCM])
      --verbose    Verbose mode
      --help       Show help

Acceptable audio formats:
    MIMIIO_RAW_PCM
    MIMIIO_FLAC_0
    MIMIIO_FLAC_1
    MIMIIO_FLAC_2
    MIMIIO_FLAC_3
    MIMIIO_FLAC_4
    MIMIIO_FLAC_5
    MIMIIO_FLAC_6
    MIMIIO_FLAC_7
    MIMIIO_FLAC_8
    MIMIIO_FLAC_PASS_THROUGH
``````````

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
を呼ぶことで実現できますが、`XFERecorder` クラスのデストラクタによって録音は自動的に終了されます。従って、この例のように、直後にプログラム全体が終了し、デストラクタが呼ばれることが明らかな場面で `rec.stop()` を明示的に呼ぶ必要はありません（呼んでも構いません）。

この時点で、`mimi_open()` 関数の引数に与えた音声送信コールバック関数と結果受信コールバック関数が libmimiio によって呼び出され始めることに留意してください。以上で、録音・信号処理・通信ストリーム確立・データ送信・結果受信について定常的に実行されている実行状態に入ります。

### メインスレッドの監視待機

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
	SampleQueue* queue = reinterpret_cast<SampleQueue*>(userdata);
	for(size_t i=0;i<buflen;++i){
		queue->push(buffer[i]);
	}
}
``````````

libmimixfe 録音コールバック関数の詳細は、libmimixfe のドキュメントを参照してください。ここでは、ユーザー定義データとして渡される `userdata*` から `SampleQueue*` を取り出して、libmimixfe の出力音声データを `SampleQueue` に逐次 push しているだけとなります。

ここで `SampleQueue` の実態は、 [../../include/BlockingQueue.h](https://github.com/FairyDevicesRD/libmimiio/blob/master/examples/include/BlockingQueue.h) に定義されている、

``````````.cpp
/**
 * \~english Blocking queue with the short type of element.
 * \~japanese Short 型のサンプルを要素に持つブロッキングキューの型エイリアス
 */
using SampleQueue = BlockingQueue<short>;
``````````

です。つまり、要素がひとつの short 型の値であるブロッキングキューです。キューはデータを push して詰め込み、pop して取り出すことができるデータ構造であり、複数のサンプルプログラムで共通に利用されています。ブロッキングキューと言っているのは、pop のときにデータがなければブロックされるキューであるという意味で、`push()` 関数、`pop()` 関数はスレッドセーフです。

### libmimiio 音声送信コールバック関数の実装

``````````.cpp
void txfunc(char *buffer, size_t *len, bool *recog_break, int* txfunc_error, void* userdata)
{
	if(interrupt_flag_ != 0){
		*recog_break = true;
		return;
	}

	std::chrono::milliseconds timeout(1000); // queue timeout
	SampleQueue* queue = static_cast<SampleQueue*>(userdata);
	auto current_queue_size = queue->size();
	int length = 0;
	std::vector<short> tmp;
	for(auto i=0;i<current_queue_size;i+=2){
		short sample = 0;
		if(queue->pop(sample, timeout)){
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

libmimiio で送信用音声を準備するためのコールバック関数で、libmimiio から定期的（約 0.1 秒ごと）に呼ばれます。ここでも `recorderCallback` と同様に、ユーザー定義データである `userdata*` から `SampleQueue*` を取り出しています。この `SampleQueue` から `recorderCallback` によって詰め込まれた音声データを取り出して、送信用データとしています。

`SampleQueue` から pop する回数として、このコールバック関数が呼ばれた時点でのキューサイズを利用しています。キューサイズ以上の回数を pop していないので、この pop では待ち時間は発生しないことが想定されます。このため、pop のタイムアウトは想定外の異常系に対応する目的で短く設定しています。また、キューサイズがゼロだった場合にはこのコールバック関数の実装では何も起こりません、いわば「空回り」することになります。空回りしたとしても、libmimiio 側で適切に制御されているため問題はありません。

`pop()` がタイムアウトしたときは、ユーザー定義エラーを発生させ、libmimiio に接続を切断するように要求します。`txfunc_error` に 0 以外の値が設定された場合に、libmimiio はユーザー定義エラーが発生したと認識し、接続の切断を試みます。ここでは、-100 を設定しています。libmimiio が持つ既定のエラーコードはすべて正の値であるため、ユーザー定義エラーコードは、負の値とすることを推奨しています。

libmimiio に与える送信用データは char* 型であり、`sizeof(short)` は 2 であることから、ひとつの short 型サンプルを与えた時に、`len` を +2 ずつ増やしていることに留意してください。

ここで、別の実装案として、`SampleQueue` から pop する回数を固定値にするという案があります。その場合、キューサイズ以上の回数を pop しようとするため、pop で待ち時間が発生します。その代わり、送信データチャンクサイズが毎回一定化できることと、pop で適切にブロックされるため、このコールバック関数が空回りすることを避けることができます。しかしながら、mimi API Service では、送信データチャンクサイズを毎回一定化することに速度・性能上のメリットはほぼありません（リモートホスト側で適切にバッファリングされています）。また、コールバック関数が空回りすることのデメリットもありませんので、pop する回数を固定値にすることのメリットはほぼありません。何らかの理由で、固定値にしなければならないとき、キューがブロックしている状態を解除するためには、キューに何らかのマジックナンバーを与えてやる（例えば、0）などする必要があることと、pop での待ち時間を、プログラムの全動作フローを鑑みて最長の値としなければならないことに留意してください。

最後に、コールバック関数の実装の冒頭

``````````.cpp
if(interrupt_flag_ != 0){
	*recog_break = true;
	return;
}
``````````

において、ユーザーが Ctrl+C を入力したときの動作を実装しています。ユーザーが Ctrl+C を入力した場合、`interrupt_flag_` が 0 以外の値になるため、その場合、`recog_break` を true とすることで、発話が終了したことを libmimiio に通知します。これにより、libmimiio は二度とこのコールバック関数を呼び出すことはなくなり、音声送信は終了したとされます。

libmimiio は `recog_break` に true が与えられた時、リモートホストに対して WebSocket のテキストフレーム上で recog-break 命令を送信します。リモートホストは recog-break 命令を受け付けた場合、これまでに受け取った音声データが全部であるとして、その最終認識結果を `recog-finished` ステートで返すことになります。この最終認識結果の計算には若干の時間を要します。

### libmimiio 結果受信コールバック関数の実装

``````````.cpp
void rxfunc(const char* result, size_t len, int* rxfunc_error, void *userdata)
{
	std::string s(result, len);
	std::cout << s << std::endl;
}
``````````

受け取った結果をそのまま画面に表示しています。



















