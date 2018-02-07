ビルド方法 {#how_to_build}
==========

@ingroup tutorial

## Installing Poco 1.6.x

libmimiio は patch/ ディレクトリ以下に同梱されているパッチが適用された Poco 1.6.x Complete Edition に依存しています．

パッチの適用は，例えば，以下のように行うことが出来ます．

~~~~~~~~~~~~~~~~~~~~~{.sh}
$ cd [poco src dir]
$ patch -p1 < [libmimiio dir]/patch/poco-1.6.0/net_sources.patch
~~~~~~~~~~~~~~~~~~~~~

Poco 1.6.x のビルドについては [Poco C++ libraries](http://pocoproject.org/) を参照して下さい．

@note Poco 1.6.x には Basic Edition と Complete Edition の2種類があります．libmimiio には SSL 関連の実装を含む Complete Edition が必要となりますので留意して下さい．

## Installing libFlac++

libFlac 及び libFlac の C++ ラッパーである libFlac++ に依存しています．libFlac のインストールについては，[http://http://xiph.org/flac/](http://xiph.org/flac/) を参照して下さい．

## Installing Portaudio

Portaudio のインストールは libmimiio のビルドに必須では有りません．portaudio は，マイクから録音した音声を libmimiio を用いて mimi(R) に送信する libmimiio 利用例である `mimiio_pa.cpp` のビルドにのみ利用されます．portaudio がシステムで利用可能でない場合は，`mimiio_pa.cpp` は単にビルドされません．

Portaudio のインストールについては，[http://www.portaudio.com/](http://www.portaudio.com/) を参照して下さい．

## libmiio 本体のインストール

~~~~~~~~~~~~~~~~~~~~~{.sh}
$ ./autogen.sh
$ ./configure
$ make
# make install
~~~~~~~~~~~~~~~~~~~~~

標準の configure オプション以外に，libmimiio は以下の configure オプションをサポートしています．標準の configure オプションを含む全ての configure オプションは ./configure --help で確認することが出来ます．

~~~~~~~~~~~~~~~~~~~~~{.sh}
--enable-debug デバッグを有効にしてビルドする（libmimiio は -g2 -O0 でコンパイルされる）
--with-poco-prefix=/path/to/poco/basedir Poco C++ libraries が標準的でない場所にある場合に指定する
--with-ssl-default-cert=/path/to/defaultcert クライアント証明書の位置．指定しなかった場合は，システムビルトインのデフォルト証明書が利用される．Linux 環境の場合は OpenSSL のデフォルト証明書，Android 環境の場合は，Android 組込SSL のデフォルト証明書（/etc/security/cacerts/b0f3e76e.0）
~~~~~~~~~~~~~~~~~~~~~

@note デバッグを有効にした場合は，examples 以下の利用例は libmimiio をスタティックリンクします．

Next: \ref	writing_callback