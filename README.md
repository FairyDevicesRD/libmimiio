# libmimiio

[![Apache License](http://img.shields.io/badge/license-APACHE2-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

libmimiio - mimi(R) WebSocket API Service client library

## 概要

WebSocket (RFC6455) 通信を利用した mimi(R) WebSocket API Service を簡単に利用するためのライブラリです。 mimi(R) との通信仕様は公開されていますので、必ずしも本ライブラリを利用する必要はありません。本ライブラリ自体は C++11 で実装されていますが、ライブラリの外部 API は C の API を提供しています。

#### ライセンスについてのご注意

本ライブラリおよびサンプルプログラムは、オープンソースソフトウェアとして公開しており、Apache License 2.0 のもとで、商用製品への組み込みを含め、自由にご利用いただけます。

ご利用の際は、本ライブラリのライセンスに加え、関連ライブラリのライセンスにも従う必要がございますので、ご留意いただけますようお願いいたします。

特に、モバイルやデスクトップのアプリなど、関連ライブラリを静的リンクや同梱して配布する際には、本ライブラリおよび以下のライブラリのライセンス表記を行う必要がある旨ご注意ください。

- OpenSSL （利用する場合）
- FLAC （利用する場合）
- cmdline ( `examples` 配下のプログラムを配布する場合）

## 構築

#### 必須

- Poco C++ libraries Complete Edition 1.8.0 以上。 Tumbler 向けのプレビルドライブラリは[こちら](https://github.com/FairyDevicesRD/tumbler.poco)、それ以外の一部の環境向けのプレビルドライブラリは[こちら](https://github.com/FairyDevicesRD/poco)に用意されています。
- pkg-config (configure 時。バージョン 0.29 で動作確認)
- libflac++ 1.3.0 以上

#### オプション

- portaudio v19_2014 以上（mimiio_pa/ 以下のサンプルプログラムのビルドのためのみに用いられます）
- libmimixfe 1.0 以上（mimiio_tumbler/ 以下のサンプルプログラムのビルドのためのみに用いられます）

#### Poco プレビルドライブラリの依存ライブラリについて

上記のプレビルドライブラリを用いる場合、Poco のビルドに必要な Poco が依存しているライブラリ群が環境中に含まれていない場合があります。その場合、libmimiio の configure に失敗するため、config.log を確認するなどして、Poco の依存ライブラリの有無を確認してください。

### ビルド

#### Linux

``````````.sh
$ autoreconf -vif
$ ./configure
$ make
$ sudo make install
``````````
- gcc 5 以上が必要です
- autoreconf が古い場合 configure が通らない場合があります。その場合は autoreconf を更新してください。

#### Fairy I/O Tumbler

``````````.sh
$ autoreconf -vif
$ ./configure --with-mimixfe-prefix=/path/to/mimixfe
$ make
$ sudo make install
``````````

- libmimixfe の位置をマニュアルで指定する必要があります
- `configure` の代わりに、`configure-tumbler` を利用することで、より最適化されたバイナリが生成できます。このスクリプトは、内部的に `configure` を呼び出しており、ビルド時の各種オプションを設定する簡易的なスクリプトです。 発展的な内容を含むため、スクリプトの内容を理解した上でご利用ください。 libmimiio は通信プログラムであるため、この最適化による効果は大きくはありません。

#### Mac OS X

``````````.sh
$ autoreconf -vif
$ CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang \
CXX=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++  ./configure
$ make
$ sudo make install
``````````
- Apple LLVM version 8.1.0 (clang-802.0.42) 以上が必要です

#### Windows

mingw, cygwin 等を利用し、Linux に準じて適宜ビルドしてください。

#### configure オプションについて

標準の configure オプション以外に、libmimiio は以下の configure オプションをサポートしています。すべてのオプションは `./configure --help` で確認することができます。

- `--enable-debug` libmimiio が `-g -O0` でコンパイルされる。サンプルプログラムは libmimiio をスタティックリンクする
- `--with-mimixfe-prefix` libmimixfe の場所を指定する。例として `/home/foo/libmimixfe` 以下に `include/`, `lib/` がある場合は `/home/foo/libmimixfe` を指定する
- `--with-poco-prefix` Poco C++ libraries が標準的ではない場所にある場合に指定する。例として `/home/foo/poco/` 以下に `include/`, `lib` がある場合は `/home/foo/poco` を指定する。プレビルドライブラリのディレクトリ構成を参照。  
- `--with-ssl-default-cert` クライアント証明書の位置を指定する。指定しなかった場合は、システムビルトインのデフォルト証明書が利用される。

## サンプルプログラム

本レポジトリの [examples/](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples) 以下を参照してください。サンプルプログラムの一部は、依存ライブラリの有無によって、ビルドされない場合があります。

実行にあたって、 libmimiio がインストールされたディレクトリを共有ライブラリ検索パスに加える必要がある場合があります。例：

```
LD_LIBRARY_PATH=/usr/local/lib /usr/local/bin/mimiio_file
```

doxygen を利用することで、本ライブラリの実装に対して自動生成ドキュメントを生成することができます。

## チュートリアル

### 1. コールバック関数を定義する

libmimiio は、コールバック型 API を備え、開発者は２つのコールバック関数を実装するだけで、mimi(R) WebSocket API Service を利用することができます。一つは、送信用音声を準備するコールバック関数で、一つは、mimi から結果を受信した時に呼ばれるコールバック関数です。つまり、WebSocket による全二重通信の送信側と受信側にそれぞれユーザー定義コールバック関数が介在することになります。

これらのコールバック関数は、libmimiio から適切なタイミングで繰り返し呼ばれます。その際、libmimiio が初期化されたスレッドとは、それぞれ異なるスレッドで呼ばれることに留意してください。特に、ユーザー定義データが共有されていた場合のデータ競合には注意してください。

#### 1.1. 送信用音声を準備するコールバック関数

このコールバック関数は、libmimiio から定期的に呼ばれ、libmimiio に対して、音声データを与えるために用いられます。与えられた音声データは、libmimiio によって、ひとつの WebSocket バイナリフレームとして、サーバーに送信されます。

##### 宣言

``````````.cpp
void (*on_tx_callback_t)(char* buffer, size_t* len, bool* recog_break, int* txfunc_error, void* userdata_for_tx);
``````````

##### 引数

|#|引数|説明|
|---|---|---|
|1|char* buffer |サーバーに送信したい音声データを指定する。データ形式は `mimi_open()` 関数で指定し、最大データ長は 32 kbyte 以内であること|
|2|size_t* len|上記音声データの長さを指定する|
|3|bool* recog_break|音声送信を終了する場合に true を指定する|
|4|int* txfunc_error|コールバック関数内での致命的エラーによって通信路を切断したい時に、ユーザー定義エラーコードを指定する。libmimiio の内部エラーコードはすべて正の値であるので、重複を避けるために、ユーザー定義エラーコードは負の値であることが好ましい|
|5|void* user_data_fo_tx|任意の型のユーザー定義データ|

##### 実装例

``````````.cpp
void txfunc(char *buffer, size_t *len, bool *recog_break, int *txfunc_error, void *userdata)
{
    size_t chunk_size = 2048; 
    *len = fread(buffer, 1, chunk_size, inputfile_); // 音声ファイルを入力とし chunk_size ずつサーバーに送信します
    if (*len < chunk_size) { // 音声ファイルを読み終わった時点で、送信の終了を宣言します
        *recog_break = true;
    }
}
``````````

この例は、仮に音声ファイルを入力と見立てて、音声ファイルをある短いサイズごとにサーバーに送信する実装例です。ファイルを読み終わった時点で、`recog_break` を true として音声送信の終了を宣言しています。

#### 1.2 結果受信用コールバック関数

##### 宣言

``````````.cpp
void (*on_rx_callback_t)(const char* result, size_t len, int* rxfunc_error, void* userdata_for_rx);
``````````

##### 引数

|#|引数|説明|
|---|---|---|
|1|const char* result|サーバーからの応答結果|
|2|size_t len|上記の長さ|
|3|int* rxfunc_error|コールバック関数内での致命的エラーによって通信路を切断したい時に、ユーザー定義エラーコードを指定する。libmimiio の内部エラーコードはすべて正の値であるので、重複を避けるために、ユーザー定義エラーコードは負の値であることが好ましい|
|4|void* user_data_for_rx|任意の型のユーザー定義データ|

##### 実装例

``````````.cpp
void rxfunc(const char *result, size_t len, int *rxfunc_error, void *userdata)
{
	std::string s(result, len);
	std::cout << s << std::endl;
}
``````````

この例では、サーバーからの応答結果を画面に表示しています。

### 2. 接続の開始と終了

#### 2.1 接続の開始

`mimi_open()` 関数を呼び出すことで、mimi(R) サーバーへの接続を開き、クライアント側／サーバー側双方の初期化を実施します。この時点では、音声の送信は開始されていないことに留意してください。

##### 宣言

``````````.cpp
MIMI_IO* mimi_open(
	const char* mimi_host,
	int mimi_port,
	void (*on_tx_callback)(char* buffer, size_t* len, bool* recog_break, int* txfunc_error, void* userdata_for_tx),
	void (*on_rx_callback)(const char* result, size_t len, int* rxfunc_error, void* userdata_for_rx),
	void* userdata_for_tx,
	void* userdata_for_rx,
	MIMIIO_AUDIO_FORMAT format,
	int samplingrate,
	int channels,
	MIMIIO_HTTP_REQUEST_HEADER* extra_request_headers,
	int custom_request_headers_len,
	const char* access_token,
	int loglevel,
	int* errorno);
``````````

##### 引数

|#|引数|説明|
|---|---|---|
|1|const char* mimi_host|mimi(R) リモートホスト名|
|2|int mimi_port|同ポート番号|
|3|void* on_tx_callback|送信側のユーザー定義コールバック関数を指定する|
|4|void* on_rx_callback|受信側のユーザー定義コールバック関数を指定する|
|5|void* userdata_for_tx|送信側のユーザー定義コールバック関数に与える任意のデータを指定する。不要な場合は nullptr を指定する|
|6|void* userdata_for_rx|受信信側のユーザー定義コールバック関数に与える任意のデータを指定する。不要な場合は nullptr を指定する|
|7|MIMI_AUDIO_FORMAT format|送信時の音声圧縮の有無と圧縮率を指定する。詳細は下記。|
|8|int samplingrate|送信音声のサンプリングレートを指定する[Hz]|
|9|int channels|送信音声のチャネル数を指定する|
|10|MIMIIO_HTTP_REQUEST_HEADER* extra_request_headers|mimi(R) WebSocket API Service 実行時設定を指定する。不要な場合は nullptr を指定する|
|11|int custom_request_headers_len|上記の長さ|
|12|const char* access_token|アクセストークンを指定する|
|13|int loglevel|libmimiio がシステムログに出力するログレベルを指定する|
|14|int* errorno|`mimi_open()` が失敗したときのエラーコードが返される|

###### MIMI_AUDIO_FORMAT

`````.cpp
typedef enum{
	  MIMIIO_RAW_PCM, //!< Raw PCM format
	  MIMIIO_FLAC_0,  //!< Flac compression level is 0 (fastest, least compression)
	  MIMIIO_FLAC_1,  //!< Flac compression level is 1
	  MIMIIO_FLAC_2,  //!< Flac compression level is 2
	  MIMIIO_FLAC_3,  //!< Flac compression level is 3
	  MIMIIO_FLAC_4,  //!< Flac compression level is 4
	  MIMIIO_FLAC_5,  //!< Flac compression level is 5 (default, preferred)
	  MIMIIO_FLAC_6,  //!< Flac compression level is 6
	  MIMIIO_FLAC_7,  //!< Flac compression level is 7
	  MIMIIO_FLAC_8,  //!< Flac compression level is 8 (slowest, most compression)
	  MIMIIO_FLAC_PASS_THROUGH //!< Input is externally encoded in flac. libmimiio do nothing with input.
  } MIMIIO_AUDIO_FORMAT;
`````

以上の音声フォーマットが選択可能です。

###### MIMIIO_HTTP_REQUEST_HEADER

``````````.cpp
typedef struct{
	char key[1024];
	char value[1024];
} MIMIIO_HTTP_REQUEST_HEADER;
``````````

HTTP リクエストヘッダーのキーと値をまとめた構造体であり、mimi(R) WebSocket API Service に指定された実行時設定を与えることができます。ここで定義した HTTP リクエストヘッダは、libmimiio が WebSocket 接続を Upgrade する際に、RFC2822 に従った key:value\r\n の形式で同時に与えられます。

#### 2.2 接続の終了

`mimi_close()` を呼び出すことで、接続を終了することができます。`mimi_close()` は、`mimi_open()` が成功した後は、任意のタイミングで呼び出すことができます。`mimi_close()` は、RFC6455 に従って WebSocket 接続が終了し、関連するリソースがすべて適切に解放されるまでブロックされます。

### 3. 音声送信の開始と終了の監視

#### 3.1. 音声送信の開始

`mimi_start()` を呼び出すことで、WebSocket 接続の確立が試行され、双方向の通信ストリームにょる音声送信及び結果受信が開始されます。ユーザー定義コールバック関数は、`mimi_start()` が成功した後に、はじめて定期的に呼び出されることになります。`mimi_start()` は成功した場合にゼロを返し、失敗した場合にはエラーコードを返します。

``````````.cpp
int errorno = mimi_start(mio);
if(errorno != 0){
    fprintf(stderr, "Could not start mimi(R) service. mimi_start() filed: %s (%d)", mimi_strerror(errorno), errorno);
    mimi_close(mio);
    return 1;
}
``````````

#### 3.2. 音声送信終了の監視

音声送信及び結果受信は、以下の場合に終了します。

- 音声送信用コールバック関数の第三引数 `recog_break` に true が設定された場合（通常の「発話終了」扱い）
- ユーザー定義コールバック関数の、`txfunc_error`, `rxfunc_error` にゼロ以外の値が設定された場合（ユーザー定義エラーの発生）
- ユーザーが明示的に `mimi_close()` を呼んだ場合（明示的な強制終了）
- その他 libmimiio の内部エラーによって、通信を継続できなかった場合（内部エラー）

ユーザーは、`mimi_start()` が成功した後には、通信ストリームが有効であるかどうかをチェックしながら、通信の終了を待つループによって、libmimiio を初期化したスレッドをブロックするようにします。通信ストリームが有効であるかを確認するためには `mimi_is_actie()` を使います。`mimi_is_active()` が false を返した場合、送受信ともに通信は終了しています。

``````````.cpp
while(mimi_is_active(mio)){
    usleep(100000); //Wait 0.1 sec to avoid busy loop
}
errorno = mimi_error(mio);
if(errorno != 0){
    fprintf(stderr, "An error occurred while communicating mimi(R) service: %s (%d)", mimi_strerror(errorno), errorno);
    mimi_close(mio);
    return 1;
}
``````````

より詳細には、送信担当スレッド、受信担当スレッドの、両方のスレッドがどちらも実行終了状態にあるときにはじめて、`mimi_is_active()` は false を返します。送信のみ、受信のみが終了している状態の場合は、`mimi_is_active()` は true を返すことに留意してください。ストリームの詳細な状態を知りたい場合は、`mimi_stream_state()` を用いることができます。

### 4. ユーティリティ関数

#### バージョン情報

`mimi_version()` によって、libmimiio のバージョン情報を示す可読文字列を得ることができます。

``````````.cpp
const char* mimi_version();
// ex. 2.1.0
``````````

#### ストリームステート取得

`mimi_stream_state()` によって、libmimiio のリモートホストへの接続状態を得ることができます。接続状態は mimiio.h で定義された MIMIIO_STREAM_STATE 型であり、5 種類あります。それぞれ、双方向ストリーム準備完了（WAIT）、双方向ストリーム終了状態（CLOSED）、双方向ストリーム確立状態（BOTH）、送信ストリームのみ有効（SEND）、受信ストリームのみ有効（RECV）です。

###### MIMIIO_STREAM_STATE

``````````.cpp
typedef enum{
	MIMIIO_STREAM_WAIT,   //!< Stream is not started.
	MIMIIO_STREAM_CLOSED, //!< Stream is closed.
	MIMIIO_STREAM_BOTH,   //!< Stream is fully-duplexed (both sending and receiving streams are active)
	MIMIIO_STREAM_SEND,   //!< Only sending stream is active.
	MIMIIO_STREAM_RECV,   //!< Only receiving stream is active.
} MIMIIO_STREAM_STATE;
``````````

#### エラーコード可読化

`mimi_strerror()` によって libmimiio の内部エラーコードを可読文字列にすることができます。負の値が指定された場合、一律ユーザー定義エラーという文字列を返します。リモートホスト側のエラーコードは素通しされ、libmimiio によって可読文字列化することはできません。

``````````.cpp
const char* mimi_strerror(int);
// ex. network error
``````````















