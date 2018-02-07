ユーティリティ関数 {#utilities}
==========================

@ingroup tutorial

チュートリアルの他のセクションで説明した関数に加えて，libmimiio はいくつかのユーティリティ関数を定義しています．

## バージョン情報

`mimi_version()` 関数によって，libmimiio のバージョン情報を得ることができます．これは，libmimiio が動的ライブラリとして利用される場合に便利です．

~~~~~~~~~~~~~~~~~~~~~{.cpp}
const char* mimi_version();
// ex. 2.1.0
~~~~~~~~~~~~~~~~~~~~~

## ストリームステート取得

`mimi_stream_state()` 関数によって，libmimiio のリモートホストへの接続状態を得ることができます．接続状態は `mimiio.h` で定義された `::MIMIIO_STREAM_STATE` 型であり，5 種類あります．それぞれ，双方向ストリーム準備完了（WAIT），双方向ストリーム終了状態（CLOSED），双方向ストリーム確立状態（BOTH），送信ストリームのみ有効（SEND），受信ストリームのみ有効（RECV）です．

~~~~~~~~~~~~~~~~~~~~~{.cpp}
const char* mimi_stream_state();
// ex. MIMIIO_STREAM_BOTH
~~~~~~~~~~~~~~~~~~~~~

## エラーコード可読化

`mimi_strerror()` 関数によって，libmimiio 内部エラーを文字列化することができます．負の数の場合，一律ユーザー定義エラーという文字列とします．またリモート側のエラーコードは，素通しされ，`mimi_strerror()` 関数によって文字列化することはできません．

~~~~~~~~~~~~~~~~~~~~~{.cpp}
const char* mimi_strerror(int);
// ex. network error
~~~~~~~~~~~~~~~~~~~~~

Previous: \ref start_communication | Next: \ref errorcodes
