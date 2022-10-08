/*
 * Project name: physics toy project laser control and communication system
 * Designer: Yuchen Lin
 * Teacher: Mr. McCormack
 * Date: 2022/5
 * Design principle: the lower level function consist an upper level
 */


/* importing Servo control library */
#include <Servo.h>


/* Servo motor register */
Servo _servoMotor;


/* global var sign up */

//fixed
    //pin layout
        int _servoMotorPin = 10;//~
        int _laserTransistorPin = 11;//~
        int _photoResistance1 = A4;//analog
        int _photoResistance2 = A5;//analog

//non-fixed
    //user-input
        String _userInput;//max is 64 char
        byte _keyArrayBit[8];
        byte _cipherArrayByte[576];//after encrypting the byte array
        int _cipherArrayByteSizeUsed = 0;
        int _cipherArrayBitSizeUsed = 0;//number count
        uint8_t _fixedSensorPin;// the sensor to foucs on 
        // when the second user give an input evaluate is it correct if incorrect halt and report mission failure been discovered
        
        byte _keyArrayBitInput[8];//user B return the key value they get from kevin's toy
        byte _cipherArrayBitRecieving[512];
        int _cipherArrayBitRecievingSizeUsed = 0;
        char _cipherArrayCharFromSensor[64];//the binary will be displayed immediately and then stored as a char
        int _cipherArrayByteFromSensorSizeUsed = 0;//number count
        String _userInputReceived;


//enviornment control recording
int _laserBrightness = 0;
int _motorAddingMax = 0;
int _motorSubtractingMax = 0;
int _motorFinalCheck = 0;
int _motorPosition = 0;

/* -------initial code has a dynamic dispatch part------been removed run out of storage------*/
int _dynamicBytePointer = 0;
byte _cache[8];
int _cacheRecieving[8];

/* arduino execution code*/

//register the pin layout
void setup() {
    Serial.begin(9600);//refresh rate on computer
    _servoMotor.attach(_servoMotorPin);
    pinMode(_laserTransistorPin, OUTPUT);
    pinMode(_photoResistance1, INPUT);
    pinMode(_photoResistance2, INPUT); 
}

//the uppermost layer to execute in loop; directly present to user
void loop() {
  InputGet();
  EncryptByte();
  WaitForUser();
  LocateLaser();
  DataSending();
  DataDecrypt();
  println("Press reset button on the arduino board to restart");
  Delay(100);
}


/*-------------- lowest type of functions--------------*/

//input ask
int AskIntInput() {

  Serial.setTimeout(5000);//respond in 5 seconds
  println("please enter the number");
  CallNewLine();
  println("**Session will be out of time in 5 seconds");
  CallNewLine();
  println("please enter the number");
  //buffer read
  while (Serial.available() == 0) {
    
  }
  //return the buffer and parse into int
  int user_input = Serial.parseInt();
  CallNewLine();
  return user_input;
} 

//extra layer protection of only 1 and 0 can be accessed
byte AskBitEncryptInput() {
  byte pending = 0;
  while (1) {
    pending = AskIntInput();
    if (pending == 0 || pending == 1) {
      break;
    }
  }
  return pending;
}

String AskStringInput() {
  Serial.setTimeout(10000);//session will be out in 10 seconds
  println("please enter the String");
  CallNewLine();
  println("Session will be out of time in 10 seconds");
  //buffer read
  while (Serial.available() == 0) {
    
  }
  //parse buffer into string using ascii full version
  String user_input = Serial.readString();
  CallNewLine();
  return (user_input);
}

//dangling reference avoidance system
void DanglingReferenceCheck(int current_pointer_location, int pointer_limit) {
    if (pointer_limit < current_pointer_location) {
        println("Dangling Reference");
        CallNewLine();
        ProgramHalt();
    }
}

//break the execution flow of the program
void ProgramHalt() {
    println("program halt!");
    for(;;) {

    }
}

//byte to bit conversion
void SingleByteConversion(byte value) {
   int i = 0;
   for(int exponent = 7; exponent >= 0; exponent--) {
      DanglingReferenceCheck(i, 7);
      int bits_value = power(2, exponent);
      
      if (value >= bits_value) {
        _cache[i] = 1;
        value = value - bits_value;
      } else {
        _cache[i] = 0;
      }
      i++;
   }
}

//should set pointer_location as global to keep track
void CacheByteRest(int pointer_location) {
  int i = 0;
  int _copyPointerLocation = pointer_location;
  for (; pointer_location < _copyPointerLocation + 8; pointer_location++) {
    DanglingReferenceCheck(i, 8);
    _cipherArrayByte[pointer_location] = _cache[i];
    Serial.print(_cache[i]);
    _cache[i] = 0;
    i++;
  }
  CallNewLine();
}

