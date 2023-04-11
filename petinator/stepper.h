class Stepper {

private:
  const unsigned int _step_pin;
  const unsigned int _direction_pin;
  const int _enable_pin = -1;
  const bool _invert_enable;
  const unsigned int _min_pulse_width;
  const unsigned int _steps_per_mm = 0;

  // step tracking
  unsigned long _micros_per_step = 0;
  long _last_step = 0;

  /// @brief Step the motor. Takes at least _min_pulse_width microseconds.
  inline void step() {
    digitalWrite(_step_pin, HIGH);  // start pulse

    // Delay the minimum allowed pulse width
    delayMicroseconds(_min_pulse_width);

    digitalWrite(_step_pin, LOW);  // end pulse
  }

public:

  /// @brief
  /// @param step_pin Driver step pin.
  /// @param direction_pin Driver direction pin.
  /// @param enable_pin Driver enable pin. Less than 0 for no enable pin.
  /// @param invert_enable_pin Whether to invert the enable pin. Disabled will be HIGH, Enabled will be LOW. Default false;
  /// @param steps_per_mm Number of steps to move a mm. If set to 0, speeds in mm/s will be ignored.
  /// @param min_pulse_width Minimum number of microseconds a pulse width can take. Default 1.
  Stepper(int step_pin, int direction_pin, int enable_pin = -1, bool invert_enable_pin = false, int steps_per_mm = 0, int min_pulse_width = 1)
    : _step_pin(step_pin), _direction_pin(direction_pin), _enable_pin(enable_pin), _invert_enable(invert_enable_pin), _steps_per_mm(steps_per_mm), _min_pulse_width(min_pulse_width) {
    pinMode(step_pin, OUTPUT);
    pinMode(direction_pin, OUTPUT);

    if (enable_pin >= 0) {
      pinMode(enable_pin, OUTPUT);
    }

    stop();
  }


  /// @brief get the speed in mm/s
  float speed_mms() {
    // TODO: Just calculate this every time the speed changes and return the variable.
    // return millimeters_per_second
    return (float)speed_sps() / _steps_per_mm;
  }


  /// @brief get the speed in steps per second
  unsigned int speed_sps() {
    // TODO: Just calculate this every time the speed changes and return the variable.
    // return steps_per_second
    return _micros_per_step * 1000000;
  }


  /// @brief return whether the stepper is currently running
  /// @return (bool) whether the stepper is currently running
  bool running() {
    return _micros_per_step;
  }


  /// @brief Sets motor speed in millimeters per second. Will not move the motor if steps per mm is not set.
  void set_speed_mms(float millimeters_per_second) {
    if (millimeters_per_second == 0) {
      stop();
      return;
    }

    int steps_per_second = millimeters_per_second * _steps_per_mm;
    set_speed(steps_per_second);
  }


  /// @brief Set speed in steps per second
  /// @param steps_per_second
  /// @param reverse_direction Whether to reverse stepper direction.
  void set_speed(int steps_per_second, bool reverse_direction = false) {
    if (steps_per_second > 1000000) {
      _micros_per_step = 0;
    } else {
      _micros_per_step = 1000000 / steps_per_second;
    }

    // set stepper direction
    digitalWrite(_direction_pin, !reverse_direction);

    // enable stepper
    if (_enable_pin >= 0) {
      // _invert_enable will either be false (HIGH) or true (LOW)
      digitalWrite(_enable_pin, !_invert_enable);
    }
  }


  /// @brief Check if a step is due. If a step is due, will take at least _min_pulse_width microseconds to run.
  void run() {
    // TODO: add acceleration

    long current_time = micros();

    if (_micros_per_step != 0 && _last_step + _micros_per_step < current_time) {
      step();
    }

    _last_step = micros();
  }


  /// @brief Stop motor movements
  /// Sets the speed to 0 and stops movement
  void stop() {
    _micros_per_step = 0;
    // _invert_enable will either be false (LOW) or true (HIGH)
    digitalWrite(_enable_pin, _invert_enable);
  }
};