README for rasctl16 in Japanese (encoded with Shift_JIS, CP932)


概要
----

RaSCSIの制御をDOSで行うためのプログラムです。
DOSマシン上にネットワークアダプタ、そのアダプタに対応するパケットドライバ、
さらにRaSCSIが導入されているRaspberry PiとそのDOSマシンがネットワーク
接続されている必要があります。

パケットドライバ用のTCP/IPスタックとして TEEN がインストールされている
場合はTEENを使用します。TEENがインストールされていない場合は内蔵のスタック
（mTCP使用）によりパケットドライバに直接アクセスしてTCP/IP接続を行います。


事前設定
--------

TEEN経由での接続を行う場合、環境変数TEENにTEEN用定義ファイルのフルパス名を
設定しておく必要があります。詳細はEthernet版TEENのドキュメントを
ご確認ください。

TEENを使わない場合は内蔵mTCPを使うため、mTCP用設定ファイルをフルパス名を
環境変数MTCPCFGに設定しておく必要があります。
mTCP設定ファイル内に最低限必要な設定項目は
  PACKETINT
  IPADDR
  NETMASK
  GATEWAY
  NAMESERVER
となります。mTCPのDHCPが使える場合はPACKETINTのみの設定でも何とか
なるでしょう。


用法
----

接続先のアドレスとポート番号の指定が必要なことを除くと、
RaSCSI付属のrasctlとほぼ同じです。

  rasctl16 [-h ADDR] [-p PORT] -i ID -c CMD [-t TYPE] [-f FILE] [-l]
  rasctl16 [-h ADDR] [-p PORT] -l

    -h    接続先のアドレス。省略時は -h 127.0.0.1 と同じ
    -p    接続先のポート番号。省略時は -p 6868 と同じ
    -i    SCSI ID
    -c    コマンド
          attach        IDにデバイスを割り当て
          detatch       IDへのデバイス割り当て解除
          insert        デバイスにメディア挿入 (CD, MO)
          eject         デバイス内のメディア取り出し (CD, MO)
          protect       メディア書き込み禁止 (MO)
    -f    デバイスやメディアのファイル名
          （attach, insertコマンド時は指定必須）
    -l    現在のデバイス一覧表示


注意事項
--------

rasctl16は、RaSCSIが（rasctlのために）内部利用しているポートに外部から
通信を試みるものですが、通信時のアクシデントに対するRaSCSI側の対処は最低限の
ものとなっています。途中で通信が途絶したり、想定外のパラメータを与えたり
するとRaSCSIが異常終了、もしくは制御不能となる場合があります。
（ところでRaSCSIのポート6868を外部に開放している人はいないと思いますが、
念のためRaspbianにufwなどのファイアーウォールを入れたほうがいいかも
しれません…）


ソース
------

rasctl16は以下のライブラリを使用しています。

mTCP (GPLv3 version: mTCP-src_2013-05-23.zip)
  http://www.brutman.com/mTCP/

TEENLIB v0.37b
  http://www.pc88.gr.jp/~teen/wiki/index.php?TEEN.LIB

ya_getopt
  https://github.com/kubo/ya_getopt


ビルドにはOpenWatcomが必要です。また、mTCPにIBM PC固有コードが含まれるため、
PC-98などで動作させるためにはパッチを適用する必要があります。
（3rdparty内にあるdiffファイルとREADMEをご確認ください）

