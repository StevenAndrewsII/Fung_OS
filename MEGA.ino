/* //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////                   Fung_OS      -    BETA 1.0                 ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Project:              Fung OS / BETA ARCHITECTURE 

overview: 

                Features: 
                    core:
                        searil communications protocol formatting 
                      
                    Multi life support: 
                        Air humidity control 
                        lighting
                        heating 
                        water cleaning 
                        waste water collection pumps
                        water level indication 
                        light / door sensor switch

                    Multi environment managment:
                        SHT20 sensor 
                        valve system ( closed system control )
                        

                    
            Fung_os( pi ) is the userinter interce via USB serial that offers: ( release date: TBA )
                    Touch screen 
                    wifi connection ( to other systems on the local network )
                    UI ( powered by pigame engine )
                    full system management & control
                    environment tracking + data logging over time 
                    usb data strorage 


Created By:           Steven Andrews II 
Creation date:        6/30/23               ( BETA 1.10.0 )


*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <EEPROM.h>                                          // reboot tracking 
#include "DFRobot_SHT20.h"                                   // SHT20 sensor lib
#include <Wire.h>                                            // wire lib 
// frame rate data 
const int     interval                     =  1000;          // 1 second in mills 
const int     frame_rate                   =  10;            // system functions 
// system clock init 
unsigned long RTC_lastTime                 =  millis();      // real time keeping ( once per second )
unsigned long FPS_lastTime                 =  millis();      // fps limited 
// memory allowocation for system backend arrays 
#define MAX_ENVIRONMENTS 8                                   // do not change ( limited by chip I2C adresses )
#define MAX_LIFE_SUPPORT 2                                   // changable ( youll eventually run out of pins - stay under 3 - 4)
#define MAX_IOPINS 7                                         // changable / (reserved for updates and expansion)
// validation //
String        version                      =  "1.10.0" ;     // system software version 
bool          validation_                  =  false;         // validation check guard - on boot 
// sensor init 
DFRobot_SHT20 sht20(&Wire, SHT20_I2C_ADDR);
// hardware reboot 
void(* resetFunc) (void) = 0;                                // in program hardware reboot 
////////////////////////////////////////////////////////////////////////////////////
// system RTC data constructor                      - used in time tracking 
////////////////////////////////////////////////////////////////////////////////////

struct RTC {
    int seconds                            = 0;
    int minutes                            = 0;
    int hours                              = 0; 
    int day                                = 0;               // counts to 7 
    int weeks                              = 0;
    int weeks_rst                          = 8;               // default hard set to 8 weeks 
    String time_of_day                     = "day";           // gets updated by light pin state            
};
RTC                             RTC_;                         // system clock 

struct SYS_SETTINGS {
  int  MAX_ENVIRONMENTS_PER_LIFESUPPORT      = 4;        
};
SYS_SETTINGS                    SYS_SETTINGS_;                // hard system settings 


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*                                                 Life support class 

DEFINITION:   
Environments relates to how many fruiting chambers / tents are fed by a life support system. 

Examples: system flow chart 

life support (A) ---> FC_1, FC_2, FC_3

life support (B) ---> FC_4, FC_5


*/         
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ENVIRONMENT  {                                                        // fruiting chamber - environment 
  public:       
    // ALL LOCAL TO FC DATA AND HANDLERS 
    String ID                         = "null";                             // ID ( indexing ) - default to null for checking
    int bus_location                  = 0;                                  // TCA9548A i2c multi plexer location 
    int sensor_refresh_rate           = 10;                                 // default - deley read 
    unsigned long sensor_refresh_LT   = millis();                           // sensor clock 
    float temp_;                                                            // sensor data 
    float humidity_;                                                        // sensor data 
    
    // valve pin 
    int   valve_pin_id                = -1;                                 // -1 not assigned. ( not enabled )   - definition: vales are literal solinoid valves to direct air flow around the systm ( use relay )
    bool  state                       = false;                              // high low  state
    bool  toggle                      = false;                              // pin mode toggle    - toggle state
    bool  init_                       = false;                              // boot up initilizer 


    // bus select for multiplexer ( 0-7 numbers to index through bus  )
    void TCA9548A(uint8_t bus){
        Wire.beginTransmission(0x70);
        Wire.write(1 << bus );
        Wire.endTransmission();
    }// EOF

    //update sensors 
    void update(){
      TCA9548A(  bus_location  );
      // read/store from that bus location
      temp_           = (9.0*sht20.readTemperature( ))/5.0+32;
      humidity_       = sht20.readHumidity( );
      Serial.println("T: "+ ID+  "  ---->  " + temp_ );
      Serial.println("H: "+ ID + "  ---->  " + humidity_);
    }
};


