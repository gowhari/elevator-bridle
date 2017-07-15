/*
see MFRC522 library examples for pin layout
*/

#include <SPI.h>
#include <EEPROM.h>
#include <MFRC522.h>

#define RST_PIN 9
#define SS_PIN 10
#define CARD_TYPE_INVALID 0
#define CARD_TYPE_UNIT 1
#define CARD_TYPE_WRITE 2
#define CARDS_ARRAY_BYTES 4
#define CARDS_ARRAY_NO_OF_CARDS (CARDS_ARRAY_BYTES * 8)
#define CARDS_ARRAY_EEPROM_ADDR 0


struct Card {
    byte type;
    int no;
};


byte cards_array[CARDS_ARRAY_BYTES];
byte last_auth_block = 0;
MFRC522 mfrc(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key mfrc_key;


void setup() {
    Serial.begin(115200);
    while (!Serial);
    SPI.begin();
    mfrc.PCD_Init();
    for (byte i = 0; i < 6; i++) {
        mfrc_key.keyByte[i] = 0xFF;
    }
    load_cards_array_from_eeprom();
    Serial.println(F("Ready"));
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
        sprintf(s, "%d: %d", i, read_state_of_card_from_ram(i));
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


void play_success_beep() {
    Serial.println("play_success_beep");
}


void play_failure_beep() {
    Serial.println("play_failure_beep");
}


/*
return corresponding bit of cards array from ram for this cord no
*/
boolean read_state_of_card_from_ram(int card_no) {
    int byte_index = card_no / 8;
    byte bit_index = card_no % 8;
    byte mask = 1 << bit_index;
    return (boolean)(cards_array[byte_index] & mask);
}


void write_byte_to_ram_and_eeprom(int index, byte new_val) {
    byte old_val = cards_array[index];
    if(new_val == old_val)
        return;
    cards_array[index] = new_val;
    EEPROM.update(CARDS_ARRAY_EEPROM_ADDR + index, new_val);
}


void load_cards_array_from_eeprom() {
    for(int i = 0; i < CARDS_ARRAY_BYTES; i++) {
        cards_array[i] = EEPROM.read(CARDS_ARRAY_EEPROM_ADDR + i);
    }
}


/*
enable elevator key pad for 30 seconds
*/
void authorize() {
    Serial.println("authorized!");
}


boolean find_a_new_card() {
    if (!mfrc.PICC_IsNewCardPresent()) {
        return false;
    }
    if (!mfrc.PICC_ReadCardSerial()) {
        return false;
    }
    MFRC522::PICC_Type picc_type = mfrc.PICC_GetType(mfrc.uid.sak);
    if (picc_type != MFRC522::PICC_TYPE_MIFARE_MINI && picc_type != MFRC522::PICC_TYPE_MIFARE_1K &&
        picc_type != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.print(F("invalid picc card: "));
        Serial.println(mfrc.PICC_GetTypeName(picc_type));
        return false;
    }
    return true;
}


byte get_trailer_block(byte block) {
    return block / 4 * 4 + 3;
}


boolean authenticate_if_needed(byte block) {
    if (last_auth_block == 0) {
        return authenticate_to_block(block);
    }
    if (get_trailer_block(block) != last_auth_block) {
        return authenticate_to_block(block);
    }
    return true;
}


boolean authenticate_to_block(byte block) {
    byte trailer_block = get_trailer_block(block);
    Serial.print("authenticate_to_block: ");
    Serial.println(trailer_block);
    MFRC522::StatusCode status;
    status = mfrc.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailer_block, &mfrc_key, &(mfrc.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("authenticate failed: "));
        Serial.println(mfrc.GetStatusCodeName(status));
        return false;
    }
    last_auth_block = trailer_block;
    return true;
}


boolean read_block_from_card(byte block, byte* buf) {
    if (!authenticate_if_needed(block)) {
        return false;
    }
    MFRC522::StatusCode status;
    byte size = 18;
    status = mfrc.MIFARE_Read(block, buf, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("read block failed: "));
        Serial.println(mfrc.GetStatusCodeName(status));
        return false;
    }
    return true;
}


