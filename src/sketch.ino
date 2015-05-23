#include <SoftwareSerial.h>
#include <EEPROMex.h>
#define PS2_CLOCK 3
#define PS2_DATA 10
#define PIN_SEGMENT_TX 2
#define PIN_MIDI_TX 8
#define PIN_BUTTON 6
#define PIN_BUTTON2 7
#define NULL_BYTE 0x00
#define LED1 11
#define LED2 12
#define LED3 13

#define MAX_DOWN_BUTTONS 10

#define PROGRAM 0
#define PERFORM 1
#define UP 0
#define DOWN 1



SoftwareSerial segment_display_serial(100000, PIN_SEGMENT_TX);
SoftwareSerial midi_serial(10000000, PIN_MIDI_TX);
int last_set_value = 0;
int selected_value = 0;
int mappings[200] = {0};


byte down_buttons[MAX_DOWN_BUTTONS] = {
  NULL_BYTE, NULL_BYTE, NULL_BYTE,
  NULL_BYTE, NULL_BYTE, NULL_BYTE,
  NULL_BYTE, NULL_BYTE, NULL_BYTE,
  NULL_BYTE
};
int mode = PROGRAM;
unsigned int key_state = UP;
byte key_value = 0xF0;



void setup() {
  Serial.begin(9600);      // Console
  midi_serial.begin(31250); // Midi Baud
  segment_display_serial.begin(9600); // Segment Baud

  // PS2 Related
  pinMode(PS2_CLOCK, INPUT);
  pinMode(PS2_DATA, INPUT);
  digitalWrite(PS2_CLOCK, HIGH);
  digitalWrite(PS2_DATA, HIGH);


  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_BUTTON2, INPUT);

  delayMicroseconds(50);


  load_mappings();

  // 0 - 255
  set_brightness(250);  // Lowest brightness
  print_text("ndi");
  set_color(255,255,255);
  delay(1000);
  set_mode(PERFORM);
}









void loop() {

  // Read key and up and down vlaue
  key_value = read_key();

  if (key_value== 0xF0) {
    key_value = read_key();
    key_state = UP;


  } else {
    key_state = DOWN;
  }

  // Switch modes
  if (key_state == UP && key_value == 118) {
    toggle_mode();
    return;
  } else if (key_value == 118) {
    return;
  }


  if (mode == PERFORM) {
    handle_perform_mode();
    return;
  }

  if (mode == PROGRAM) {
    handle_program_mode();
    return;
  }
}



// Data
void save_mappings() {
  set_color(255,255,0);
  print_text("saV");
  EEPROM.writeBlock(0, mappings);
  set_color(0,255,255);
  print_text("done");
}
void load_mappings() {
  EEPROM.readBlock(0, mappings);
}
void clear_down_buttons() {
  for (int i = 0; i < MAX_DOWN_BUTTONS; i++) {
    down_buttons[i] = NULL_BYTE;
  }
}







void handle_program_mode() {
  if (key_state == UP) {

    if ((int) key_value == 114 && mappings[selected_value] > 0) {
      // down
      mappings[selected_value]--;
      last_set_value = mappings[selected_value];
    } else if ((int) key_value == 117 && mappings[selected_value] < 200) {
      // up
      mappings[selected_value]++;
      last_set_value = mappings[selected_value];
    } else if ((int) key_value == 93) {
      // recall last set value
      mappings[selected_value] = last_set_value;
    } else if ((int) key_value == 113) {
      save_mappings();
      return;
    } else {
      selected_value = key_value;
    }

    // display and trigger
    trigger_pad(mappings[selected_value], true);
    trigger_pad(mappings[selected_value], false);
    print_text((String) mappings[selected_value]);
  }
}
void sync_dots() {
  int dots = 0;
  for (int i = 0; i < MAX_DOWN_BUTTONS; i++) {
    if (down_buttons[i] != NULL_BYTE) {
      dots++;
    }
  }
  print_dots(dots);
}


