// made for momentary (non latching) buttons
// debounce provided

#define DEBOUNCE 200

const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] =  {A0, A1, A2, A3, A4};
const int digitalInputs[NUM_SLIDERS] = {2,  3,  4,  5,  6};

int digitalInputsStates[NUM_SLIDERS] = {HIGH};
int digitalInputsPrevStates[NUM_SLIDERS] = {HIGH};
long digitalInputsTimes[NUM_SLIDERS] = {0};

int analogSliderValues[NUM_SLIDERS];

void setup() { 
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }

  Serial.begin(9600);
}

void loop() {
  updateSliderValues();
  updateButtonStates();
  sendSliderValues(); // Actually send data (all the time)
  // printSliderValues(); // For debug
  delay(10);
}

void updateSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
     analogSliderValues[i] = analogRead(analogInputs[i]);
  }
}

void updateButtonStates() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    int reading = digitalRead(digitalInputs[i]);
    
    if (reading == HIGH && digitalInputsPrevStates[i] == LOW && millis() - digitalInputsTimes[i] > DEBOUNCE) {
      digitalInputsStates[i] ^= 1; // invert value
      
      digitalInputsTimes[i] = millis();    
    }
  
    digitalInputsPrevStates[i] = reading;
  }
}


void sendSliderValues() {
  String builtString = String("");

  for (int i = 0; i < NUM_SLIDERS; i++) {
    builtString += String((int)analogSliderValues[i]);
    builtString += ",";
    builtString += String((int)digitalInputsStates[i]);
    
    if (i < NUM_SLIDERS - 1) {
      builtString += String("|");
    }
  }
  
  Serial.println(builtString);
}

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
