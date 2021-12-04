#include <ESP8266WiFi.h>
#include<FirebaseArduino.h>

// credentials
#define FIREBASE_HOST "water-flow-4ace3-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "W645aD6xOQZDWuqqSXwnR1DBlh92oB8xmfWWEN26"
#define WIFI_SSID "iBall-Baton" // input your home or public wifi name
#define WIFI_PASSWORD "January2022" //password of wifi

// room sensors pin initialization
#define RelayFan 14 //D5
int relFan;
int buttonState1 = 0;
int relLight;
int ledPin = 12; // choose pin for the LED
int inputPin = 13; // choose input pin (for Infrared sensor) 
int val = 0; // variable for reading the pin status

// ADXL335 Accelerometer
#define buzzer 12 // buzzer pin
#define x A0 // x_out pin of Accelerometer
#define y A1 // y_out pin of Accelerometer
#define z A2 // z_out pin of Accelerometer
/*variables*/
int xsample=0;
int ysample=0;
int zsample=0;
long start;
int buz=0;
/*Macros*/
#define samples 50
#define maxVal 20 // max change limit
#define minVal -20 // min change limit
#define buzTime 5000 // buzzer on time

// relay pin initialization
#define Relay 14 //D5
int rel;
int buttonState = 0;

// Flow meter pin initialization
#define SENSOR  2 //D4
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 65   ;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;
String Resivedata ;
 
void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}
void setup()
{
  Serial.begin(115200);
  //relay Pin input
  pinMode(Relay,OUTPUT);
  digitalWrite(Relay,HIGH);
  
  //room sensors input and output
  pinMode(ledPin, OUTPUT); 
  pinMode(inputPin, INPUT); 
  pinMode(RelayFan,OUTPUT);
  digitalWrite(RelayFan,HIGH);

  // Flow meter input
  pinMode(SENSOR, INPUT_PULLUP);
 
  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;
                                                                                                                            
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING); 
  
  WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status()!=WL_CONNECTED){
  Serial.print(".");
  delay(500);
  }
  Serial.println();
  Serial.print("connected:");
  Serial.println(WiFi.localIP());
  //Firebase initialisation
  Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
  //Firebase Variable Setup
  Firebase.setInt("Switch",0);
  Firebase.setInt("SwitchLight",0);
  Firebase.setInt("SwitchFan",0);

  delay(1000);
  pinMode(buzzer, OUTPUT);
  buz=0;
  digitalWrite(buzzer, buz);
  for(int i=0;i<samples;i++) // taking samples for calibration
  {
  xsample+=analogRead(x);
  ysample+=analogRead(y);
  zsample+=analogRead(z);
  }
 
  xsample/=samples; // taking avg for x
  ysample/=samples; // taking avg for y
  zsample/=samples; // taking avg for z
}
//firebase reconnect function
void firebasereconnect()
{
Serial.println("Trying to reconnect");
Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}
void loop(){
  if (Firebase.failed()){
    Serial.print("setting number failed:");
    Serial.println(Firebase.error());
    return;
 }

  //room controls code
  relLight=Firebase.getString("SwitchLight").toInt();
  if(relLight==1){
    digitalWrite(ledPin,HIGH);
  }
  else{
    digitalWrite(ledPin,LOW);
  }
  // RelayFan code
    relFan=Firebase.getString("SwitchFan").toInt();
    if(relFan==1)
    // If, the Status is 1, turn on the RelayFan
    {
    digitalWrite(RelayFan,LOW);
    Serial.println("RelayFan ON");
    }
    if(relFan==0)
    // If, the Status is 0, turn Off the RelayFan
    {
    digitalWrite(RelayFan,HIGH);
    Serial.println("RelayFan OFF");
    }
    
  val = digitalRead(inputPin); // read input value 
   if (val == HIGH)
   { // check if the input is HIGH
      digitalWrite(ledPin, HIGH); // turn LED OFF
      Firebase.setString("RoomVacancy","NO");  
   } 
   else 
   { 
      digitalWrite(ledPin, LOW); // turn LED ON 
      Firebase.setString("RoomVacancy","YES"); 
   } 
   
  // Relay code
    rel=Firebase.getString("Switch").toInt();
    if(rel==1)
    // If, the Status is 1, turn on the Relay
    {
    digitalWrite(Relay,LOW);
    Serial.println("Relay ON");
    }
    if(rel==0)
    // If, the Status is 0, turn Off the Relay
    {
    digitalWrite(Relay,HIGH);
    Serial.println("Relay OFF");
    }
  
    // FlowMeter Code
    currentMillis = millis();
    if (currentMillis - previousMillis > interval) 
    {
    
    pulse1Sec = pulseCount;
    pulseCount = 0;
 
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
 
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    flowLitres = (flowRate / 60);
 
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    totalLitres += flowLitres;
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(float(flowRate));  // Print the integer part of the variable
    Serial.print("L/min");
    Serial.print("\t");       // Print tab space
    Serial.println();
    
    Firebase.setFloat("Volume",totalLitres);
    Firebase.setInt("Volume in mL",totalMilliLitres);

    //Limit initialization
    Resivedata = Firebase.getString("Limit");
    Serial.println(totalMilliLitres);
    String data=Resivedata.substring(1,sizeof(Resivedata)-1);
    Serial.println(data.toInt());
    if (totalMilliLitres==data.toInt()){
        Firebase.setInt("Switch",0);
    }

    // Earthquake Code
    int value1=analogRead(x); // reading x out
    int value2=analogRead(y); //reading y out
    int value3=analogRead(z); //reading z out
 
    int xValue=xsample-value1; // finding change in x
    int yValue=ysample-value2; // finding change in y
    int zValue=zsample-value3; // finding change in z
 
    /*displying change in x,y and z axis values over firebase*/
    Firebase.setInt("buzXvalue",xValue);
    Firebase.setInt("buzYvalue",yValue);
    Firebase.setInt("buzZvalue",zValue);
    delay(100);
 
    /* comparing change with predefined limits*/
    if(xValue < minVal || xValue > maxVal || yValue < minVal || yValue > maxVal || zValue < minVal || zValue > maxVal)
    {
    if(buz == 0)
    Firebase.setInt("buz",buz);
    start=millis(); // timer start
    buz=1; // buzzer / led flag activated
    }
 
    else if(buz == 1) // buzzer flag activated then alerting earthquake
    {
    Firebase.setInt("buz",buz);
    if(millis()>= start+buzTime)
    buz=0;
    }
 
    else
    {
    Firebase.setInt("buz",buz);
    }
 
    digitalWrite(buzzer, buz); // buzzer on and off command
    
    // handle error
    if (Firebase.failed()) {
    Serial.println(Firebase.error());
    return;
  }
 }
}
