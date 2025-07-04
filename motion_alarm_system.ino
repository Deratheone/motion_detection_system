// Pin Definitions
#define TRIG_PIN 7          // Ultrasonic trigger pin
#define ECHO_PIN 8          // Ultrasonic echo pin
#define BUZZER_PIN 4        // Buzzer pin
#define TOUCH_PIN 2         // Touch sensor pin
#define LDR_PIN 9           // Light sensor digital pin
#define RED_PIN 3           // RGB LED Red pin
#define GREEN_PIN 5         // RGB LED Green pin
#define BLUE_PIN 6          // RGB LED Blue pin

// System Constants
#define DETECTION_DISTANCE 100 // Detection distance in cm
#define DEBOUNCE_DELAY 50      // Touch sensor debounce delay
#define ALARM_DURATION 2000    // Alarm duration (8 seconds)
#define MODE_DISPLAY_TIME 2000 // Time to display mode change
#define DEBUG_INTERVAL 2000    // Debug output every 2 seconds

// Audio note frequencies (in Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047

// System Variables
enum SystemMode {
  DAY_ONLY = 0,    // Red mode - active only during day
  NIGHT_ONLY = 1,  // Blue mode - active only during night
  ALWAYS_ON = 2    // Green mode - always active
};

SystemMode currentMode = ALWAYS_ON;     // Start with always-on mode
bool lastTouchState = LOW;              // Previous touch sensor state
bool systemArmed = false;               // Whether system is currently armed
bool alarmActive = false;               // Whether alarm is currently sounding
unsigned long alarmStartTime = 0;       // When alarm started
unsigned long lastModeChange = 0;       // When mode was last changed
unsigned long lastDebugOutput = 0;      // When debug info was last printed
bool displayingMode = false;            // Whether we're showing mode color

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  Serial.println("=== MOTION DETECTION SYSTEM - ENHANCED AUDIO ===");
  Serial.println("Starting system initialization...");
  
  // Configure pin modes
  pinMode(TRIG_PIN, OUTPUT);    // Ultrasonic trigger as output
  pinMode(ECHO_PIN, INPUT);     // Ultrasonic echo as input
  pinMode(BUZZER_PIN, OUTPUT);  // Buzzer as output
  pinMode(TOUCH_PIN, INPUT);    // Touch sensor as input
  pinMode(LDR_PIN, INPUT);      // Light sensor as input
  pinMode(RED_PIN, OUTPUT);     // RGB Red as output
  pinMode(GREEN_PIN, OUTPUT);   // RGB Green as output
  pinMode(BLUE_PIN, OUTPUT);    // RGB Blue as output
  
  // Initialize outputs to OFF state
  digitalWrite(BUZZER_PIN, LOW);
  setRGBColor(0, 0, 0);  // Turn off RGB LED
  
  // Play startup sequence
  Serial.println("Playing startup sequence...");
  playStartupTune();
  
  // Test outputs
  Serial.println("Testing outputs...");
  testOutputs();
  
  // Display initial mode with audio feedback
  displayCurrentMode();
  
  Serial.println("=== SYSTEM READY ===");
  Serial.println("Touch sensor to cycle modes, wave hand to trigger alarm");
  Serial.println("========================");
}

void loop() {
  // Debug output every 2 seconds
  if (millis() - lastDebugOutput > DEBUG_INTERVAL) {
    printDebugInfo();
    lastDebugOutput = millis();
  }
  
  // Check for touch sensor input to change modes
  handleTouchInput();
  
  // Handle mode display timing
  handleModeDisplay();
  
  // Determine if system should be armed based on current mode and light conditions
  updateSystemArmedState();
  
  // Always check for motion and show distance (for debugging)
  long distance = measureDistance();
  
  // If system is armed and not displaying mode, check for motion
  if (systemArmed && !displayingMode) {
    if (distance > 0 && distance <= DETECTION_DISTANCE) {
      if (!alarmActive) {
        triggerAlarm();
        Serial.println("*** INTRUDER DETECTED ***");
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm");
      }
    }
  }
  
  // Handle alarm timing and patterns
  handleAlarm();
  
  // Update LED status when not displaying mode or alarm
  if (!displayingMode && !alarmActive) {
    updateStatusLED();
  }
  
  // Small delay to prevent excessive processing
  delay(50);
}