void handle_perform_mode() {

  if (key_state == DOWN) {

    // return if key is already down
    for (int i = 0; i < MAX_DOWN_BUTTONS; i++) {
      if (down_buttons[i] == key_value) { return; }
    }

    // key's not down, trigger and add to down buttons array
    trigger_pad(mappings[key_value], true);
    for (int i = 0; i < MAX_DOWN_BUTTONS; i++) {
      if (down_buttons[i] == NULL_BYTE) {
        down_buttons[i] = key_value;
        break;
      }
    }
  } else {
    // trigger and unset from down buttons array
    trigger_pad(mappings[key_value], false);
    for (int i = 0; i < MAX_DOWN_BUTTONS; i++) {
      if (down_buttons[i] == key_value) {
        down_buttons[i] = NULL_BYTE;
        break;
      }
    }
  }

  sync_dots();
}

void set_mode(int md) {
  if (md == PROGRAM) {
    mode = PROGRAM;
    set_color(255,0,0);
    print_text("prg");
  } else {
    mode = PERFORM;
    clear_down_buttons();
    set_color(0,255,0);
    print_text("prf");
  }
}

void toggle_mode() {
  if (mode == PERFORM) {
    set_mode(PROGRAM);
  } else {
    set_mode(PERFORM);
  }
}



void trigger_pad(int midi_note, boolean on) {
  noteOn(0x90, midi_note, on ? 127 : 0);
  print_text((String) midi_note);
}

void print_text(String text) {
  segment_display_serial.write(0x76);  // Clear display command
  segment_display_serial.print(text);
}
void print_dots(int number) {
  set_decimals(0b000000);

  if (number <= 0) {
    set_decimals(0b000000);
  } else if (number == 1) {
    set_decimals(0b000001);
  } else if (number == 2) {
    set_decimals(0b000011);
  } else if (number == 3) {
    set_decimals(0b000111);
  } else if (number == 4) {
    set_decimals(0b001111);
  } else if (number == 5) {
    set_decimals(0b011111);
  } else if (number >= 6) {
    set_decimals(0b111111);
  }
}



void set_brightness(byte value) {
  // 0 - 127
  segment_display_serial.write(0x7A);  // Set brightness command byte
  segment_display_serial.write(value);  // brightness data byte
}

// Turn on any, none, or all of the decimals.
//  The six lowest bits in the decimals parameter sets a decimal 
//  (or colon, or apostrophe) on or off. A 1 indicates on, 0 off.
//  [MSB] (X)(X)(Apos)(Colon)(Digit 4)(Digit 3)(Digit2)(Digit1)
void set_decimals(byte decimals) {
  segment_display_serial.write(0x77);
  segment_display_serial.write(decimals);
}

// Send a MIDI note-on/off message. 
void noteOn(char cmd, char data1, char data2) {
  midi_serial.write(cmd);
  midi_serial.write(data1);
  midi_serial.write(data2);
}

// ps2
byte read_key() {
  byte _start = 0;
  byte buf = 0;
  byte _parity = 0;
  byte _stop = 0;
  wait_clock_low();
  _start = digitalRead(PS2_DATA);

  if (_start == 0) {
    wait_clock_low();
    for (byte c=0; c<8; c++) {
      buf = buf | (digitalRead(PS2_DATA) << c);
      wait_clock_low();
    }
    _parity = digitalRead(PS2_DATA);
    wait_clock_low();
    _stop = digitalRead(PS2_DATA);

  }

  return buf;
}

void wait_clock_low() {
  if (digitalRead(PS2_CLOCK) == LOW) {
    while (digitalRead(PS2_CLOCK) == LOW) {}
  }
  while (digitalRead(PS2_CLOCK) == HIGH) {}
}



void set_color(int red, int green, int blue) {
  analogWrite(LED1, red);
  analogWrite(LED2, green);
  analogWrite(LED3, blue);
}
