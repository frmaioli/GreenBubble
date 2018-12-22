Recompiling WiringPi
WiringPi is a submodule, the necessary libraries are already pre-compiled and with the includes presented in ./lib and ./include folders, respectively.
So, nothing extra is needed. But, if you need to checkout the submodule and recompile it, run this command:
	git submodule update --init wiringPi/
To install the result in the desired folder, edit wiringPi/wiringPi/Makefile to have:
	DESTDIR?=../../GreenBubbleD
	PREFIX?=

Compiling:
1. Just run make


Check the Daemon is running:
- ps -xj | grep GreenBubbleD
	The output should be similar to this one:
	+------+------+------+------+-----+-------+------+------+------+-----+
	| PPID | PID  | PGID | SID  | TTY | TPGID | STAT | UID  | TIME | CMD |
	+------+------+------+------+-----+-------+------+------+------+-----+
	|    1 | 3387 | 3386 | 3386 | ?   |    -1 | S    | 1000 | 0:00 | ./  |
	+------+------+------+------+-----+-------+------+------+------+-----+
	What you should see here is:
		The daemon has no controlling terminal (TTY = ?)
		The parent process ID (PPID) is 1 (The init process)
		The PID != SID which means that our process is NOT the session leader
		(because of the second fork())
		Because PID != SID our process can't take control of a TTY again

- grep GreenBubble /var/log/syslog
	The output should be similar to this one:
	Dec 22 14:06:10 aspire-3810t firstdaemon[26138]: GreenBubble daemon started.
