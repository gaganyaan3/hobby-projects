## Proxmox laptop power management - homelab

In case of power outage. it will shutdown all the vm if battary is below 60%.

- make sure these files available.
```
cat /sys/class/power_supply/ACAD/online
cat /sys/class/power_supply/BAT1/capacity
```