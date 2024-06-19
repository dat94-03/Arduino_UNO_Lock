#include <EEPROM.h> 
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <Servo.h>

#define RST_PIN 9
#define SS_PIN 10 
#define RED_LED A1
#define GREEN_LED A2
#define SERVO_PIN A0
#define EEPROM_NUMS_CARD 4
#define EEPROM_CARD_ADDR 5
#define EEPROM_PASSWORD_ADDR  0 
                                                
//#include <SPI.h>
bool is_locked =true; 
// 0-unloked, 1-locked. Default is lock when initialize system
//int x = 0;
// */
// SDA (SS)    10          53
// SCK         13          52
// MOSI        11          51
// MISO        12          50
// GND         GND         GND
// RST         9           9
// 3.3V        3.3V        3.3V
// */

MFRC522 mfrc522(SS_PIN, RST_PIN);
const int ROW_NUM = 4;  
const int COLUMN_NUM = 4; 
// keypad mapping
char keys[ROW_NUM][COLUMN_NUM] = {{'1', '2', '3', 'A'},
                                  {'4', '5', '6', 'B'},
                                  {'7', '8', '9', 'C'},
                                  {'*', '0', '#', 'D'}};

byte pin_rows[ROW_NUM] = {A3, 8, 7, 6}; // keypad row connect to pin 6,7,8,A3
byte pin_column[COLUMN_NUM] = {5, 4, 3,2}; // keypad column connect to pin 2,3,4,5
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);
Servo myservo;
const byte iconLocked[8] = {
    B01110, B10001, B10001, B11111, B11011, B11011, B11111,
};

const byte iconUnlocked[8] = {
    B01110, B10000, B10000, B11111, B11011, B11011, B11111,
};
const byte keyChar[8] = {B01110, B11011, B11011, B01110,
                         B00100, B00110, B00100, B00110};


// init keypad and lcd I2C display
LiquidCrystal_I2C lcd(0x27, 16, 2);
String input_password; // store input
byte rfid[4] = {0, 0, 0, 0};
bool isNewCard = 1;

void showStartupMessage() {
    lcd.begin(16, 2);
    lcd.setCursor(4, 0);
    lcd.print("Welcome!");
    delay(1000);
    lcd.setCursor(1, 1);
    char message[] = "HUST LOCK v1.0";
    int messageLength = sizeof(message) - 1;
    for (int i = 0; i < messageLength; i++) {
        lcd.print(message[i]);
        delay(50);
    }
    delay(500);
}

void setCode(String newCode) {
    for (byte i = 0; i < 4; i++) {
        EEPROM.write(EEPROM_PASSWORD_ADDR + i, newCode[i] - '0');
    }
}

bool checkPass(String pass) {
    for (byte i = 0; i < 4; i++) {
        if (pass[i] != (EEPROM.read(i) + '0'))
            return false;
    }
    input_password = "";
    return true;
}

void lockLogic(bool correct) {
    if (correct) {
        correctPassword();
    } else {
        incorrectPassword();
    }
}

String inputSecretCode(String message) {
    lcd.clear();
    int a = (16 - message.length()) / 2;
    lcd.setCursor(a, 0);
    lcd.print(message);
    if (a >= 1) {
        lcd.setCursor(0, 0);
        lcd.write(byte(2));
        lcd.setCursor(15, 0);
        lcd.write(byte(2));
    }
    lcd.setCursor(5, 1);
    lcd.print("[____]");
    lcd.setCursor(6, 1);
    String result = "";
    while (result.length() < 4) {
        char key = keypad.getKey();
        if (key >= '0' && key <= '9') {
            lcd.print('*');
            result += key;
        } else if (key == 'C') {
            result = "";
            lcd.clear();
            lcd.setCursor(1, 0);
            lcd.print("Password Clear");
            lcd.setCursor(5, 1);
            lcd.print("[____]");
            delay(800);
            return inputSecretCode(message);
        }
    }
    return result;
}

void incorrectPassword() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INCORRECT!");
    lcd.setCursor(0, 1);
    lcd.print("ACCESS DENIED!");
    digitalWrite(RED_LED,HIGH);
    for (int i = 0; i < 5; i++) {
        lcd.setBacklight(LOW);
        delay(250);
        lcd.setBacklight(HIGH);
        delay(250);
    }
    digitalWrite(RED_LED,LOW);
    lcd.clear();
}