//special case for the byte conversion space saver feature
void CacheByteRestSlient() {
  for (int i = 0; i < i + 8; i++) {
    _cache[i] = 0;
  }
}

//bits to byte conversion// the function is used with a cache 8 byte //the data here is after the decryption//cache recieving is specifically design for after recieving translation
int BitToByteConversion() {
  println("calculating:");
  CallNewLine();
  int char_code = 0;
  int count = 0;
  for(int i = 7; i > 0; i--) {
    Serial.print(_cacheRecieving[i]);
    CallNewLine();
    if (_cacheRecieving[i] == 1) {
      char_code = char_code + power(2, count);
    }
    count++;
  }
  Serial.print(char_code);
  CallNewLine();
  return(char_code);
}

//int to char appending //char code is given by the previous code
void IntAppend (int pointer_location, int char_code) {
  DanglingReferenceCheck(pointer_location, 64);
  _cipherArrayCharFromSensor[pointer_location] = char_code;
}

//bit tracker // the function is been called after the bit is safely copied
int BitTracker(int current_pointer) {
  return(current_pointer + 8);
}

int CharTracker(int current_pointer) {
  return(current_pointer + 1);
}

void DynamicBytePointerTracker () {
  if (_dynamicBytePointer == 0) {
    println("Now INIT the _dynamicBytePointer.");
    CallNewLine();
    _dynamicBytePointer = _cipherArrayByteSizeUsed;
  } else {
    _dynamicBytePointer = BitTracker(_dynamicBytePointer);
  }
}

//when the bit is existly the 8th bit  the function will return 0 remind to clear everything up before continue else will be 1
int BitsCorrespond(int number_index) {
  if (number_index == 0) {
    return(0);
  }
  number_index = number_index + 1;
  number_index = number_index % 8;
  if (number_index == 0) {
    return(0); 
  } else {
    return(1);
  }
  
}

int BitsCorrespondSimple(int number_index) {
  if (number_index == 0) {
    return(0);
  }
  return(number_index % 8);
  
}

//calculation
//power has an accuracy problem
int power(int base, int exponent) {
  int value = 1;
  if (base == 0) {
    return (0);
  } else {
    for(int i = 0; i < exponent; i ++) {
      value = value * base; 
    }
    return (value);
  }
}

//byte calculation
byte BitsOperationForward(byte bits, byte key_bits) {
  byte cipher = bits + key_bits;
  if (cipher  == 2) {
    cipher = 0;
  } else if (bits > 2) {
    println("Error in the program");
    ProgramHalt();
  } else {
  }
  return cipher;
}

byte BitsOperationBackward(byte bits, byte key_bits) {
  byte message = 0;
  if (bits == 0 && key_bits == 1) {
    message = 1;
  } else {
    message = bits - key_bits;
  }
  return message; 
}

//debug trait
void CallNewLine() {
  println("\n");
}

void println(String long_char) {
    Serial.print(long_char);
}

void SequenceOneToEightBit(int index) {
  println("Please enter the ");
  switch (index) {
    case 0:
      println("first ");
      break;
    case 1:
      println("second ");
      break;
    case 2:
      println("third ");
      break;
    case 3:
      println("forth ");
      break;
    case 4:
      println("fifth ");
      break;
    case 5:
      println("sixth ");
      break;
    case 6:
      println("seventh ");
      break;
    case 7:
      println("eighth ");
      break;
  }
  println("digits of the key.");
  println("(p.s: the default is the 0 if invalid attempt are detected");
  CallNewLine();
}

void SerialDisplay(String description, int value) {
//in the same scope calling for different type is invalid
  Serial.print(description);
  Serial.print(value);
  println("\n");
}

//timer control in seconds which one sec is delay(1000);
    //for this function just Delay(1);
void Delay(float delay_timer) {
  delay_timer = delay_timer * 1000;
  delay(delay_timer);
  delay_timer = 0;
}

//laser brightness controlling system
void LaserBrightness(uint8_t port, int analog_val) {
  analogWrite(port, analog_val);
  _laserBrightness = analog_val;
}

//get the sensor read
int SensorRead(uint8_t port) {
  Delay(0.06);
  return(analogRead(port));
}

