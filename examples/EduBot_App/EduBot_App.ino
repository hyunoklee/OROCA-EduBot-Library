/* Authors: byeongkyu, baram */

#define EDUBOT_DRIVER_BLE

#include <EduBot.h>
#include <image/EduBoy.h>


/* TODO

- 모터 구동 블럭 함수 처리

*/

EduBot edubot;


#define MOTOR_SERVICE_UUID                           "34443c32-3356-11e9-b210-d663bd873d93"
#define MOTOR_CHARACTERISTIC_SET_STEP_UUID           "34443c33-3356-11e9-b210-d663bd873d93"
#define MOTOR_CHARACTERISTIC_SET_SPEED_UUID          "34443c34-3356-11e9-b210-d663bd873d93"
#define MOTOR_CHARACTERISTIC_SET_DISTANCE_UUID       "34443c35-3356-11e9-b210-d663bd873d93"
#define MOTOR_CHARACTERISTIC_SET_ACCEL_UUID          "34443c36-3356-11e9-b210-d663bd873d93"
#define MOTOR_CHARACTERISTIC_WAIT_RESULT_UUID        "34443c37-3356-11e9-b210-d663bd873d93"

#define MISC_SERVICE_UUID                            "34443c38-3356-11e9-b210-d663bd873d93"
#define MISC_CHARACTERISTIC_COLOR_LED_UUID           "34443c39-3356-11e9-b210-d663bd873d93"
#define MISC_CHARACTERISTIC_PLAY_SOUND_UUID          "34443c40-3356-11e9-b210-d663bd873d93"
#define MISC_CHARACTERISTIC_BUTTON_UUID              "34443c41-3356-11e9-b210-d663bd873d93"
#define MISC_CHARACTERISTIC_SET_TEXT_OLED_UUID       "34443c42-3356-11e9-b210-d663bd873d93"
#define MISC_CHARACTERISTIC_SET_IMAGE_OLED_UUID      "34443c43-3356-11e9-b210-d663bd873d93"

#define SENSOR_SERVICE_UUID                          "34443c44-3356-11e9-b210-d663bd873d93"
#define SENSOR_CHARACTERISTIC_FLOOR_SENSORS_UUID     "34443c45-3356-11e9-b210-d663bd873d93"
#define SENSOR_CHARACTERISTIC_DISTANCE_SENSOR_UUID   "34443c46-3356-11e9-b210-d663bd873d93"
#define SENSOR_CHARACTERISTIC_IMU_SENSOR_UUID        "34443c47-3356-11e9-b210-d663bd873d93"
#define SENSOR_CHARACTERISTIC_BATTERY_LEVEL_UUID     "34443c48-3356-11e9-b210-d663bd873d93"


int8_t value_motor_set_accel[2] = {0, 0};
uint8_t value_misc_button = 1;
uint8_t value_sensor_battery_level = 0;
uint8_t value_motor_wait_result = 0;
uint8_t value_sensor_floor_sensors[4] = {0, 0, 0, 0};
uint16_t value_sensor_distance_sensors[2] = {0, 0};
int16_t value_sensor_imu_sensor[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

uint8_t status_led_count = 0;
uint8_t status_battery_count = 0;
uint8_t status_update_sensors_count = 0;
bool device_connected = false;

bool status_text_displayed = false;

std::string display_text = "";
int8_t request_display_text = 0;
uint8_t display_image_index = 0;
int8_t request_display_image = 0;

int8_t request_motor_wait_result = 0;

uint8_t ble_mac_addr[6] = {0, };


BLECharacteristic *mCharMotorWaitResult = NULL;
BLECharacteristic *mCharMiscButton = NULL;
BLECharacteristic *mCharSensorBatteryLevel = NULL;
BLECharacteristic *mCharSensorFloorSensors = NULL;
BLECharacteristic *mCharSensorDistanceSensors = NULL;
BLECharacteristic *mCharSensorImuSensor = NULL;



class MyBLEServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    device_connected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    device_connected = false;
  }
};

class MyMotorSetStepCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if(value.length() == 10) {
      int32_t l_step  = (value[0] << 24) + (value[1] << 16) +  (value[2] << 8)  +  value[3];
      int32_t r_step  = (value[4] << 24) + (value[5] << 16) +  (value[6] << 8)  +  value[7];
      int16_t max_vel = (value[8] << 8) + value[9];

      edubot.motor.setStepNoWait(l_step, r_step, max(min((int)max_vel, 300), 0));
      request_motor_wait_result = 1;
    }
  }
};

class MyMotorSetSpeedCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if(value.length() == 6) {
      int16_t l_speed  = (value[0] << 8) + value[1];
      int16_t r_speed  = (value[2] << 8) + value[3];
      int16_t delay_ms = (value[4] << 8) + value[5];

      edubot.motor.setSpeed(max(min((int)l_speed, 300), -300), max(min((int)r_speed, 300), -300), max((int)delay_ms, 0));
    }
  }
};

class MyMotorSetDistanceCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if(value.length() == 6) {
      int16_t l_distance  = (value[0] << 8) + value[1];
      int16_t r_distance  = (value[2] << 8) + value[3];
      int16_t max_vel = (value[4] << 8) + value[5];


      int32_t l_step = edubot.motor.distanceToStep(l_distance);
      int32_t r_step = edubot.motor.distanceToStep(r_distance);

      edubot.motor.setStepNoWait(l_step, r_step, max(min((int)max_vel, 300), 0));
      request_motor_wait_result = 1;
    }
  }
};

class MyMotorSetAccelCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if(value.length() == 2) {
      int8_t l_accel  = value[0];
      int8_t r_accel  = value[1];
    
      edubot.motor.setAcc(l_accel, r_accel);
    }
  }
};

class MyMiscSetColorLEDCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if(value.length() == 6) {
      uint8_t left_r = value[3];
      uint8_t left_g = value[4];
      uint8_t left_b = value[5];
      uint8_t right_r = value[0];
      uint8_t right_g = value[1];
      uint8_t right_b = value[2];
      
      edubot.led.leftBright(left_r, left_g, left_b);
      edubot.led.rightBright(right_r, right_g, right_b);
    }
  }
};

class MyMiscPlaySoundCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if(value.length() == 6) {
      
    }
  }
};

class MyMiscSetTextOLEDCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    display_text = value.c_str();
    request_display_text = 1;   
  }
};

class MyMiscSetImageOLEDCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    display_image_index = value[0];
    request_display_image = 1;
  }
};


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);  

  Serial.println("===============\nStarting BLE work!");

  
 
  BLEDevice::init("OROCA_EduBot");

  BLEAddress ble_addr = BLEDevice::getAddress();  
  memcpy(ble_mac_addr, *ble_addr.getNative(), 6);

  BLEServer *mServer = BLEDevice::createServer();
  mServer->setCallbacks(new MyBLEServerCallbacks());

  //************************************************
  // Motor Service
  BLEService *mServiceMotor = mServer->createService(BLEUUID(MOTOR_SERVICE_UUID), 20);

  // Set Step
  BLECharacteristic *mCharMotorSetStep = mServiceMotor->createCharacteristic(
                                         MOTOR_CHARACTERISTIC_SET_STEP_UUID,
                                         BLECharacteristic::PROPERTY_WRITE);
  BLEDescriptor *mDescMotorSetStep = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMotorSetStep->setValue("Motor SetStep");  
  mCharMotorSetStep->addDescriptor(mDescMotorSetStep);
  mCharMotorSetStep->setCallbacks(new MyMotorSetStepCallbacks());

  // Set Speed
  BLECharacteristic *mCharMotorSetSpeed = mServiceMotor->createCharacteristic(
                                         MOTOR_CHARACTERISTIC_SET_SPEED_UUID,
                                         BLECharacteristic::PROPERTY_WRITE);
  BLEDescriptor *mDescMotorSetSpeed = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMotorSetSpeed->setValue("Motor SetSpeed");  
  mCharMotorSetSpeed->addDescriptor(mDescMotorSetSpeed);
  mCharMotorSetSpeed->setCallbacks(new MyMotorSetSpeedCallbacks());

  // Set Distance
  BLECharacteristic *mCharMotorSetDistance = mServiceMotor->createCharacteristic(
                                         MOTOR_CHARACTERISTIC_SET_DISTANCE_UUID,
                                         BLECharacteristic::PROPERTY_WRITE);
  BLEDescriptor *mDescMotorSetDistance = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMotorSetDistance->setValue("Motor SetDistance");  
  mCharMotorSetDistance->addDescriptor(mDescMotorSetDistance);
  mCharMotorSetDistance->setCallbacks(new MyMotorSetDistanceCallbacks());

  value_motor_set_accel[0] = 1;
  value_motor_set_accel[1] = 1;

  // Set Accel
  BLECharacteristic *mCharMotorSetAccel = mServiceMotor->createCharacteristic(
                                         MOTOR_CHARACTERISTIC_SET_ACCEL_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  mCharMotorSetAccel->setValue((uint8_t*)value_motor_set_accel, 2);
  BLEDescriptor *mDescMotorSetAccel = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMotorSetAccel->setValue("Motor SetAccel");  
  mCharMotorSetAccel->addDescriptor(mDescMotorSetAccel);
  mCharMotorSetAccel->setCallbacks(new MyMotorSetAccelCallbacks());

  // Wait Result
  mCharMotorWaitResult = mServiceMotor->createCharacteristic(
                                         MOTOR_CHARACTERISTIC_WAIT_RESULT_UUID,
                                         BLECharacteristic::PROPERTY_NOTIFY);
  BLEDescriptor *mDescMotorWaitResult = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMotorWaitResult->setValue("Motor WaitResult");  
  mCharMotorWaitResult->setValue(&value_motor_wait_result, 1);
  mCharMotorWaitResult->addDescriptor(mDescMotorWaitResult);
  mCharMotorWaitResult->addDescriptor(new BLE2902());

  mServiceMotor->start();
  //************************************************



  //************************************************
  // Misc Service
  BLEService *mServiceMisc = mServer->createService(BLEUUID(MISC_SERVICE_UUID), 20);

  // SetColorLED
  BLECharacteristic *mCharMiscSetColorLED = mServiceMisc->createCharacteristic(
                                         MISC_CHARACTERISTIC_COLOR_LED_UUID,
                                         BLECharacteristic::PROPERTY_WRITE);
  BLEDescriptor *mDescMiscSetColorLED = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMiscSetColorLED->setValue("NeoPixel Color RGB");  
  mCharMiscSetColorLED->addDescriptor(mDescMiscSetColorLED);
  mCharMiscSetColorLED->setCallbacks(new MyMiscSetColorLEDCallbacks());

  // PlaySound
  BLECharacteristic *mCharMiscPlaySound = mServiceMisc->createCharacteristic(
                                         MISC_CHARACTERISTIC_PLAY_SOUND_UUID,
                                         BLECharacteristic::PROPERTY_WRITE);
  BLEDescriptor *mDescMiscPlaySound = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMiscPlaySound->setValue("Play Sound");  
  mCharMiscPlaySound->addDescriptor(mDescMiscPlaySound);
  mCharMiscPlaySound->setCallbacks(new MyMiscPlaySoundCallbacks());

  // Button
  mCharMiscButton = mServiceMisc->createCharacteristic(
                                         MISC_CHARACTERISTIC_BUTTON_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  mCharMiscButton->setValue(&value_misc_button, 1);
  BLEDescriptor *mDescMiscButton = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMiscButton->setValue("User Button");  
  mCharMiscButton->addDescriptor(mDescMiscButton);
  mCharMiscButton->addDescriptor(new BLE2902());

  // setTextOLED
  BLECharacteristic *mCharMiscSetTextOLED = mServiceMisc->createCharacteristic(
                                         MISC_CHARACTERISTIC_SET_TEXT_OLED_UUID,
                                         BLECharacteristic::PROPERTY_WRITE);
  BLEDescriptor *mDescMiscSetTextOLED = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMiscSetTextOLED->setValue("SetText OLED");  
  mCharMiscSetTextOLED->addDescriptor(mDescMiscSetTextOLED);
  mCharMiscSetTextOLED->setCallbacks(new MyMiscSetTextOLEDCallbacks());

  // setImageOLED
  BLECharacteristic *mCharMiscSetImageOLED = mServiceMisc->createCharacteristic(
                                         MISC_CHARACTERISTIC_SET_IMAGE_OLED_UUID,
                                         BLECharacteristic::PROPERTY_WRITE);
  BLEDescriptor *mDescMiscSetImageOLED = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescMiscSetImageOLED->setValue("SetImage OLED");  
  mCharMiscSetImageOLED->addDescriptor(mDescMiscSetImageOLED);
  mCharMiscSetImageOLED->setCallbacks(new MyMiscSetImageOLEDCallbacks());

  mServiceMisc->start();
  //************************************************



  //************************************************
  // Sensor Service
  BLEService *mServiceSensor = mServer->createService(BLEUUID(SENSOR_SERVICE_UUID), 20);

  // Floor Sensor
  mCharSensorFloorSensors = mServiceSensor->createCharacteristic(
                                         SENSOR_CHARACTERISTIC_FLOOR_SENSORS_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  mCharSensorFloorSensors->setValue((uint8_t*)&value_sensor_floor_sensors, 4);
  BLEDescriptor *mDescSensorFloorSensors = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescSensorFloorSensors->setValue("Floor Sensors");  
  mCharSensorFloorSensors->addDescriptor(mDescSensorFloorSensors);
  mCharSensorFloorSensors->addDescriptor(new BLE2902());

  // Distance Sensor
  mCharSensorDistanceSensors = mServiceSensor->createCharacteristic(
                                         SENSOR_CHARACTERISTIC_DISTANCE_SENSOR_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  mCharSensorDistanceSensors->setValue((uint8_t*)&value_sensor_distance_sensors, 4);
  BLEDescriptor *mDescSensorDistanceSensors = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescSensorDistanceSensors->setValue("Distance Sensors");  
  mCharSensorDistanceSensors->addDescriptor(mDescSensorDistanceSensors);
  mCharSensorDistanceSensors->addDescriptor(new BLE2902());

  // Imu Sensor
  mCharSensorImuSensor = mServiceSensor->createCharacteristic(
                                         SENSOR_CHARACTERISTIC_IMU_SENSOR_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  mCharSensorImuSensor->setValue((uint8_t*)&value_sensor_imu_sensor, 18);
  BLEDescriptor *mDescSensorImuSensor = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescSensorImuSensor->setValue("Imu Sensor");  
  mCharSensorImuSensor->addDescriptor(mDescSensorImuSensor);
  mCharSensorImuSensor->addDescriptor(new BLE2902());
  

  // BatteryLevel
  mCharSensorBatteryLevel = mServiceSensor->createCharacteristic(
                                         SENSOR_CHARACTERISTIC_BATTERY_LEVEL_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  mCharSensorBatteryLevel->setValue(&value_sensor_battery_level, 1);
  BLEDescriptor *mDescSensorBatteryLeve = new BLEDescriptor((uint16_t)0x2901); // Characteristic User Description
  mDescSensorBatteryLeve->setValue("Battery Level");  
  mCharSensorBatteryLevel->addDescriptor(mDescSensorBatteryLeve);
  mCharSensorBatteryLevel->addDescriptor(new BLE2902());
  
  mServiceSensor->start();
  //************************************************

  

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  
  pAdvertising->addServiceUUID(MOTOR_SERVICE_UUID);
  pAdvertising->addServiceUUID(MISC_SERVICE_UUID);
  pAdvertising->addServiceUUID(SENSOR_SERVICE_UUID);
  
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("Ready.!!");

  edubot.begin(115200);  
}


void loop() {
  // put your main code here, to run repeatedly:
  delay(10);

  // Status LED
  status_led_count++;
  if(status_led_count > 50) {
    if(device_connected) {
      edubot.ledOff();

      if(status_text_displayed) {
        display_text = " 접속완료!";
        request_display_text = 2;   
        status_text_displayed = false;
      }
    }
    else {
      edubot.ledToggle();

      if(!status_text_displayed) {
        display_text = "접속대기 중!";
        request_display_text = 1; 
        status_text_displayed = true; 
      } 
    }
    status_led_count = 0;
  }


  if(request_display_text) {
    edubot.lcd.printf(16, (16*1), display_text.c_str());

    if (request_display_text == 1) {
      edubot.lcd.printf(18, (16*3), "%02X:%02X:%02X:%02X", ble_mac_addr[2], ble_mac_addr[3], ble_mac_addr[4], ble_mac_addr[5]);
    }
    edubot.lcd.display();
    edubot.lcd.clearDisplay();
    
    request_display_text = 0;
  }

  if(request_display_image) {

    edubot.lcd.drawBitmap((128-48)/2, (64-48)/2, &edubot_logo[display_image_index*48*48/8], 48, 48, 1);
    edubot.lcd.display();
    edubot.lcd.clearDisplay();

    request_display_image = 0;
  }

  if(request_motor_wait_result) {
    if(!edubot.motor.isBusy()) {
      request_motor_wait_result = 0;
      mCharMotorWaitResult->setValue(&value_motor_wait_result, 1);
      mCharMotorWaitResult->notify();
    }
  }


  // Button
  if(edubot.buttonGetPressed()) {
    if(device_connected) {
      if(digitalRead(0) != value_misc_button) {
        value_misc_button = 0;
        mCharMiscButton->setValue(&value_misc_button, 1);
        mCharMiscButton->notify();
      }
    }
    value_misc_button = 0;
  }
  else {
    value_misc_button = 1;
    mCharMiscButton->setValue(&value_misc_button, 1);
  }

  // Battery
  status_battery_count++;
  if(status_battery_count > 200) {
    
    value_sensor_battery_level = edubot.batteryGetVoltage();
    mCharSensorBatteryLevel->setValue(&value_sensor_battery_level, 1);

    if(device_connected && value_sensor_battery_level < 32) {
      mCharSensorBatteryLevel->notify();
    }
    status_battery_count = 0;
  }

  // Update Sensors
  status_update_sensors_count++;
  if(status_update_sensors_count > 5) {
    
    value_sensor_floor_sensors[0] = edubot.floor_sensor.getRightOut();
    value_sensor_floor_sensors[1] = edubot.floor_sensor.getRightIn();
    value_sensor_floor_sensors[2] = edubot.floor_sensor.getLeftIn();
    value_sensor_floor_sensors[3] = edubot.floor_sensor.getLeftOut();
    mCharSensorFloorSensors->setValue((uint8_t*)&value_sensor_floor_sensors, 4);

    value_sensor_distance_sensors[0] = edubot.tof_L.distance_mm;
    value_sensor_distance_sensors[1] = edubot.tof_R.distance_mm;
    mCharSensorDistanceSensors->setValue((uint8_t*)&value_sensor_distance_sensors, 4);

    value_sensor_imu_sensor[0] = (int16_t)(edubot.imu.getRoll()  * 100.0);
    value_sensor_imu_sensor[1] = (int16_t)(edubot.imu.getPitch() * 100.0);
    value_sensor_imu_sensor[2] = (int16_t)(edubot.imu.getYaw()   * 100.0);
    value_sensor_imu_sensor[3] = (int16_t)(edubot.imu.getAccX()  * 100.0);
    value_sensor_imu_sensor[4] = (int16_t)(edubot.imu.getAccY()  * 100.0);
    value_sensor_imu_sensor[5] = (int16_t)(edubot.imu.getAccZ()  * 100.0);
    value_sensor_imu_sensor[6] = (int16_t)(edubot.imu.getGyroX() * 100.0);
    value_sensor_imu_sensor[7] = (int16_t)(edubot.imu.getGyroY() * 100.0);
    value_sensor_imu_sensor[8] = (int16_t)(edubot.imu.getGyroZ() * 100.0);
    mCharSensorImuSensor->setValue((uint8_t*)&value_sensor_imu_sensor, 18);

    if(device_connected) {
      mCharSensorFloorSensors->notify(); 
      mCharSensorDistanceSensors->notify(); 
      mCharSensorImuSensor->notify();
    }

    status_update_sensors_count = 0;
  }
}
