/*

Project Name: Blood Pressure and Heart Rate Detector 
Names: Ariel Hsieh (ach573), Rameen Mahmood (rkm352)
Class: ECE 4144: Introduction to Embedded Systems 
Professor: Matthew Campisi 
Due Date: May 16, 2022

*/

#include <Arduino.h>
#include "Adafruit_CircuitPlayground.h"
#include "Wire.h"
#define SENSOR_ADDRESS 0x18


void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);

  CircuitPlayground.begin(); // to set up the neopixels 
}

//function to communicate with the sensor and obtain readings
float GetFromSensor()
{
  // steps for getting readings with sensors 
  uint8_t StatusByte;
  uint8_t dataByte1; 
  uint8_t dataByte2; 
  uint8_t dataByte3;  

  //communicating with the pressure sensor according to the steps of the datasheet
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(0xAA);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission(false);
  delay(5);
  Wire.requestFrom(SENSOR_ADDRESS,4,true);

  //reading the data from the pressure sensor 
  if (Wire.available()>1){
    StatusByte=Wire.read();
    dataByte1=Wire.read();
    dataByte2=Wire.read();
    dataByte3=Wire.read();
  }
  uint32_t output_pressure = ((uint32_t)dataByte1 << 16) + ((uint32_t)dataByte2 << 8) + ((uint32_t)dataByte3); 

  // algorithm to calculate the pressure in mmHg from analog readings 
  float output_min = 419430; // 2.5% of 2^24 counts 
  float output_max = 3774873; //22.5% of 2^24 counts 
  float  p_max = 300; // maximum pressure according to the datasheet
  float  p_min = 0; // minimum pressure according to the datasheet 
  float  one = (float)output_pressure - output_min; 
  float  two = p_max - p_min; 
  float  three = output_max - output_min; 
  float numerator = one*two; 
  float divided = numerator / three; 
  float  pressure = divided + p_min; 
  return pressure;
}

//define global variables 

float prev_pressure = 150; // establish the previous pressure 
float rate; // rate at which pressure is increased/decreased 
float max_pressure = -10; // local maximum of pressure at each oscillation 
float min_pressure = 1000; // local minimum of pressure at each oscillation 
float first_osc = 0; // flag to find the first oscillation  
float first_peak_pressure; // pressure of the first oscillation peak to find systolic 
float p2p; // peak to peak pressure --> (local max - local min) 
float prev_max_p2p = 0;  // the maximum peak to peak from previous oscillations 
float max_p2p_pressure = 0; // peak pressure at the maximum oscillation 
float last_pressure; // diastolic pressure 
float prev_slope; // slope from previous readings 
int reached_160 = 0; // flag to count when the pressure reaches 160 
int max_flag = 0; // flag to see when it's from a min to a max, not a max to a min

// we want to take current max - previous min = p2p, not the current min - previous max, because 
// the pressure keeps decreasing. Therefore, we need max_flag to see when it's a current max. 

float time_counter = 0; // timer to count to 10 so it's one second, in order to calculate heart rate 
int max_counter = 0; // counter to find the number of oscillations in 15 seconds, to calculate heart rate 
int beats = 0; // how many beats in 15 seconds 

