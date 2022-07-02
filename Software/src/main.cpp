#include <Arduino.h>
#include "BluetoothA2DPSink.h"

/* Start playing music automatically when bluetooth connects */
#define START_MUSIC_WHEN_CONNECTED

#define BLUETOOTH_NAME "BLAUPUNKT SQR 48"


/* Class for bluetooth audio */
BluetoothA2DPSink a2dp_sink;

/* Pin configuration */
/* Inputs */
const int radio_on_pin = 5;
const int next_button = 16;
const int prev_button = 4;
const int pause_button = 15;

/* Outputs */
const int bluetooth_active_pin = 17;
const int bluetooth_active_led_pin = 19;

/* I2S */
const int bck_pin = 23;
const int ws_pin = 21;
const int data_pin = 22;

/* State enums */
enum Bluetooth_state_enum {
  DISCONNECTED,
  CONNECTED  
};

enum Radio_state_enum {
  OFF,
  ON
};

enum Music_state_enum {
  PAUSED,
  PLAYING  
};

Bluetooth_state_enum Bluetooth_state;
Radio_state_enum Radio_state;
Music_state_enum Music_state;

/* Set initial states */
void init_states(){
  Bluetooth_state = DISCONNECTED;
  Music_state = PAUSED;
  Radio_state = OFF;
}

/* Function needed to call from main loop. Check if one of the buttons is 
pressed and handles it's functionality. */
void button_handle(int button){
    int button_state = digitalRead(button);

    if(button_state == LOW && Bluetooth_state == CONNECTED){
        Serial.println("Button pressed");

        switch (button)
        {
        case next_button:
          a2dp_sink.next();
          Serial.println("Next");
          break;
        case prev_button:
          a2dp_sink.previous();
          Serial.println("Previous");
          break;
        case pause_button:
          if(Music_state == PAUSED){
            a2dp_sink.play();
            Serial.println("Play");
            Music_state = PLAYING;
          } else if (Music_state == PLAYING) {
            a2dp_sink.pause();
            Serial.println("Pause");
            Music_state = PAUSED;
          }
          break;
          
        default:
          break;
        }

        /* Debounce, ugly but works*/
        delay(300);
    }
}

/* Function needed to call from main loop. Polls radio state and starts bluetooth sink 
if radio is on. In situations where radio is turned off and again on, bluetooth is recovered
by reboot. Hacky, but didn't found other ways to handle it more nicely. */
void radio_on_handler(){

  enum Radio_state_enum old_radio_state = Radio_state;

  int radio_state_now = digitalRead(radio_on_pin);

  if(radio_state_now == HIGH){
    Radio_state = ON;
  } else {
    Radio_state = OFF;
  }

  if(old_radio_state != Radio_state){
    Serial.print("Radio state: ");
    Serial.println(Radio_state);

    if(Radio_state == ON){
      if(old_radio_state == OFF){
        a2dp_sink.start(BLUETOOTH_NAME, true);
      }
    } else if (Radio_state == OFF){
      if(old_radio_state == ON){
        a2dp_sink.pause();
        ESP.restart();
      }
    }
  }
}

/* Function needed to call from main loop. Check bluetooth state and updates it.*/
void Bluetooth_state_handler(){

  int bluetooth_state_now = a2dp_sink.is_connected();

  Bluetooth_state_enum old_bluetooth_state = Bluetooth_state;

  if(bluetooth_state_now == true){
    digitalWrite(bluetooth_active_pin, HIGH);
    digitalWrite(bluetooth_active_led_pin, HIGH);
    Bluetooth_state = CONNECTED;

  } else {
    digitalWrite(bluetooth_active_pin, LOW);
    digitalWrite(bluetooth_active_led_pin, LOW);
    Bluetooth_state = DISCONNECTED;
    Music_state = PAUSED;
  }

  if(old_bluetooth_state != Bluetooth_state){
    Serial.print("Bluetooth state: ");
    Serial.println(Bluetooth_state);
    if(old_bluetooth_state == DISCONNECTED && Bluetooth_state == CONNECTED){
      #ifdef START_MUSIC_WHEN_CONNECTED
        delay(1000);
        a2dp_sink.play();
        Music_state = PLAYING;
      #endif
    }
  
  }
}


void setup() {

  Serial.begin(115200); 

  /* Changes the radio to CD-in mode */
  pinMode(bluetooth_active_pin, OUTPUT);
  digitalWrite(bluetooth_active_pin, LOW);

  /* Led illuminates when bluetooth is on */
  pinMode(bluetooth_active_led_pin, OUTPUT);
  digitalWrite(bluetooth_active_led_pin, LOW);

  /* Button inputs */
  pinMode(next_button, INPUT);
  pinMode(prev_button, INPUT);
  pinMode(pause_button, INPUT);

  /* High, when radio is on */
  pinMode(radio_on_pin, INPUT);

  init_states();
  
  /* Configure Bluetooth audio, use I2S*/
  i2s_pin_config_t my_pin_config = {
        .bck_io_num = bck_pin,
        .ws_io_num = ws_pin,
        .data_out_num = data_pin,
        .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  a2dp_sink.set_pin_config(my_pin_config);

}



void loop() {
  button_handle(next_button);
  button_handle(prev_button);
  button_handle(pause_button);
  radio_on_handler();
  Bluetooth_state_handler();
}