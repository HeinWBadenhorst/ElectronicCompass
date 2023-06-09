/*
BM004_Arduino_compass_tilt_compensated:  This program reads the magnetometer and
accelermoter registers from ST Micro's LSM303DLHC.  The register values are used to generate 
a tilt compensated heading value.  NOTE:  placing the compass near metallic objects can impact
readings.

Schematics associated with the BM004 moudle may be used for hardware wiring information.
see www.solutions-cubed.com for additional information.

*/

#include <Wire.h>
#include <Math.h>
const int CLK = 9; //Set the CLK pin connection to the display
const int DIO = 8; //Set the DIO pin connection to the display


int NumStep = 0;  //Variable to interate


#include <TM1637Display.h>

//TM1637Display display(CLK, DIO);  //set up the 4-Digit Display.

    float Heading;
    float Pitch;
    float Roll;

    float Accx;
    float Accy;
    float Accz;

    float Magx;
    float Magy;
    float Magz;
    float Mag_minx;
    float Mag_miny;
    float Mag_minz;
    float Mag_maxx;
    float Mag_maxy;
    float Mag_maxz;

/*  
Send register address and the byte value you want to write the accelerometer and 
loads the destination register with the value you send
*/
void WriteAccRegister(byte data, byte regaddress)
{
    Wire.beginTransmission(0x19);   // Use accelerometer address for regs >=0x20
    Wire.write(regaddress);
    Wire.write(data);  
    Wire.endTransmission();     
}

/*  
Send register address to this function and it returns byte value
for the accelerometer register's contents 
*/
byte ReadAccRegister(byte regaddress)
{
    byte data;
    Wire.beginTransmission(0x19);   // Use accelerometer address for regs >=0x20  
    Wire.write(regaddress);
    Wire.endTransmission();

    delayMicroseconds(100);

    Wire.requestFrom(0x19,1);   // Use accelerometer address for regs >=0x20
    data = Wire.read();
    Wire.endTransmission();   

    delayMicroseconds(100);

    return data;  
}  

/*  
Send register address and the byte value you want to write the magnetometer and 
loads the destination register with the value you send
*/
void WriteMagRegister(byte data, byte regaddress)
{
    Wire.beginTransmission(0x1E);   // Else use magnetometer address
    Wire.write(regaddress);
    Wire.write(data);  
    Wire.endTransmission();     

    delayMicroseconds(100);
}

/*  
Send register address to this function and it returns byte value
for the magnetometer register's contents 
*/
byte ReadMagRegister(byte regaddress)
{
    byte data;
    Wire.beginTransmission(0x1E);   // Else use magnetometer address  
    Wire.write(regaddress);
    Wire.endTransmission();

    delayMicroseconds(100);

    Wire.requestFrom(0x1E,1);   // Else use magnetometer address
    data = Wire.read();
    Wire.endTransmission();   

    delayMicroseconds(100);

    return data;  
}  

void init_Compass(void)
{
    WriteAccRegister(0x67,0x20);  // Enable accelerometer, 200Hz data output

    WriteMagRegister(0x9c,0x00);  // Enable temperature sensor, 220Hz data output
    WriteMagRegister(0x20,0x01);  // set gain to +/-1.3Gauss
    WriteMagRegister(0x00,0x02);  // Enable magnetometer constant conversions
}

/*
Readsthe X,Y,Z axis values from the accelerometer and sends the values to the 
serial monitor.
*/
void get_Accelerometer(void)
{

  // accelerometer values
  byte xh = ReadAccRegister(0x29);
  byte xl = ReadAccRegister(0x28);
  byte yh = ReadAccRegister(0x2B);
  byte yl = ReadAccRegister(0x2A);
  byte zh = ReadAccRegister(0x2D);
  byte zl = ReadAccRegister(0x2C);

  // need to convert the register contents into a righ-justified 16 bit value
  Accx = (xh<<8|xl); 
  Accy = (yh<<8|yl); 
  Accz = (zh<<8|zl); 

}  

/*
Reads the X,Y,Z axis values from the magnetometer sends the values to the 
serial monitor.
*/
void get_Magnetometer(void)
{  
  // magnetometer values
  byte xh = ReadMagRegister(0x03);
  byte xl = ReadMagRegister(0x04);
  byte yh = ReadMagRegister(0x07);
  byte yl = ReadMagRegister(0x08);
  byte zh = ReadMagRegister(0x05);
  byte zl = ReadMagRegister(0x06);

  // convert registers to ints
  Magx = (xh<<8|xl); 
  Magy = (yh<<8|yl); 
  Magz = (zh<<8|zl); 
}  

/*
Converts values to a tilt compensated heading in degrees (0 to 360)
*/
void get_TiltHeading(void)
{
  // You can use BM004_Arduino_calibrate to measure max/min magnetometer values and plug them in here.  The values
  // below are for a specific sensor and will not match yours
  Mag_minx = -221;
  Mag_miny = -275;
  Mag_minz = -262;
  Mag_maxx = 396;
  Mag_maxy = 304;
  Mag_maxz = 439;

  // use calibration values to shift and scale magnetometer measurements
  Magx = (Magx-Mag_minx)/(Mag_maxx-Mag_minx)*2-1;  
  Magy = (Magy-Mag_miny)/(Mag_maxy-Mag_miny)*2-1;  
  Magz = (Magz-Mag_minz)/(Mag_maxz-Mag_minz)*2-1;  

  // Normalize acceleration measurements so they range from 0 to 1
  float accxnorm = Accx/sqrt(Accx*Accx+Accy*Accy+Accz*Accz);
  float accynorm = Accy/sqrt(Accx*Accx+Accy*Accy+Accz*Accz);

  // calculate pitch and roll
  Pitch = asin(-accxnorm);
  Roll = asin(accynorm/cos(Pitch));

  // tilt compensated magnetic sensor measurements
  float magxcomp = Magx*cos(Pitch)+Magz*sin(Pitch);
  float magycomp = Magx*sin(Roll)*sin(Pitch)+Magy*cos(Roll)-Magz*sin(Roll)*cos(Pitch);

  // arctangent of y/x converted to degrees
  Heading = 180*atan2(magycomp,magxcomp)/PI;

  if (Heading < 0)
      Heading +=360;

    Serial.print("Heading=");
    Serial.println(Heading);    
  }  

void setup() {
    Wire.begin();
    Serial.begin(9600);  // start serial for output
    init_Compass();
    //display.setBrightness(0x0a);  //set the diplay to maximum brightness
}

void loop() {
    get_Accelerometer();
    get_Magnetometer();
    get_TiltHeading();
    //display.showNumberDec(Heading); //Display the Variable value;
    delay(100);
}
