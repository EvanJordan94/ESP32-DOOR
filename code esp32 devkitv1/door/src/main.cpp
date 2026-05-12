#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32_Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Keypad.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Fingerprint.h>
#include <Preferences.h>

// ====== Flash lưu trữ ======
Preferences preferences;

// ====== MQTT Config ======
const char *mqtt_server = "0959e8f58b7b4eff970d0713cc9c4bfd.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char *mqtt_username = "EvanJordan";
const char *mqtt_password = "Thanh94@";

// ====== Hardware Setup ======
#define RST_PIN 4
#define SS_PIN 5
#define SERVO_PIN 2
#define BUZZER_PIN 27
#define FINGER_RX 16
#define FINGER_TX 17

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo s1;
WiFiClientSecure espClient;
PubSubClient client(espClient);
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);

// ====== Keypad ======
const byte ROWS = 4, COLS = 4;
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {12, 13, 14, 15};
byte colPins[COLS] = {25, 26, 32, 33};
Keypad keypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

// ====== Biến ======
std::vector<String> allowedRFID;
String currentPassword = "1234";
String inputPassword = "";
bool isChangingPassword = false;
bool isEnteringPassword = false;
bool useFingerprintMode = false;
bool isVerifyingOldPassword = false;
int currentMenuLevel = 0;
bool isScanningRFID = false;

void showMainMenu();
void showRFIDMenu();
void showFingerMenu();
void showFaceMenu();
void showPasswordMenu();
void showSettingMenu();
void openDoor();

void handleRFID();
void addRFID();
void removeRFID();

void checkFingerprint();
void enrollFingerprint();
void deleteFingerprint();

void faceScan();
void faceAdd();
void faceDelete();

void handlePasswordInput();
void handlePasswordChange();
void processPassword();
void inputPasswordChar(char key);

void resetWiFi();
void logAccess(const String &method, const String &value);
void showMainMenu()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1: RFID 2: V.Tay");
  lcd.setCursor(0, 1);
  lcd.print("3: K.Mat 4: M.Khau");
  lcd.setCursor(0, 2);
  lcd.print("5: Cai dat");
  lcd.setCursor(0, 3);
  lcd.print("Chon muc (1-5)");
  currentMenuLevel = 0;
}

void showRFIDMenu()
{
  lcd.clear();
  lcd.print("1: Quet the");
  lcd.setCursor(0, 1);
  lcd.print("2: Them the");
  lcd.setCursor(0, 2);
  lcd.print("3: Xoa the");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  currentMenuLevel = 1;
}

void showFingerMenu()
{
  lcd.clear();
  lcd.print("1: Quet van tay");
  lcd.setCursor(0, 1);
  lcd.print("2: Them van tay");
  lcd.setCursor(0, 2);
  lcd.print("3: Xoa van tay");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  currentMenuLevel = 2;
}

void showFaceMenu()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vui long nhin thang ");
  lcd.setCursor(0,1);
  lcd.print("vao camera!");
  //lcd.setCursor(0, 1);
  //lcd.print("2. Them khuon mat");
  //lcd.setCursor(0, 2);
  //lcd.print("3. Xoa khuon mat");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  currentMenuLevel = 3;
}

void showPasswordMenu()
{
  lcd.clear();
  lcd.print("1: Nhap mat khau");
  lcd.setCursor(0, 1);
  lcd.print("2: Doi mat khau");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  currentMenuLevel = 4;
}

void showSettingMenu()
{
  lcd.clear();
  lcd.print("1: Reset WiFi");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  currentMenuLevel = 5;
}
void faceScan()
{
  //lcd.clear();
  //lcd.print("Dang quet khuon mat");
  //Serial.println("[FACE] Đang quét khuôn mặt...");
  client.publish("esp32cam/face", "scan");
  unsigned long start = millis();
  while (millis() - start < 5000)
  {
    char k = keypad.getKey();
    if (k == '*')
    {
      showMainMenu();
      return;
    }
    delay(100);
  }
}
void faceAdd()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nhap ID + ten tren");
  lcd.setCursor(0, 1);
  lcd.print("server Flask...");

  client.publish("esp32/face/cmd", "add");
}

void faceDelete()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nhap ID tren server");
  lcd.setCursor(0, 1);
  lcd.print("de xoa khuon mat");

  client.publish("esp32/face/cmd", "delete");
}

