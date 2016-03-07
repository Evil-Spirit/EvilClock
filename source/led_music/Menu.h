#define MENU_DISABLED    0
#define MENU_CHOOSE      1
#define MENU_MODIFY      2

#define MENU_ITEM_EDITABLE    0
#define MENU_ITEM_READONLY    1
#define MENU_ITEM_ENUM        2

byte fromChar(char c) {
    switch(c) {
    			//01234567
    	case ' ': return B00000000;
    	case 'O':
    	case '0': return B01110111;
    	case '1': return B00010100;
    	case '2': return B10110011;
    	case '3': return B10110110;
    	case '4': return B11010100;
        case 'S': 
    	case '5': return B11100110;
    	case '6': return B11100111;
    	case '7': return B00110100;
    	case '8': return B11110111;
    	case '9': return B11110110;
    	case 'A': return B11110101;
    	case 'b': return B11000111;
    	case 'C': return B01100011;
    	case 'c': return B10000111;
    	case 'd': return B10010111;
    	case 'E': return B11100011;
    	case 'F': return B11100001;
    	case 'g': return B11110011;
    	case 'h': return B11000101;
    	case 'H': return B11010101;
    	case 'o': return B10000111;
    	case 'r': return B10000100;
    	case 'u': return B00000111;
    	case 'I':
    	case 'l': return B01000100;
        case 'i': return B00000100;
        case 't': return B11000110;
        case 'U': return B01010111;
        case 'P': return B11110100;
        case 'j': return B00010011;
        case '-': return B10000000;
    };
    return B01010101;
}

#define MESSAGE_NULL         0
#define MESSAGE_QUIT         1
#define MESSAGE_CHASI        2
#define MESSAGE_BRIGHT       3
#define MESSAGE_ON           4
#define MESSAGE_OFF          5
#define MESSAGE_SETUP        6
#define MESSAGE_ADJUST       7
#define MESSAGE_CORRECT      8

byte messages[] = {
    fromChar('n'), fromChar('u'), fromChar('l'), fromChar('l'), 0,
    fromChar('q'), fromChar('u'), fromChar('i'), fromChar('t'), 0,
    fromChar('4'), fromChar('A'), fromChar('C'), fromChar('b'), fromChar('I'),
    fromChar('b'), fromChar('r'), fromChar('i'), fromChar('h'), fromChar('t'),
    fromChar('O'), fromChar('n'), 0, 0, 0,
    fromChar('O'), fromChar('F'), fromChar('F'), 0, 0,
    fromChar('S'), fromChar('E'), fromChar('t'), fromChar('U'), fromChar('P'),
    fromChar('A'), fromChar('d'), fromChar('j'), fromChar('S'), fromChar('t'),
    fromChar('C'), fromChar('r'), fromChar('E'), fromChar('c'), fromChar('t'),
};

class MenuItem {
public:
    byte message;
    byte type;
    long min;
    long max;
    long delta;
    long *value;
    
    MenuItem() {
        type = 0;
        message= 0;
        delta = 1;
        min = 0;
        max = 10;
        value = NULL;
    }
    MenuItem(byte message, byte type, long min, long max, long delta, long *value) {
        this->type = type;
        this->message = message;
        this->min = min;
        this->max = max;
        this->delta = delta;
        this->value = value;
    }
};


class Menu {
    
    MenuItem *items;
    byte num_items;
    byte cur_item;
    byte mode;
    byte display[5];
    unsigned long last_activity;
    
    void updateActivity() {
        last_activity = millis();
    }
    
    void checkActivity() {
        if((unsigned long)(millis() - last_activity) > 10000) {
            setMode(MENU_DISABLED);
        }
    }
    
public:
    
    Menu(MenuItem *items, byte num_items) {
        this->items = items;
        this->num_items = num_items;
        cur_item = 0;
        mode = MENU_DISABLED;
        last_activity = millis();
    }
    
    
    void setMode(byte mode) {
        this->mode = mode;
    }
    
    void change(int delta) {
        updateActivity();
        switch(mode) {
            case MENU_DISABLED: 
                break;
            case MENU_CHOOSE:
                cur_item = (cur_item + num_items + 1 + delta) % (num_items + 1);
                break;
            case MENU_MODIFY:
                if(items[cur_item].type == MENU_ITEM_READONLY) break;
                if(items[cur_item].value == NULL) break;
                long *value = items[cur_item].value;
                if(items[cur_item].type != MENU_ITEM_ENUM) delta *= items[cur_item].delta;
                *value += delta;
                if(items[cur_item].type == MENU_ITEM_ENUM) {
                    long min = items[cur_item].min;
                    long max = items[cur_item].max;
                    long len = max - min + 1;
                    *value = (*value - min + len) % len + min;
                } else {
                    if(*value > items[cur_item].max) *value = items[cur_item].max;
                    if(*value < items[cur_item].min) *value = items[cur_item].min;
                }
                break;
        }
    }
    
    void up() {
        change(+1);
    }
    
    void down() {
        change(-1);
    }
    
    void use() {
        updateActivity();
        switch(mode) {
            case MENU_DISABLED:
                cur_item = 0;
                mode = MENU_CHOOSE; 
                break;
            case MENU_CHOOSE:
                if(cur_item == num_items) {
                    mode = MENU_DISABLED;
                    break;
                }
                mode = MENU_MODIFY;
                break;
            case MENU_MODIFY:
                mode = MENU_DISABLED;
                break;
        }
    }
    
    byte *getDisplay() {
        
        if(mode == MENU_CHOOSE) {
            
            // exit message by default
            byte message = 1;
            
            if(cur_item < num_items) message = items[cur_item].message;
            
            return &messages[message * 5];              
        }
        
        if(mode == MENU_MODIFY) {
            int cur_letter = 0;
            if(items[cur_item].value == NULL) return &messages[0];
            long value = *items[cur_item].value;
            if(items[cur_item].type == MENU_ITEM_ENUM) {
                byte message = items[cur_item].delta + value - items[cur_item].min;
                return &messages[message * 5];
            }
            
            
            for(int i=0; i<5; i++) {
                display[i] = 0;
            }
            
            byte neg = 0;
            if(value < 0) {
                neg = 1;
                value = -value;
            }
            
            cur_letter = 4;
            long div = 1;
            
            while(cur_letter >= 0) {
                long dig = (value / div) % 10;
                display[cur_letter--] = fromChar(dig + '0');
                value -= dig * div;
                if(value == 0) break;
                div *= 10;
            }
            if(cur_letter >= 0 && neg) display[cur_letter] = fromChar('-');
            return display;
        }
        return &messages[0];
    }
    
    byte isEnabled() {
        return mode != MENU_DISABLED;
    }
    
    void update() {
        switch(mode) {
            case MENU_DISABLED:
                break;
            case MENU_CHOOSE:
                checkActivity();
                break;
            case MENU_MODIFY:
                checkActivity();
                break;
        }
    }
    
    byte getCurrentItem() {
        return cur_item;
    }
    
    byte getMode() {
        return mode;
    }
    
};
