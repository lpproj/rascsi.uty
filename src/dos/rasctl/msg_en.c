
const char err_invalid_id[] = "ERROR: invalid ID";
const char err_invalid_cmd[] = "ERROR: invalid command";
const char err_invalid_dev[] = "ERROR: invalid device/medium type";
const char err_file_not_specified_s[] = "ERROR: file not specified to %s";
const char err_bridge_not_found[] = "ERROR: RaSCSI bridge device not found";
const char err_bridge_old[] = "ERROR: RaSCSI bridge device is old (required 1.52 or later)";
const char err_scsi_trans[] = "ERROR: SCSI transfer error";

const char msg_to_help[] = "Type rasctl -? to help";

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
        "  -A ADAPTER   ASPI Adapter ID (0 = default)\n"
#endif
        "  -i ID        device ID (0..6)\n"
        "  -u UNIT      logical unit number (0..1)\n"
        "  -c CMD       command for the ID:\n"
        "                 attach    assign a device\n"
        "                 detach    remove device (and a medium)\n"
        "                 insert    insert a medium to the drive (mo, cd)\n"
        "                 eject     eject a medium from the drive (mo, cd)\n"
        "                 protect   write-protect for the medium (mo)\n"
        "  -t TYPE      device type:\n"
        "                 hd        HDD (SASI/SCSI)\n"
        "                 mo        3.5 MO drive\n"
        "                 cd        CD drive\n"
        "                 bridge    Bridge device (RASETHER, RASDRV)\n"
        "  -l           list all devices\n"
        "  --stop       stop the process of RaSCSI\n"
        "  --shutdown   shutdown Raspberr Pi\n"
        ;