void handleMenu(char key)
{
  if (currentMenuLevel == 0)
  {
    if (key == '1')
      showRFIDMenu();
    else if (key == '2')
      showFingerMenu();
    else if (key == '3')
      showFaceMenu();
    else if (key == '4')
      showPasswordMenu();
    else if (key == '5')
      showSettingMenu();
  }
  else if (key == '*')
  {
    showMainMenu();
  }
  else
  {
    switch (currentMenuLevel)
    {
    case 1:
      if (key == '1')
      {
        isScanningRFID = true;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan your card...");
        lcd.setCursor(0, 3);
        lcd.print("*: Back");
      }
      else if (key == '2')
        addRFID();
      else if (key == '3')
        removeRFID();
      break;
    case 2:
      if (key == '1')
        checkFingerprint();
      else if (key == '2')
        enrollFingerprint();
      else if (key == '3')
        deleteFingerprint();
      break;
    //case 3:
      //if (key == '1')
        //faceScan();
      //else if (key == '2')
        //faceAdd();
      //else if (key == '3')
        //faceDelete();
      //break;
    case 4:
      if (key == '1')
        handlePasswordInput();
      else if (key == '2')
        handlePasswordChange();
      break;
    case 5:
      if (key == '1')
        resetWiFi();
      break;
    }
  }
}

// ====== Reset WiFi Settings ======
void resetWiFi()
{
  lcd.clear();
  lcd.print("Reset WiFi...");
  Serial.println("[WIFI] Dang xoa cau hinh WiFi");
  WiFi.disconnect(true, true); // Xoa toan bo cau hinh
  delay(3000);
  ESP.restart();
}

// ====== Lưu danh sách RFID vào flash ======
void saveRFIDList()
{
  String data = "";
  for (size_t i = 0; i < allowedRFID.size(); i++)
  {
    data += allowedRFID[i];
    if (i < allowedRFID.size() - 1)
      data += ",";
  }
  preferences.putString("rfid_list", data);
  Serial.println("[FLASH] Đã lưu danh sách RFID");
}

// ====== Mở cửa ======
void openDoor()
{
  Serial.println("[DOOR] Mở cửa...");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(200);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(200);
  s1.write(0);
  delay(2000);
  s1.write(170);
}

// ====== RFID ======
void handleRFID()
{
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    return;
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
    uid += String(mfrc522.uid.uidByte[i], HEX);
  uid.toUpperCase();

  Serial.println("[RFID] UID: " + uid);
  lcd.clear();
  lcd.print("UID: " + uid);
  for (String id : allowedRFID)
  {
    if (uid == id)
    {
      Serial.println("[RFID] Hợp lệ → mở cửa");
      openDoor();
      break;
    }
  }
  mfrc522.PICC_HaltA();
}

void addRFID()
{
  Serial.println("[RFID] Thêm thẻ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan new card...");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    char k = keypad.getKey();
    if (k == '*')
    {
      showMainMenu();
      return;
    }
    delay(100);
  }
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
    uid += String(mfrc522.uid.uidByte[i], HEX);
  uid.toUpperCase();
  for (String id : allowedRFID)
    if (uid == id)
    {
      lcd.clear();
      lcd.print("UID Exists!");
      delay(2000);
      showMainMenu();
      return;
    }

  allowedRFID.push_back(uid);
  saveRFIDList();
  lcd.clear();
  lcd.print("UID Added:");
  lcd.setCursor(0, 1);
  lcd.print(uid);
  Serial.println("[RFID] Đã thêm: " + uid);
  delay(2000);
  mfrc522.PICC_HaltA();
  showMainMenu();
  return;
}

void removeRFID()
{
  Serial.println("[RFID] Xóa thẻ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan to delete");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    char k = keypad.getKey();
    if (k == '*')
    {
      showMainMenu();
      return;
    }
    delay(100);
  }
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
    uid += String(mfrc522.uid.uidByte[i], HEX);
  uid.toUpperCase();
  bool found = false;
  for (size_t i = 0; i < allowedRFID.size(); i++)
  {
    if (allowedRFID[i] == uid)
    {
      allowedRFID.erase(allowedRFID.begin() + i);
      found = true;
      break;
    }
  }
  lcd.clear();
  if (found)
  {
    saveRFIDList();
    lcd.print("UID Removed");
    Serial.println("[RFID] Đã xóa: " + uid);
  }
  else
  {
    lcd.print("UID Not Found");
    Serial.println("[RFID] Không tồn tại: " + uid);
  }
  delay(2000);
  mfrc522.PICC_HaltA();
  showMainMenu();
  return;
}

