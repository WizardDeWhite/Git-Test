1. Top

shift + "p": order by cpu usage
shift + "m": order by memory usage

2. iostat

iostat -x 1 10

rrqm/s: ÿ����� merge �Ķ�������Ŀ���� delta(rmerge)/s
wrqm/s: ÿ����� merge ��д������Ŀ���� delta(wmerge)/s
r/s:    ÿ����ɵĶ� I/O �豸�������� delta(rio)/s
w/s:    ÿ����ɵ�д I/O �豸�������� delta(wio)/s
rsec/s: ÿ������������� delta(rsect)/s
wsec/s: ÿ��д���������� delta(wsect)/s
rkB/s:  ÿ���K�ֽ������� rsect/s ��һ�룬��Ϊÿ������СΪ512�ֽڡ�(��Ҫ����)
wkB/s:  ÿ��дK�ֽ������� wsect/s ��һ�롣(��Ҫ����)
avgrq-sz: ƽ��ÿ���豸I/O���������ݴ�С (����)��delta(rsect+wsect)/delta(rio+wio)
avgqu-sz: ƽ��I/O���г��ȡ��� delta(aveq)/s/1000 (��Ϊaveq�ĵ�λΪ����)��
await:  ƽ��ÿ���豸I/O�����ĵȴ�ʱ�� (����)���� delta(ruse+wuse)/delta(rio+wio)
svctm:  ƽ��ÿ���豸I/O�����ķ���ʱ�� (����)���� delta(use)/delta(rio+wio)
%util:   һ�����аٷ�֮���ٵ�ʱ������ I/O ����������˵һ�����ж���ʱ�� I/O �����Ƿǿյġ��� delta(use)/s/1000 (��Ϊuse�ĵ�λΪ����)

3. /proc/loadavg

/proc/loadavg
              The  first  three fields in this file are load average figures giving the number of jobs in the run queue (state
              R) or waiting for disk I/O (state D) averaged over 1, 5, and 15 minutes.  They are the same as the load  average
              numbers  given  by  uptime(1) and other programs.  The fourth field consists of two numbers separated by a slash
              (/).  The first of these is the number of currently executing kernel scheduling entities  (processes,  threads);
              this  will  be  less  than  or  equal  to the number of CPUs.  The value after the slash is the number of kernel
              scheduling entities that currently exist on the system.  The fifth field is the PID of the process that was most
              recently created on the system.

load average usually be less than 2(?).

4. Case Study

4.1 idle system with high load

Behavior: loadaverage in top is high, while id shows system is idle.

Reason: contain "D+" process. Use ps -axjf to list "D+" process.

4.2 kswapd is high

Behavior: top shows sy is high. And the one use cpu most is kswapd0.

Check: use free to verify the memory is low.

0. Reference:

https://blog.csdn.net/ahjxhy2010/article/details/100144077
https://www.cnblogs.com/wuchanming/p/9294678.html
https://help.aliyun.com/document_detail/41225.html