void correctPassword() {
    digitalWrite(GREEN_LED,HIGH);
    if (!is_locked) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CORRECT!");
        lcd.setCursor(0, 1);
        lcd.print("DOOR LOCKED!");
        myservo.write(90);
        delay(2000);
    } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("CORRECT!");
        lcd.setCursor(0, 1);
        lcd.print("DOOR UNLOCKED!");
        myservo.write(0);
        delay(2000);
    }
    digitalWrite(GREEN_LED,LOW);
    is_locked = !is_locked;
    lcd.clear();
}

void introductionMessage() {
    if (is_locked) {
        lcd.setCursor(1, 0);
        lcd.write(byte(1));
        lcd.setCursor(4, 0);
        lcd.print("LOCKED!!");
        lcd.setCursor(14, 0);
        lcd.write(byte(1));
        lcd.setCursor(0, 1);
        lcd.print("Press A-> Unlock");

    } else {
        lcd.setCursor(1, 0);
        lcd.write(byte(0));
        lcd.setCursor(4, 0);
        lcd.print("UNLOCKED");
        lcd.setCursor(14, 0);
        lcd.write(byte(0));
        lcd.setCursor(0, 1);
        lcd.print("Press A-> Lock");
    }
}

void changePassWord() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CHANGE PASSWORD");
    delay(800);
    lcd.clear();
    delay(400);
    lcd.setCursor(0, 0);
    lcd.print("CHANGE PASSWORD");
    delay(400);
    lcd.clear();
    while (1) {
        String oldPassword = inputSecretCode("Old password");
        if (checkPass(oldPassword)) {
            String newPassword = inputSecretCode("New password");
            String confirmPassword = inputSecretCode("Confirm password");
            if (newPassword == confirmPassword) {
                setCode(newPassword);
                digitalWrite(GREEN_LED,HIGH);
                showMessage("CHANGE PASSWORD", "SUCCESSFULLY");
                delay(1500);
                digitalWrite(GREEN_LED,LOW);
                lcd.clear();
                break;
            } else {
                digitalWrite(RED_LED,HIGH);
                showMessage("CHANGE PASSWORD", "FAILED!!");
                delay(1500);
                digitalWrite(RED_LED,LOW);
                lcd.clear();
            }
        } else {
            digitalWrite(RED_LED,HIGH);
            showMessage("INCORRECT", "");
            delay(1500);
            digitalWrite(RED_LED,LOW);
            lcd.clear();
        }
    }
}

void showMessage(String line1, String line2) {
    lcd.clear();
    int padding1 = (16 - line1.length()) / 2;
    int padding2 = (16 - line2.length()) / 2;
    lcd.setCursor(padding1, 0);
    lcd.print(line1);
    lcd.setCursor(padding2, 1);
    lcd.print(line2);
}

void unlockByCard() {
    showMessage("TAP THE CARD", "**************");
    bool correct = false;
    readCard();
    int len = EEPROM.read(4);
    for (byte i = 0; i < len; i++) {
        bool flag = true;
        for (byte j = 0; j < mfrc522.uid.size; j++) {
            if (EEPROM.read(EEPROM_CARD_ADDR + 4 * i + j) != rfid[j]) {
                flag = false;
            }
        }
        if (flag)
            correct = true;
    }

    lockLogic(correct);
}

void readCard() {
    while (true) {
        if (mfrc522.PICC_IsNewCardPresent()) {
            if (mfrc522.PICC_ReadCardSerial()) {
                for (byte i = 0; i < mfrc522.uid.size; i++) {
                    rfid[i] = mfrc522.uid.uidByte[i];
                }
                break;
            }
        }
    }
}