// ====== Vân tay ======
void checkFingerprint()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan Finger...");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  Serial.println("[FINGER] Đợi vân tay...");

  while (true)
  {
    // Nếu người dùng nhấn '*' để thoát
    char key = keypad.getKey();
    if (key == '*')
    {
      showMainMenu();
      return;
    }

    // Nếu có vân tay được đặt vào cảm biến
    if (finger.getImage() == FINGERPRINT_OK)
    {
      if (finger.image2Tz() == FINGERPRINT_OK)
      {
        if (finger.fingerSearch() == FINGERPRINT_OK)
        {
          Serial.println("[FINGER] ID: " + String(finger.fingerID));
          lcd.clear();
          lcd.print("ID: ");
          lcd.print(finger.fingerID);
          logAccess("fingerprint", String(finger.fingerID));
          openDoor();
          delay(2000);
          showMainMenu();
          return;
        }
        else
        {
          Serial.println("[FINGER] Không hợp lệ");
          lcd.clear();
          lcd.print("Khong hop le");
          delay(2000);
          lcd.clear();
          lcd.print("Thu lai hoac *");
        }
      }
    }
    delay(100);
  }
}

void enrollFingerprint()
{
  int id = 1;
  while (finger.loadModel(id) == FINGERPRINT_OK)
    id++;
  Serial.println("[FINGER] Thêm ID: " + String(id));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place finger");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  while (true)
  {
    char k = keypad.getKey();
    if (k == '*')
    {
      showMainMenu();
      return;
    }
    if (finger.getImage() == FINGERPRINT_OK)
      break;
    delay(100);
  }
  finger.image2Tz(1);
  lcd.clear();
  lcd.print("Remove finger");
  delay(2000);
  while (true)
  {
    char k = keypad.getKey();
    if (k == '*')
    {
      showMainMenu();
      return;
    }
    if (finger.getImage() == FINGERPRINT_NOFINGER)
      break;
    delay(100);
  }
  lcd.clear();
  lcd.print("Again...");

  while (true)
  {
    char k = keypad.getKey();
    if (k == '*')
    {
      showMainMenu();
      return;
    }
    if (finger.getImage() == FINGERPRINT_OK)
      break;
    delay(100);
  }
  finger.image2Tz(2);
  if (finger.createModel() == FINGERPRINT_OK && finger.storeModel(id) == FINGERPRINT_OK)
  {
    lcd.clear();
    lcd.print("Enrolled ID: ");
    lcd.print(id);
    Serial.println("[FINGER] Đã lưu ID: " + String(id));
    delay(2000);
    showMainMenu();
    return;
  }
  else
  {
    lcd.clear();
    lcd.print("Enroll Failed");
    Serial.println("[FINGER] Lỗi thêm vân tay");
  }
  delay(2000);
}

void deleteFingerprint()
{
  lcd.clear();
  lcd.print("Del ID (1-127)");
  lcd.setCursor(0, 2);
  lcd.print("#: Delete");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  Serial.println("[FINGER] Nhập ID để xóa...");
  String idStr = "";
  while (true)
  {
    char k = keypad.getKey();
    if (k == '*')
    {
      showMainMenu();
      return;
    }
    if (k >= '0' && k <= '9')
    {
      idStr += k;
      lcd.setCursor(0, 1);
      lcd.print(idStr);
    }
    else if (k == '#')
      break;
  }
  int id = idStr.toInt();
  if (finger.deleteModel(id) == FINGERPRINT_OK)
  {
    lcd.clear();
    lcd.print("Deleted ID: ");
    lcd.print(id);
    Serial.println("[FINGER] Đã xóa ID: " + String(id));
    delay(2000);
    showMainMenu();
    return;
  }
  else
  {
    lcd.clear();
    lcd.print("Delete Failed");
    Serial.println("[FINGER] Xóa thất bại");
  }
  delay(2000);
}

// ====== Password ======
void handlePasswordInput()
{
  Serial.println("[PASSWORD] Nhap mat khau");
  inputPassword = "";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  isChangingPassword = false;
  isEnteringPassword = true;
}

void handlePasswordChange()
{
  Serial.println("[PASSWORD] Doi mat khau");
  inputPassword = "";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mat khau cu:");
  lcd.setCursor(0, 3);
  lcd.print("*: Back");
  isVerifyingOldPassword = true;
  isChangingPassword = false;
  isEnteringPassword = true;
}

