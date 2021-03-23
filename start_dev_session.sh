#!/bin/bash

#Run only first time
#cd $HOME
#mkdir bin
##open $HOME/bash.rc and add export PATH="$HOME/bin:$PATH" there
#ln -s $HOME/Developer/Postman-linux-x64-8.0.7/Postman/Postman $HOME/bin/postman

#Open code
cd $HOME/Developer/
ctags -R angharad/ b3603/ GreenBubble/ stm8flash/ ulfius/ WebSite/node_modules/admin-lte/*.html
gvim GreenBubble/GreenBubbleD/README.md GreenBubble/GreenBubbleD/Makefile GreenBubble/GreenBubbleD/*.[ch] &
gvim b3603/README.md b3603/stm8/Makefile b3603/stm8/*.[ch] b3603/stm8/*.py &
gvim WebSite/node_modules/admin-lte/index-paludarium.html WebSite/node_modules/admin-lte/le_light_cfg.html WebSite/node_modules/admin-lte/le_light_sts.html &

#Discover Raspberry Pi IP address
RASP_IP=$(nmap -sP 192.168.1.0/24 >/dev/null && arp -an | grep b8:27:eb:00:71:b9 | awk '{print $2}' | sed 's/[()]//g')

#Open filezila
filezilla sftp://pi:pegasus@$RASP_IP &

#Open Chrome
google-chrome --disable-web-security --user-data-dir WebSite/node_modules/admin-lte/index-paludarium.html WebSite/node_modules/admin-lte/le_light_cfg.html WebSite/node_modules/admin-lte/le_light_sts.html &

#Open Postman
postman &

#Connect to Pi
sleep 10
vncviewer $RASP_IP &
##log with pi/pegasus
