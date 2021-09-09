kcontrol_events
=======
A simple tool to monitor ALSA control events.

Usage
-------------
```
kcontrol_events <options>

Available options:
  -h,--help      this help
  -c,--card N    select the card, default 0
  -D,--device N  select the device, default 'hw:0'
  -n,--name      control name to tap on
```
If `control name` is not specified, `kcontrol_evernts` will listen events on all controls on the selected ALSA card.

Compilation
-------------
With make:
```bash
make
```

Or cmake:
```bash
mkdir build && cd build && cmake ../ && make
```

Or just compile it:
```bash
gcc -Wall -g kcontrol_events.c -o kcontrol_events -lasound
```
