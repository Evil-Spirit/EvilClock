#include <Time.h>
#include <Wire.h>
#include <avr/eeprom.h>

#define DEBUG

#include <SPI.h>
#include "OneWire.h"
#include "MusicPlayer.h"
#include "Clock.h"

#define PIN_SOUND	9
#define PIN_OE		10
#define PIN_SDI		11
#define PIN_LE		12
#define PIN_CLK		13
#define PIN_LIGHT	A0
#define PIN_TEMP	A2
#define PIN_RTC_SDA     A4
#define PIN_RTC_SCL     A5


#define B3	247 * 2
#define FS4 370 * 2
#define B3	247 * 2
#define G4	392 * 2
#define E4	330 * 2
#define D4	294 * 2
#define CS4 277 * 2


#define SEGMENT_N			B00000000
#define SEGMENT_M			B10000000
#define SEGMENT_LU			B01000000
#define SEGMENT_U			B00100000
#define SEGMENT_RU			B00010000
#define SEGMENT_P			B00001000
#define SEGMENT_LB			B00000100
#define SEGMENT_B			B00000010
#define SEGMENT_RB			B00000001
#define SEGMENT_CELSIUS		        B11110000
#define SEGMENT_UNDER                   B00000010


#define TEMP_SEGMENT_M			B00000001
#define TEMP_SEGMENT_CELSIUS		B00001111
#define TEMP_SEGMENT_UNDER              B01000000
#define TEMP_SEGMENT_P                  B00010000

// notes in the melody:
short melody[] =		{ B3, FS4, B3, G4, FS4, E4, FS4, E4, FS4, G4, G4, FS4, E4, B3, FS4, B3, G4, FS4, E4, D4, E4, D4, CS4, CS4, D4, CS4, B3 };
char noteDurations[] =	{ 2,  4,  4,  2,  4,   4,  2,	4,	4,	 4,	 4,	 4,	  4,  2,  4,   4,  2,  4,	4, 2,  4,  4,  4,	4,	 4,	 4, 2 };

short beep[] = {1000};
char beep_duration[] = {4};
int temp = 200;
unsigned int storedCorrection;

byte temp_digits[17] = {
       //01234567	
        B11101110,	// 0
        B00101000,	// 1
	B11001101,	// 2
	B01101101,	// 3
	B00101011,      // 4
	B01100111,	// 5
	B11100111,	// 6
	B00101100,	// 7
	B11101111,      // 8
	B01101111,      // 9
	B11110101,
	B11000111,
	B01100011,
	B10010111,
	B11100011,
	B11100001,
	B11111111
};

byte digits[17] = {
	B01110111,	// 0
	B00010100,	// 1
	B10110011,	// 2
	B10110110,	// 3
	B11010100,	// 4
	B11100110,	// 5
	B11100111,	// 6
	B00110100,
	B11110111,
	B11110110,
	B11110101,
	B11000111,
	B01100011,
	B10010111,
	B11100011,
	B11100001,
	B11111111
};

byte letters[] = {
//	 12345678   90123456
	B10011001, B00101101, // 'a'
	B10011001, B11101001, // 'б'
	B10011001, B11101011, // 'в'
	B10010001, B00000001, // 'г'
	B00000010, B11100110, // 'д'
	B10011001, B11000001, // 'е'
	B01100110, B00010010, // 'ж'
	B10000000, B11010011, // 'з'
	B00010011, B00100110, // 'и'
	B00010011, B00100110, // 'и'
	B00010011, B00010010, // 'к'
	B00000010, B00100110, // 'л'
	B00110001, B00100110, // 'м'
	B00011001, B00101100, // 'н'
	B10010001, B11100101, // 'о'
	B10010001, B00100101, // 'п'
	B10011001, B00001101, // 'р'
	B10010001, B11000001, // 'с'
	B11000100, B00000001, // 'т'
	B00011000, B11101100, // 'у'
	B11011100, B00001101, // 'ф'
	B00100010, B00010010, // 'х'
	B00011000, B00101100, // 'ч'
	B01010101, B11100100, // 'ш'
	B00010001, B11110100, // 'ц'
	B00011101, B01100100, // 'ы'
	B11010100, B10101000, // 'ъ'
	B00011001, B11101000, // 'ь'
	B01010101, B11110100, // 'щ'
	B10000000, B11101101, // 'э'
	B01011101, B10100101, // 'ю'
	B10011010, B00101101, // 'я'
};