/*
 * Play startup/reset tune - welcoming and professional
 */
void playStartupTune() {
  // Professional startup sequence - rising scale
  int startupMelody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
  int startupDurations[] = {200, 200, 200, 400};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, startupMelody[i]);
    delay(startupDurations[i]);
    noTone(BUZZER_PIN);
    delay(50);
  }
  
  // Brief pause then confirmation beep
  delay(200);
  tone(BUZZER_PIN, NOTE_G5);
  delay(150);
  noTone(BUZZER_PIN);
  
  Serial.println("System initialized successfully!");
}

/*
 * Play mode change feedback sound
 */
void playModeChangeTune(SystemMode mode) {
  switch (mode) {
    case DAY_ONLY:
      // Bright ascending notes for day mode
      tone(BUZZER_PIN, NOTE_C5);
      delay(150);
      tone(BUZZER_PIN, NOTE_E5);
      delay(150);
      tone(BUZZER_PIN, NOTE_G5);
      delay(200);
      break;
      
    case NIGHT_ONLY:
      // Lower, mysterious notes for night mode
      tone(BUZZER_PIN, NOTE_G4);
      delay(150);
      tone(BUZZER_PIN, NOTE_E4);
      delay(150);
      tone(BUZZER_PIN, NOTE_C4);
      delay(200);
      break;
      
    case ALWAYS_ON:
      // Confident, steady notes for always-on mode
      tone(BUZZER_PIN, NOTE_A4);
      delay(100);
      tone(BUZZER_PIN, NOTE_A4);
      delay(100);
      tone(BUZZER_PIN, NOTE_A5);
      delay(250);
      break;
  }
  noTone(BUZZER_PIN);
}

/*
 * Print comprehensive debug information
 */
void printDebugInfo() {
  Serial.println("--- SYSTEM STATUS ---");
  
  // System state
  Serial.print("Mode: ");
  printCurrentMode();
  Serial.print("Armed: ");
  Serial.println(systemArmed ? "YES" : "NO");
  Serial.print("Alarm: ");
  Serial.println(alarmActive ? "ACTIVE" : "INACTIVE");
  
  // Sensor readings
  long distance = measureDistance();
  Serial.print("Distance: ");
  if (distance > 0) {
    Serial.print(distance);
    Serial.println(" cm");
  } else {
    Serial.println("No reading");
  }
  
  bool ldrState = digitalRead(LDR_PIN);
  Serial.print("Light: ");
  Serial.println(ldrState ? "DARK" : "BRIGHT");
  
  Serial.println("--------------------");
}

/*
 * Test all outputs during startup
 */
void testOutputs() {
  Serial.println("Testing RGB LED...");
  setRGBColor(255, 0, 0);  // Red
  delay(300);
  setRGBColor(0, 255, 0);  // Green
  delay(300);
  setRGBColor(0, 0, 255);  // Blue
  delay(300);
  setRGBColor(0, 0, 0);    // Off
  
  Serial.println("Testing alarm sounds...");
  
  // Test brief alarm preview
  tone(BUZZER_PIN, 1800);
  delay(200);
  tone(BUZZER_PIN, 1200);
  delay(200);
  noTone(BUZZER_PIN);
  
  Serial.println("Output test complete!");
}

/*
 * Handle touch sensor input for mode switching
 */
void handleTouchInput() {
  bool currentTouchState = digitalRead(TOUCH_PIN);
  
  // Check for touch press (LOW to HIGH transition with debounce)
  if (currentTouchState == HIGH && lastTouchState == LOW) {
    delay(DEBOUNCE_DELAY);  // Simple debounce
    
    // Confirm touch is still active after debounce
    if (digitalRead(TOUCH_PIN) == HIGH) {
      // Cycle to next mode
      currentMode = (SystemMode)((currentMode + 1) % 3);
      
      // Log mode change
      Serial.print("*** MODE CHANGED to: ");
      printCurrentMode();
      
      // Play mode change sound
      playModeChangeTune(currentMode);
      
      // Display the new mode
      displayCurrentMode();
      
      // Stop any active alarm when mode changes
      if (alarmActive) {
        stopAlarm();
        playSystemResetTune();
      }
    }
  }
  
  lastTouchState = currentTouchState;
}

