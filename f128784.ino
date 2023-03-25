#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
// defining the up and down arrow characters
#define DOWN_ARROW 0
#define UP_ARROW 1

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// all the states used in the program.
enum state_e{ SYNCHRONISATION, MAIN};
enum state_b{ DISPLAY_CHN, DISPLAY_SEL};
extern char *__brkval;

int freeMemory() {
char top;
return (int) &top - (int) __brkval;
}

struct channel {
  char Name;
  char* description;
  byte minV;
  byte maxV;
  byte value;
  bool changed;
  byte* recent;
  byte pointer;
  bool full;
};

struct admin{
  state_e state = SYNCHRONISATION;
  state_b substate = DISPLAY_CHN;
  byte menu_pointer = 255;
  channel* channels = (channel*) malloc(26 * sizeof(channel));
  bool exist_array[26] = {false};
  unsigned long lastq = 0;
  unsigned long lastsel = 0;
  bool has_shown = false;

  void main(){
    this->state = MAIN;
    lcd.setBacklight(7);
    Serial.println("BASIC");
  }

  void synchronisation(){
    if(this-> state == SYNCHRONISATION){
      if(millis() - this->lastq > 1000){
        this->lastq = millis();
        Serial.print("Q");
      }
    }
  }

  void input(String str){
    if (this->state == SYNCHRONISATION) {
      if (str == "X") {
        this->main();
      }
    } else {
      this->process_input(str);
    }
  }

  boolean channel_exists(char chr){
    for(byte i = 0; i < 26; i++){
      if(exist_array[i] & channels[i].Name == chr){
        return true;
      }
    }
    return false;
  }

  void update_description(char chr, String str){
    str.toCharArray(channels[chr - 65].description, 16);
  }

  void create_channel(char chr, String str){
    channels[chr - 65].Name = chr;
    channels[chr - 65].description = (char*)malloc(16 * sizeof(char));
    str.toCharArray(channels[chr - 65].description, 16);
    channels[chr - 65].minV = 0;
    channels[chr - 65].maxV = 255;
    channels[chr - 65].value = 0;
    channels[chr - 65].changed = false;
    exist_array[chr - 65] = true;
    channels[chr - 65].recent = (byte*)malloc(64 * sizeof(byte));
    channels[chr - 65].pointer = 0;
    channels[chr - 65].full = false;
    Serial.print(chr);
    Serial.print(0);
    Serial.print("-");
    Serial.println(255);
    
    if(this->menu_pointer == 255){
      this->menu_pointer = chr - 65;
    }
  }

  void set_value(char chr, byte val){
    channels[chr - 65].value = val; 
    channels[chr - 65].changed = true;
    channels[chr - 65].recent[channels[chr - 65].pointer] = val;
    if(channels[chr - 65].pointer == 63){
      channels[chr - 65].full = true;
    }
    channels[chr - 65].pointer = (channels[chr - 65].pointer + 1) % 64;
  }

  byte get_average(byte index) {
    int sum = 0;
    if(channels[index].full){
      for (byte i = 0; i < 64; i++) {
        sum += channels[index].recent[i];
      }
      return sum/channels[index].pointer;      
    }else{
     for (byte i = 0; i < channels[index].pointer; i++) {
        sum += channels[index].recent[i];
      }
      return sum/channels[index].pointer;
    }
  }

  void set_min(char chr, byte val){
    channels[chr - 65].minV = val;
    Serial.print(chr);
    Serial.print(channels[chr - 65].minV);
    Serial.print("-");
    Serial.println(channels[chr - 65].maxV); 
  }

  void set_max(char chr, byte val){
    channels[chr - 65].maxV = val;
    Serial.print(chr);
    Serial.print(channels[chr - 65].minV);
    Serial.print("-");
    Serial.println(channels[chr - 65].maxV);
  }

  void format_channel(byte index, char* channel){
    if(channels[index].changed){
      sprintf(channel, "%c%3d, %3d", channels[index].Name, channels[index].value, get_average(index));
    }else{
      sprintf(channel, "%c", channels[index].Name);
    }
  }

  void display_channel(){
    lcd.clear();
    char channel[17];
    if(this->menu_pointer != 255){

      byte before_index = this->get_before_channel();

      if(before_index != 255){
        lcd.setCursor(0,0);
        lcd.write(UP_ARROW);
      }
      
      lcd.setCursor(1,0);
      format_channel(this->menu_pointer, channel);
      lcd.print(channel);

      byte next_index = this->get_next_channel(this->menu_pointer);
      byte after_next_index = this->get_next_channel(next_index);
      
      if(next_index != 0){
        if(after_next_index != 0){
          lcd.setCursor(0,1);
          lcd.write(DOWN_ARROW);
        }
        format_channel(next_index, channel);
        lcd.setCursor(1,1);
        lcd.print(channel);             
      }
    }
  }

  byte get_next_channel(byte ptr){
    for(byte i = ptr + 1; i < 26 ;i++){
      if(exist_array[i]){
        return i;
      }
    }
    return 0;
  }

  byte get_before_channel(){
    for(byte i = this->menu_pointer - 1; i < 255 ; i--){
      if(exist_array[i]){
        return i;
      }
    }
    return 255;
  }

