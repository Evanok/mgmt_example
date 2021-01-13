# mgmt_example

enable/disable hci and basic stuff through mgmt protocol (bluetooth socket HCI_CONTROL).


## Summary

- Code to manipulate hci through mgmt api in a asynchronous way.
- inspired by https://github.com/kismetwireless/bluez-basic-monitor/blob/master/bluez-mgmtsock.c and btmgmt binary from Bluez.
- Use synchrone code only. Always check reply from mgmt and double check current setting after command.
- Add a retry system due to a lot of problem to get a success from mgmt (busy, failed, etc..)
- Have some problem also to make it work just after init hci device with hciattach. Have to add a little millisecond delay between
the first usage and hciattach to avoid weird error from mgmt.
- Does not implement index correctly. Always used hci0 (index 0) by default
- Need to run this code with sudo permission


## test


The code integrate a main to run basic tests. enable/disable hci 20 times


To run the test :

```
[arthur * main] make
gcc -o mutex.o -c mutex.c -W -Wall -Wformat -Wformat-security -Wextra  -Wno-unused-result -Wextra -Wno-long-long -Wno-variadic-macros -Werror -Wno-clobbered -Wno-missing-field-initializers -O0 -std=gnu99 -DDEBUG_MODE="" -D__DIR__=\"mgmt_example\" -Wno-unused-variable -Wno-unused-label -Wno-unused-parameter -Wno-unused-function -g
gcc -o simple_mgmt.o -c simple_mgmt.c -W -Wall -Wformat -Wformat-security -Wextra  -Wno-unused-result -Wextra -Wno-long-long -Wno-variadic-macros -Werror -Wno-clobbered -Wno-missing-field-initializers -O0 -std=gnu99 -DDEBUG_MODE="" -D__DIR__=\"mgmt_example\" -Wno-unused-variable -Wno-unused-label -Wno-unused-parameter -Wno-unused-function -g
gcc -o simple_mgmt mutex.o simple_mgmt.o -lpthread
[arthur * main] ./simple_mgmt
[mgmt_example][INFO] Enable HCI.. (0)
[mgmt_example][DEBUG] HCI interface is up
[mgmt_example][DEBUG] HCI interface is up
[mgmt_example][INFO] Enable HCI.. Done.
[mgmt_example][DEBUG] Check hci0 status..
[mgmt_example][DEBUG] Supports BR/EDR 1
[mgmt_example][DEBUG] Supports LE 1
[mgmt_example][DEBUG] Supports Power 1
[mgmt_example][DEBUG] Supports Discoverable 1
[mgmt_example][DEBUG] Supports Advertising 0
[mgmt_example][DEBUG] Supports Bondable 0
(...)
```

## issue
