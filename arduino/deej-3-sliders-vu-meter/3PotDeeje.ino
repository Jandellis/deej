const int NUM_SLIDERS = 3;
const int analogInputs[NUM_SLIDERS] = {A5, A6, A7};

const int LEDAnalogInput[NUM_SLIDERS] = {3,5,6};


int analogSliderValues[NUM_SLIDERS];

//bool muteState[NUM_BUTTONS] = {LOW, LOW, LOW}; // Set the default state of all channels to unmuted. Change to HIGH if you want a channel muted by default.

void setup() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }


  Serial.begin(9600);
}

void loop() {
  updateSliderValues();
  //updateButtonValues(); // Update button values to check what buttons are pressed and change muteState accordingly.
  //muteSliderValues(); // Actually mute the appropriate channel(s).
  sendSliderValues();
  updateLEDFromInput();
   //printSliderValues();
   //printButtonValues(); // To check what the buttons are outputting.
   //printLEDValues();
  delay(10);
}

void updateSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    analogSliderValues[i] = analogRead(analogInputs[i]);
  }
}


void sendSliderValues() {
  String builtString = String("");

  for (int i = 0; i < NUM_SLIDERS; i++) {

    //extra code
    builtString += ",";
    builtString += "1";
    //extra code end

    // if(i== 0) {
    builtString += String((int)analogSliderValues[i]);

    // } else {
    // builtString += "0";

    // }



    //conver it to correct output range, max value is 255
    // int outputValue = map(analogSliderValues[i], 0, 1023, 0, 255);
    // if(i== 2) {
    //   //the third one has a issue where the 2nd channel will send a value to the 3rd for some unknown reason
    //   if (outputValue > 15 ) {
    //     analogWrite(LEDAnalogInput[i], outputValue);
    //   } else {        
    //     analogWrite(LEDAnalogInput[i], 0);
    //   }

    // } else {
    // analogWrite(LEDAnalogInput[i], outputValue);

    // }

    if (i < NUM_SLIDERS - 1) {
      builtString += String("|");
    }
  }

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