  void display_channel_description(){
    static unsigned int tpos = 0, bpos = 0;
    static unsigned long now = millis();
    
    if(this->menu_pointer != 255){
      lcd.setCursor(11,0);
      if(strlen(channels[this->menu_pointer].description) > 5){
        lcd.print(channels[this->menu_pointer].description + tpos);
      }else{
        lcd.print(channels[this->menu_pointer].description);
      }

      byte next_index = this->get_next_channel(this->menu_pointer);
      
      if(next_index != 0){
        lcd.setCursor(11, 1);
        if(strlen(channels[next_index].description) > 5){
           lcd.print(channels[next_index].description + bpos);
        }else{
          lcd.print(channels[next_index].description);
        }
      }

      if(millis() - now > 500){
       now = millis();
       if(++tpos > strlen(channels[this->menu_pointer].description) - 5){
         tpos = 0;
       }
       if(++bpos > strlen(channels[next_index].description) - 5){
         bpos = 0;
       }
      }      
    }
  }

  void process_input(String str){
    bool success = true;
    if(str.length() > 1){
      String substr = str.substring(0,2);
      if(isAlpha(str[1]) & isUpperCase(str[1])){
        if(str[0] == 'C') {
          substr = str.substring(2, -1);
          char desc[16];
          substr.toCharArray(desc, 16);
          if(channel_exists(str[1])){
            this->update_description(str[1], desc);
            if(this->substate == DISPLAY_CHN){
              this->display_channel();
            }
          }else{
            this->create_channel(str[1], desc);
            if(this->substate == DISPLAY_CHN){
              this->display_channel();
            }
          }
        } else if(str.length() < 6) {
            substr = str.substring(2,5);
            if(substr.toInt() > 0 & substr.toInt() < 256 || substr == "0"){
              switch(str[0]){
                case 'V':
                  this->set_value(str[1], substr.toInt());
                  if(this->substate == DISPLAY_CHN){
                    this->display_channel();
                  }
                  break;
                  
                case 'N':
                  this->set_min(str[1], substr.toInt());
                  break;
                  
                case 'X':
                  this->set_max(str[1], substr.toInt());
                  break;
                  
                default:
                  Serial.println("ERROR: " + str);
                  success = false;
              }
            }
        }
      }else{
        Serial.println("ERROR: " + str);
        success = false;
      }
      if (success & this->substate == DISPLAY_SEL) {
        this->display_select();
      }else if(success){
        this->check_backlight();
      }
    }
  }

  void process_updown(int last_button){
    int b = lcd.readButtons();

    if(b == 0){
      if(last_button == 4){
        byte next_index = this->get_next_channel(this->menu_pointer);

        if(next_index != 0){
          this->menu_pointer = next_index;
          this->display_channel();
        }
      }else{
        byte before_index = this->get_before_channel();
        
        if(before_index != 255){
          this->menu_pointer = before_index;
          this->display_channel();
        }
      }
    }
  }

  void change_select(){
    this->substate = DISPLAY_SEL;
    this->lastsel = millis();    
  }

  void process_select(byte b){
    if (b == 0) {
      this->substate = DISPLAY_CHN;
      this->display_channel();
      this->check_backlight();
      this->has_shown = false;
    }else if (this->lastsel + 1000 <= millis() & !this->has_shown) {
      this->display_select();
      this->has_shown = true;
    }
  }

  void display_select(){
    lcd.clear();
    lcd.setBacklight(5);
    lcd.setCursor(0,0);
    lcd.print("F128784");
    lcd.setCursor(0,1);
    lcd.print(freeMemory());    
  }

  void check_backlight(){
    byte colour = 7;
    for(byte i = 0; i < 26; i++){
      if(exist_array[i]){
        if(channels[i].minV <= channels[i].maxV){
          if(channels[i].value < channels[i].minV){
            if(colour == 7){
              colour = 2;
            }else if(colour == 1){
              colour = 3;
              break;
            }
          }else if (channels[i].value > channels[i].maxV){
            if(colour == 7){
              colour = 1;
            }else if(colour == 2){
              colour = 3;
              break;
            }
          }
        }        
      }
    }
    lcd.setBacklight(colour);
  }
};

void setup() {
  // values for the custom characters, a - down array, b - up arrow
  byte a[] = { B00000, B00100, B00100, B00100, B00100, B10101, B01110, B00100};
  byte b[] = { B00000, B00100, B01110, B10101, B00100, B00100, B00100, B00100};
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.createChar(DOWN_ARROW, a);
  lcd.createChar(UP_ARROW, b);
  lcd.setBacklight(5);
}

void loop() {
  static admin controller;
  int b = lcd.readButtons();
  int last_b = b;

  //proc
  if(Serial.available()){
    controller.input(Serial.readString());
  }
  
  if(controller.substate == DISPLAY_CHN){
    controller.display_channel_description();
    
    if(b & (BUTTON_UP | BUTTON_DOWN)){
      controller.process_updown(last_b);
    } 
  }

  if(controller.substate == DISPLAY_SEL){
    controller.process_select(b);
  } else if(b & BUTTON_SELECT){
    controller.change_select();
  }
  
  controller.synchronisation();
}
