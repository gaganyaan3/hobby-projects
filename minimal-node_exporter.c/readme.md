## Minimal node_exporter with CPU, RAM, DISK usage


### static binary with musl

```
#compile
musl-gcc -static -O2 -o node_exporter min_node_exporter.c

#verify
ldd node_exporter

#run
./node_exporter

#size 77kb
-rwxr-xr-x 1 root root  74K Aug 22 16:02 node_exporter
```

- http://0.0.0.0:9100/metrics

```
# HELP cpu_usage CPU usage in percent
# TYPE cpu_usage gauge
cpu_usage 3.34
# HELP memory_usage RAM usage in percent
# TYPE memory_usage gauge
memory_usage 0.15
# HELP disk_usage Disk usage in percent
# TYPE disk_usage gauge
disk_usage 69.97

```

