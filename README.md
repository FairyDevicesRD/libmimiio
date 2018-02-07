# libmimiio

[![Apache License](http://img.shields.io/badge/license-APACHE2-blue.svg)](http://www.apache.org/licenses/LICENSE-2.0)

libmimiio - mimi(R) WebSocket API Service client library

## 概要

WebSocket (RFC6455) 通信を利用した mimi(R) WebSocket API Service を簡単に利用するためのライブラリです。 mimi(R) との通信仕様は公開されていますので、必ずしも本ライブラリを利用する必要はありません。本ライブラリは、C++11 で実装され、上記ライセンスに基づき、ソースコードが公開されます。

## 構築

### 依存ライブラリ

#### 必須

- Poco C++ libraries 1.8.1 以上
- libflac 1.3.0 以上
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

mingw, cigwin 等を利用し、Linux に準じて適宜ビルドしてください。

