���νӴ����⻯�����Ǿ͵�����̨�����һ������װ���ɡ�

# ��װһ̨�����

## ��ǰ׼��

�ͺ�����Ҫ��װ�����һ���������ڰ�װ�����֮ǰҲҪ�������ȵ�׼����Ҫ׼����ô����������

* ��Ҫ��ϵͳ���
* ������������������
* ����guestϵͳ��iso����

### ��Ҫ��ϵͳ���

����Ϊ���֣�Ӳ���������������ֿ��Է�Ϊ�ں�ģ���Ӧ�ó���

����оƬ��Ҫ֧��kvm������x86�ļܹ���Ҫ��vmx�������linuxϵͳ��ͨ�����cat /proc/cpuinfo | grep vmx�����жϡ�����������־��˵��оƬ��֧�ֵġ�

Ȼ���ں��Ƿ�֧�֡�ͨ�����lsmod | grep kvm�����������������ģ��kvm, kvm_intel����ô��ʾ��ǰ���ں��Ѿ�������������ģ�顣���û�м��أ�����ػ�����Ҫ���±����ںˡ�

�����û��qemu��װ�����ÿ�����а�Ŀ�ִ���ļ������ֻ����в�𣬵���������qemu��ͷ���еĽ�qemu-kvm, qemu-system-x86_64�ȡ�����ʹ�ð������������qemu����װ��

### ������������������

����ͺ�����װ�������Ҫ�����������Ӳ��һ�������ǰ�װ�������Ҳ��Ҫ��������̵ġ�

��������ж�����ʽ�͸�ʽ������Ͳ�չ���ˡ������������������host�Ͼ���һ���ļ������ǿ��������������һ��������̡�

```
qemu-img create -f raw fedora.img 30G
```

�������ڵ�ǰĿ¼�´�����һ����СΪ30G����������ļ�fedora.img�����������̵ĸ�ʽ��raw�ġ����ڸ�ʽ������������ܡ�����Ȥ��ͯЬ�������в������ϡ�

### ����guestϵͳ��iso����

��ϲ��װʲôϵͳ����ʲôϵͳ�ɡ���������һ����Ҳ�У���װ���С�

## ��ʼ��װ

����ܼ�
```
qemu-kvm -m 4096 -smp 4 -hda fedora.img -cdrom Fedora.iso -boot dc
```

����ô�򵥣���һ������Ϳ�����������������밲װ�Ľ����ˡ�

�򵥽���һ�¸�������

-m 4096:    ������ڴ���4096MB
-smp 4:      �������4��vcpu
-hda:          Ӳ�̵��ļ�
-cdrom:     �����ļ�
-boot:        ����˳�򣬴ӹ�������

��������ܿ�������ʵ�������װһ���Ľ��档������ʾһ���������߾ͺ��ˡ����ڵķ��а��Ѿ����ú����Ի��ˡ�

# ���������

����Ҳ�ܼ򵥣�һ�����

����ܼ�
```
qemu-kvm -m 4096 -smp 4 -hda fedora.img
```

���ǰ�cdrom������˳��Ĳ���ȥ���Ϳ����ˡ�

���ˣ���ӭ������������硣

# С���� -- ͨ���ַ����氲װ�����

�е�ʱ������û��ͼ�ν��棬�����޷��ܺõ�ʹ������������������ڷ������ϰ�װ��������ܴ�������ǲ���װͼ������ģ�������������ܵ����ƣ�ʹ��ͼ�ν�������������

���ˣ�С���������׼�����ؼ���ͨ���ַ����氲װ�������

��ע�� �÷�����һ���ʺ����з��а档��������Ӱ�װ����rhel6.7��

## ���������iso

����iso��Ϊ���ҵ����е�kernel��initrd��
```
sudo mount -o loop rhel-server-6.7-x86_64-dvd.iso tmp/
```

## �ҵ� ����kernel �� initrd

һ�������������kernel��initrd�ڹ����ļ���isolinuxĿ¼�ڡ�

������rhel6.7�֣��ļ�������

> isolinux/vmlinuz
> isolinux/initrd.img

·�����������б仯��������̽����

## ��ʼ��װ

���ˣ����Ҫ��ʼ��װ�˹�����ʹ�õ������ǣ�

```
sudo qemu-system-x86_64 --enable-kvm -m 4096 -smp 4 \
ubuntu.img \
-cdrom rhel-server-6.7-x86_64-dvd.iso \
--nographic \
-append console=ttyS0 \
-kernel tmp/isolinux/vmlinuz \
-initrd tmp/isolinux/initrd.img
```

ǰ���������װ����һ���ģ����Ƕ��˺����ĸ�ѡ�

```
--nographic  ��ʹ��ͼ�ν���
--append     ����ʱ�����ں˵Ĳ���
--kernel     ʹ���ĸ��ں�
--initrd     ʹ���ĸ�initrd 
```

# �ٽ�һ��--һ����װ

���ˣ����˵�����Ҫ����һ�¡���Ȼ���ڵİ�װ�����Ѿ����Ѻã�����Ҳ�ܼ��ˣ���������ܹ�һ����װ����������ж�ô���á�

�٣����¶��������С��������������ˡ�
�����������kickstart��

ʹ��kickstart�ķ�ʽ�м��֣��������ṩһ�ְѽű��ŵ�iso�ļ��ķ�ʽ������ķ�ʽ��ҿ�����������������

## ��дkickstart�ű�

kickstart����һ������д�õĽű�������д���˰�װ�����е���Ϣ���������һ�����԰�װ�����ӡ�

PS: kickstart��Ҫ����RHEL��CentOS��

```
# This is an installation
install
# From cdrom
cdrom
lang en_US.UTF-8
keyboard us
# root password, plaintest
#rootpw --plaintext abc123
# root password encrypted
rootpw --iscrypted $6$27qsH/RZygFeKCF9$iDAZJsCnN7N
firewall --service=ssh
autoconfig --enableshadow --passalog=sha512 --enablefingerprint
selinux --enforcing
# power off after installation
poweroff
timezone --utc America/New_York
bootloader --location=mbr --driverorder=sda --append="console=ttyS0 crashkernel=auto"
# clear the disk
clearpart --all --drivers=sda --initlabel
autopart
zerombr
repo --name="Red Hat Enterprise Linux" --baseurl=file:///mnt/source --cost=100

%packages --nobase
@core

%end
```

�����ű�Ҳ���ã���Щ�����ر����ף����ǻ����Ķ����ܿ������м������ر�ע��һ�£�

*  poweroff  ����м���ѡ�reboot, halt������һ�£�ֻ����poweroff��ʱ��װ��qemu���˳��ģ������Ķ��Ῠ���ǡ�
* autopart һ��Ҫ���ϣ������ǻ���ʾ��Ҫ��ô����

��ҿ������kickstart�ű����ƻ��ǻ᲻֪��Ҫ��ôд�����ߴ���������Խ���������ӵĵط���

* RHEL�ֶ���װ������һ���ű���/rootĿ¼�£�����������Ӿ��Ǵ�����ļ�����������
* RHEL����һ�����õ������ system-config-kickstart����Щѡ�֪����ôд�����Խ���������ġ�


## ��������kiskstart�ű���iso

�������ü򵥵ġ�

> mkisofs -o ks.iso ks.cfg

�����������һ������kickstart�ű���iso��

�ű����ļ���һ����ks.cfg
�ű����ļ���һ����ks.cfg
�ű����ļ���һ����ks.cfg

�����Ҹĳ�������ֲ�װ�ɹ��ˡ�

## ��˵��һ����װ

���ˣ���ʼ��װ�ɡ���ʵ����һ�����

```
sudo qemu-system-x86_64 --enable-kvm -m 4096 -smp 4 \
ubuntu.img \
-drive file=rhel-server-6.7-x86_64-dvd.iso,media=cdrom,index=1 \
-drive file=ks.iso,media=cdrom,index=2 \
--nographic \
-append "console=ttyS0 ks=cdrom:/ks.cfg"\
-kernel tmp/isolinux/vmlinuz \
-initrd tmp/isolinux/initrd.img
```

Hope you enjoy it~