class Button {
	byte pin;
	word pause;
	unsigned long time;
	unsigned long start;
	boolean state;
	boolean is_down;
	boolean is_up;
public:

	Button(byte pin) {
		this->pin = pin;
		pinMode(pin,INPUT);
		digitalWrite(pin,HIGH);
		time = millis();
		start = millis();
		state = false;
		is_down = false;
		is_up = false;
		pause = 100;
	}

	boolean pressed() {
		return state;
	}

	boolean down() {
		return is_down;
	}

	void update() {
		boolean old = state;
		if (millis() - time > 20) {
			state = digitalRead(pin) == LOW;
			time = millis();
		}
		is_down = state == true && old == false;
		is_up = state == false && old == true;
		if (is_down) {
			start = time;
			pause = 1000;
		} else
			if (state && millis() - start > pause) {
				is_down = true;
				start += pause;
				pause = 250;
			}
	}

};

Button mode_btn(2);
Button hour_btn(7);
Button min_btn(6);
Button zero_btn(5);
Button plus_btn(4);
Button minus_btn(3);
MusicPlayer music_player(PIN_SOUND);
Clock clocks;

float temperature = 0.0f;

byte main_display[6];
byte main_display_old[6];
byte text_display[20];
byte temp_display[6];
byte temp_display_old[6];
byte mode = 0;

void setup() {
        //SPI.setBitOrder(MSBFIRST);
        //SPI.setClockDivider(SPI_CLOCK_DIV2);
        //SPI.setDataMode(SPI_MODE0);
        //SPI.begin(); 
    clocks.setCorrection(eeprom_read_dword(0));
    storedCorrection = clocks.getCorrection();
	Serial.begin(9600);

	pinMode(PIN_SDI, OUTPUT);
	pinMode(PIN_CLK, OUTPUT);
	pinMode(PIN_LE, OUTPUT);
	pinMode(PIN_OE, OUTPUT);
    pinMode(PIN_TEMP, INPUT);
	digitalWrite(PIN_OE, LOW);
	
	for (int i=0; i<6; i++) {
            temp_display[i] = 0xFF;
            main_display[i] = 0;            
        }
	
        for (int i=0; i<20; i++) {
		text_display[i] = 0x0;
	}
	
    main_display[0] = B11010100;
    main_display[1] = digits[0xA];
    main_display[2] = digits[0xC];
    main_display[3] = digits[0xB];
    main_display[4] = B01000001;

	displayUpdate();
	delay(1000);
	/*
	// startup effect
	for (int i = 255; i>=0; i--) {
		analogWrite(PIN_OE, i);
		delay(2);
	}
	delay(500);
	for (int i = 0; i<=255; i++) {
		analogWrite(PIN_OE, i);
		delay(2);
	}
	delay(500);
    */
	music_player.play(melody, noteDurations, 27);
   /*
        if(0) {
            #define TIME(a) (__TIME__[a] - 48)
            byte h = TIME(0) * 10 + TIME(1);
            byte m = TIME(3) * 10 + TIME(4);
            byte s = TIME(6) * 10 + TIME(7);
            #undef TIME
            clock.setHours(h);
            clock.setMinutes(m);
            clock.setSeconds(s + 15);
        }
    */
}

void displayUpdate()
{
	//for(int i = 0; i<20; i++) {
	//	shiftOut(PIN_SDI, PIN_CLK, MSBFIRST, text_display[i]);
	//}
        // remap non-working segment
        main_display[5] = (main_display[1] & B00000010) ? 0xFF : 0x00;
        
        // temp display
        byte need_update = 0;
        for(int i=0; i<6; i++) {
            if(temp_display_old[i] != temp_display[i]) need_update = 1;
            temp_display_old[i] = temp_display[i];
        }
        
        // main display
        if(need_update == 0) {
            for(int i=0; i<6; i++) {
                if(main_display_old[i] != main_display[i]) need_update = 1;
                main_display_old[i] = main_display[i];
            }
        }
        
        if(need_update) {        
            for(int i=0; i<6; i++) {
    	        shiftOut(PIN_SDI, PIN_CLK, MSBFIRST, temp_display[5 - i]);
                //SPI.transfer(temp_display[5 - i]);
            }
            
            for(int i=0; i<6; i++) {
    	        shiftOut(PIN_SDI, PIN_CLK, MSBFIRST, main_display[5 - i]);
                //SPI.transfer(main_display[5 - i]);
            }
            digitalWrite(PIN_LE, HIGH);
            delayMicroseconds(1);
            digitalWrite(PIN_LE, LOW);
       }
}

OneWire ds(PIN_TEMP);
long ds_request = 0;
byte addr[8];
long found = 0;
long raw_temperature = 0;
byte error = 1;
byte first = 0;

void updateTempSensor() {
	
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	
	if(ds_request == 0) {
                if(!found) { 
        		if(!ds.search(addr)) {
        			ds.reset_search();
                                temperature = 0.0f;
        			//delay(250);
        			return;
        		} else {
                            found = 1;
                            first = 1;
                        }
                }
		ds.reset();
		ds.select(addr);
		ds.write(0x44, 1); // start conversion, with parasite power on at the end
		ds_request = millis();
                //delay(1000);
		return;
	}

	if (millis() - ds_request < 1000) return;
	ds_request = 0;
	
	// we might do a ds.depower() here, but the reset will take care of it.
	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE); // Read Scratchpad
	
	ds.read_bytes(data, 9);

	raw_temperature = ((data[1] << 8) | data[0]);
        error = ds.crc8(data, 8) != data[8];
        if(error) return;
        
        //if(raw_temperature == -1) return;
        if(raw_temperature < -1200) return;
        if(raw_temperature > +1200) return;
        
 	float new_temperature = float(raw_temperature) / 16.0f;
        if(first) {
            temperature = new_temperature;
            first = 0;
        } else {
            temperature = temperature * 0.7f + new_temperature * 0.3f;
        }
}

void displayTime(const Clock &clock) {
	main_display[0] = digits[clock.getHours() / 10];
	main_display[1] = digits[clock.getHours() % 10];
	main_display[2] = (clock.isTire()) ? SEGMENT_M : SEGMENT_N;
	main_display[3] = digits[clock.getMinutes() / 10];
	main_display[4] = digits[clock.getMinutes() % 10];
}

void displayTimeMinSec(const Clock &clock) {
	main_display[0] = digits[clock.getMinutes() / 10];
	main_display[1] = digits[clock.getMinutes() % 10];
	main_display[2] = (clock.isTire()) ? SEGMENT_M : SEGMENT_N;
	main_display[3] = digits[clock.getSeconds() / 10];
	main_display[4] = digits[clock.getSeconds() % 10];
}

void tempDisplayClear() {
	for (int i=0; i<6; i++) {
		temp_display[i] = SEGMENT_N;
	}
}
void displayClear() {
	for (int i=0; i<6; i++) {
		main_display[i] = SEGMENT_N;
	}
}

void displayFloat(float value) {
	char string[128];
	displayClear();
	dtostrf(value, 2, 2, string);
	byte di = 0;
	for (int i=0; i<128; i++) {
		if (string[i] == '\0') break;
		if (string[i] == '-') {
			main_display[di] = SEGMENT_M;
			di++;
		}
		else if (string[i] == '.' && i > 0) {
			main_display[di - 1] |= SEGMENT_P;
		}
		else if (string[i] >= '0' && string[i] <= '9') {
			main_display[di] = digits[string[i] - '0'];
			di++;
		}
		if (di > 4) break;
	}
}

void displayInt(long value) {
	char string[128];
	displayClear();
	itoa(value, string, 10);
	byte di = 0;
	for (int i=0; i<128; i++) {
		if (string[i] == '\0') break;
		if (string[i] == '-') {
			main_display[di] = SEGMENT_M;
			di++;
		}
		else if (string[i] == '.' && i > 0) {
			main_display[di - 1] |= SEGMENT_P;
		}
		else if (string[i] >= '0' && string[i] <= '9') {
			main_display[di] = digits[string[i] - '0'];
			di++;
		}
		if (di > 4) break;
	}
}

