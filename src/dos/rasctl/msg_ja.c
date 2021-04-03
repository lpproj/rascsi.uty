
const char err_invalid_id[] = "ERROR: IDもしくはLUN指定が無効";
const char err_invalid_cmd[] = "ERROR: コマンド指定が無効";
const char err_invalid_dev[] = "ERROR: ドライブ指定が無効";
const char err_file_not_specified_s[] = "ERROR: %sコマンドにはファイル指定が必要";
const char err_bridge_not_found[] = "ERROR: RaSCSIブリッジデバイスが見つからない";
const char err_bridge_old[] = "ERROR: バージョン1.52以上のRaSCSIを使用してください";
const char err_scsi_trans[] = "ERROR: SCSI転送時にエラーが発生";

const char msg_to_help[] = "ヘルプを表示したい場合は rasctl -? と入力してください";

const char msg_usage[] = 
        "SCSI Target Emulator RaSCSI Controller "
#ifdef DEFAULT_55BIOS
        "(55BIOS)"
#else
        "(ASPI)"
#endif
        "\n"
#ifdef DEFAULT_55BIOS
        "Usage: rasctl -i ID [-u UNIT] -c CMD [-t TYPE] [-f FILE] [-l]\n"
        "  or:  rasctl -l\n"
        "  or:  rasctl --stop\n"
        "  or:  rasctl --shutdown\n"
        "\n"
#else
        "Usage: rasctl [-a ADAPTER] -i ID [-u UNIT] -c CMD [-t TYPE] [-f FILE] [-l]\n"
        "  or:  rasctl [-a ADAPTER] -l\n"
        "  or:  rasctl [-a ADAPTER] --stop\n"
        "  or:  rasctl [-a ADAPTER] --shutdown\n"
        "\n"
        "  -A ADAPTER   ASPIホストアダプタID (オプション省略時は0)\n"
#endif
        "  -i ID        ドライブのSCSI ID (0..6)\n"
        "  -u UNIT      ドライブの論理ユニット番号 (オプション省略時は0)\n"
        "  -c CMD       ドライブ操作コマンド:\n"
        "                 attach    ドライブ取付\n"
        "                 detach    ドライブ除去\n"
        "                 insert    ドライブにメディア挿入 (mo,cd)\n"
        "                 eject     ドライブ内のメディア取り出し (mo,cd)\n"
        "                 protect   メディア書き込み禁止 (mo)\n"
        "  -t TYPE      ドライブ種別:\n"
        "                 hd        HDD (SASI/SCSI)\n"
        "                 mo        3.5インチMOドライブ\n"
        "                 cd        CDドライブ\n"
        "                 bridge    RaSCSIブリッジデバイス (RASETHER, RASDRV)\n"
        "  -l           デバイスリスト表示\n"
        "  --stop       RaSCSIの動作終了\n"
        "  --shutdown   Raspberry Piをシャットダウン\n"
        ;