//sensor read combined with motor turning //if mode == 0 then ++ else --
//function return the ideal max point they found
int MotorMotion(uint8_t port, int from_angle, int to_angle, int mode, float times) {
  int current_value = 0;
  int current_value_sec = 0;// the second sample taken
  int highest_value = 0;
  int correspond_angle = 0;
  LaserBrightness(_laserTransistorPin, 255);
  if(mode == 0) {
    
    for(; from_angle < to_angle; from_angle++) {
      _servoMotor.write(from_angle);
      current_value = SensorRead(port);
      current_value_sec = SensorRead(port);
      if (current_value > highest_value && current_value_sec > highest_value) { 
        if (current_value >= 700) {
          correspond_angle = from_angle;
          highest_value = (current_value+current_value_sec) / 2;
        }
      } else {
        //println("invalid sensor input");
      }
      Delay(times);
    }
  } else if(mode == 1) {
    for(; from_angle > to_angle; from_angle--) {
      _servoMotor.write(from_angle);
      current_value = SensorRead(port);
      current_value_sec = SensorRead(port);
      if (current_value > highest_value && current_value_sec > highest_value) { 
        if (current_value >= 700) {
          correspond_angle = from_angle;
          highest_value = (current_value+current_value_sec) / 2;
        }
      } else {
        //println("invalid sensor input");
      }
      Delay(times);
    }
  } else {
    println("invalid mode");
    ProgramHalt();
  }
  LaserBrightness(_laserTransistorPin, 0);
  _motorPosition = from_angle;
  return(correspond_angle);
}

//the value is read from the _cache
void ByteSend(uint8_t port, byte current, int i) {
    long time = 0;
    long now_time = 0;
    int count = 0;
    if (current == 0) {
      LaserBrightness(_laserTransistorPin, 255);
      time = micros();
      for (;;) {
        SensorRead(port);
        now_time = micros();
        if (now_time - time > 500000) {
          println("the sensor recieves ");
          Serial.print(i);
          println(" times of laser under 9600 buad");
          _cacheRecieving[i] = 0;
          break;
        }
        i++;
      }
      // to make the data looks more realiable add a normal range for the number of i will be for a minute e.g in theory 960 so the range is from 900 to 1000
    } else {
      LaserBrightness(_laserTransistorPin, 255);
      time = micros();
      for (;;) {
        SensorRead(port);
        now_time = micros();
        if (now_time - time > 1500000) {
          println("the sensor recieves ");
          Serial.print(i);
          println(" times of laser under 9600 buad");
          _cacheRecieving[i] = 1;
          break;
        }
      // to make the data looks more realiable add a normal range for the number of i will be for a minute e.g in theory 1920 so the range is from 1860 to 2000
    }
  }
  println(" => ");
  Serial.print(_cacheRecieving[i]);
  CallNewLine();
  LaserBrightness(_laserTransistorPin, 0);
  Delay(0.5);
}

//wait for user to catch the ball
void WaitingOnly() {
  String unlock_string = "i get the key.";//to the index 12 which the 13 number
  byte unlock_byte[13];
  unlock_string.getBytes(unlock_byte, 13);
  for (;;) {
    CallNewLine();
    println("Please type in ");
    Serial.print(unlock_string);
    println(" to enter the following section.");
    CallNewLine();
    String user_input = AskStringInput();
    byte parse_byte[13];
    user_input.getBytes(parse_byte, 13);
    for(int i = 0; i < 13; i++) {
      int current_byte_user = parse_byte[i];
      if (current_byte_user > 64 && current_byte_user < 91) {
        current_byte_user = current_byte_user + 32;
        Serial.print("add 32");
      }
      Serial.print(current_byte_user);
      Serial.print(",");
      Serial.print(unlock_byte[i]);
      if (current_byte_user == unlock_byte[i]) {
        break;
      }
    }
  }
  //can add a limit sensor on the robot therefore if the robot actually catch the ball it will terminate the loop for the following code as well
}

//not config yet
/*------------higher level control system---------*/
void InputGet() {
  println("Hi. Welcome to the laser project made by Kevin and YuchenLin.");
  CallNewLine();
  println("Our project illustrate the projectile motion, and the potential of using a laser as a pointer and a commuicator.");
  CallNewLine();
  println("Please the first user enter the message you want to send to user b.");
  _userInput = AskStringInput();
  println("You just entered: ");
  println(_userInput);
  CallNewLine();
  _cipherArrayByteSizeUsed = _userInput.length();
  Serial.print(_cipherArrayByteSizeUsed);
  println(" bytes are being occupied");
  CallNewLine();
  _userInput.getBytes(_cipherArrayByte, _cipherArrayByteSizeUsed);
  println("when converting the chars array, the String you just enter will look like following: ");
  CallNewLine();
  for(int i = 0; i < _cipherArrayByteSizeUsed; i++) {
    Serial.print(_cipherArrayByte[i]);
    CallNewLine();
  }
  println("please enter the key for the computer to encrypt the code.");
  for (int i = 0; i< 8; i++) {
    SequenceOneToEightBit(i);
    _keyArrayBit[i] = AskBitEncryptInput();
  }
  println("the encrypting code is: ");
  for(int i = 0; i < 8; i++) {
    Serial.print(_keyArrayBit[i]); 
  }
  Serial.print(".");
  CallNewLine();
  DynamicBytePointerTracker();
}