void cardHandler() {
    showMessage("1. ADD NEW CARD", "2. REMOVE CARD ");
    while (true) {
        char key = keypad.getKey();
        if (key == '1') { // add card
            showMessage("TAP THE CARD", "**************");
            readCard();

            if (checkPass(inputSecretCode("Enter Password"))) {
                int numsCard = EEPROM.read(4);
                bool cardExists = false;

                // Check if the card already exists
                for (int i = 0; i < numsCard; i++) {
                    bool match = true;
                    for (int j = 0; j < 4; j++) {
                        if (EEPROM.read(EEPROM_CARD_ADDR + 4 * i + j) !=
                            rfid[j]) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        cardExists = true;
                        break;
                    }
                }

                if (cardExists) {
                    showMessage("CARD", "ALREADY EXISTS!");
                } else {
                    bool cardAdded = false;

                    // Find an empty slot to add the card
                    for (int i = 0; i < numsCard; i++) {
                        bool emptySlot = true;
                        for (int j = 0; j < 4; j++) {
                            if (EEPROM.read(EEPROM_CARD_ADDR + 4 * i + j) !=
                                0) {
                                emptySlot = false;
                                break;
                            }
                        }

                        if (emptySlot) {
                            for (int j = 0; j < 4; j++) {
                                EEPROM.write(EEPROM_CARD_ADDR + 4 * i + j,
                                             rfid[j]);
                            }
                            cardAdded = true;
                            break;
                        }
                    }

                    if (!cardAdded) {
                        for (int j = 0; j < 4; j++) {
                            EEPROM.write(EEPROM_CARD_ADDR + 4 * numsCard + j,
                                         rfid[j]);
                        }
                        EEPROM.write(4, numsCard + 1);
                    }
                    digitalWrite(GREEN_LED,HIGH);
                    showMessage("CARD ADDED!!", "");
                }

                delay(2000);
                digitalWrite(GREEN_LED,LOW);
            } else {
              digitalWrite(RED_LED,HIGH);
                showMessage("PASSWORD WRONG", "ADD CARD FAILED!");
                delay(2000);
              digitalWrite(RED_LED,LOW);
            }
            break;
        } else if (key == '2') { // remove card
            showMessage("TAP THE CARD", "**************");
            readCard();
            bool cardRemoved = false;
            int numsCard = EEPROM.read(4);

            for (int i = 0; i < numsCard; i++) {
                bool match = true;
                for (int j = 0; j < 4; j++) {
                    if (EEPROM.read(EEPROM_CARD_ADDR + 4 * i + j) != rfid[j]) {
                        match = false;
                        break;
                    }
                }

                if (match) {
                    for (int j = 0; j < 4; j++) {
                        EEPROM.write(EEPROM_CARD_ADDR + 4 * i + j, 0);
                    }
                    showMessage("CARD REMOVED!!", "");
                    cardRemoved = true;
                    delay(2000);
                    break;
                }
            }

            if (!cardRemoved) {
                showMessage("CARD NOT EXIST", "!!!!!!!!!!!!");
                delay(2000);
            }
            break;
        }
    }
}

void reset() {
    for (int i = 0; i < 1023; i++) {
        EEPROM.write(i, 0);
    }
    Serial.println("\nLOCK RESET");
}
void logMemory() {
    Serial.flush();
    Serial.print("\nPASSWORD: ");
    for (byte i = 0; i < 4; i++) {

        Serial.print(EEPROM.read(i));
    }
    Serial.print("\nNUMBERS OF CARD: ");
    Serial.print(EEPROM.read(4));
    Serial.print("\n");
    for (byte i = 0; i < 10; i++) {
        for (int j = 0; j < 4; j++) {
            Serial.print(EEPROM.read(EEPROM_CARD_ADDR + 4 * i + j));
            Serial.print("-");
        }
        Serial.print("\n");
    }
    Serial.print("------------------------");
}

void setup() {

    Serial.begin(9600);
    SPI.begin();
    mfrc522.PCD_Init();
    lcd.init(); 
    lcd.backlight();
    // Create new characters for icon :
    lcd.createChar(0, iconUnlocked);
    lcd.createChar(1, iconLocked);
    lcd.createChar(2, keyChar);
    pinMode(RED_LED,OUTPUT);
    pinMode(GREEN_LED,OUTPUT);
    myservo.attach(SERVO_PIN);
    //myservo.write(0);
    //-------------------------
    showStartupMessage();
}

void loop() {
    introductionMessage();
    char key = keypad.getKey();

    if (key) {
        if (key == 'A') {
            input_password = inputSecretCode("Enter Password");
            lockLogic(checkPass(input_password));
        } else if (key == 'C') {
            changePassWord();
        } else if (key == 'B') {
            unlockByCard();
            lcd.clear();
        } else if (key == 'D') {
            cardHandler();
            lcd.clear();
        }
        //uncomment when testing
        //else if (key == '*') {
        //   reset();
        //} else if (key == '#') {
        //    logMemory();
        //}
    }
}