/*
 * Play system reset/disarm tune
 */
void playSystemResetTune() {
  // Descending scale to indicate system reset/disarm
  int resetMelody[] = {NOTE_G5, NOTE_E5, NOTE_C5, NOTE_G4};
  int resetDurations[] = {150, 150, 150, 300};
  
  for (int i = 0; i < 4; i++) {
    tone(BUZZER_PIN, resetMelody[i]);
    delay(resetDurations[i]);
    noTone(BUZZER_PIN);
    delay(30);
  }
}

/*
 * Display current mode by lighting appropriate RGB color
 */
void displayCurrentMode() {
  displayingMode = true;
  lastModeChange = millis();
  
  Serial.print("Displaying mode: ");
  switch (currentMode) {
    case DAY_ONLY:
      setRGBColor(255, 0, 0);  // Red for day-only mode
      Serial.println("RED (Day Only)");
      break;
    case NIGHT_ONLY:
      setRGBColor(0, 0, 255);  // Blue for night-only mode
      Serial.println("BLUE (Night Only)");
      break;
    case ALWAYS_ON:
      setRGBColor(0, 255, 0);  // Green for always-on mode
      Serial.println("GREEN (Always On)");
      break;
  }
}

/*
 * Handle mode display timing
 */
void handleModeDisplay() {
  if (displayingMode && (millis() - lastModeChange > MODE_DISPLAY_TIME)) {
    displayingMode = false;
    Serial.println("Mode display finished");
  }
}

/*
 * Update system armed state based on current mode and light conditions
 */
void updateSystemArmedState() {
  bool shouldBeArmed = false;
  bool isDark = digitalRead(LDR_PIN);  // Read digital LDR (HIGH = dark, LOW = bright)
  bool isDaytime = !isDark;            // Invert for daytime logic
  
  switch (currentMode) {
    case DAY_ONLY:
      shouldBeArmed = isDaytime;
      break;
    case NIGHT_ONLY:
      shouldBeArmed = !isDaytime;
      break;
    case ALWAYS_ON:
      shouldBeArmed = true;  // Always armed in this mode
      break;
  }
  
  // Log state changes and play audio feedback
  if (systemArmed != shouldBeArmed) {
    systemArmed = shouldBeArmed;
    Serial.print("*** System ");
    Serial.print(systemArmed ? "ARMED" : "DISARMED");
    Serial.print(" - Light: ");
    Serial.print(isDark ? "DARK" : "BRIGHT");
    Serial.print(", Mode: ");
    printCurrentMode();
    
    // Audio feedback for arming/disarming
    if (systemArmed) {
      playArmingTune();
    } else {
      playDisarmingTune();
    }
  }
}

/*
 * Play arming confirmation tune
 */
void playArmingTune() {
  // Two quick rising beeps to confirm arming
  tone(BUZZER_PIN, NOTE_G4);
  delay(100);
  tone(BUZZER_PIN, NOTE_C5);
  delay(150);
  noTone(BUZZER_PIN);
}

/*
 * Play disarming confirmation tune
 */
void playDisarmingTune() {
  // Two quick falling beeps to confirm disarming
  tone(BUZZER_PIN, NOTE_C5);
  delay(100);
  tone(BUZZER_PIN, NOTE_G4);
  delay(150);
  noTone(BUZZER_PIN);
}

/*
 * Measure distance using ultrasonic sensor
 */
long measureDistance() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Send 10 microsecond pulse to trigger pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pin, get travel time in microseconds
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 30ms timeout
  
  // If no echo received, return -1
  if (duration == 0) {
    return -1;
  }
  
  // Calculate distance
  long distance = (duration * 0.0343) / 2;
  
  return distance;
}

/*
 * Trigger the alarm system
 */
void triggerAlarm() {
  alarmActive = true;
  alarmStartTime = millis();
  
  Serial.println("*** SECURITY BREACH - ALARM ACTIVATED ***");
  
  // Start with high pitch tone
  tone(BUZZER_PIN, 3000);
  
  // Flash red LED
  setRGBColor(255, 0, 0);
}

/*
 * Stop the alarm system and immediately restore the appropriate LED color for the current mode
 */
