from machine import Pin, time_pulse_us
from time import sleep

TRIG = Pin(3, Pin.OUT)
ECHO = Pin(2, Pin.IN)

def get_distance():
    TRIG.low()
    sleep(0.002)  # Let sensor settle
    
    # Send a 10µs pulse to trigger
    TRIG.high()
    sleep(0.00001)
    TRIG.low()

    # Measure time for echo
    try:
        duration = time_pulse_us(ECHO, 1, 30000)  # 30ms timeout
    except OSError as ex:
        print("Pulse timed out")
        return None

    # Distance calculation: time (us) × speed of sound (cm/us) / 2
    distance_cm = (duration * 0.0343) / 2
    return round(distance_cm, 2)

# Main loop
while True:
    dist = get_distance()
    if dist:
        print("Distance:", dist, "cm")
    else:
        print("No distance measured.")
    sleep(1)