void processPassword()
{
  if (isVerifyingOldPassword)
  {
    if (inputPassword == currentPassword)
    {
      Serial.println("[PASSWORD] Xác minh thành công");
      lcd.clear();
      lcd.print("Nhap mat khau moi:");
      lcd.setCursor(0, 3);
      lcd.print("*: Back");
      inputPassword = "";
      isVerifyingOldPassword = false;
      isChangingPassword = true;
      isEnteringPassword = true;
    }
    else
    {
      Serial.println("[PASSWORD] MK cu sai");
      lcd.clear();
      lcd.print("Mat khau cu sai!");
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(2000);
      inputPassword = "";
      isVerifyingOldPassword = false;
      isEnteringPassword = false;
      showMainMenu();
    }
    return;
  }

  if (isChangingPassword)
  {
    if (inputPassword.length() < 4)
    {
      lcd.clear();
      lcd.print("MK >= 4 ky tu");
      Serial.println("[PASSWORD] Quá ngắn");
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(2000);
      inputPassword = "";
      isChangingPassword = false;
      isEnteringPassword = false;
      showMainMenu();
      return;
    }

    currentPassword = inputPassword;
    preferences.putString("password", currentPassword);
    lcd.clear();
    lcd.print("Da doi mat khau");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
    Serial.println("[PASSWORD] Đã đổi mật khẩu");

    delay(2000);
    inputPassword = "";
    isChangingPassword = false;
    isEnteringPassword = false;
    showMainMenu();
    return;
  }

  // Nhập mật khẩu để mở khóa
  if (inputPassword == currentPassword)
  {
    lcd.clear();
    lcd.print("Mat khau dung!");
    Serial.println("[PASSWORD] Đúng → mở cửa");
    logAccess("password", inputPassword);
    openDoor();
    delay(1000);
  }
  else
  {
    lcd.clear();
    lcd.print("Sai mat khau!");
    Serial.println("[PASSWORD] Sai!");
    digitalWrite(BUZZER_PIN, HIGH);
    delay(2000);
    digitalWrite(BUZZER_PIN, LOW);
  }

  inputPassword = "";
  isEnteringPassword = false;
  isChangingPassword = false;
  showMainMenu();
}

void inputPasswordChar(char key)
{
  if (key == '*')
  {
    inputPassword = "";
    isEnteringPassword = false;
    isChangingPassword = false;
    showMainMenu();
    return;
  }

  if (key == '#')
  {
    processPassword();
    return;
  }

  if (inputPassword.length() < 10)
  {
    inputPassword += key;
    lcd.setCursor(0, 1);
    lcd.print("                "); // Xóa dòng cũ
    lcd.setCursor(0, 1);
    for (int i = 0; i < inputPassword.length(); i++)
      lcd.print("*");
  }
}

// ====== MQTT ======
void callback(char *topic, byte *message, unsigned int length)
{
  String msg = "";
  for (unsigned int i = 0; i < length; i++)
    msg += (char)message[i];
  if (String(topic) == "esp32cam/face")
  {
    String msgStr = String((char *)message).substring(0, length);
    Serial.println("[MQTT] Nhận từ camera: " + msgStr);

    if (msgStr.startsWith("door:1"))
    {
      // Tách tên nếu có
      String name = "unknown";
      int nameIndex = msgStr.indexOf("name:");
      if (nameIndex != -1)
      {
        name = msgStr.substring(nameIndex + 5); // bỏ qua "name:"
      }

      Serial.println("[MQTT] Mở cửa bằng AI - Người: " + name);
      logAccess("face", name); // ghi log tên người
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Nhan dien: ");
      lcd.setCursor(0, 1);
      lcd.print(name);
      openDoor();
      showMainMenu();
    }
  }
  if (String(topic) == "esp32/face/status")
  {
    String msgStr = String((char *)message).substring(0, length);
    Serial.println("[MQTT] Trạng thái khuôn mặt: " + msgStr);

    lcd.clear();
    lcd.setCursor(0, 0);

    if (msgStr == "waiting")
      lcd.print("Dang doi server...");
    else if(msgStr == "name_exists")
      lcd.print("Ten da ton tai voi ID khac!");
    else if (msgStr == "start_capture")
      lcd.print("Bat dau chup anh");
    else if(msgStr == "done_capture")
      lcd.print("Da chup hinh xong");
    else if(msgStr == "start_training")
      lcd.print("Dang tranning");
    else if(msgStr == "done_training")
      lcd.print("Da tranning xong");
    else if (msgStr == "done")
    {
      lcd.print("Them thanh cong!");
      delay(1000);
      showFaceMenu();
    }
    else if (msgStr == "error"){
      lcd.print("Loi! Vui long thu lai");
      delay(1000);
      showFaceMenu();
    }
    else if (msgStr == "not_found")
      lcd.print("ID khong ton tai!");
    else if (msgStr == "deleted")
      lcd.print("Da xoa khuon mat");
    else if (msgStr == "delete_error")
      lcd.print("Xoa that bai!");
    lcd.setCursor(0, 3);
    lcd.print("*: Back");
  }
}