void stopAlarm() {
  alarmActive = false;
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("*** ALARM DEACTIVATED ***");
  
  // Set LED color immediately based on current mode (not just armed status)
  switch (currentMode) {
    case DAY_ONLY:
      // Set to red mode color (brighter if actively showing mode)
      setRGBColor(50, 0, 0);  // Dimmer red as standby
      break;
    case NIGHT_ONLY:
      // Set to blue mode color
      setRGBColor(0, 0, 50);  // Dimmer blue as standby
      break;
    case ALWAYS_ON:
      // Set to green mode color
      setRGBColor(0, 50, 0);  // Dimmer green as standby
      break;
  }
  
  // Reset any timers that might affect immediate retriggering
  lastModeChange = 0;
  displayingMode = false;
}

/*
 * Handle alarm timing with simple high pitch sound
 */
void handleAlarm() {
  if (alarmActive) {
    // Check if alarm duration has elapsed
    if (millis() - alarmStartTime > ALARM_DURATION) {
      stopAlarm();
      Serial.println("Alarm auto-stopped - timeout reached");
    } else {
      // Calculate elapsed time for alarm
      unsigned long elapsed = millis() - alarmStartTime;
      
      // Simple high pitch sound for first 2 seconds only
      if (elapsed < 2000) {
        // High pitched tone (3000 Hz)
        tone(BUZZER_PIN, 3000);
      } else {
        // No sound after 2 seconds but keep the visual alarm going
        noTone(BUZZER_PIN);
      }
      
      // Synchronized strobe effect continues
      createStrobeEffect(elapsed);
    }
  }
}

/*
 * Create synchronized strobe light effect
 */
void createStrobeEffect(unsigned long elapsed) {
  // Fast strobe synchronized with alarm pattern
  int strobeInterval = 100;  // 100ms strobe cycle
  bool strobeOn = ((elapsed / strobeInterval) % 2) == 0;
  
  if (strobeOn) {
    setRGBColor(255, 0, 0);  // Bright red
  } else {
    setRGBColor(100, 0, 0);  // Dim red (not completely off for visibility)
  }
}

/*
 * Update status LED when system is not in alarm or mode display
 */
void updateStatusLED() {
  // No need to update if we're displaying mode or alarm is active
  if (displayingMode || alarmActive) return;
  
  // Choose colors based on current mode
  switch (currentMode) {
    case DAY_ONLY:
      // Red mode - breathing effect if armed, dimmer if disarmed
      if (systemArmed) {
        unsigned long breathTime = millis() % 2000;
        int brightness = (breathTime < 1000) ? 
                        10 + (breathTime / 50) : 
                        30 - ((breathTime - 1000) / 50);
        setRGBColor(brightness, 0, 0);  // Breathing red effect
      } else {
        setRGBColor(5, 0, 0);  // Very dim red when disarmed
      }
      break;
      
    case NIGHT_ONLY:
      // Blue mode - breathing effect if armed, dimmer if disarmed
      if (systemArmed) {
        unsigned long breathTime = millis() % 2000;
        int brightness = (breathTime < 1000) ? 
                        10 + (breathTime / 50) : 
                        30 - ((breathTime - 1000) / 50);
        setRGBColor(0, 0, brightness);  // Breathing blue effect
      } else {
        setRGBColor(0, 0, 5);  // Very dim blue when disarmed
      }
      break;
      
    case ALWAYS_ON:
      // Green mode - breathing effect if armed, dimmer if disarmed
      if (systemArmed) {
        unsigned long breathTime = millis() % 2000;
        int brightness = (breathTime < 1000) ? 
                        10 + (breathTime / 50) : 
                        30 - ((breathTime - 1000) / 50);
        setRGBColor(0, brightness, 0);  // Breathing green effect
      } else {
        setRGBColor(0, 5, 0);  // Very dim green when disarmed
      }
      break;
  }
}

/*
 * Set RGB LED color
 */
void setRGBColor(int red, int green, int blue) {
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

/*
 * Print current mode to serial monitor
 */
void printCurrentMode() {
  switch (currentMode) {
    case DAY_ONLY:
      Serial.println("DAY ONLY (Red)");
      break;
    case NIGHT_ONLY:
      Serial.println("NIGHT ONLY (Blue)");
      break;
    case ALWAYS_ON:
      Serial.println("ALWAYS ON (Green)");
      break;
  }
}
