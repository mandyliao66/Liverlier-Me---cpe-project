#include <M5Core2.h>
#include "images.cpp"
#include "MAX30100.h"
#include <TimeLib.h>
#define LIGHTSLEEP_T 4200

//definitions and declarations
typedef uint16_t rawOxiData;
#define SAMPLING_RATE               MAX30100_SAMPRATE_100HZ
#define IR_LED_CURRENT              MAX30100_LED_CURR_24MA
#define RED_LED_CURRENT             MAX30100_LED_CURR_27_1MA
#define PULSE_WIDTH                 MAX30100_SPC_PW_1600US_16BITS
#define HIGHRES_MODE                true

#define SAMP_PERIOD 5 //seconds
#define BUFF_SIZE SAMPLING_RATE*SAMP_PERIOD
rawOxiData buff_secret[BUFF_SIZE];
MAX30100 sensor; 

long _last_time = 0;
int  _buff_i = 0;
long timer;
long beginTimer, endTimer= 0;
float gyroX, gyroY, gyroZ;
float accX, accY, accZ;
int activateTimer, interruptions, stage_sleepTime, sleepTimerInterruption, deepsleepInterruption;
int lightSleep, deepSleep, totalSleep;
int total_lightSleep, total_deepSleep;

#define NUM_OPT 3
#define notif_01 (my_image)

void print_startpg();
void run_settings();
void setdefaulttxt();
void print_menu(int selection);
int process_menu(int selection);


int u_age = 30;     // Global variable with the Age
int u_checkup = 1;  // Global variable with the checkup
const char menu_str[NUM_OPT][21] =
{ "Age                 ",
  "Checkup Frequency   ",
  "Exit"
};

bool notif;


/////////program///////

void setup(){
  M5.begin();
  Serial.begin(115200);
  print_startpg();
  M5.IMU.Init();
  
  Serial.print("Initializing MAX30100..");
  M5.Lcd.fillScreen(BLACK);
  setup_sensor();
}

void loop(){
  
  M5.update();
  if (M5.BtnA.wasReleased()) {
    //settings
    int current_sel = 0;
    M5.Lcd.fillScreen(BLACK);
    print_menu(current_sel);
    M5.update();
    if (M5.BtnA.wasReleased())   // USer presses Menu Down
     current_sel++;
    if (current_sel >= NUM_OPT)
    current_sel = 0;
    if (M5.BtnB.wasReleased())    // USer presses enter
      process_menu(current_sel);
     }
     
    M5.update();
    
   if (M5.BtnB.wasReleased()){
    //sleep display page
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5,110);
    M5.Lcd.print("Sleep tracking in progress");
    //sleep code 
      if (millis() - timer > 1000)
    M5.IMU.getGyroData(&gyroX,&gyroY,&gyroZ);
    M5.IMU.getAccelData(&accX,&accY,&accZ);
  
    Serial.print("Updating things");
  
    updateActiveTimer();
    Serial.print("Update 1");
    measureSleep();
    Serial.print("Update 2");
    measureStageSleep();
    Serial.print("Update 3");
   
    Serial.print(beginTimer);
    Serial.print(",");
    if (lightSleep >= 900) //if the light sleep is more than 15 minutes
    {
      Serial.print(lightSleep / 60);
      Serial.print(" , ");
      Serial.print(deepSleep / 60 );
      Serial.print(" , ");
      Serial.print(total_lightSleep / 60 );
      Serial.print(" , ");
      Serial.print(total_deepSleep / 60 );
      Serial.print(" , ");
      Serial.print(totalSleep / 60 );
      Serial.print(" ; ");
    }
    else
    {
      Serial.print(0);
      Serial.print(" , ");
      Serial.print(0);
      Serial.print(" , ");
      Serial.print(total_lightSleep / 60 );
      Serial.print(" , ");
      Serial.print(total_deepSleep / 60 );
      Serial.print(" , ");
      Serial.print(totalSleep / 60 );
     
    }
    //return button
    M5.Lcd.setCursor(0, M5.Lcd.height() - 14);
    M5.Lcd.printf("Home");
    if(M5.BtnA.wasReleased()){
   
      break;
    }
    
  }
  else if(M5.BtnC.wasReleased()){
    while (1){
      //heartrate code 
      M5.Lcd.fillScreen(BLACK);
      bool buff_ready = false;
      readSensorSignal(buff, buff_ready);
      float peak_dist = 0.5;

     if(buff_ready){
    float HR = measure_HRV(buff, 30000,  peak_dist*SAMPLING_RATE);
    M5.Lcd.setCursor(0,0);
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.printf("HR = %2.1d bpm", HR);
     }
 
    //return button
    M5.Lcd.setCursor(0, M5.Lcd.height() - 14);
    M5.Lcd.printf("Home");
    if(M5.BtnA.wasReleased()){
      
      break;
  }
}
  }
 //time
  time_t ti=now();
  time_t te ti+ month(u_checkup);
  M5_update();
  
  if (now()=te){
    notif = true
    ti=now();
  }
 
  }

