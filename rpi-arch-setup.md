## Setting up Arch Linux on RPi

- Put the following in /boot/config.txt and reboot

```
gpu_mem_512=8
gpu_mem_256=8
cma_lwm=16
cma_hwm=32
cma_offline_start=16

start_file=start_x.elf
fixup_file=fixup_x.dat
```

- Setup a swapfile

```
fallocate -l 512M /swapfile
chmod 600 /swapfile
mkswap /swapfile
swapon /swapfile
# Edit /etc/fstab with
/swapfile none swap defaults 0 0
```

- Synchronize package databases
  - `pacman -Sy`
- Full system update
  - `pacman -Su`
- Install necessary packages
 - redis
 - git
 - vim
 - automake, autoconf, libtool, pkg-config, make
 - gcc
 - zsh
- libmodbus
- hiredis
- ldconfig
```
vim /etc/ld.so.conf.d/libc.conf
 # add this: /usr/local/lib
ldconfig
```
- rbenv and ruby-build

- WiFi
  - `wifi-menu``
  - `wpa_passphrase SSID password`
  -  In /etc/netctl/wlan--SSID Replace value in Key= with returned value of psk in above command, adding \" to the beginning
  - `netctl start wlan0-SSID`
  - `systemctl enable netctl-ifplugd@wlan0.service`
  - `netctl enable wlan0-SSID`
