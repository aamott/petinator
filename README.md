# petinator
Firmware for bottle-to-filament machines

Follow the wiring in the demo or using the pinout inside configuration.h  
Most changes should be made inside configuration.h, including default stepper speed, default temperature, and pinout. 

# Instructions
The most helpful visual is probably this simulation on Wokwi: [Simulation with stepper motor](https://wokwi.com/projects/333363618182595154). Although this uses a stepper motor, you can also use a PWM driven DC motor with the driver connected to the step pin. Just comment out `USES_STEPPER` in `configuration.h` and set `DEFAULT_SPEED` as appropriate.

1. Clone this code into your Arduino directory: `git clone https://github.com/aamott/petinator.git`
2. Open in the Arduino IDE.
3. Edit `configuration.h` according to your needs. If you're using the same layout as in Wokwi, including a non-I2C display, just copy the configuration from there.
4. Wire it up, either by the pinouts listed in `configuration.h` or matching Wokwi.
5. Upload the code to your Arduino (or ESP32).
6. Test and tune the PID values and motor speed as needed and reupload.

# TODO: 
- [ ] Thermal panic protection when:
    - [ ] Temperature doesn't change fast enough when outside range
    - [ ] Temperature suddenly changes by more than x degrees
- [ ] Store settings between shutdowns
- [ ] PID Autotune
