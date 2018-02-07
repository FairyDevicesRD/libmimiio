# libmimiio

[![Apache License](http://img.shields.io/badge/license-APACHE2-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

libmimiio - mimi(R) WebSocket API Service client library

## 概要

WebSocket (RFC6455) 通信を利用した mimi(R) WebSocket API Service を簡単に利用するためのライブラリです。 mimi(R) との通信仕様は公開されていますので、必ずしも本ライブラリを利用する必要はありません。本ライブラリは、C++11 で実装されています。

## 構築

### 依存ライブラリ

#### 必須

- Poco C++ libraries Complete Edition 1.8.1 以上
- libflac++ 1.3.0 以上

#### オプション

- portaudio v19_2014 以上（mimiio_pa/ 以下のサンプルプログラムのビルドのためのみに用いられます）
- libmimixfe 1.0 以上（mimiio_tumbler/ 以下のサンプルプログラムのビルドのためのみに用いられます）

### ビルド

#### Linux

``````````.sh
$ autoreconf -vif
$ ./configure
$ make
$ sudo make install
``````````
- gcc 5 以上を推奨します。

#### Fairy I/O Tumbler

``````````.sh
$ autoreconf -vif
$ ./configure-tumbler
$ make
$ sudo make install
``````````

#### Mac OS X

``````````.sh
$ autoreconf -vif
$ CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang \
CXX=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++  ./configure
$ make
$ sudo make install
``````````
- Apple LLVM version 8.1.0 (clang-802.0.42) 以上を推奨します。

#### Windows

mingw, cygwin 等を利用し、Linux に準じて適宜ビルドしてください。

#### configure オプションについて

標準の configure オプション以外に、libmimiio は以下の configure オプションをサポートしています。すべてのオプションは `./configure --help` で確認することができます。

- `--enable-debug` libmimiio が `-g -O0` でコンパイルされる。サンプルプログラムは libmimiio をスタティックリンクする
- `--with-poco-prefix` Poco C++ libraries が標準的ではない場所にある場合に指定する
- `--with-ssl-default-cert` クライアント証明書の位置を指定する。指定しなかった場合は、システムビルトインのデフォルト証明書が利用される。

## サンプルプログラム

本レポジトリの [examples/](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples) 以下を参照してください。サンプルプログラムの一部は、依存ライブラリの有無によって、ビルドされない場合があります。

doxygen を利用することで、本ライブラリの実装に対して自動生成ドキュメントを生成することができます。

## チュートリアル

### 1. コールバック関数を定義する

libmimiio は、コールバック型 API を備え、開発者は２つのコールバック関数を実装するだけで、mimi(R) WebSocket API Service を利用することができます。一つ目は、送信用音声を準備するコールバック関数 `txfunc()` 、二つ目は、mimi から結果を受信した時に呼ばれるコールバック関数 `rxfunc()` です。

これらのコールバック関数は、libmimiio から適切なタイミングで繰り返し呼ばれます。`txfunc()` と `rxfunc()` は、libmimiio が初期化されたスレッドとは、それぞれ異なるスレッドで実行されることに留意してください。

#### 1.1. 送信用音声を準備するコールバック関数 `txfunc`

このコールバック関数は、libmimiio から定期的に呼ばれ、libmimiio に対して、音声データを与えるために用いられます。与えられた音声データは、libmimiio によって、ひとつの WebSocket バイナリフレームとして、サーバーに送信されます。

##### 宣言

``````````.cpp
void txfunc(char *buffer, size_t *len, bool *recog_break, int *txfunc_error, void *userdata)
``````````

##### 引数

|#|引数|説明|
|---|---|---|
|1|char* buffer |サーバーに送信したい音声データを指定する。データ形式は `mimi_open()` 関数で指定し、最大データ長は 32 kbyte 以内であること|
|2|size_t* len|上記音声データの長さを指定する|
|3|bool* recog_break|音声送信を終了する場合に true を指定する|
|4|int* txfunc_error|コールバック関数内での致命的エラーによって通信路を切断したい時に、ユーザー定義エラーコードを指定する。libmimiio の内部エラーコードはすべて正の値であるので、重複を避けるために、ユーザー定義エラーコードは負の値であることが好ましい|
|5|void* user_data|任意の型のユーザー定義データ|

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

#### 1.2 結果受信用コールバック関数 `rxfunc`

##### 宣言

``````````.cpp
void rxfunc(const char *result, size_t len, int *rxfunc_error, void *userdata)
``````````

##### 引数

|#|引数|説明|
|---|---|---|
|1|const char* result|サーバーからの応答結果|
|2|size_t len|上記の長さ|
|3|int* rxfunc_error|コールバック関数内での致命的エラーによって通信路を切断したい時に、ユーザー定義エラーコードを指定する。libmimiio の内部エラーコードはすべて正の値であるので、重複を避けるために、ユーザー定義エラーコードは負の値であることが好ましい|
|4|void* user_data|任意の型のユーザー定義データ|

##### 実装例

``````````.cpp
void rxfunc(const char *result, size_t len, int *rxfunc_error, void *userdata)
{
	std::string s(result, len);
	std::cout << s << std::endl;
}
``````````

この例では、サーバーからの応答結果を画面に表示しています。