void EncryptByte() {
  _cipherArrayBitSizeUsed = _cipherArrayByteSizeUsed * 8 + _cipherArrayByteSizeUsed;
  //Serial.print(_cipherArrayBitSizeUsed);
  println("The following illustrate how to convert a byte to the bit we need.");
  CallNewLine();
  for(int i = 0; i < _cipherArrayByteSizeUsed; i++) {
    SingleByteConversion(_cipherArrayByte[i]);
    CacheByteRest(_dynamicBytePointer);
    DynamicBytePointerTracker();
  }
  println("After the encryption process the bit will be the following: ");
  CallNewLine();
  for(int i = _cipherArrayByteSizeUsed; i < _cipherArrayBitSizeUsed; i++) {
    int key_bit_info = BitsCorrespondSimple(i - _cipherArrayByteSizeUsed);
    _cipherArrayByte[i] = BitsOperationForward(_cipherArrayByte[i], _keyArrayBit[key_bit_info]);
    if (key_bit_info == 0) {
      CallNewLine(); 
    }
    Serial.print(_cipherArrayByte[i]);
  }
}

//for turning only first turn fast then second turn then third turn
// in the end if( _last_term != current_position) {
// value to turn = current_positon - last_term;  see if + use ++ else use --

void WaitForUser() {
  CallNewLine();
  println("The last thing to do for user A is to decide a port which you and user b are going to communicate.");
  CallNewLine();
  println("Type in '0' if you decide to use the first pin and Type in '1' if you decide to use the second pin");
  CallNewLine();
  println("Any invalid input will be set as using the default pin 0"); 
  CallNewLine();
  if (AskBitEncryptInput() == 1) {
    _fixedSensorPin = _photoResistance2;
    println("The pin you choose is ");
    Serial.print("A5");
  } else {
    _fixedSensorPin = _photoResistance1;
    println("The pin you choose is ");
    Serial.print("A4");
  }
  println(".");
  CallNewLine();
  for(;;) {
    CallNewLine();
    println("User B. please type in number 1 to unlock the program");
    CallNewLine();
    if (AskBitEncryptInput() == 1) {
      break; 
    }
  }
}

void LocateLaser() {
  println("Hi User B. Please specify the sensor that user A uses.");
  CallNewLine();
  println("The wrong input will cause your data leak to your enemy; thus failling the game.");
  CallNewLine();
  println("Please enter the pin. type '1' for A5 or type '0' for A4. Default is pin A4");
  CallNewLine();
  if (AskBitEncryptInput() == 1) {
    if (_fixedSensorPin == _photoResistance2) {
      println("Sensor Matches");
      CallNewLine();
    } else {
      for (int i = 0; i < 100; i++) {
        println("!");
        CallNewLine();
      }
      println("You have been detected by your enemy. Mission failure.");
      ProgramHalt();
    }
  } else {
    if (_fixedSensorPin == _photoResistance1) {
      println("Sensor Matches");
      CallNewLine();
    } else {
      for (int i = 0; i < 100; i++) {
        println("!");
        CallNewLine();
      }
      println("You have been detected by your enemy. Mission failure.");
      ProgramHalt();
    }
  }
  println("Now the motor is turning the sensor to find the sensor requested.");
  CallNewLine();
  _motorAddingMax = MotorMotion(_fixedSensorPin, _motorPosition, 180, 0, 0.02);
  _motorSubtractingMax = MotorMotion(_fixedSensorPin, _motorPosition, 0, 1, 0.02);
  println("The range of the sensor is: ");
  Serial.print(_motorAddingMax);
  println(" ~ ");
  Serial.print(_motorSubtractingMax);
  println(".");
  CallNewLine();
  if(_motorAddingMax != 0 && _motorSubtractingMax != 0) {
    for(;_motorPosition < floor((_motorAddingMax + _motorSubtractingMax)/2) + 1; _motorPosition++) {
      _servoMotor.write(_motorPosition);
      Delay(0.05);
    }
  }
  else if(_motorAddingMax != 0) {
    for(;_motorPosition < _motorAddingMax; _motorPosition++) {
      _servoMotor.write(_motorPosition);
      Delay(0.05);
    }
  } else if(_motorSubtractingMax != 0) {
    for(;_motorPosition < _motorSubtractingMax; _motorPosition++) {
      _servoMotor.write(_motorPosition);
      Delay(0.05);
    }
  } else {
    println("detection error");
    ProgramHalt();
  }
  
}

