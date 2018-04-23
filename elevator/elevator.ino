/*
see MFRC522 library examples for pin layout
*/

#include <EEPROM.h>
#include <MFRC522Helper.h>

#define RST_PIN 9
#define SS_PIN 10
#define AUTHORIZE_PIN 7
#define BEEP_PIN 8
#define CARD_TYPE_INVALID 0
#define CARD_TYPE_UNIT 1
#define CARD_TYPE_COMMAND 2
#define CMD_UPDATE 1
#define CMD_ERASE_AND_UPDATE 2
#define CMD_SET_NO_CONTROL_MODE 3
#define CMD_CLEAR_NO_CONTROL_MODE 4
#define CMD_BEEP_FREQ_INC 5
#define CMD_BEEP_FREQ_DEC 6
#define CMD_TOGGLE 7
#define CARDS_ARRAY_BYTES 512
#define CARDS_ARRAY_NO_OF_CARDS (CARDS_ARRAY_BYTES * 8)
#define EEPROM_ADDR_CARDS_ARRAY 0
#define BEEP_FREQ_DEFAULT 2000
#define BEEP_FREQ_MIN 1000
#define BEEP_FREQ_MAX 2500

struct Card {
    byte type;
    int no;
};


const int EEPROM_ADDR_OTHER_DATA = EEPROM_ADDR_CARDS_ARRAY + CARDS_ARRAY_BYTES;
const int EEPROM_ADDR_NO_CONTROL_MODE = EEPROM_ADDR_OTHER_DATA + 0;
const int EEPROM_ADDR_BEEP_FREQ = EEPROM_ADDR_OTHER_DATA + 1;
byte cards_array[CARDS_ARRAY_BYTES];
byte last_auth_block = 0;
MFRC522Helper card_helper;
byte auth_key_val[] = {90, 206, 26, 227, 18, 6};
byte magic_num[] = {101, 214, 190, 66};
bool no_control_mode = false;
int beep_freq = BEEP_FREQ_DEFAULT;


void setup() {
    pinMode(AUTHORIZE_PIN, OUTPUT);
    digitalWrite(AUTHORIZE_PIN, HIGH);
    Serial.begin(115200);
    while (!Serial);
    load_config();
    SPI.begin();
    card_helper.mfrc.PCD_Init();
    memcpy(card_helper.mfrc_key.keyByte, auth_key_val, 6);
    load_cards_array_from_eeprom();
    // dump_cards_array();
    Serial.println(F("Ready"));
}


void load_config() {
    no_control_mode = (bool)EEPROM.read(EEPROM_ADDR_NO_CONTROL_MODE);
    beep_freq = (byte)EEPROM.read(EEPROM_ADDR_BEEP_FREQ) * 10;
    if (beep_freq < BEEP_FREQ_MIN || beep_freq > BEEP_FREQ_MAX) {
        beep_freq = BEEP_FREQ_DEFAULT;
    }
}


void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}


void dump_cards_array() {
    char s[50];
    for (byte i = 0; i < CARDS_ARRAY_NO_OF_CARDS; i++) {
        sprintf(s, (char*)F("%d: %d"), i, read_state_of_card_from_ram(i));
        Serial.println(s);
    }
}


byte get_checksum(byte* data, byte size) {
    byte c = 0;
    for(byte i = 0; i < size; i++) {
        c ^= (data[i] + i) & 0xff;
    }
    return c & 0xff;
}


void play_and_wait(word freq, word duration, word wait_after) {
    tone(BEEP_PIN, freq, duration);
    delay(duration);
    noTone(BEEP_PIN);
    if (wait_after > 0) {
        delay(wait_after);
    }
}


void play_success_beep() {
    play_and_wait(beep_freq, 100, 0);
}


void play_failure_beep() {
    play_and_wait(beep_freq, 200, 200);
    play_and_wait(beep_freq, 200, 200);
    play_and_wait(beep_freq, 200, 0);
}