// pre alocate memory for FRUITING CHAMBER ( EVIRONMENTS )
ENVIRONMENT ENVIRONMENT_BUFFER[ MAX_ENVIRONMENTS ];  
// creates new FC environments 
// tri state check for overflow
void CREATE_ENVIRONMENT( String ID , int bus_location , int valve_pinout = -1){
  int index = 0;
  for (int check_ = 0 ; check_ < sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]); check_++){
      if ( ENVIRONMENT_BUFFER[check_].ID == ID ){
        return;                                               // found duplicate
      }
     if (ENVIRONMENT_BUFFER[check_].ID == "null"){ 
       index++;                                               // indexing unregistered FCS 
     }

     if (check_ == (sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0])-1) && index == 0 ){
       return;                                                // all space is occupied 
     }
  } // subtract unregisted FCS from total = front to back indexing 
  Serial.println(((sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]))-index));
  ENVIRONMENT_BUFFER[((sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]))-index)].ID                     = ID;                        // set id 
  ENVIRONMENT_BUFFER[((sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]))-index)].bus_location           = bus_location;              // set bus location 
  ENVIRONMENT_BUFFER[((sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]))-index)].valve_pin_id           = valve_pinout;              // select valve pin
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*                                                 Life support class 

DEFINITION:   
life support is the heart of the system, life support is a set of pins to controll air,humidity,pumps,heating,etc ...
The system allows for you to continue to add life support moduels operating indipendantly of each other.

The system is fully expandable via the current architecture. 

pin out order: example :

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                    pin_select = {              ///////   -1 = unasigned pin   ///////
                                      -1,       // lighting 
                                      -1,       // pumps 
                                      -1,       // air
                                      -1,       // water cleaning 
                                      -1,       // heating
                                      -1        // water sensor
                    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

functions: 
- pin controller   :  controller for all output pins for life support 
- Lighting         :  light cycle controller 
- waste pumps      :  pumps for FC waste water collection 
- air              :  humidity / air exhanger 
- water cleaning   :  usually ozone or any other auzillary device (optional ozone release valave)
- heating          :  active heating algorythem for simistable temperature flux 

*/         
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LIFE_SUPPORT  {                                                    
  public:       
    // ALL LOCAL TO FC DATA AND HANDLERS 
    String ID                                 = "null";            // ID ( indexing ) - default to null for checking
    
  struct settings{    // default settings, change on/after creation...
      
      // enablers for  sub systems 
      bool valve_system                       = false;
      bool averge_system = "true";

      // clock setting 
      int air_on_time                         = 6;                 // run time 
      int air_auto_on_time                    = 4;                 // in hours 

      int water_cleaning_on_time              = 4;                 // run time 
      int water_cleaning_auto_on_time         = 6;                 // in hours 

      int pump_on_time                        = 6;                 // run time 
      int pump_auto_on_time                   = 4;                 // in days 

      int heat_on_time                        = 0  ;               // not active by default 
      int heat_auto_on_time                   = 0 ;                // not active by default 

      int day_length                          = 11;                // length of 'day' time 
      
      // environment control 
      int ideal_temp                          = 80;                // in F degrees 
      int temp_offset                         = 1;                 // in F degrees            
      int temp_over_regulation_deley          = 10;                // in minutes

      // humidity control 
      int ideal_humidity                      = 90;                // humidity target 
      int humidity_offset                     = 3;                 //
      int humidity_over_regulation_deley      = 1;                 // in minutes 

  };
  settings                      settings_;


  struct core { // do not edit : internally used 
      
      // ls v3
      bool  lightCycle_override               = false;             // door/light switch 
      bool  invert_lightCycle                 = false;             // invert toggle 
      bool  day_cycle[2]                      = {false,true};      // cycle state order 
      
      // pumps
      bool  pump_toggle                       = false;             // toggle for pumps
      int   pump_clk                          = 0 ;                // ontime clock
      int   pump_day_clk                      = 0;                 // pump day clock track       // in days 
      
      // air
      bool  air_toggle                        = false;             // toggle for air
      int   air_clk                           = 0 ;                // ontime clock
      int   air_hour_clk                      = 0;                 // air day clock track        // in hrs
      int   humidity_over_regulation_clk      = 0;                 // deley for activation 
      
      // air valves 
      int   valve_index                       = 0;                 // current valve index
      int   valve_clk                         = 0;                 // clock

      // heating pads
      int   heating_over_regulation_clk       = 0;                 // deley between toggling a system on or off 

      // cleaning 
      bool  cleaning_toggle                   = false;             // toggle for air
      int   cleaning_clk                      = 0 ;                // ontime clock
      int   cleaning_hour_clk                 = 0;                 // air day clock track        // in hrs

      // water level sensor 
      int   water_sensor                      = 0;                 // water sesnor ref 

      // indexing number of connected fc environments 
      int   ENVRIONMENTS_INDEX                = 0 ;
      
    
      // sensor refresh 
      unsigned long sensor_refresh_LT        =  millis();
  };
  core                            core_; 
  
 // list of environment IDs for look up ( pre allowocated ammount of memory (4) for beta )
 String enviroments_connected[ MAX_ENVIRONMENTS ];

  // pin out for life support 
  struct pin {
        int   pinNumber                =  -1;                     // -1 = not enabled by default
        bool  state                    = false;                   // high low  state
        bool  toggle                   = false;                   // pin mode toggle    - toggle state
        bool  input                    = false;                   // input identifier   - pin mode 
        bool  init_                    = false;                   // boot up initilizer 
        int   value                     = 0;                      // value from digital read event 
  };
  pin IO_pinout[ MAX_IOPINS ]; // create pin place holders  

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // system pin controllers - light / air / waste pumps / air + humidity / temperature control...
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  void Environment_waterSensor(){              // basic indicator 
    if ( IO_pinout[ 5 ].pinNumber > -1 ){
      if ( IO_pinout[ 5 ].value == 1) {
        core_.water_sensor  = "true"; 
      }else{
        core_.water_sensor  = "false"; 
      }
    }
  }// EOF
  
  
  
  
  void Environment_heating(){                                                 // BETA 1.10.0
      float sum_     = 0.0;
      float average_  = 0.0;
      // environment cluster temperature summation 
      if ( IO_pinout[ 4 ].pinNumber > -1 ){
        for (int i = 0 ; i < sizeof(enviroments_connected)/sizeof(enviroments_connected[0]); i++){
          for (int check_ = 0 ; check_ < sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]); check_++){
            if (enviroments_connected[i] == ENVIRONMENT_BUFFER[check_].ID ) {
             sum_ = sum_ + ENVIRONMENT_BUFFER[ check_ ].temp_;
            }
          }
        }
        // calculate / enable pin for heater pin / disable 
        average_                                    = sum_ / core_.ENVRIONMENTS_INDEX;

        // initiate heating 
        if (average_ < settings_.ideal_temp - settings_.temp_offset ){      // swing offset
          core_.heating_over_regulation_clk ++;
          if (core_.heating_over_regulation_clk > (settings_.temp_over_regulation_deley*frame_rate)*60){
            // enable pin 
            if ( IO_pinout[ 4 ].state == false ){
             Serial.print("heating system has been activated");
             IO_pinout[ 4 ].toggle                 = true;
            }
          }
        }
        // stop heating 
        if ((average_ > settings_.ideal_temp )){    // swing offset
          // disable pin
          if ( IO_pinout[ 4 ].state == true ){
            Serial.print("heating system has been de_activated");
            IO_pinout[ 4 ].toggle                   = true;
          }
          core_.heating_over_regulation_clk         = 0;
        }
      }
  }// EOF (  Environment_heating  )




  void Environment_WastePumps(){                                                // BETA 1.10.0
    if ( IO_pinout[ 3 ].pinNumber > -1 ){

      ///////    auto waste pumps day toggle 
      if (core_.pump_day_clk >= settings_.pump_auto_on_time ){
        core_.pump_toggle                   = true;
        core_.pump_day_clk                  = 0;
      }

      ///////    pin toggle control 
      if   ( core_.pump_toggle == true){
        // toggle pin on - if off 
        if ( IO_pinout[ 3 ].state == false ){
          Serial.print("Waste pumps have been activated");
          IO_pinout[ 3 ].toggle             = true;
        }
        core_.pump_clk                      ++;
        // time out 
        if (core_.pump_clk > ( settings_.pump_on_time*frame_rate )*60 ) {
          core_.pump_clk                    = 0 ;
          core_.pump_toggle                 = false ;
          IO_pinout[ 3 ].toggle             = true;
          Serial.print("Waste pumps have been de_activated");
        }
      }
    }
  }// EOF (  Environment_Pumps  )




  // ozone - close valves - needed to do 
  void Environment_water_tank_cleaning(){
    if ( IO_pinout[ 2 ].pinNumber > -1 ){
      ///////    auto air day toggle  water_cleaning_on_time
      if (settings_.water_cleaning_auto_on_time > 0){
        if (core_.cleaning_hour_clk >= settings_.water_cleaning_auto_on_time && core_.air_toggle == false ){
        core_.cleaning_toggle                 = true;
        core_.cleaning_hour_clk               = 0;
        }
      }

      ///////    pin toggle control 
      if   ( core_.cleaning_toggle == true){
        // toggle pin on - if off 
        if (IO_pinout[ 2 ].state == false){
          Serial.print("Water tank cleaning has been activated");
          IO_pinout[ 2 ].toggle               = true;
        }
        core_.cleaning_clk                      ++;
        // time out 
        if (core_.cleaning_clk > (settings_.water_cleaning_on_time*frame_rate)*60){
          core_.cleaning_clk                  = 0 ;
          core_.cleaning_toggle               = false ;
          IO_pinout[ 2 ].toggle               = true;
          Serial.print("Water tank cleaning has been de_activated");
        }
      }
    }
  }// EOF (  Environment_water_tank_cleaning  )



// utility 
void valve_select(bool rst_ = false){
  if (rst_ == true){
    core_.valve_index = 0;
    
    for (int i = 0 ; i < sizeof(enviroments_connected)/sizeof(enviroments_connected[0]); i++){
      for (int check_ = 0 ; check_ < sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]); check_++){
        if (enviroments_connected[i] == ENVIRONMENT_BUFFER[check_].ID ) {
          if (ENVIRONMENT_BUFFER[check_].valve_pin_id > 0 ){
            Serial.println("toggleing off...... reset");                                                
            if (ENVIRONMENT_BUFFER[check_].state == true){             // toggle off
              ENVIRONMENT_BUFFER[check_].toggle               = true;
            }
          }
        }
      }
    }
    return;
  }

  // operate
  int index = -1;
  for (int i = 0 ; i < sizeof(enviroments_connected)/sizeof(enviroments_connected[0]); i++){
    for (int check_ = 0 ; check_ < sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]); check_++){
      if (enviroments_connected[i] == ENVIRONMENT_BUFFER[check_].ID ) {
        if (ENVIRONMENT_BUFFER[check_].valve_pin_id > 0 ){
          index++;
          // controller 
          if (index == core_.valve_index ){
            if (ENVIRONMENT_BUFFER[check_].state == false){            // toggle on
              ENVIRONMENT_BUFFER[check_].toggle               = true;
            }
            core_.valve_index ++;                                      // index buffer forward 
            return;
          }else{                                          
            if (ENVIRONMENT_BUFFER[check_].state == true){             // toggle off
              ENVIRONMENT_BUFFER[check_].toggle               = true;
            }
          }
          
        }
      }
    }
  }
} // EOF ( valve_select )



  void Environment_Air(){
    float sum_        = 0;
    float average_    = 0;
    if ( IO_pinout[ 1 ].pinNumber > -1 ){
      /////////////////////////////////////////////
      // Active regulation 
      /////////////////////////////////////////////

      //////     summation for humidity regulation. 
      for (int i = 0 ; i < sizeof(enviroments_connected)/sizeof(enviroments_connected[0]); i++){
        for (int check_ = 0 ; check_ < sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]); check_++){
          if (enviroments_connected[i] == ENVIRONMENT_BUFFER[check_].ID ) {
            sum_ = sum_ + ENVIRONMENT_BUFFER[ check_ ].humidity_;
          }
        }
      }
      // average all envrionments 
      average_ = average_ / core_.ENVRIONMENTS_INDEX ;

      if (average_ < settings_.ideal_humidity - settings_.humidity_offset ){      // swing offset
        if ( core_.air_toggle == false && core_.cleaning_toggle == false){
          core_.humidity_over_regulation_clk ++;
          if (core_.humidity_over_regulation_clk > (settings_.humidity_over_regulation_deley*frame_rate)*60){
           // enable
            Serial.print("humidity/air system has been activated - humidity low !!! ");
            core_.humidity_over_regulation_clk     = 0;
            core_.air_toggle                       = true;
          }
        }
      }
      

      /////////////////////////////////////////////
      // timer activation 
      /////////////////////////////////////////////
      
      ///////    auto air day toggle 
      if (settings_.air_auto_on_time > 0){
        if (core_.air_hour_clk >= settings_.air_auto_on_time && core_.cleaning_toggle == false){
        core_.air_toggle                   = true;
        core_.air_hour_clk                 = 0;
        }
      }

      ///////    pin toggle control 
      if   ( core_.air_toggle == true){
        // toggle pin on - if off 
        if (IO_pinout[ 1 ].state == false){
          valve_select();
          Serial.print("Air exhange has been activated");
          IO_pinout[ 1 ].toggle            = true;
        }
        core_.air_clk                      ++;
        core_.valve_clk                    ++;
       
        // valve operation 
        int t_ = ((settings_.air_on_time*frame_rate)*60)  / 2 ;
        if (core_.valve_clk >= t_) {
          valve_select();
          core_.valve_clk = 0;
        }

        if (core_.air_clk > (settings_.air_on_time*frame_rate)*60){
          core_.air_clk                    = 0 ;
          core_.air_toggle                 = false ;
          // duel control overwrite check
          if (IO_pinout[ 1 ].state == true){
            valve_select(true);            // reset valve system
            IO_pinout[ 1 ].toggle          = true;
          }
          Serial.print("Air exhange has been de_activated");
        }
      }
    }
  }// EOF (  Environment_Air  )



  void Environment_Lighting(){
    // invert toggle light cycle states
    if ( core_.invert_lightCycle == true ){
      if( core_.day_cycle[0] == false ) {
          core_.day_cycle[0]              = true;
          core_.day_cycle[1]              = false;
      }else{
          core_.day_cycle[0]              = false;
          core_.day_cycle[1]              = true;
      }
      core_.invert_lightCycle             = false;
    }

    // pin control 
    if ( IO_pinout[ 0 ].pinNumber > -1 ){
      if (RTC_.hours > settings_.day_length && RTC_.hours <= 60 &&  IO_pinout[0].state == core_.day_cycle[1] ) { 
        IO_pinout[0].toggle = true; // togle off 
        Serial.println("TIME OF DAY: day time : "+ String(IO_pinout[0].state));
      }
      if (RTC_.hours < settings_.day_length &&  IO_pinout[0].state == core_.day_cycle[0] ) { 
        IO_pinout[0].toggle = true; // toggle on 
        Serial.println("TIME OF DAY: day time : "+ String(IO_pinout[0].state));
      }
    }
  } // EOF ( environment lighting )



 // pin controller 
 void pin_controller(){
  for (int i = 0 ; i < sizeof(IO_pinout)/sizeof(IO_pinout[0]); i++){
    /////   initial set up for pins (auto handler for pin mode )
    if (IO_pinout[ i ].pinNumber != -1 ){
      if ( IO_pinout[ i ].init_ == false ){                                
        if ( IO_pinout[ i ].input == false ){  
          pinMode        (  IO_pinout[ i ].pinNumber, OUTPUT );                               // default - output 
          digitalWrite   (  IO_pinout[ i ].pinNumber, LOW    );                               // default - low 
        }else{
          pinMode        (  IO_pinout[ i ].pinNumber, INPUT  );                    
        }
        Serial.println("BOOTING PIN : "+String(IO_pinout[ i ].pinNumber));      
        IO_pinout[ i ].init_                        = true;                           
      }else{

        // digital read from sesnors 
        if (IO_pinout[ i ].input == true){
          IO_pinout[ i ].value = digitalRead(  IO_pinout[ i ].pinNumber );
        }
        // toggle
        if (IO_pinout[ i ].toggle == true && IO_pinout[ i ].input == false){
            // state check 
            if (IO_pinout[ i ].state == false){
              IO_pinout[ i ].state                                = true;                     //  set high state 
              digitalWrite(  IO_pinout[ i ].pinNumber, HIGH  );
            }else{
              IO_pinout[ i ].state                                = false;                    //  set low state 
              digitalWrite(  IO_pinout[ i ].pinNumber, LOW  );
            }
            IO_pinout[ i ].toggle                                 = false;                    //  reset toggle 
        }
      }
    }
  }
 }// EOF (  pin_controller  ) 



 // pin controller 
 void vlave_pin_controller(){
  for (int i = 0 ; i < sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]); i++){
    /////   initial set up for pins (auto handler for pin mode )
    if (ENVIRONMENT_BUFFER[ i ].valve_pin_id > 0 ){
      if ( ENVIRONMENT_BUFFER[ i ].init_ == false ){                                
        pinMode        (  ENVIRONMENT_BUFFER[ i ].valve_pin_id, OUTPUT );                               // default - output 
        digitalWrite   (  ENVIRONMENT_BUFFER[ i ].valve_pin_id, LOW    );                               // default - low 
        Serial.println("BOOTING PIN : "+String(ENVIRONMENT_BUFFER[ i ].valve_pin_id));      
        ENVIRONMENT_BUFFER[ i ].init_                        = true;                           
      }else{

        // toggle
        if (ENVIRONMENT_BUFFER[ i ].toggle == true ){
            // state check 
            if (ENVIRONMENT_BUFFER[ i ].state == false){
              ENVIRONMENT_BUFFER[ i ].state                                = true;                     //  set high state 
              digitalWrite(  ENVIRONMENT_BUFFER[ i ].valve_pin_id, HIGH  );
            }else{
              ENVIRONMENT_BUFFER[ i ].state                                = false;                    //  set low state 
              digitalWrite(  ENVIRONMENT_BUFFER[ i ].valve_pin_id, LOW  );
            }
            ENVIRONMENT_BUFFER[ i ].toggle                                 = false;                    //  reset toggle 
        }
      }
    }
  }
 }// EOF (  pin_controller  ) 


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 // updater functions //  not completed for beta: 
 ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 void update_air(int a, int b ){
    settings_.air_auto_on_time          = b;
    settings_.air_on_time               = a;
  };//EOF
    
  void update_pump(int a, int b ){
    settings_.pump_auto_on_time         = b;
    settings_.pump_on_time              = a;
  };//EOF

  void update_heat(int a, int b ){          
    settings_.heat_auto_on_time         = b;
    settings_.heat_on_time              = a;
  };//EOF
    
  void update_temp(int a, int b, int c) {
    settings_.ideal_temp                = a;    
  };//EOF


}; // EO Class ( LIFE SUPPORT )







// pre alocate memory for life support system - bound to FC targets 
LIFE_SUPPORT LIFE_SUPPORT_BUFFER[ MAX_LIFE_SUPPORT ];  

void CREATE_LIFE_SUPPORT( String ID, String connect_IDs[] , int pin_select[]){
  int index = 0;

  for (int check_ = 0 ; check_ < sizeof(ENVIRONMENT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]); check_++){
      if ( LIFE_SUPPORT_BUFFER[check_].ID == ID ){
        return;                                               // found duplicate
      }
     if (LIFE_SUPPORT_BUFFER[check_].ID == "null"){ 
       index++;                                               // indexing unregistered FCS 
     }

     if (check_ == (sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0])-1) && index == 0 ){
       return;                                                // all space is occupied 
     }
  } // subtract unregisted FCS from total = front to back indexing 
  LIFE_SUPPORT_BUFFER[((sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]))-index)].ID                              = ID;                       // set id 
  


  for (int i = 0 ; i < sizeof(connect_IDs); i++){
    if (i < SYS_SETTINGS_.MAX_ENVIRONMENTS_PER_LIFESUPPORT) {
      LIFE_SUPPORT_BUFFER[((sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]))-index)].enviroments_connected[i]    = connect_IDs[i];           // update ids 
      LIFE_SUPPORT_BUFFER[((sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]))-index)].core_.ENVRIONMENTS_INDEX  = sizeof(connect_IDs);
    }
  }

  for (int i = 0 ; i < MAX_IOPINS; i++){
    LIFE_SUPPORT_BUFFER[((sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]))-index)].IO_pinout[i].pinNumber        = pin_select[i];            // update pin numbers 
    // marking last 2 pins as digital sensor pins 
    if ( i >= MAX_IOPINS - 2 ){ 
      LIFE_SUPPORT_BUFFER[((sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]))-index)].IO_pinout[i].input          = true;
    }
  }

} // EOF (  CREATE_LIFE_SUPPORT  )



// initiates SHT20 sensors for each environment. 
void Boot_Evironment_Sensors(){
     for (int index = 0 ; index < sizeof( ENVIRONMENT_BUFFER )/sizeof( ENVIRONMENT_BUFFER[0] ); index++){
      if (ENVIRONMENT_BUFFER[ index ].ID != "null"){
          ENVIRONMENT_BUFFER[ index ].TCA9548A(ENVIRONMENT_BUFFER[ index ].bus_location);     // get bus location of sensor 
          sht20.initSHT20();                                      // init sensor 
          sht20.checkSHT20();                                     // precheck ( remove for beta )
      }
    }
} // EOF (  Boot_Evironment_Sensors  )



////////////////////////////////////////////////////////////////////////////////////
// system set up  - if running head less " un-comment code "
////////////////////////////////////////////////////////////////////////////////////
void setup() {     
  Serial.begin(57600);                                   //   init serial 
  Wire.begin();                                          //   init i2c 
  delay(500);                                            //   hardware wait - ( pi over-read patch )
////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////
  /// create environments 
  /* copy:
                    CREATE_ENVIRONMENT ( 
                                             name ,                 ( unique identifier )
                                             i2c sensor socket ,    ( i2c multplexer bus location ) 
                                             valve pin              ( optional value for valve system ) 
                      ) ;                                                         // creates FC/Tent envirenment 
  */
  /////////////////////////////////////////////

  CREATE_ENVIRONMENT( "Avadacadava" , 1 , 26 )  ;                 //   create a fruiting chamber envrionment  - name can be anything ( i2c multi FC )                
  CREATE_ENVIRONMENT( "jango"       , 2 , 27 )  ;                       //   create a fruiting chamber envrionment  - name can be anything ( i2c multi FC )

  /////////////////////////////////////////////
  /// create life support air systems ///
  /*copy:
                     CREATE_LIFE_SUPPORT( 
                                              name ( unique id ), 
                                              list of environment names to attach to ,
                                              list of system pins,
                        );                                                            // creates lifesupport system 
  */
  /////////////////////////////////////////////
  String   attach[2] = { 
                                "Avadacadava",            // select environments to be attached to a life support.. 
                                "jango"
  };     
  // SELECT PIN OUT HERE: 
  int      pin_select[7] = {                              // Note!!!!!! ---->                                         ///   -1 to disable pin   /// 
      8 ,                                                 // Lighting pin       
      10,                                                 // Air / humidity     ( air exhange )
      11,                                                 // waste pumps        ( FC waste water drain )
      24,                                                 // tank cleaning      ( ozone )
      25,                                                 // heating            ( heating pads or other devices.. )
      12,                                                 // water level sensor 
      -1                                                  // door sensor ( dicscontinued )
  };

  CREATE_LIFE_SUPPORT("UNIT_A",attach,pin_select);       // creates life upport system 

  // boots sensor ports (must leave alone)
  Boot_Evironment_Sensors();                             //   boot sensors for chambers 
}






/////////////////       RAM DEBUGGER   (ripped from arduino docs)     ///////////////////
void display_freeram() {
  Serial.print(F("- SRAM left: "));
  Serial.println(freeRam());              // byte amount remaining 
  //Serial.println(EEPROM.length());        // print epprom 4k max storage 
}

int freeRam() {
  extern int __heap_start,*__brkval;
  int v;
  return (int)&v - (__brkval == 0  
    ? (int)&__heap_start : (int) __brkval);  
}

//////////////////////////////////////        PROTOCOL FUNCTIONS         ////////////////////////////////////////////

int ping(String id, String pack[]) {
  if (id == "ping"){
      Serial.println("Function ping has been called");
      return 1;
  }else{
    return -2; // error code for incorrect id feild // unused 
  }
}

//  [[[[           Software Validation         ]]]]

// software version validation ( request )
int validate(String id, String pack[]) { // Used to validate software with hosts
  if (id == "validate" ){
      // send validation response 
      Serial.print("validation:"+version);
      return 1;
  }else{
    return -2; // error code for incorrect id feild // unused 
  }
} // EOF (  validate  )

// software version validation ( response )
int validateaccept(String id, String pack[]) { // Used to validate software with hosts
  if (id == "v_accept"){
        // guard 
        if (pack[1]){
          if (pack[1] == "_accepted_"){
            Serial.println("arduino validation_accepted");
            validation_ = true;
          }
        }
      return 1;
  }else{
    return -2; // error code for incorrect id feild // unused 
  }
} // EOF (  validateaccept  )


//  [[[[           System data pull request         ]]]]
int pull_updata(String id, String pack[]) {
  if (id == "cyf" && validation_ == true ){
      // pull up all data from core 
      Serial.println("Function pull_updata has been called");
      return 1;
  }else{
    return -2; // error code for incorrect id feild // unused 
  }
} // EOF (  pull_updata )


///////////////////////////////////    Plugin function indexing ( add in for new protocol functions )   ///////////////////////////////////////
int (*protocol[])(String id, String pack[]) = {
  ping,
  validate,
  validateaccept,
  pull_updata,
};



///////////////////////////////////////////////////////////////////////////////////
/////////////////////          Serial down link           ////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/*

Serial down link 

