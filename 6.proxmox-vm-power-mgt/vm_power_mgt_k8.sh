#!/bin/bash

# CONFIG
BATTERY_THRESHOLD=40      # percent
LOG_FILE="/var/log/vm_power_status.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" >> "$LOG_FILE"
}

# Returns 1 if charging, 0 if not
is_charging() {
    if [ -f /sys/class/power_supply/ADP0/online ]; then
        cat /sys/class/power_supply/ADP0/online #check the file first then add here
    else
        CHARGING=$(upower -i $(upower -e | grep AC) | grep "online" | awk '{print $2}')
        [[ "$CHARGING" == "yes" ]] && echo 1 || echo 0
    fi
}

# Returns battery percentage
get_battery_level() {
    if [ -f /sys/class/power_supply/BAT0/capacity ]; then
        cat /sys/class/power_supply/BAT0/capacity #check the file first then add here
    else
        upower -i $(upower -e | grep BAT) | grep -E "percentage" | awk '{gsub(/%/, "", $2); print $2}'
    fi
}

shutdown_all_process() {
    log "Shutting down all running VMs..."
    systemctl stop ollama
    systemctl stop kubelet
    crictl rm -af
    docker compose -f /root/docker/openweb-compose.yml down
    touch /var/log/running_container_list.txt
}

start_all_process() {
    log "Starting all shutdown VMs..."
    systemctl start ollama
    systemctl start kubelet
    sleep 10
    docker compose -f /root/docker/openweb-compose.yml up -d
}


log "Starting power monitor... battery capacity: $(cat /sys/class/power_supply/BAT0/capacity)"

charging=$(is_charging)
battery=$(get_battery_level)

if [[ $charging -eq 0 ]]; then
    log "Power lost, battery at ${battery}%"
    if [[ $battery -le $BATTERY_THRESHOLD ]]; then
        log "Triggering VM running shutdown (battery <= ${BATTERY_THRESHOLD}%)"
        shutdown_all_process
    fi
else
    if [[ -f /var/log/running_container_list.txt ]]; then
        start_all_process
        rm -rf /var/log/running_container_list.txt
    fi
fi
echo "battery charging: $(cat /sys/class/power_supply/ADP0/online)"
echo "battery capacity: $(cat /sys/class/power_supply/BAT0/capacity)"
