/*
 * Hardware setup:
 * 
 */

//Setup configuration in arrays where the index refers to a particular Prop-Sensor-Trigger setup
//e.g., the first sensor would have index 0 and sensorTrigPins[0] would be its sensor trigger pin
const int numSensors = 4;
int sensorIndex;

const int POLLING_MODE    = 0;
const int TRIGGERED_MODE  = 1;
const int RESET_MODE      = 2;

// establish sensor configs
boolean sensorActive[numSensors]              = {true, true, true, true}; 
int sensorStates[numSensors]                  = {POLLING_MODE, POLLING_MODE, POLLING_MODE, POLLING_MODE};
int transistorGatePins[numSensors]            = {2, 5,  8, 23}; // transistorGatePin will flip the transistor to allow curent to flow
int sensorTrigPins[numSensors]                = {3, 6,  9, 24}; // sensorTrigPin tells distance sensor to start/stop a reading
int sensorEchoPins[numSensors]                = {4, 7, 10, 25}; // sensorEchoPin is where distance sensor provides feedback
int minTriggerDistances[numSensors]           = {10, 10, 10, 10}; // minimum distance, inches,  object must be away in order to trigger
int maxTriggerDistances[numSensors]           = {72, 72, 72, 72}; // maximum distance, inches, object must be away in order to trigger
int requiredHitCounts[numSensors]             = {2, 2, 2, 2}; // multiple distance pollings are used to reduce errors, this is how many hits between min/max need to be met
unsigned long holdTriggerPinMillis[numSensors] = {50, 500, 500, 500}; // number of milliseconds to maintain trigger pin signal
unsigned long resetDelayMillis[numSensors]     = {30000, 5000, 5000, 5000}; // number of milliseconds before prop can be triggered again after being triggered - should reflect any time prop needs to prepare to be triggered again
unsigned long sensorTriggerTimes[numSensors]; //this will hold the timestamp in millis of when the sensor triggered, used to compare against resetDelayMillis to see if it is time to return to POLLING_MODE


// establish other constants
long ConsoleBps = 19200;

// establish variables for ultrasonic distance sensor
long duration, inches;

boolean serialOn = true; //sometimes writing to Serial hangs board, so this flag turns off writing to Serial in log()
#define pollingLED RED_LED
#define triggeredLED GREEN_LED
#define BLED BLUE_LED

void setup() {
  // initialize serial communication bits per second:
  Serial.begin(ConsoleBps); 

  for(sensorIndex=0; sensorIndex<numSensors; ++sensorIndex) {

    if(sensorActive[sensorIndex]) {
      log("defaulting state on sensor " + String(sensorIndex));
      sensorStates[sensorIndex] = POLLING_MODE;
      
      log("Setting output on transistorGatePin for senosr" + String(sensorIndex));
      pinMode(transistorGatePins[sensorIndex], OUTPUT);
      
      // ensure trigger is not going off
      digitalWrite(transistorGatePins[sensorIndex], LOW);      
      log("transistorGatePin for sensor" + String(sensorIndex) + " set to LOW voltage - ensuring it is off");
      
      //setup sensor pins
      pinMode(sensorTrigPins[sensorIndex], OUTPUT);
      log("Sensor Trig pin set to output");
      pinMode(sensorEchoPins[sensorIndex], INPUT);
      log("Sensor Echo pin set to input");
      
      //init sensorTriggerTimes
      sensorTriggerTimes[sensorIndex] = 0;
    }

  }

  
  pinMode(pollingLED, OUTPUT);
  pinMode(triggeredLED, OUTPUT);
  
  //mark pollingLED on
  digitalWrite(pollingLED, HIGH);
  digitalWrite(BLED, HIGH);

  log("Finished setup()...");
}

void loop() {

  if(sensorIndex >= numSensors) sensorIndex = 0;
  
  if(sensorActive[sensorIndex]) processSensor(sensorIndex);

  sensorIndex++;
  
}

void processSensor(int sensorIndex) {

  if(sensorStates[sensorIndex] == POLLING_MODE) processPollingMode(sensorIndex);
  if(sensorStates[sensorIndex] == TRIGGERED_MODE) processTriggeredMode(sensorIndex);
  if(sensorStates[sensorIndex] == RESET_MODE) processResetMode(sensorIndex);
  
}

