��Ŀ¼��¼��ʹ��bochs������С�ں˵ıʼǡ�

# ��װ���

���������������ubuntu������ִ�еģ������Ѿ���װ�˻����Ŀ������ߡ�

```
sudo apt install bochs
sudo apt install bochs-x
sudo apt install nasm
```

# ��������

�ȱ���

```
make
```

��ִ��

```
make run

����

./vm_start.sh
```

# �鿴�������ļ�

��vim
```
vim -b boot.bin
:%!xxd
```

��hexdump

```
hexdump -C boot.bin
```
