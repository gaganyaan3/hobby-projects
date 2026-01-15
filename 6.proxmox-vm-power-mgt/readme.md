## Proxmox laptop power management - homelab

In case of power outage. it will shutdown all the vm if battary is below 60%.

- make sure these files available.
```
# add cron
*/5 * * * * /opt/cron/vm_power_mgt.sh

# Hp laptop
cat /sys/class/power_supply/ACAD/online
cat /sys/class/power_supply/BAT1/capacity

# Lenovo laptop
cat /sys/class/power_supply/ADP0/online
cat /sys/class/power_supply/BAT0/capacity
```


### logrotate

```
root@lp-knode-2:/etc/logrotate.d# cat vmlog 
/var/log/vm_power_status.log
{
	rotate 4
	weekly
	missingok
	notifempty
	compress
	delaycompress
	sharedscripts
}

```