コールバック関数を定義する {#writing_callback}
==========================

@ingroup tutorial

## コールバック関数の概要

libmimiio では，ユーザーが2つのコールバック関数を指定することで，mimi(R) にアクセスすることができます．第一に，送信用音声を準備するコールバック関数 `txfunc()` ，第二に，mimi から結果を受信した時に呼ばれるコールバック関数 `rxfunc()` です．

@note `txfunc()` は，libmimiio 内で生成される音声送信用スレッドから呼び出され，`rxfunc()` は libmimiio 内で生成される結果受信用スレッドから呼び出されることに留意して下さい．

## 送信用音声を準備するコールバック関数 `txfunc`

### 宣言

~~~~~~~~~~~~~~~{.c}

void txfunc(char *buffer, size_t *len, bool *recog_break, int* txfunc_error, void* userdata);

~~~~~~~~~~~~~~~

### 引数

|#|引数|引数の説明|
|---|---|---|
|1|char* buffer|読み込んだ音声データ（RAW PCM）を書き込むバッファ．形式は RAW PCM データ（リトルエンディアン）であり，signed short （16bit）サンプルであること．サンプリングレートとチャネル数は，`mimi_open()` 関数で指定する．libmimiio 内でメモリは確保されている．最大長は mimi WebSocket API 仕様定義の 32kbyte．|
|2|size_t* len|第一引数で指定した buffer に実際に書き込まれている音声ファイルのバイト数．|
|3|bool* recog_break|音声ファイルの区切りを示す recog-break フラグ．音声の区切りにおいて，recog-break を送信することは仕様上必須．|
|4|int* txfunc_error|txfunc内で継続不能なエラーが発生した場合に，ユーザーエラーを示すのユーザー定義数値を libmimiio に伝えるときに利用する．ユーザー定義エラーコードは，libmimiio が利用するエラーコードとの重複を避けるため，必ずマイナスの数値であること．txfunc_error がゼロ以外だった場合，libmimiio はエラー状態となり，全処理を終了しようとする．|
|6|void* userdata|ユーザー定義|

### 実装例

音声ファイルの一部を mimi(R) に送信する場合の例を以下に示します．

~~~~~~~~~~~~~~~{.c}

void txfunc(char *buffer, size_t *len, bool *recog_break, int* txfunc_error, void* userdata){
	size_t chunk_size = 4096;// byte
	*len = fread(buffer, 1, chunk_size, inputfile_);
	if(*len < chunk_size){
		*recog_break = true;
	}
}

~~~~~~~~~~~~~~~
[Line 34 of file mimiio_file.cpp](mimiio__file_8cpp_source.html#l00034)

## 結果受信用コールバック関数 `rxfunc`

### 宣言

~~~~~~~~~~~~~~~{.c}

void rxfunc(char* result, size_t len, int* rxfunc_error, void *userdata)

~~~~~~~~~~~~~~~

### 引数

|#|引数|引数の説明|
|---|---|---|
|1|char* result|サーバーからの応答結果．テキストの場合は NULL 終端されている．rxfunc() の終了まで有効．|
|2|size_t len|第一引数 result の長さ．|
|3|int* rxfunc_error|rxfunc内で継続不能なエラーが発生した場合に，ユーザーエラーをユーザー定義数値を libmimiio に伝えるときに利用する．ユーザー定義エラーコードは，libmimiio が利用するエラーコードとの重複を避けるため，必ずマイナスの数値であること．rxfunc_error がゼロ以外だった場合，libmimiio はエラー状態となり，全処理を終了しようとする．|
|4|void* userdata|ユーザー定義|

### 実装例

受信した結果を単に表示する例です．テキストフレームを受信した場合は，result は NULL 終端されます．

~~~~~~~~~~~~~~~{.c}
void rxfunc(const char* result, size_t len, int* rxfunc_error, void *userdata)
{
	printf("RESULT: %s (%zd byte)\n", result, len);
}
~~~~~~~~~~~~~~~
[Line 55 of file mimiio_file.cpp](mimiio__file_8cpp_source.html#l00034)


Previous: \ref how_to_build | Next: \ref	open_and_close_connection