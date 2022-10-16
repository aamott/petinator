# petinator
Firmware for bottle-to-filament machines

Follow the wiring in the demo or using the pinout inside configuration.h  
Most changes should be made inside configuration.h, including default stepper speed, default temperature, and pinout. 

# Demo
[Simulation with stepper motor](https://wokwi.com/projects/333363618182595154)

# TODO: 
- [ ] Thermal panic protection when:
    - [ ] Temperature doesn't change fast enough when outside range
    - [ ] Temperature suddenly changes by more than x degrees
- [ ] Store settings between shutdowns
- [ ] Faster menu library https://myhomethings.eu/en/dynamic-arduino-lcd-menu/
