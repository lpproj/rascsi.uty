README for RASCPY
(in Japanese, encoded with CP932 Shift_JIS)

�T��
----

ASPI�𗘗p���āARaSCSI�̃����[�g�h���C�u�i�u���b�W�f�o�C�X�j��
DOS�}�V���ԂŃt�@�C���]�����s���T���v���v���O�����ł��B


�����
--------

����ɂ̓o�[�W����3�ȏ��DOS�ƁADOS�p��ASPI�}�l�[�W�����K�v�ƂȂ�܂��B
�����p��SCSI�A�_�v�^�ɕt���A�������͕ʓr�̔��^�z�z����Ă��铖�Y�@��p��
ASPI�h���C�o�iAdaptec EZ-SCSI�Ȃǁj���C���X�g�[�����Ă����Ă��������B
PC-98�n�̏ꍇ�ASCSI BIOS��L���ɂ��Ă���Ȃ�Έȉ��̃t���[�̃h���C�o��
�g����悤�ł��B
�i�Ƃ������A����m�F�����̃h���C�o�ōs���܂����j

http://www.vector.co.jp/soft/dos/hardware/se000679.html


�p�@
----

RASCPY�łł��邱�Ƃ͈ȉ��̒ʂ�ł��B

1. �����[�g�h���C�u��̃f�B���N�g���\���idir, ls�j
2. �����[�g�h���C�u���z�X�g�̃t�@�C���]���iget�j
3. �z�X�g�������[�g�h���C�u�̃t�@�C���]���iput�j


�f�B���N�g���\��:

  RASCPY [-In] dir [-b] �p�X��          �iDOS���ۂ������̕\���j
  RASCPY [-In] ls [-1] [-a] �p�X��      �iUnix���ۂ������̕\���j

    -I    �u���b�W�f�o�C�X��SCSI ID��0�`6�܂ł̐����Ŏw��B�ȗ����� -I6
    -b    �t�@�C���A�f�B���N�g�����̂ݕ\��
    -1    -b�Ɓi�قځj����
    -a    �B���t�@�C���i�擪���s���I�h�j���\��
  �p�X��  �\���������p�X���B�t�@�C����������DOS�`���̃��C���h�J�[�h�g�p��


�����[�g�h���C�u����̃t�@�C���擾:

  RASCPY [-In] get [-f] �����[�g�h���C�u���p�X�� [�z�X�g���p�X��]

    -I    dir�Ɠ���
    -f    �z�X�g���ɓ����t�@�C��������ꍇ���㏑���m�F���Ȃ�

  ���p�X���Ƀ��C���h�J�[�h�͎g���܂���


�����[�g�h���C�u�ւ̃t�@�C���]��:

  RASCPY [-In] put [-f] �z�X�g���p�X�� �����[�g�h���C�u���p�X��

    -I    dir�Ɠ���
    -f    �����[�g���ɓ����t�@�C��������ꍇ���㏑���m�F���Ȃ�

  ���p�X���Ƀ��C���h�J�[�h�͎g���܂���
  ���i�d�v�jrascsi�͒ʏ�root�����œ��삵�Ă���A�����[�g�h���C�u�ɓ]�����ꂽ
     �t�@�C����root�����ƂȂ�܂��B�܂�Arascsi�̃����[�g�h���C�u���
     �A�N�Z�X�ł���t�@�C���́u�قڂ��ׂāv�㏑���\�ł��B
     �g���������Raspbian�̃V�X�e�����ȒP���s�p�ӂɔj��ł��Ă��܂����߁A
     �p�X���̋L�q�ɂ͏\���Ȓ��ӂ𕥂��Ă��������B


�\�[�X
------

LSI-C86 3.30c���H�ŁATurbo C++ 1.01�AOpenWatcom 1.9�ŃR���p�C���ł���
���Ƃ��m�F���Ă��܂��B
�\�[�X�̗��p������ UNLICENSE �ɏ����܂��B�܂�u�قڃp�u���b�N�h���C��
�ł��薳�ۏ؁v�ł��B���C�Z���X�̏ڍׂ͈ȉ������m�F���������B

http://unlicense.org/
https://ja.wikipedia.org/wiki/Unlicense


����
----

2018-09-24
    get/put�R�}���h�� -f �I�v�V�������g���Ȃ����������C��

2018-07-31
    �������̂��ߓ]���o�b�t�@�T�C�Y����(512->8192bytes)

2018-01-19
    ����


