#include <DS1307RTC.h>

class Clock {
	
	byte seconds;
	byte minutes;
	byte hours;
	byte is_tire;
	byte hour_sound;
        byte use_rtc;
	unsigned long start;
        unsigned long correction;
        //RTC_DS1307 *rtc;
	
	void updateInternal() {
		is_tire = (micros() - start) / 500000 % 2 == 0;
		
		// seconds
		if ((unsigned long)(micros() - start) >= 1000000 + correction) {
			seconds++;
			start += 1000000 + correction;
		}
		
		// minutes
		if (seconds >= 60) {
			seconds -= 60;
			minutes++;
                        //updateRTC();
		}
		
		// hours
		if (minutes >= 60) {
			minutes -= 60;
			hours++;
			hour_sound = 1;
		}
		
		// days
		if (hours >= 24) {
			hours -= 24;
		}
		
	}

	void updateRTC() {
            //if(!use_rtc) return;
		
            tmElements_t tm;
              if (RTC.read(tm)) {
                  if(tm.Second != seconds) {
                      start = micros();
                  }
                  is_tire = micros() - start < 500000;
                  seconds = tm.Second;
                  minutes = tm.Minute;
                  if(hours != tm.Hour) hour_sound = 1;
                  hours = tm.Hour;
              } else {
                if (RTC.chipPresent()) {

                  Serial.println("The DS1307 is stopped.  Please run the SetTime");
                  Serial.println("example to initialize the time and begin running.");
                  Serial.println();
                } else {
                  Serial.println("DS1307 read error!  Please check the circuitry.");
                  Serial.println();
                }
              }
	}
	
	void initRTC() {
            if(!use_rtc) return;
	}

        void setTime() {
            //if(use_rtc) {
                tmElements_t tm;
                tm.Second = seconds;
                tm.Minute = minutes;
                tm.Hour = hours;
                RTC.write(tm);
            //} else {
                start = micros();
            //}
        }
        
public:
	
	Clock(byte rtc = 0) {
		seconds = 0;
		minutes = 0;
		hours = 0;
		is_tire = 0;
		start = micros();
                use_rtc = rtc;
                correction = 600 + 24 + 22 + 6 - 20;
	        initRTC();
	}
	
	void update() {
                if(use_rtc) {
		   updateRTC();
                   use_rtc = 0;
                } else {
                    updateInternal();
                }
	}
	
	byte getSeconds() const {
		return seconds;
	}
	
	void setSeconds(byte sec) {
		seconds = (sec + 60) % 60;
                setTime();
	}
	
	byte getMinutes() const {
		return minutes;
	}
	
	void setMinutes(byte min) {
		minutes = (min + 60) % 60;
                setTime();
	}
	
	byte getHours() const {
		return hours;
	}
	
	void setHours(byte hour) {
		hours = (hour + 24) % 24;
                setTime();
	}
	
	byte isHourSound() {
		byte ret = hour_sound;
		hour_sound = 0;
		return ret;
	}
	
	byte isTire() const {
		return is_tire;
	}
	
};