boolean write_block_to_card(byte block, byte* buf) {
    if (!authenticate_if_needed(block)) {
        return false;
    }
    MFRC522::StatusCode status;
    Serial.print(F("Writing data into block "));
    Serial.print(block);
    Serial.println(F(" ..."));
    dump_byte_array(buf, 16);
    Serial.println();
    status = mfrc.MIFARE_Write(block, buf, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc.GetStatusCodeName(status));
    }
    Serial.println();
}


void stop_card_communication() {
    last_auth_block = 0;
    mfrc.PICC_HaltA();
    mfrc.PCD_StopCrypto1();
}


/*
decrypt 16 bytes initial card data and check if it's correctly encrypted or no
the return value is a Card struc, for invalid encryption return {CARD_TYPE_INVALID, 0}
*/
Card decrypt_first_block_data(byte* data) {
    byte type = data[0];
    int no = (((word)data[1]) << 8) | ((word)data[2]);
    if(type != CARD_TYPE_UNIT && type != CARD_TYPE_WRITE) {
        type = CARD_TYPE_INVALID;
        no = 0;
    }
    Card card = {type, no};
    return card;
}


void process_unit_card(Card card) {
    boolean state = read_state_of_card_from_ram(card.no);
    if (state == true) {
        play_success_beep();
        authorize();
    }
    else {
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
boolean process_write_card(Card card) {
    byte block = 4;
    byte data[18];
    byte i, high, low, value, chk;
    int index;
    boolean is_more_data = true;
    char str[100];
    while (block < 63 && is_more_data) {
        if (!read_block_from_card(block, data))
            return false;
        chk = get_checksum(data, 15);
        if (data[15] != chk) {
            Serial.println("invalid checksum");
            return false;
        }
        for (i = 0; i <= 12 ; i += 3) {
            high = data[i];
            if(!(high & 0x80)) {
                is_more_data = false;
                break;
            }
            low = data[i + 1];
            value = data[i + 2];
            index = ((high & 0x7f) << 8) | low;
            write_byte_to_ram_and_eeprom(index, value);
            sprintf(str, "i=%02d  high=%02x  low=%02x  index=%02x  value=%02x", i, high, low, index, value);
            Serial.println(str);
        }
        if(!is_more_data)
            break;
        block += 1;
        if (block % 4 == 3)
            block += 1;
    }
    play_success_beep();
}


void process_new_card(byte* data) {
    Card card = decrypt_first_block_data(data);
    char s[50];
    sprintf(s, "card info: type=%d no=%d", card.type, card.no);
    Serial.println(s);
    if (card.type == CARD_TYPE_INVALID) {
        Serial.println("invalid card");
        return;
    }
    else if (card.type == CARD_TYPE_UNIT) {
        process_unit_card(card);
    }
    else if (card.type == CARD_TYPE_WRITE) {
        process_write_card(card);
    }
}


void loop() {
    byte buf[18];
    if (!find_a_new_card())
        return;
    if (!authenticate_to_block(1))
        return;
    if (read_block_from_card(1, buf)) {
        process_new_card(buf);
    }
    stop_card_communication();


    // write data on write-card
    // byte buf[][16] = {
    //     {128, 0, 3, 128, 1, 15, 128, 2, 7, 0, 0, 0, 0, 0, 0, 145},
    //     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15},
    // };
    // byte rows = sizeof(buf) / 16;
    // if (!find_a_new_card())
    //     return;
    // if (!authenticate_to_block(7))
    //     return;
    // for(int i = 0; i < rows; i++) {
    //     write_block_to_card(i + 4, buf[i]);
    // }
    // stop_card_communication();


    // // write 1st record on card
    // // byte buf[] = {CARD_TYPE_UNIT, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // byte buf[] = {CARD_TYPE_WRITE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    // if (!find_a_new_card())
    //     return;
    // if (!authenticate_to_block(1))
    //     return;
    // write_block_to_card(1, buf);
    // stop_card_communication();


    // // dump cards array
    // dump_cards_array();
    // Serial.println();
    // delay(1000);
    // exit(0);



    // erase eeprom
    // int n = EEPROM.length();
    // for(int i = 0; i < n; i++) {
    //     EEPROM.write(i, 0);
    // }
    // Serial.println("Done");
}
