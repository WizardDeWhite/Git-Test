��Ŀ¼��¼��ʹ��bochs������С�ں˵ıʼǡ�

# ��װ���

���������������ubuntu������ִ�еģ������Ѿ���װ�˻����Ŀ������ߡ�

```
sudo apt install bochs
sudo apt install bochs-x
sudo apt install nasm
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

# ��bochs����

## �鿴����

```
help
help pb
help info
```

## �ϵ�����

����ĵ�ַ�������ַ
```
pb 0x7c00
```

�鿴�����õĶϵ�
```
info break
```

## �����

```
u 0x7c00 0x7c1e
```

## �鿴�Ĵ���
```
sreg
r
```

## �鿴�ڴ�

�ӵ�ַ0x000916ed��ʾ����8��word��

```
xp /8xw 0x000916ed
```
