# Prototype of Linux userspace driver for Xbox One Wireless (WiFi) controller

The wireless dongle will show up as a WiFi interface:

```bash
$ ip l show wlp0s20u3u1u4
3: wlp0s20u3u1u4: <NO-CARRIER,BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state DORMANT mode DORMANT group default qlen 1000
    link/ieee802.11/radiotap 62:45:b4:f4:41:51 brd ff:ff:ff:ff:ff:ff
```

In order to make this program work, set the interface to monitoring mode and select channel 40:
```bash
$ sudo ifconfig wlp0s20u3u1u4 down
$ sudo iwconfig wlp0s20u3u1u4 mode monitor
$ sudo ifconfig wlp0s20u3u1u4 up
$ sudo iwconfig wlp0s20u3u1u4 channel 40
```

Compile the program:
```bash
$ g++ main.cpp -lpthread
```

Run it:
```bash
$ sudo ./a.out wlp0s20u3u1u4
```

Pair your controller by pressing the little button on it. Afterwards, the program will display input data.