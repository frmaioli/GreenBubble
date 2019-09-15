# GreenBubble

## Architecture
[Main components and overall software architecture](docs/GB_Arch.png)


## Setup for Development
The compilation should take place inside Raspberry Pi. It will facilitate the setup, instead of cross-compiling.
Go to Emulation Session to setup an emulator for the RP.

Inside the emulator, run:
sudo apt-get install wiringpi

Compiling:
Just run make

Starting
sudo ./GreenBubbleD

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

- grep GreenBubbleD /var/log/syslog
	The output should be similar to this one:
	Dec 22 14:06:10 aspire-3810t GreenBubbleD[26138]: GreenBubble daemon started.


Emulating Raspberry Pi, so it can be easier to compile and test
$ mkdir qemu_vms
Get latest Raspbian: http://www.raspberrypi.org/downloads
I am using 2018-11-13-raspbian-stretch.img
Download kernel-qemu to ~/qemu_vms/
$ git clone https://github.com/dhruvvyas90/qemu-rpi-kernel
$ sudo apt-get install qemu-system
$ file 2018-11-13-raspbian-stretch.img
$ qemu-system-arm -kernel qemu-rpi-kernel/kernel-qemu-4.4.34-jessie -cpu arm1176 -m 256 -M versatilepb -no-reboot -serial stdio -append "root=/dev/sda2 panic=1" -hda 2018-11-13-raspbian-stretch.img -redir tcp:5022::22
$ qemu-system-arm -kernel qemu-rpi-kernel/kernel-qemu-4.4.34-jessie -cpu arm1176 -m 256 -M versatilepb -no-reboot -serial stdio -net user, smb=/home/frmaioli/Developer/qemu_vms/Shared -append "root=/dev/sda2 panic=1" -hda 2018-11-13-raspbian-stretch.img -redir tcp:5022::22

To change anything in the img it is possible to mount it. From the output of the file command, take the partition 2 'startsector' value an multiply by 512, and use this figure as the offset value in the mount command below.
$ sudo mount 2018-11-13-raspbian-stretch.img -o offset=50331648 /mnt
But unmount it for safety after:
$ sudo umount 2018-11-13-raspbian-stretch.img /mnt

##Remote Connection and Pi Setup
Qemu gives you a root shell, run:
Login as pi
Password raspberry

Connecting Remotely
It is easier to connect to the Pi remotely for development than using VM. VNCViewer can be used and also filezilla to transfer files.

Installing Ulfius
sudo apt install libulfius-dev
How to use: https://babelouest.github.io/ulfius/API.html

It is not needed to install wiringPi, it comes by default on Raspbian

##Rest Tests
Use Postman to test the endpoints. They are:
http://192.168.1.66:8537/GBBL/led/blue/enable/1
angharad.c and angharad.service.js are good exemples to continue

