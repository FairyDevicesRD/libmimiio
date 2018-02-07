音声送信の開始と終了の監視 {#start_communication}
==========================

@ingroup tutorial

## 音声送信の開始

`mimi_start()` 関数を呼び出すことで，mimi(R) リモートホストへ，双方向の通信ストリームによる音声送信及び結果受信が開始されます．より詳細には，`mimi_start()` 関数を呼び出すことで，libmimiio 内部の音声送信用スレッド，結果受信用スレッドが動き出し，それぞれが，`txfunc()`, `rxfunc()` を定期的に呼び出すことで，音声送信及び結果受信が行われます．

`mimi_start()` 関数は，双方向の通信ストリームの開始に成功した場合はゼロを返します．失敗した場合には，エラーコードを返します．このエラーコードは，`mimi_strerror()` 関数を通すことで可読化することができます．

~~~~~~~~~~~~~~~~~~~~~{.cpp}
int errorno = mimi_start(mio);
if(errorno != 0){
    fprintf(stderr, "Could not start mimi(R) service. mimi_start() filed: %s (%d)", mimi_strerror(errorno), errorno);
    mimi_close(mio);
    fclose(inputfile_);
    return 1;
}
~~~~~~~~~~~~~~~~~~~~~
[Line 101 of file mimiio_file.cpp](mimiio__file_8cpp_source.html#l00101)

## 音声送信終了の監視

音声送信及び結果受信は，以下の場合に終了します．

- `txfunc()` の第3引数 `recog_break` に true が設定された場合（通常の発話終了）
- `txfunc()` の第4引数 `txfunc_error`, `rxfunc()` の第3引数 `rxfunc_error` にゼロ以外の値が設定された場合（ユーザー定義エラーの発生）
- ユーザーが明示的に `mimi_close()` 関数を呼んだ場合（明示的な強制終了）
- その他 libmimiio の内部エラーによって，通信を継続できなかった場合（その他のエラー）

ユーザーは，`mimi_start()` 関数が成功した後は，双方向の通信ストリームが有効であるかどうかをチェックしながら，通信の終了を待つループを実装しなければなりません．通信ストリームが有効であるかどうかをチェックするためには，`mimi_is_active()` 関数を使います．

`mimi_is_active()` 関数が，false を返した場合，送受信共に通信は終了しています．ユーザーが明示的に `mimi_close()` 関数を呼び出した場合，もしくは，正常終了であれば，直後の `mimi_error()` 関数のチェックで，ゼロが返されます．

~~~~~~~~~~~~~~~~~~~~~{.cpp}
while(mimi_is_active(mio)){
    //Wait 0.5 sec to avoid busy loop
    usleep(500000);
}
errorno = mimi_error(mio);
if(errorno != 0){
    fprintf(stderr, "An error occurred while communicating mimi(R) service: %s (%d)", mimi_strerror(errorno), errorno);
    mimi_close(mio);
    fclose(inputfile_);
    return 1;
}
~~~~~~~~~~~~~~~~~~~~~
[Line 108 of file mimiio_file.cpp](mimiio__file_8cpp_source.html#l00108)

@note より詳細には，送信担当スレッド，受信担当スレッドの，両方のスレッドがどちらも実行終了状態にある場合に，mimi_is_active() は false を返します．送信のみ，受信のみが終了している状態の場合は，mimi_is_active() 関数は true を返すことに留意して下さい．ストリームの詳細な状態を知りたい場合は，mimi_stream_state() 関数を用いることができます．

Previous: \ref open_and_close_connection | Next: \ref utilities
