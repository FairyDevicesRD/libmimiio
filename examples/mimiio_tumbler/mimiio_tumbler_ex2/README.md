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

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) と同一です

## 主要部解説

### 接続情報の準備

`````````.cpp
Session::ConnectionParam param(p.get<std::string>("host"), p.get<int>("port"), access_token,
			 p.get<int>("rate"), p.get<int>("channel"), af, p.exist("verbose"));
Session session(param);
``````````

`ConnectionParam` クラスはリモートホストへの接続情報等をまとめたデータクラスです。`Session` クラスは後述しますが、発話単位の音声データと通信ストリームを保持するクラスです。`XFERecorder` のコンストラクタのユーザー定義データとして `Session` クラスのインスタンスのポインタを与えます。

### libmimixfe による XFERecorder の初期化と録音の開始

[mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1) とほぼ同一です。一部 XFE のパラメータを高速寄りにチューニングしている場合があります。ユーザー定義データとして、上記の通り、`Session` クラスのインスタンスのポインタを与えています。

### メインスレッドの監視待機

``````````.cpp
``````````

メインスレッドは、`XFERecorder` の状態のみを定期的に確認しながら待機するためのループに入ります。このループがビジーループにならないように、適切な時間を `sleep()` する必要があります。このスリープ時間は応用アプリケーションによって適切に決定してください。

ユーザーによって Ctrl+C が入力されると、interrupt_flag_ が 0 以外の値となります。録音を終了し、待機ループを抜け全体が終了します。

### libmimixfe 録音コールバック関数の実装

``````````.cpp
``````````

# libmimiio 音声送信コールバック関数の実装

``````````.cpp
``````````

# Session クラスの実装

``````````.cpp
``````````


# libmimiio 結果受信コールバック関数の実装

``````````.cpp
``````````
