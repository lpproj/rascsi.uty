README for rasctl16 in Japanese (encoded with Shift_JIS, CP932)


�T�v
----

RaSCSI�̐����DOS�ōs�����߂̃v���O�����ł��B
DOS�}�V����Ƀl�b�g���[�N�A�_�v�^�A���̃A�_�v�^�ɑΉ�����p�P�b�g�h���C�o�A
�����RaSCSI����������Ă���Raspberry Pi�Ƃ���DOS�}�V�����l�b�g���[�N
�ڑ�����Ă���K�v������܂��B

�p�P�b�g�h���C�o�p��TCP/IP�X�^�b�N�Ƃ��� TEEN ���C���X�g�[������Ă���
�ꍇ��TEEN���g�p���܂��BTEEN���C���X�g�[������Ă��Ȃ��ꍇ�͓����̃X�^�b�N
�imTCP�g�p�j�ɂ��p�P�b�g�h���C�o�ɒ��ڃA�N�Z�X����TCP/IP�ڑ����s���܂��B


���O�ݒ�
--------

TEEN�o�R�ł̐ڑ����s���ꍇ�A���ϐ�TEEN��TEEN�p��`�t�@�C���̃t���p�X����
�ݒ肵�Ă����K�v������܂��B�ڍׂ�Ethernet��TEEN�̃h�L�������g��
���m�F���������B

TEEN���g��Ȃ��ꍇ�͓���mTCP���g�����߁AmTCP�p�ݒ�t�@�C�����t���p�X����
���ϐ�MTCPCFG�ɐݒ肵�Ă����K�v������܂��B
mTCP�ݒ�t�@�C�����ɍŒ���K�v�Ȑݒ荀�ڂ�
  PACKETINT
  IPADDR
  NETMASK
  GATEWAY
  NAMESERVER
�ƂȂ�܂��BmTCP��DHCP���g����ꍇ��PACKETINT�݂̂̐ݒ�ł����Ƃ�
�Ȃ�ł��傤�B


�p�@
----

�ڑ���̃A�h���X�ƃ|�[�g�ԍ��̎w�肪�K�v�Ȃ��Ƃ������ƁA
RaSCSI�t����rasctl�Ƃقړ����ł��B

  rasctl16 [-h ADDR] [-p PORT] -i ID -c CMD [-t TYPE] [-f FILE] [-l]
  rasctl16 [-h ADDR] [-p PORT] -l

    -h    �ڑ���̃A�h���X�B�ȗ����� -h 127.0.0.1 �Ɠ���
    -p    �ڑ���̃|�[�g�ԍ��B�ȗ����� -p 6868 �Ɠ���
    -i    SCSI ID
    -c    �R�}���h
          attach        ID�Ƀf�o�C�X�����蓖��
          detatch       ID�ւ̃f�o�C�X���蓖�ĉ���
          insert        �f�o�C�X�Ƀ��f�B�A�}�� (CD, MO)
          eject         �f�o�C�X���̃��f�B�A���o�� (CD, MO)
          protect       ���f�B�A�������݋֎~ (MO)
    -f    �f�o�C�X�⃁�f�B�A�̃t�@�C����
          �iattach, insert�R�}���h���͎w��K�{�j
    -l    ���݂̃f�o�C�X�ꗗ�\��


���ӎ���
--------

rasctl16�́ARaSCSI���irasctl�̂��߂Ɂj�������p���Ă���|�[�g�ɊO������
�ʐM�����݂���̂ł����A�ʐM���̃A�N�V�f���g�ɑ΂���RaSCSI���̑Ώ��͍Œ����
���̂ƂȂ��Ă��܂��B�r���ŒʐM���r�₵����A�z��O�̃p�����[�^��^������
�����RaSCSI���ُ�I���A�������͐���s�\�ƂȂ�ꍇ������܂��B
�i�Ƃ����RaSCSI�̃|�[�g6868���O���ɊJ�����Ă���l�͂��Ȃ��Ǝv���܂����A
�O�̂���Raspbian��ufw�Ȃǂ̃t�@�C�A�[�E�H�[������ꂽ�ق�����������
����܂���c�j


�\�[�X
------

rasctl16�͈ȉ��̃��C�u�������g�p���Ă��܂��B

mTCP (GPLv3 version: mTCP-src_2013-05-23.zip)
  http://www.brutman.com/mTCP/

TEENLIB v0.37b
  http://www.pc88.gr.jp/~teen/wiki/index.php?TEEN.LIB

ya_getopt
  https://github.com/kubo/ya_getopt


�r���h�ɂ�OpenWatcom���K�v�ł��B�܂��AmTCP��IBM PC�ŗL�R�[�h���܂܂�邽�߁A
PC-98�Ȃǂœ��삳���邽�߂ɂ̓p�b�`��K�p����K�v������܂��B
�i3rdparty���ɂ���diff�t�@�C����README�����m�F���������j