void displayTemperature(float value) {
	char string[128];
	tempDisplayClear();
	dtostrf(value, 2, 1, string);
	byte di = 0;
	for (int i=0; i<128; i++) {
		if (string[i] == '\0') break;
		if (string[i] == '-') {
			temp_display[di] = TEMP_SEGMENT_M;
			di++;
		}
		else if (string[i] == '.' && i > 0) {
			temp_display[di - 1] |= TEMP_SEGMENT_P;
		}
		else if (string[i] >= '0' && string[i] <= '9') {
			temp_display[di] = temp_digits[string[i] - '0'];
			di++;
		}
		if (di > 3) break;
	}
	temp_display[di] = TEMP_SEGMENT_CELSIUS | ((error) ? TEMP_SEGMENT_UNDER : 0);
}

//char text[] = {'п','р','и','в','е','т','\0'};
char text[] = {'а','б','в','г','д','e','\0'};
void displayText(const char *txt) {
	int len = strlen(text);
	//displayInt(len);
	for (int i=0; i<len; i++) {
		byte index = 0;
		displayInt(text[i]);
		if (byte(text[i]) >= byte('р') && byte(text[i]) <= byte('я')) index = byte(text[i]) - byte('р') + 16;
		if (byte(text[i]) >= byte('а') && byte(text[i]) <= byte('п')) index = byte(text[i]) - byte('а');

		text_display[i * 2] = letters[index * 2];
		text_display[i * 2 + 1] = letters[index * 2 + 1];
	}
}

long lighting_accum = 0;
long lighting_num = 0;
long smooth_light = 0;
long clock_per_sec = 0;
long clock_per_sec_accum = 0;
long clock_start = millis();

void loop() {
        long now_millis = millis();
        clock_per_sec_accum++;
	if(now_millis - clock_start >= 1000) {
            clock_start = now_millis;
            clock_per_sec = clock_per_sec_accum;
            clock_per_sec_accum = 0;
        }
        
	clocks.update();
	// play sound every hour
	if(clocks.isHourSound()) {
		music_player.play(beep, beep_duration, 1);
	}
	music_player.update();
	
	// display brightness
	long curr_lighting = 1023 - analogRead(PIN_LIGHT);
	lighting_accum += curr_lighting;
	lighting_num += 1;
	if(lighting_num > 10) {
		smooth_light = lighting_accum / lighting_num;
		lighting_accum = 0;
		lighting_num = 0;
	}
	
	long brightness = 5 + long(smooth_light / 1.6f);
	if(brightness > 255) brightness = 255;
	
	//if(!music_player.isPlaying()) {
	//  analogWrite(PIN_OE, 255 - brightness);
	//} else {
	  digitalWrite(PIN_OE, 0);
	//}
	
	// buttons
	min_btn.update();
	hour_btn.update();
	zero_btn.update();
	plus_btn.update();
	//minus_btn.update();
        mode_btn.update();
	
	// min button
	if (min_btn.down()) {
        switch(mode) {
		    case 0: clocks.setMinutes(clocks.getMinutes() + 1); break;
            case 2: clocks.setCorrection(clocks.getCorrection() + 1); break;
        }
		//start = millis();
	}
	
	// hour button
	if (hour_btn.down()) {
        switch(mode) {
            case 0: clocks.setHours(clocks.getHours() + 1); break;
            case 2: clocks.setCorrection(clocks.getCorrection() - 1); break;
        }
		//start = millis();
	}
	
	// zero clock button
	if (zero_btn.pressed()) {
		clocks.setSeconds(0);
		//start = millis();
	}
	
	// plus button
	//if (plus_btn.down()) {
	//}
	
	// minus button
	//if (minus_btn.down()) {
	//}
    
        if(mode_btn.down()) {
            mode = (mode + 1) % 3;
            if(mode == 0 && storedCorrection != clocks.getCorrection()) {
                   storedCorrection = clocks.getCorrection();
                   eeprom_write_dword(0, storedCorrection);
            }
        }
	
	//byte is_temp = ((clock.getSeconds() / 3) % 2) == 1 && hour_btn.pressed() == false && min_btn.pressed() == false;
	
	
	//if (is_temp) {
		updateTempSensor();
		displayTemperature(temperature);
                //displayInt(raw_temperature);
	//} else {
                if(mode == 0) {
		    displayTime(clocks);
                } else 
                if(mode == 1) {
                    displayTimeMinSec(clocks);
                } else 
                if(mode == 2) {
                    displayInt(clocks.getCorrection());
                }
	//}
	
	displayUpdate();
        delay(10);
	
}