void DataSending() {
  _dynamicBytePointer = _cipherArrayByteSizeUsed;
  int i = 0;
  int number_record = 0;
  for(; _dynamicBytePointer < _cipherArrayBitSizeUsed; _dynamicBytePointer++) {
    //Serial.print(_cipherArrayByte[_dynamicBytePointer]);
    //CallNewLine();
    //Serial.print(BitsCorrespondSimple(i));
    ByteSend(_fixedSensorPin, _cipherArrayByte[_dynamicBytePointer], BitsCorrespondSimple(i));
    i++;
    if (i == 8) {
      println("_cacheRecieveingCount:");
      for (int count = 0; count < 8; count++) {
        /*Serial.print(number_record);
        println(";");
        Serial.print(count);
        println(";");*/
        Serial.print(_cacheRecieving[count]);
        //println(";");
        _cipherArrayBitRecieving[number_record] = _cacheRecieving[count];
        
        /*Serial.print(_cipherArrayBitRecieving[number_record]);
        println(";");
        Serial.print(count);
        println(";");*/
        _cacheRecieving[count] = 0;
        number_record++;
      }
      CallNewLine();
      i = 0;
    }
    //println("The data we recieved is :");
  CallNewLine();
  /*for (int i = 0; i < _cipherArrayBitRecievingSizeUsed; i++){
    //Serial.print(i);
    //println(";");
    Serial.print(_cipherArrayBitRecieving[i]);
    //println(";");
    if (BitsCorrespond(i) == 0 && i != 0) {
      CallNewLine();
    }
  }*/
    _cipherArrayBitRecievingSizeUsed = number_record;
  }

  println("The data we recieved is :");
  CallNewLine();
  println("After the encryption process the bit will be the following: ");
  CallNewLine();
  for(int i = _cipherArrayByteSizeUsed; i < _cipherArrayBitSizeUsed; i++) {
    int key_bit_info = BitsCorrespondSimple(i - _cipherArrayByteSizeUsed);
    if (key_bit_info == 0) {
      CallNewLine(); 
    }
    Serial.print(_cipherArrayByte[i]);
  }
  CallNewLine();
}

void DataDecrypt() {
  int counts = 0;
  //ask user for key bit input then decrypt then translate to byte //ask port before turning 
  for(int i = _cipherArrayByteSizeUsed; i < _cipherArrayBitSizeUsed; i++) {
    int key_bit_info = BitsCorrespondSimple(i - _cipherArrayByteSizeUsed);
    _cipherArrayBitRecieving[counts] = BitsOperationBackward(_cipherArrayByte[i], _keyArrayBit[key_bit_info]);
    counts++;
  }
  println("After the dycrpting process, the bits look like the following");
  CallNewLine();
  for(int i = 0; i < _cipherArrayBitRecievingSizeUsed; i++) {
    Serial.print(_cipherArrayBitRecieving[i]);
    if (BitsCorrespond(i) == 0 && i != 0) {
      CallNewLine();
    }
  }

  println("When prasing the value back to byte we get:");
  CallNewLine();
  int count = 0;
  for(int i = 0; i < _cipherArrayBitRecievingSizeUsed; i++) {
    _cacheRecieving[BitsCorrespondSimple(i)] = _cipherArrayBitRecieving[i];
    if (BitsCorrespond(i) == 0 && i != 0) {
      for(int iz = 0; iz < 8; iz++) {
        Serial.print(_cacheRecieving[iz]);
      }
      CallNewLine();
      _cipherArrayCharFromSensor[count] = BitToByteConversion();
      count ++;
    }
  }
  CallNewLine();
  println("Finally userB you recieve:");
  CallNewLine();
  println("After decrypting and converting the data turn back into ASCII code that we familiar with");
  CallNewLine();
  CallNewLine();
  CallNewLine();
  for(int i = 0; i < count; i++) {
    Serial.print(_cipherArrayCharFromSensor[i]);
  }
  CallNewLine();
}
