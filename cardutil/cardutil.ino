/*
see MFRC522 library examples for pin layout
*/

#include <SPI.h>
#include <MFRC522Helper.h>

#define RST_PIN 9
#define SS_PIN 10
#define CMD_PING 1
#define CMD_DUMP_CARD_DATA 2
#define CMD_DUMP_CARD_DATA_BY_LIB 3
#define CMD_GET_CARD_UID 4
#define CMD_WRITE_CARD_BLOCK 5
#define CMD_USE_AUTH_KEY 6
#define CMD_SET_CARD_AUTH_KEYS 7
#define CMD_FILL_CARD_RANDOM 8
#define CMD_SHOW_INFO_MESSAGES 9
#define CMD_READ_CARD_BLOCK 10
#define CARD_FINDING_TIMEOUT 10000


MFRC522Helper card;
byte auth_key_val[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
bool show_info_messages = false;


void setup() {
    randomSeed(analogRead(0));
    Serial.begin(115200);
    while (!Serial);
    Serial.setTimeout(5000);
    SPI.begin();
    card.mfrc.PCD_Init();
    for (byte i = 0; i < 6; i++) {
        card.mfrc_key.keyByte[i] = auth_key_val[i];
    }
    // Serial.println(F("Ready"));
}


void info(char* msg) {
    if (!show_info_messages)
        return;
    Serial.println(msg);
}


void info(const __FlashStringHelper* msg) {
    info((char*)msg);
}


byte get_checksum(byte* data, byte size) {
    byte c = 0;
    for(byte i = 0; i < size; i++) {
        c ^= (data[i] + i) & 0xff;
    }
    return c & 0xff;
}


void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}


bool read_serial(byte* buf, int n) {
    int m = Serial.readBytes(buf, n);
    if (m == n)
        return true;
    Serial.print(F("error: expect "));
    Serial.print(n);
    Serial.print(F(" bytes but received "));
    Serial.println(m);
    return false;
}


bool is_valid_checksum(byte* buf, int n) {
    if (get_checksum(buf, n) == buf[n])
        return true;
    Serial.println(F("error: invalid checksum"));
    return false;
}


bool card_found_near_reader() {
    unsigned long start_time = millis();
    while (!card.find_a_new_card()) {
        if (millis() > start_time + CARD_FINDING_TIMEOUT) {
            Serial.println(F("error: no card found"));
            return false;
        }
    }
    return true;
}


bool write_block_to_card(byte block, byte* buf) {
    if (card.write_block_to_card(block, buf))
        return true;
    card.stop_card_communication();
    Serial.print(F("error: can not write to block "));
    Serial.println(block);
    return false;
}


bool ping() {
    byte buf[2];
    if (!read_serial(buf, 1)) {
        return false;
    }
    Serial.println(buf[0] + 1);
    return true;
}


bool dump_card_data() {
    byte buf[18];
    byte res;
    if (!card_found_near_reader())
        return false;
    for (int i = 63; i >= 0; i--) {
        res = card.read_block_from_card(i, buf);
        Serial.write(i);
        Serial.write(res);
        Serial.write(buf, 16);
    }
    card.stop_card_communication();
    return true;
}


bool dump_card_data_by_lib() {
    if (!card_found_near_reader())
        return false;
    card.mfrc.PICC_DumpMifareClassicToSerial(&(card.mfrc.uid), card.mfrc.PICC_GetType(card.mfrc.uid.sak), &card.mfrc_key);
    card.stop_card_communication();
    return true;
}


bool get_card_uid() {
    if (!card_found_near_reader())
        return false;
    Serial.write(card.mfrc.uid.uidByte, card.mfrc.uid.size);
    card.stop_card_communication();
    return true;
}


bool read_from_serial_and_write_to_card() {
    byte buf[20];
    if (!read_serial(buf, 18))
        return false;
    if (!is_valid_checksum(buf, 17))
        return false;
    if (!card_found_near_reader())
        return false;
    byte block = buf[0];
    byte* data = &buf[1];
    if (!write_block_to_card(block, data))
        return false;
    card.stop_card_communication();
    Serial.println("success");
    return true;
}