// function to display the pressure level and changes by using neopixels 
void display_pressure(float pressure){ 
  // neopixels light up with every 10mmHg change in pressure 
  if (pressure < 160 && pressure > 150){ // when pressure is between 150-160 
    CircuitPlayground.clearPixels(); // reset all pixels 
    for (int n = 0; n < 10; n++){ // all 10 neopixels should be lit up 
      CircuitPlayground.setPixelColor(n, 171, 39, 79); // set all neopixels purple color 
    }
  }
  else if (pressure < 149 && pressure > 140){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 9; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 139  && pressure > 130){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 8; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 129 && pressure > 120){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 7; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 119 && pressure > 110){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 6; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 109  && pressure > 100){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 5; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 99 && pressure > 90){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 4; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 89  && pressure > 80){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 3; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 79  && pressure > 70){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 2; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 69  && pressure > 60){
    CircuitPlayground.clearPixels();
    for (int n = 0; n < 1; n++){
      CircuitPlayground.setPixelColor(n, 171, 39, 79);
    }
  }
  else if (pressure < 60){
    CircuitPlayground.clearPixels(); 
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  float curr_pressure = GetFromSensor(); // get the current pressure from function 
  Serial.print("Current Pressure: "); 
  Serial.println(curr_pressure); // print out the current pressure that is read 
  display_pressure(curr_pressure); // display the current pressure that will be on neopixels 
  rate = prev_pressure - curr_pressure; // calculate the rate at which pressure is increased or decreased 
  
  // controlling deflation rate using measured data 
  // range of 2mmHg-6mmHg per second to get around the rate of decreasing 4mmHg per second 
  if (rate > 0.6){ // if the rate is greater than 6mmHg/sec, the user is deflating too fast
    Serial.println("Release rate too fast");
  }
  else if (rate < 0.2 && rate > 0){ // if the rate is below 2mmHg/sec, the user is deflating too slow
    Serial.println("Release rate too slow");
  }
  else if (rate < 0){ // if the rate is negative, it means the pressure is increasing / user is inflating
    Serial.println("Increasing Pressure");  
  }
  else{
    Serial.println("Release rate just right"); 
  }
   
  float slope = (curr_pressure - prev_pressure); // calculate the slope 
  
  if (curr_pressure > 160){ // if current pressure gets up to 160, the flag is set, so we can start looking at readings 
    reached_160 = 1; 
  }
  if (reached_160 == 1 and curr_pressure < 160){ // once the pressure gets up to 160, wait till it goes below 160 to look for oscillations
    if (slope > 0 && prev_slope < 0){ // if the current slope positive, and previous slope was negative --> local minimum
      max_flag = 0; // minimum so it's not a max, turn off the max flag
      min_pressure = prev_pressure; // set the local minimum pressure to the previous pressure 
    }
    else if (slope < 0 && prev_slope > 0){ // current slope is negative, and previous slope was positive --> local maximum
      max_flag = 1; // turn on the max flag 
      if (curr_pressure < 120 && curr_pressure > 60){ // while the pressure is between 120 and 60, start counting # of oscillations
         max_counter = max_counter + 1; // increment the # of oscillations (used for heart rate calculation) 
      }
      if (first_osc == 0){ // if the first oscillation flag is not set, this means that this is our first oscillation
        first_peak_pressure = prev_pressure; // record the first oscillation pressure as the previous pressure 
        first_osc = 1; // set the first oscillation flag as 1 
      }
      max_pressure = prev_pressure; // set local max pressure to the previous pressure 
    }
    if (max_flag == 1){ // if the max flag is set, that is, only when we encounter a local maximum 
      p2p = max_pressure - min_pressure; // get the peak to peak voltage by subtracting local max - local min
      if (p2p > prev_max_p2p){ // if the current peak to peak voltage is greater than the previous maximum peak to peak 
        max_p2p_pressure = max_pressure; // make the max oscillation pressure the current local maximum pressure 
        prev_max_p2p = p2p; // make the max peak to peak = current peak to peak
      }
     }
  }

  if (time_counter == 150){ // the timer counts up to 15 seconds (our delay is 100ms), based upon standard regulation of time to count beats to calculate heart rate 
    if (curr_pressure < 120 && curr_pressure > 60){ 
      beats = max_counter; // # of beats in 15 seconds is the counter of how many oscillations there was 
      max_counter = 0; // reset the oscillation counter (which represents the beats)
    }
    time_counter = 0; //reset the timer counter 
    
  }


  time_counter = time_counter +1; // increment timer counter for every loop (every 100ms)

  last_pressure = max_p2p_pressure - (first_peak_pressure - max_p2p_pressure); // diastolic pressure is same distance from max oscillation pressure
  // as systolic pressure is away from max oscillation pressure 

  //print out systolic, diastolic, and heart rate for user to see 
  Serial.print("Systolic : "); 
  Serial.print(first_peak_pressure); 
  Serial.println(" mmHg"); 

  Serial.print("Diastolic: "); 
  Serial.print(last_pressure);
  Serial.println(" mmHg"); 

  Serial.print ("Heart Rate: ");
  Serial.print(beats*4);
  Serial.println(" bpm"); 

  prev_pressure = curr_pressure;
  prev_slope = slope; 
  
  delay(100);
  

  
}