void connectMQTT()
{
  while (!client.connected())
  {
    Serial.print("MQTT... ");
    if (client.connect("ESP32Client", mqtt_username, mqtt_password))
    {
      Serial.println("Connected!");
      client.subscribe("esp32cam/face");
      client.subscribe("esp32/face/status");
    }
    else
    {
      Serial.print("Failed: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

String getValidFingerprintIDs()
{
  String idList = "";
  for (int id = 1; id < 128; id++)
  {
    if (finger.loadModel(id) == FINGERPRINT_OK)
    {
      idList += String(id) + ",";
    }
  }
  if (idList.length() > 0)
  {
    idList.remove(idList.length() - 1); // Xóa dấu ',' cuối cùng
  }
  return idList;
}

void sendFlashDataToMQTT()
{
  String savedPassword = preferences.getString("password", "1234");
  String savedRFIDList = preferences.getString("rfid_list", "");
  String fingerIDList = getValidFingerprintIDs();

  Serial.println("[MQTT] Gửi dữ liệu từ Flash:");
  Serial.println("Password: " + savedPassword);
  Serial.println("RFID List: " + savedRFIDList);
  Serial.println("Fingerprint IDs: " + fingerIDList);

  client.publish("esp32/data/password", savedPassword.c_str());
  client.publish("esp32/data/rfid_list", savedRFIDList.c_str());
  client.publish("esp32/data/finger_ids", fingerIDList.c_str());
}

void logAccess(const String &method, const String &value)
{
  String logMsg = "PT:" + method + "," + value;
  Serial.println("[MQTT] Log mở cửa: " + logMsg);
  client.publish("esp32/access_log", logMsg.c_str());
}

unsigned long lastSendTime = 0;
const unsigned long interval = 15000; // 15 giây

// ====== SETUP ======
void setup()
{
  preferences.begin("doorlock", false);
  currentPassword = preferences.getString("password", "1234");

  // Load RFID list
  String rawUIDs = preferences.getString("rfid_list", "");
  allowedRFID.clear();
  if (rawUIDs.length() > 0)
  {
    int start = 0, end = 0;
    while ((end = rawUIDs.indexOf(',', start)) != -1)
    {
      allowedRFID.push_back(rawUIDs.substring(start, end));
      start = end + 1;
    }
    if (start < rawUIDs.length())
      allowedRFID.push_back(rawUIDs.substring(start));
  }

  pinMode(SERVO_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.begin(115200);
  lcd.begin();
  lcd.backlight();
  showMainMenu();
  s1.attach(SERVO_PIN);
  s1.write(170);
  SPI.begin();
  mfrc522.PCD_Init();
  WiFiManager wm;
  wm.autoConnect("ESP32-DOOR");
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  fingerSerial.begin(57600, SERIAL_8N1, FINGER_RX, FINGER_TX);
  finger.begin(57600);
}

// ====== LOOP ======
void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - lastSendTime >= interval)
  {
    lastSendTime = currentMillis;
    if (client.connected())
    {
      sendFlashDataToMQTT();
    }
  }

  if (!client.connected())
  {
    connectMQTT();
  }
  client.loop();

  // ==== Ưu tiên nhập mật khẩu nếu đang trong chế độ đó ====
  char key = keypad.getKey();
  if (key)
  {
    if (isEnteringPassword)
      inputPasswordChar(key);
    else
      handleMenu(key);
  }

  // ==== Chế độ quét vân tay hoặc RFID ====
  if (!isEnteringPassword)
  {
    if (useFingerprintMode)
    {
      checkFingerprint();
      useFingerprintMode = false;
    }
    else if (isScanningRFID)
    {
      if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
      {
        String uid = "";
        for (byte i = 0; i < mfrc522.uid.size; i++)
          uid += String(mfrc522.uid.uidByte[i], HEX);
        uid.toUpperCase();

        Serial.println("[RFID] UID: " + uid);
        lcd.clear();
        lcd.print("UID: " + uid);

        bool found = false;
        for (String id : allowedRFID)
        {
          if (uid == id)
          {
            found = true;
            Serial.println("[RFID] Hợp lệ → mở cửa");
            logAccess("rfid", uid); // 👈 GỬI LOG
            openDoor();
            break;
          }
        }

        if (!found)
        {
          lcd.setCursor(0, 1);
          lcd.print("Khong hop le");
          Serial.println("[RFID] Không hợp lệ");
        }

        delay(2000);
        showMainMenu();
        isScanningRFID = false;

        mfrc522.PICC_HaltA();
      }
    }
  }
}