bool use_auth_key() {
    byte buf[8];
    if (!read_serial(buf, 7))
        return false;
    if (!is_valid_checksum(buf, 6))
        return false;
    for (byte i = 0; i < 6; i++) {
        card.mfrc_key.keyByte[i] = buf[i];
    }
    // Serial.print(F("this key will be use from now:"));
    // dump_byte_array(buf, 6);
    // Serial.println();
    Serial.println("success");
    return true;
}


bool set_card_auth_keys() {
    byte n;
    byte buf[20];
    if (!read_serial(buf, 7))
        return false;
    if (!is_valid_checksum(buf, 6))
        return false;
    if (!card_found_near_reader())
        return false;
    byte other_bytes[] = {0xff, 0x07, 0x80, 0, 0, 0, 0, 0, 0, 0};
    for(byte i = 0; i < 10; i++) {
        buf[i + 6] = other_bytes[i];
    }
    // Serial.print(F("we will write this data:"));
    // dump_byte_array(buf, 16);
    // Serial.println(F("  to all trailer blocks"));
    for(byte i = 0; i < 16; i++) {
        if (!write_block_to_card(i * 4 + 3, buf))
            return false;
    }
    card.stop_card_communication();
    Serial.println("success");
    return true;
}


bool set_show_info_messages() {
    byte buf[2];
    if (!read_serial(buf, 1))
        return false;
    show_info_messages = (buf[0] != 0);
    return true;
}


bool fill_card_random() {
    byte buf[18];
    if (!card_found_near_reader())
        return false;
    for (byte blk = 1; blk < 64; blk++) {
        if (blk % 4 == 3)
            continue;
        for (byte i = 0; i < 16; i++) {
            buf[i] = random(0, 256);
        }
        if (!write_block_to_card(blk, buf))
            return false;
    }
    card.stop_card_communication();
    Serial.println("success");
    return true;
}


bool read_card_block() {
    byte buf[18];
    if (!read_serial(buf, 1))
        return false;
    if (!card_found_near_reader())
        return false;
    byte block = buf[0];
    if (!card.read_block_from_card(block, buf)) {
        Serial.println(F("error: can not read block"));
        return false;
    }
    Serial.write(buf, 16);
    card.stop_card_communication();
    return true;
}


void loop() {
    int b;
    byte cmd;
    char buf[20];

    b = Serial.read();
    if (b == -1)
        return;
    cmd = (byte)b;
    if (cmd == CMD_PING) {
        info(F("ping"));
        ping();
    }
    else if (cmd == CMD_DUMP_CARD_DATA) {
        info(F("dump card data"));
        info(F("waiting for card..."));
        dump_card_data();
    }
    else if (cmd == CMD_DUMP_CARD_DATA_BY_LIB) {
        info(F("dump card data by lib"));
        info(F("waiting for card..."));
        dump_card_data_by_lib();
    }
    else if (cmd == CMD_GET_CARD_UID) {
        info(F("get card uid"));
        info(F("waiting for card..."));
        get_card_uid();
    }
    else if (cmd == CMD_WRITE_CARD_BLOCK) {
        info(F("write card block"));
        info(F("waiting for card..."));
        read_from_serial_and_write_to_card();
    }
    else if (cmd == CMD_USE_AUTH_KEY) {
        info(F("use auth key"));
        use_auth_key();
    }
    else if (cmd == CMD_SET_CARD_AUTH_KEYS) {
        info(F("set card auth keys"));
        info(F("waiting for card..."));
        set_card_auth_keys();
    }
    else if (cmd == CMD_FILL_CARD_RANDOM) {
        info(F("fill card random"));
        info(F("waiting for card..."));
        fill_card_random();
    }
    else if (cmd == CMD_SHOW_INFO_MESSAGES) {
        info(F("show info message"));
        set_show_info_messages();
    }
    else if (cmd == CMD_READ_CARD_BLOCK) {
        info(F("read card block"));
        read_card_block();
    }
    else {
        sprintf(buf, (char*)F("invalid cmd: %d"), cmd);
        info(buf);
    }
}