//each "mode" handling function manages a state transition - they must be fast to avoid blocking other sensors, so avoid long-lived actions and delays where possible
//check to see if the transition is ready - if so quickly perform the action, if not, check back in next run - either way quickly return control
void processPollingMode(int sensorIndex) {

  log("Processing POLLING_MODE on sensor " + String(sensorIndex));

  if(isSensorTriggered(sensorIndex)) {
    log("Activate trigger pin for sensor " + String(sensorIndex) + " and shift to TRIGGER_MODE");
    digitalWrite(transistorGatePins[sensorIndex], HIGH);     // powers transistor to allow current to flow
    sensorTriggerTimes[sensorIndex] = millis();
    sensorStates[sensorIndex] = TRIGGERED_MODE;
  }
}

void processTriggeredMode(int sensorIndex) {

  log("Processing TRIGGER_MODE on sensor " + String(sensorIndex));
  
//  log("sensorTriggerTimes[sensorIndex] " + String(sensorTriggerTimes[sensorIndex]) + " holdTriggerPinMillis[sensorIndex] " + String(holdTriggerPinMillis[sensorIndex]) + " sum: " + String(sensorTriggerTimes[sensorIndex] + holdTriggerPinMillis[sensorIndex]));
  if( sensorTriggerTimes[sensorIndex] + holdTriggerPinMillis[sensorIndex] < millis() ) {
    log("Deactivate trigger pin for sensor " + String(sensorIndex) + " and shift to RESET_MODE");
    digitalWrite(transistorGatePins[sensorIndex], LOW);     // cease powering transistor to block current flow
    sensorStates[sensorIndex] = RESET_MODE;
  }
}

void processResetMode(int sensorIndex) {

  log("Processing RESET_MODE on sensor " + String(sensorIndex));

  if( sensorTriggerTimes[sensorIndex] + holdTriggerPinMillis[sensorIndex] + resetDelayMillis[sensorIndex] < millis() ) {
    log("Shifting sensor " + String(sensorIndex) + " to POLLING_MODE");
    sensorStates[sensorIndex] = POLLING_MODE;
  }
}

/*
void triggerSensor(int sensorIndex) {

  if(millis() - sensorATriggerTime > resetDelayMillis) {
    if(isSensorATriggered() )
    {
      log("setting LEDs to triggered mode...");
      //mark pollingLED off
      digitalWrite(pollingLED, LOW);
      //mark triggeredLED on
      digitalWrite(triggeredLED, HIGH);
     
      log("Activate trigger pin...");
      digitalWrite(transistorGatePin, HIGH);     // powers transistor and an LED so we can see it happen
     
      delay(holdTriggerAPinMillis);
      
      log("Deactivate trigger pin...");
      digitalWrite(transistorGatePin, LOW);      // power off transistor 
    } else {
      log("Not considered triggered");
      //mark pollingLED on
      digitalWrite(pollingLED, HIGH);
      //mark triggeredLED off
      digitalWrite(triggeredLED, LOW);
    }
  } else {
      log("in reset delay window");
  }
}

*/

boolean isSensorTriggered(int sensorIndex) {
  int countOfHits = 0;
  int countOfAttempts = 0;

  log("Checking sensor " + String(sensorIndex));
  
  while (countOfHits < requiredHitCounts[sensorIndex] && countOfAttempts <3) {
    inches = getSensorDistance(sensorIndex);
    log("Run " + String(countOfAttempts) + ": " + String(inches) + "in");
    if(inches > minTriggerDistances[sensorIndex] && inches < maxTriggerDistances[sensorIndex]) {
        countOfHits = countOfHits+1;
    }
    countOfAttempts++;
    delay(1);
  }

  log("Count of hits: " + String(countOfHits));
  
  if(countOfHits >= requiredHitCounts[sensorIndex] )
  {
    return true;
  }  else {
    return false; 
  }
}

long getSensorDistance(int sensorIndex)
{
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:  
  digitalWrite(sensorTrigPins[sensorIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(sensorTrigPins[sensorIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(sensorTrigPins[sensorIndex], LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  duration = pulseIn(sensorEchoPins[sensorIndex], HIGH);

  // convert the time into a distance
  return microsecondsToInches(duration);
}

long microsecondsToInches(long microseconds)
{
// According to Parallax's datasheet for the PING))), there are
// 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
// second). This gives the distance travelled by the ping, outbound
// and return, so we divide by 2 to get the distance of the obstacle.
// See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
return microseconds / 74 / 2;
}

void log(String logLine)
{
  if(serialOn) Serial.println(String(millis()) + ": " + logLine);
}
