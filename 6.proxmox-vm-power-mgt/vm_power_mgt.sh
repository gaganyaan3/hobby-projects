#!/bin/bash

# CONFIG
BATTERY_THRESHOLD=60      # percent
LOG_FILE="/var/log/vm_power_status.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" >> "$LOG_FILE"
}

# Returns 1 if charging, 0 if not
is_charging() {
    if [ -f /sys/class/power_supply/ACAD/online ]; then
        cat /sys/class/power_supply/ACAD/online #check the file first then add here
    fi
}

# Returns battery percentage as integer
get_battery_level() {
    if [ -f /sys/class/power_supply/BAT1/capacity ]; then
        cat /sys/class/power_supply/BAT1/capacity #check the file first then add here
    fi
}

shutdown_all_vms() {
    log "Shutting down all running VMs..."
    qm list | awk '/running/ {print $1}' > /var/log/running_vm_list.txt #it make sure script does not affect other vms
    running_vm_list=$(cat /var/log/running_vm_list.txt)
    for vmid in $running_vm_list; do
        log "Shutting down VM ID $vmid"
        qm shutdown "$vmid"
    done
}

start_all_vms() {
    log "Starting all shutdown VMs..."
    shutdown_vm_list=$(cat /var/log/running_vm_list.txt) #it make sure script does not affect other vms
    for vmid in $shutdown_vm_list; do
        log "Starting VM ID $vmid"
        qm start "$vmid"
    done
}


log "Starting power monitor..."

charging=$(is_charging)
battery=$(get_battery_level)

if [[ $charging -eq 0 ]]; then
    log "Power lost, battery at ${battery}%"
    if [[ $battery -le $BATTERY_THRESHOLD ]]; then
        log "Triggering VM running shutdown (battery <= ${BATTERY_THRESHOLD}%)"
        shutdown_all_vms
    fi
else
    if [[ -f /var/log/running_vm_list.txt ]]; then
        start_all_vms
        rm -rf /var/log/running_vm_list.txt
    fi
fi
echo "battery charging: $(cat /sys/class/power_supply/ACAD/online)"
echo "battery capacity: $(cat /sys/class/power_supply/BAT1/capacity)"