const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] = {A3, A4, A5, A6, A7};

const int LEDAnalogInput[NUM_SLIDERS] = {11, 10, 9, 6, 5};
const int LEDMute[NUM_SLIDERS] = {12, 13, A0, A1, A2};

const int NUM_BUTTONS = 5;
const int buttonInputs[NUM_BUTTONS] = {8, 7, 4, 3, 2};

int analogSliderValues[NUM_SLIDERS];
bool digitalButtonValues[NUM_BUTTONS];
bool lastDigitalButtonValues[NUM_BUTTONS];

bool muteState[NUM_BUTTONS] = {LOW, LOW, LOW, LOW, LOW}; // Set the default state of all channels to unmuted. Change to HIGH if you want a channel muted by default.

void setup() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttonInputs[i], INPUT_PULLUP);
  }

  Serial.begin(9600);
}

void loop() {
  updateSliderValues();
  updateButtonValues(); // Update button values to check what buttons are pressed and change muteState accordingly.
  muteSliderValues(); // Actually mute the appropriate channel(s).
  sendSliderValues();
  updateLEDFromInput();
   //printSliderValues();
   //printButtonValues(); // To check what the buttons are outputting.
   //printLEDValues();
  delay(10);
}

void muteSliderValues() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (Serial.available()){
      if ((int)muteState[i] == 1) {
        analogSliderValues[i] = 0; // Overwrites the value of the muted pot to be 0.
      analogWrite(LEDMute[i], 0);
      } else {
      analogWrite(LEDMute[i], 255);
      }
    } else {  
      // serial not available means deej software is off, so turn off the leds     
      analogWrite(LEDMute[i], 0);
    }
  }
}


void updateSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    analogSliderValues[i] = analogRead(analogInputs[i]);
  }
}

void updateButtonValues() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    digitalButtonValues[i] = digitalRead(buttonInputs[i]);
    if (digitalButtonValues[i] != lastDigitalButtonValues[i]) {
      if (digitalButtonValues[i] == LOW) {
        muteState[i] = !muteState[i];
      }
      lastDigitalButtonValues[i] = digitalButtonValues[i];
    }
  }
}

void sendSliderValues() {
  String builtString = String("");

  for (int i = 0; i < NUM_SLIDERS; i++) {
    int sliderValue = (int) analogSliderValues[i];
    builtString += String(sliderValue);

    //extra code
    builtString += ",";
    if (muteState[i] == LOW) {
      builtString += "0";
    } else {
      builtString += "1";
    }
    //extra code end

    //conver it to correct output range, max value is 255
    // int outputValue = map(sliderValue, 0, 1023, 0, 255);
    //  analogWrite(LEDAnalogInput[i], outputValue);

    if (i < NUM_SLIDERS - 1) {
      builtString += String("|");
    }
  }
  // builtString += "\n";

  Serial.println(builtString);
}

void updateLEDFromInput(){
  if (Serial.available()){
    Serial.readStringUntil('-');
      for (int i = 0; i < NUM_SLIDERS; i++) {
        int outputValue = Serial.readStringUntil('|').toInt();
        outputValue = map(outputValue, 0, 100, 0, 255);
        analogWrite(LEDAnalogInput[i], outputValue);
      }
  }
}


// Functions for debugging.
void printSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    String printedString = String("Slider #") + String(i + 1) + String(": ") + String(analogSliderValues[i]) + String(" mV");
    Serial.write(printedString.c_str());

    if (i < NUM_SLIDERS - 1) {
      Serial.write(" | ");
    } else {
      Serial.write("\n");
    }
  }
}

void printLEDValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    String printedString = String("LED #") + String(i + 1) + String(": ") + String(analogRead(LEDAnalogInput[i])) + String("-")+ String(map(analogSliderValues[i], 0, 1023, 0, 255));
    Serial.write(printedString.c_str());

    if (i < NUM_SLIDERS - 1) {
      Serial.write(" | ");
    } else {
      Serial.write("\n");
    }
  }
}

  void printButtonValues() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    String printedString = String("Button #") + String(i + 1) + String(": ") + String(digitalButtonValues[i]);
    Serial.write(printedString.c_str());

    if (i < NUM_BUTTONS - 1) {
      Serial.write(" | ");
    } else {
      Serial.write("\n");
    }
  }
}