*/
////////////////////////////////////////////////////////////////////////////
void down(){
  // guard for incoming serial data 
  if (Serial.available() > 0 ){
    // constructor for location determination of delimiters 
    String DATA               = Serial.readString();        // raw string 
    int DATA_len              = DATA.length();              // bit length 
    int location              = 0  ;                        // location index of delimiter 
    int item_count            = 0  ;                        // index delimiter location into dict 
    int *location_buffer      = new int[(DATA_len/2)+1];    // allocated  memory for serch array * update, reduced memory demand *

 
    for (int i = 0 ; i< DATA_len; i++){
      int index_                                  = DATA.indexOf( ";" , location + 1 ); // index location forward 1 character past previous delimiter for next search

      if (index_ > -1) {
        item_count                                =   item_count  + 1 ;  // index new found delimiter 
        location                                  =   index_          ;  // location update to current index position
        location_buffer[item_count-1]             =   location        ;  // load the buffer at the index of the previous item count number with the current
      }else{
        item_count                                =   item_count  + 1 ;
        location                                  =   DATA_len        ;
        location_buffer[item_count-1]             =   location        ; 
        break; // exacpe at last delimiter. 
      }
    }

    String *s_buffer      = new String [item_count];    // allocated  memory - buffer for each string contents 
    
    // Recover chars from string between delimiters 
    int last_grab_location = 0 ;
    for (int i = 0 ; i< item_count; i++){                                           // for each delimiter                                                                                             
      for (int i_ = 0 ; i < DATA_len; i_++ ){                                       // search the string until next delimiter location 

          if ( i_ <  location_buffer[  i  ] && last_grab_location <=  i_ ){         // if we are searching in betwwen two delimiters 
              s_buffer[i] +=  DATA.charAt(i_  );                                    // concatinate strings into buffer 
          }else if(i_ >= location_buffer[  i  ]){                                   // end of current grab / break opp 
              break;
          }

      }
      last_grab_location =  location_buffer[  i  ]+1;                               // update search position
    }

    // package s_buffer for exicution 
    for (int plugins_index = 0 ; plugins_index < sizeof(protocol)/sizeof(protocol[0]); plugins_index++){
      int rt_ = (*protocol[plugins_index])(s_buffer[0],s_buffer);
      if (rt_ == 1){ // early escapment - speed up linear search 
        break;
      }
    }

    delete[] location_buffer;
    delete[] s_buffer;
    
  }
} // EOF ( DOWN )




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////                                   MAIN LOOP                                      ////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  down();                                                              // serial down (real time)

  //////    RTC - Real time clock based on the internal crystal osscillator frequency ( adjust interval to tune )
  if (millis() - RTC_lastTime >= interval ) {
    // state machines for clander
    RTC_.seconds++;
    if (RTC_.seconds > 60){                                            // seconds to minitus 
    RTC_.minutes++;
    RTC_.seconds           = 0 ;
    }

    if (RTC_.minutes > 60){                                            // minutes to hours 
    RTC_.hours++;
      for (int i = 0 ; i < sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]); i++){
        if (LIFE_SUPPORT_BUFFER[i].ID != "null"){
            LIFE_SUPPORT_BUFFER[i].core_.air_hour_clk++; 
        }
      }
    RTC_.minutes           = 0 ;
    }

    if (RTC_.hours > 24){                                              // hours to days 
    RTC_.day++;
      for (int i = 0 ; i < sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]); i++){
        if (LIFE_SUPPORT_BUFFER[i].ID != "null"){
            LIFE_SUPPORT_BUFFER[i].core_.pump_day_clk++; 
        }
      }
    RTC_.hours             = 0 ;
    }

    if (RTC_.day > 7){                                                 // days to weeks 
    RTC_.weeks++;
    RTC_.day               = 0 ;
    }
    if (RTC_.weeks > RTC_.weeks_rst){                                  // calander reset // overflow  
    RTC_.weeks             = 0 ;
    }
    RTC_lastTime                                   = millis();         // update  
  }


  // system internal updaters 
  if (millis() - FPS_lastTime >= interval / frame_rate ) {
     for (int i = 0 ; i < sizeof(LIFE_SUPPORT_BUFFER)/sizeof(LIFE_SUPPORT_BUFFER[0]); i++){
       if (LIFE_SUPPORT_BUFFER[i].ID != "null"){
          LIFE_SUPPORT_BUFFER[i].pin_controller();
          LIFE_SUPPORT_BUFFER[i].vlave_pin_controller();
          LIFE_SUPPORT_BUFFER[i].Environment_WastePumps();
          LIFE_SUPPORT_BUFFER[i].Environment_Air();
          LIFE_SUPPORT_BUFFER[i].Environment_Lighting();
          LIFE_SUPPORT_BUFFER[i].Environment_water_tank_cleaning();
          LIFE_SUPPORT_BUFFER[i].Environment_heating();
          LIFE_SUPPORT_BUFFER[i].Environment_waterSensor();
       }
    }
    FPS_lastTime                                   = millis();        // update
  }

  // sensor update 
  for (int i = 0 ; i < sizeof(ENVIRONMENT_BUFFER)/sizeof(ENVIRONMENT_BUFFER[0]); i++){
    if (ENVIRONMENT_BUFFER[i].ID != "null"){
       if ((millis() - ENVIRONMENT_BUFFER[i].sensor_refresh_LT) >= (interval)*ENVIRONMENT_BUFFER[i].sensor_refresh_rate ){
          ENVIRONMENT_BUFFER[i].update();
          // update clock
          ENVIRONMENT_BUFFER[i].sensor_refresh_LT  = millis();        // update 
       }
    }
  }


}// EOF ( LOOP )