void play_toggle_off_beep() {
    play_and_wait(beep_freq, 50, 50);
    play_and_wait(beep_freq, 50, 0);
}


void change_beep_freq(int dir) {
    int new_freq = beep_freq + 10 * dir;
    Serial.print(beep_freq);
    Serial.print(" ");
    Serial.println(new_freq);
    if (new_freq >= BEEP_FREQ_MIN && new_freq <= BEEP_FREQ_MAX) {
        beep_freq = new_freq;
        EEPROM.update(EEPROM_ADDR_BEEP_FREQ, beep_freq / 10);
    }
    play_success_beep();
}


void erase_eeprom() {
    int n = EEPROM.length();
    for (int i = 0; i < n; i++) {
        EEPROM.write(i, 0);
    }
}


/*
return corresponding bit of cards array from ram for this cord no
*/
bool read_state_of_card_from_ram(int card_no) {
    int byte_index = card_no / 8;
    byte bit_index = card_no % 8;
    byte mask = 1 << bit_index;
    return (bool)(cards_array[byte_index] & mask);
}


void write_byte_to_ram_and_eeprom(int index, byte new_val) {
    byte old_val = cards_array[index];
    if(new_val == old_val)
        return;
    cards_array[index] = new_val;
    EEPROM.update(EEPROM_ADDR_CARDS_ARRAY + index, new_val);
}


void load_cards_array_from_eeprom() {
    for(int i = 0; i < CARDS_ARRAY_BYTES; i++) {
        cards_array[i] = EEPROM.read(EEPROM_ADDR_CARDS_ARRAY + i);
    }
}


void lock_elevator() {
    digitalWrite(AUTHORIZE_PIN, HIGH);
}


void unlock_elevator() {
    digitalWrite(AUTHORIZE_PIN, LOW);
}


/*
enable elevator key pad for 30 seconds
*/
void authorize() {
    Serial.println(F("authorized!"));
    unlock_elevator();
    delay(10000);
    lock_elevator();
}


void decrypt_data(byte* data, byte data_size, byte* res) {
    byte first_byte = data[0];
    byte h_byte, l_byte, h_nib, l_nib, val;
    bool use_low_nibble = (first_byte % 2 == 0);
    for (int j = 0, i = 1; i < data_size; j++, i += 2) {
        h_byte = data[i] ^ first_byte;
        l_byte = data[i + 1] ^ first_byte;
        if (use_low_nibble) {
            l_nib = l_byte & 0x0f;
            h_nib = h_byte & 0x0f;
        }
        else {
            l_nib = l_byte >> 4;
            h_nib = h_byte >> 4;
        }
        val = (h_nib << 4) | l_nib;
        res[j] = val;
    }
}

/*
read 16 bytes initial card data and check if it's correctly encrypted or no
the return value is a Card struct, for invalid encryption return {CARD_TYPE_INVALID, 0}
*/
Card read_first_block_data(byte* encrypted_data) {
    byte data[16];
    byte type;
    int no;
    decrypt_data(encrypted_data, 15, data);
    dump_byte_array(data, 16);
    Serial.println();
    if (is_valid_card(data)) {
        type = data[0];
        no = (((word)data[1]) << 8) | ((word)data[2]);
    }
    else {
        type = CARD_TYPE_INVALID;
        no = 0;
    }
    Card card = {type, no};
    return card;
}


bool is_valid_card(byte* data) {
    byte type = data[0];
    byte* magic = &(data[3]);
    if (magic[0] != magic_num[0] || magic[1] != magic_num[1] || magic[2] != magic_num[2] || magic[3] != magic_num[3])
        return false;
    if (type != CARD_TYPE_UNIT && type != CARD_TYPE_COMMAND)
        return false;
    return true;
}


void set_no_control_mode(bool val) {
    no_control_mode = val;
    EEPROM.update(EEPROM_ADDR_NO_CONTROL_MODE, (byte)val);
    if (no_control_mode) {
        unlock_elevator();
    }
    else {
        lock_elevator();
    }
    play_success_beep();
}