///////functions////////

//menu and settings display
void print_startpg(){
  //title
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(YELLOW);
 // M5.Lcd.setTextColor(167, 119, 179);
  M5.Lcd.setCursor(20,110);
  M5.Lcd.print("LIVELIER ME");

  //menu
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(0, M5.Lcd.height() - 14);
  M5.Lcd.print("Settings   Sleep  Heartrate");
}


void print_menu(int selection) {
  M5.Lcd.setCursor(0, 0);
  for (int i = 0; i < NUM_OPT; i++) {
    M5.Lcd.setTextColor(WHITE, BLACK);
    M5.Lcd.setTextSize(2);

    if (i == selection)
      M5.Lcd.setTextColor(BLACK, WHITE);

    M5.Lcd.println(menu_str[i]);
  }
  M5.Lcd.setCursor(0, M5.Lcd.height() - 14);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.print("Down      Enter      ");
}

int process_menu(int selection) {
  if (selection == 0) {
    while (1) {
      M5.Lcd.setCursor(70, 100);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Lcd.setTextSize(4);
      M5.Lcd.printf("Age: %i     ", u_age);

      M5.Lcd.setCursor(0, M5.Lcd.height() - 14);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print("Okay        -        +");
     
      M5.update();
      if (M5.BtnA.wasReleased()) {
        break;
      } 
      else if (M5.BtnB.wasReleased() && (u_age >= 13) ) {
        u_age = u_age - 1;
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.printf("Age: %i     ", u_age);

      }
      if (M5.BtnC.wasReleased() && ( u_age <= 100)) {
        u_age = u_age + 1;
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.printf("Age: %i     ", u_age);

      }
    }
  }
  else if (selection == 1) {
    while (1) {
      M5.Lcd.setCursor(0, 100);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Lcd.setTextSize(3);
      M5.Lcd.printf("Check up every %i  %s   ", u_checkup,"months");
      M5.Lcd.setCursor(0, M5.Lcd.height() - 14);
      M5.Lcd.setTextSize(2);
      M5.Lcd.print("Okay        -        +");

      M5.update();
      if (M5.BtnA.wasReleased() && (u_checkup >= 1) ) {
        break;
      } 
      else if (M5.BtnB.wasPressed() && (u_checkup >= 1) ) {
        u_checkup -= 1;
      M5.Lcd.setCursor(0, 100);
      M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
      M5.Lcd.setTextSize(3);        
      M5.Lcd.printf("Check up every %i  %s   ", u_checkup,"months");
      }
      if (M5.BtnC.wasPressed()) {
        u_checkup += 1;
        M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Lcd.printf("Check up every %i  %s   ", u_checkup,"months");
      }
    }
  } else if (selection == 2) {
    return 2; // Exit from menu command
  }

  return 1;
}

void setdefaulttxt(){
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(2);
}


//heart rate functions
void setup_sensor() {
  while (!sensor.begin()) {
    Serial.println("Sensor not found");
    delay(1000);
  }

  // Set up the sensor parameters.
  sensor.setMode(MAX30100_MODE_SPO2_HR);
  sensor.setLedsCurrent(IR_LED_CURRENT, RED_LED_CURRENT);
  sensor.setLedsPulseWidth(PULSE_WIDTH);
  sensor.setSamplingRate(SAMPLING_RATE);
  sensor.setHighresModeEnabled(HIGHRES_MODE);
}

uint16_t ir_last_raw = 0, led_last_raw = 0, k_last_raw = 0;
uint16_t value1 = 0, value2 = 0, value3 = 0, value4 = 0;
uint16_t Alpha = 0.3 * 256;


void readSensorSignal(rawOxiData* buff , bool &buff_ready ) {
  long _time = millis();
  long dt = _time - _last_time;
  sensor.update();

  // Wait for 10 ms to have passed (1/SAMPLING_RATE)
  if (dt >= 10){
    
    uint16_t ir, red;
    if (sensor.getRawValues(&ir, &red)) {
      _last_time = _time;
      if (_buff_i < BUFF_SIZE) {
        buff_secret[_buff_i] = (rawOxiData) ir;

        value1 = (uint16_t) abs((int)(ir * 10.0)-40000*10);
       
        Serial.printf("%i, %i, %i, %i \n",value1, 0, 0, 0);
        
      } else {
        for (int i = 0; i < BUFF_SIZE; i++) {
          buff[i] = buff_secret[i];
        }
        buff_ready = true;
        _buff_i = 0;
        return;
      }
      _buff_i++;
    }
  }
}


float measure_HRV(rawOxiData *buff, rawOxiData peak_prom, int min_peak_dist) {
  int num_peaks = 0;
  int i_prev = 0;

  for (int i = 1; i < BUFF_SIZE - 1; i++) {
    rawOxiData prev = buff[i - 1];
    rawOxiData curr = buff[i];
    rawOxiData next = buff[i + 1];

    if ((curr < prev) && (curr < next) && (curr < peak_prom)) {

      int peak_dist = i - i_prev;
      if (peak_dist > min_peak_dist) {
        num_peaks++;
        i_prev = i;
      }
    }
  }

  float _time = ((float) SAMP_PERIOD);
  float HRV = (float) num_peaks / _time * 60.0;
  return HRV;
}

//notification
void notif(bool nf){
  
  if nf = true
    M5.Lcd.drawBitmap(0,0,M5.Lcd.height(), M5.Lcd.width(), (uint16_t *) notif_01);
}

//sleep functions
int updateActiveTimer(){
  if (activateTimer == 0)
  {
    if ((gyroX<=20 || gyroX >=-20) && (gyroY<=20 || gyroY >=-20) && ( gyroZ <=20 || gyroZ >=-20)) //if no movement is detected
    {
      beginTimer = millis()/1000 - endTimer; //start the timer
      if (beginTimer == 300) // Activate the timer only if more than 5  in have passed
      {
        activateTimer = 1; //activate the timer to 1
      }
    }

    if ((gyroX >=20 || gyroX <= -20) && (gyroY >=20 || gyroY <= -20) && (gyroZ >=20 || gyroZ <= -20)) //if movement is detected
    {
      endTimer = millis()/1000; //end timer
    }  
  }  
  return activateTimer;
}

int measureSleep(){
  if (activateTimer == 1) //if the timer is activated to 1
  {
    lightSleep = (millis()/1000) - endTimer; //start counting the time for light sleep

    if (interruptions == 0) // if there are no interruptions
    {
      if (lightSleep >= LIGHTSLEEP_T) //if lightsleep has been counted for more than 70 minutes
      {
        if (deepsleepInterruption > LIGHTSLEEP_T)
        {
          if (lightSleep - sleepTimerInterruption >=600)
          {
            deepSleep = lightSleep - deepsleepInterruption;
          }
        }
      }
    }
    lightSleep = lightSleep - deepSleep;

    if ((gyroX >=20 || gyroX <= -20) && (gyroY >=20 || gyroY <= -20) && (gyroZ >=20 || gyroZ <= -20) //if movement is detected
    & ((millis()/1000) - endTimer - sleepTimerInterruption > 8) )           // and it's at least 8 seconds since the last interruption
    {
      sleepTimerInterruption = (millis()/1000) - endTimer;
      deepsleepInterruption = lightSleep;
      interruptions = interruptions + 1; //count the number of interruptions
    }

    if ((millis()/1000) - endTimer - sleepTimerInterruption > 300 ) //if the last interruption was 5 minutes before
    {
      interruptions = 0; //interruptions will be set to 0
    }
    else if (interruptions >= 5) //if there are 5 or more interruptions within 5 minutes
    {
        endTimer = (millis()/1000); // User is awake so update the last waken time
        if (lightSleep >= 900) //if light sleep is greater than 15 minutes
        {
          total_lightSleep = total_lightSleep + lightSleep; //start counting the total light sleep
          total_deepSleep = total_deepSleep  + deepSleep; //start counting the total deep sleep
          totalSleep = total_lightSleep + total_deepSleep; //calculate the total sleep time
        }
        activateTimer = 0;
        interruptions = 0;
        deepSleep = 0;
        lightSleep = 0;
        sleepTimerInterruption = 0;
        deepsleepInterruption = 0;
    }
  }
  return total_lightSleep;
  return total_deepSleep;
  return totalSleep;
}

int measureStageSleep(){

  stage_sleepTime = lightSleep + deepSleep;
  if (stage_sleepTime >= 5400) //if the sleep time is more than 90 minutes
  {
    endTimer = (millis()/1000);
    total_lightSleep = total_lightSleep + lightSleep; //calculate the light sleep time
    total_deepSleep = total_deepSleep + deepSleep; //calculate the deep sleep time
    totalSleep = total_lightSleep + total_deepSleep;  //calculate the total sleep
    activateTimer = 0;
    interruptions = 0;
    deepSleep = 0;
    lightSleep = 0;
    sleepTimerInterruption = 0;
    deepsleepInterruption = 0;  
  }
  return total_lightSleep;
  return total_deepSleep;
  return totalSleep;
 
}
