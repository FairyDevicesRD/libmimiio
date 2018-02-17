# サンプルプログラム

## 概要

このディレクトリには、libmimiio を利用したサンプルプログラムが格納されています。

## ビルド

libmimiio をビルドすると、サンプルプログラムも同時にビルドされます。サンプルプログラムだけをリビルドする場合は、このディレクトリで `make` をすることもできます。一部のサンプルプログラムは、依存関係が整っていない場合、ビルドされないので御留意ください。

## サンプルプログラムの概要

各サンプルプログラムの詳細は、該当するサブディレクトリの README.md を参照してください。

### mimiio_file

音声ファイルを入力として、音声データをサーバーに送信するサンプルプログラムです。音声ファイルを入力とする場合は、リアルタイム処理が不要であるので、libmimiio によるリアルタイム処理を要求する必要はなく、mimi HTTP API Service を直接利用する方が好適です。

### mimiio_pa

クライアント側システムに搭載されたマイクロフォンを入力として、録音された音声をリアルタイムでサーバーに送信し、リアルタイムでサーバーから認識結果を受信する最も単純なサンプルプログラムです。マイク入力場合、リアルタイム処理が重要であるので、libmimiio による WebSocket 通信（mimi WebSocket API Service）を利用することが好適です。

マイクからの録音は [portaudio（外部サイト）](http://www.portaudio.com/) を利用しています。ビルド環境に portaudio が無い場合、本サンプルプログラムはビルドされません。また、portaudio  が対応していない OS やハードウェア環境においては、本サンプルプログラムは利用できません。Fairy I/O Tumbler 上では portaudio を利用した録音をすることはできません。

### mimiio_tumbler

Fairy I/O Tumbler 上で、libmimixfe と組み合わせて利用する場合のサンプルプログラムです。ビルド環境に libmimixfe が無い場合、本サンプルプログラムはビルドされません。本サンプルプログラムの分類は順次追加されます。

#### [mimiio_tumbler_ex1](https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex1)

Fairy I/O Tumbler 上で、[libmimixfe](https://github.com/FairyDevicesRD/libmimixfe)と組み合わせ（機能制限有）、録音及び信号処理された音声を入力として、リアルタイムでサーバーに送信し、リアルタイムでサーバーから認識結果を受信する最も単純なサンプルプログラムです。サンプルプログラムの単純化のために、機能制限として、libmimixfe の設定上、同時検出する音源数が 1 の場合のみに対応し、同時複数音源を抽出する設定になっている場合には対応していません。また、libmimixfe の VAD の結果を無視し、最終認識結果を除き、途中認識結果はすべて `recog-in-progress` ステートで得られます。

#### mimiio_tumbler_ex2(https://github.com/FairyDevicesRD/libmimiio/tree/master/examples/mimiio_tumbler/mimiio_tumbler_ex2)

ex1 と同様ですが、ibmimixfe の VAD の結果を利用し、発話毎にリモートホストへのコネクションを準備し、発話毎に `recog-finished` ステートを得るサンプルプログラムです。

#### mimiio_tumbler_ex3

Fairy I/O Tumbler 上で、[libmimixfe](https://github.com/FairyDevicesRD/libmimixfe)と組み合わせ、録音及び信号処理された音声を入力として、リアルタイムでサーバーに送信し、リアルタイムでサーバーから認識結果を受信する libmimixfe の機能制限のないサンプルプログラムであり、N 個の同時抽出音源に対し、N 本の WebSocket 通信路を確立し、同時認識を行います。