void process_unit_card(Card card) {
    if (no_control_mode) {
        Serial.println(F("no control mode"));
        return;
    }
    bool state = read_state_of_card_from_ram(card.no);
    if (state == true) {
        Serial.println(F("play_success_beep"));
        play_success_beep();
        authorize();
    }
    else {
        Serial.println(F("play_failure_beep"));
        play_failure_beep();
    }
}

/*
data starts from block 4 and later
  on each block there are utmost 5 operative data.
  5 * 3 bytes = 15 bytes, and last byte of block is checksum.
in each 3 bytes:
  two bytes to represent index (index of cards array)
  and next byte is the value to write on that index.
bit 15 of index (bit 7 of high byte):
  1 means there is data to write
  0 means no data in these 3 bytes and not any more data
byte 15:
  is checksum of all previous bytes (0 to 14)
*/
bool process_update_card(bool toggle) {
    byte block = 4;
    byte data[18];
    byte i, high, low, value, chk;
    int index;
    bool is_more_data = true;
    bool is_toggle_off = false;
    char str[100];
    while (block < 63 && is_more_data) {
        if (!card_helper.read_block_from_card(block, data))
            return false;
        chk = get_checksum(data, 15);
        if (data[15] != chk) {
            Serial.println(F("invalid checksum"));
            return false;
        }
        for (i = 0; i <= 12 ; i += 3) {
            high = data[i];
            if (!(high & 0x80)) {
                is_more_data = false;
                break;
            }
            low = data[i + 1];
            value = data[i + 2];
            index = ((high & 0x7f) << 8) | low;
            if (toggle && cards_array[index] != 0) {
                value = 0;
                is_toggle_off = true;
            }
            write_byte_to_ram_and_eeprom(index, value);
            sprintf(str, (char*)F("i=%02d  high=%02x  low=%02x  index=%02x  value=%02x"), i, high, low, index, value);
            Serial.println(str);
        }
        if(!is_more_data)
            break;
        block += 1;
        if (block % 4 == 3)
            block += 1;
    }
    is_toggle_off ? play_toggle_off_beep() : play_success_beep();
}


void process_new_card(byte* data) {
    Card card = read_first_block_data(data);
    char s[50];
    sprintf(s, "card info: type=%d no=%d", card.type, card.no);
    Serial.println(s);
    if (card.type == CARD_TYPE_INVALID) {
        Serial.println(F("invalid card"));
    }
    else if (card.type == CARD_TYPE_UNIT) {
        process_unit_card(card);
    }
    else if (card.type == CARD_TYPE_COMMAND && card.no == CMD_UPDATE) {
        process_update_card(false);
    }
    else if (card.type == CARD_TYPE_COMMAND && card.no == CMD_ERASE_AND_UPDATE) {
        erase_eeprom();
        process_update_card(false);
    }
    else if (card.type == CARD_TYPE_COMMAND && card.no == CMD_TOGGLE) {
        process_update_card(true);
    }
    else if (card.type == CARD_TYPE_COMMAND && card.no == CMD_SET_NO_CONTROL_MODE) {
        set_no_control_mode(true);
    }
    else if (card.type == CARD_TYPE_COMMAND && card.no == CMD_CLEAR_NO_CONTROL_MODE) {
        set_no_control_mode(false);
    }
    else if (card.type == CARD_TYPE_COMMAND && card.no == CMD_BEEP_FREQ_DEC) {
        change_beep_freq(-1);
    }
    else if (card.type == CARD_TYPE_COMMAND && card.no == CMD_BEEP_FREQ_INC) {
        change_beep_freq(+1);
    }
    else {
        Serial.println(F("invalid command"));
    }
}


void loop() {
    byte buf[18];
    if (!card_helper.find_a_new_card())
        return;
    if (!card_helper.authenticate_to_block(1))
        return;
    if (card_helper.read_block_from_card(1, buf)) {
        process_new_card(buf);
    }
    card_helper.stop_card_communication();
}
