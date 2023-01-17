
/* -------DIALYSIS MONITORING SYSTEM V1----------- 
 * Author:
 * Muhammed Shibli K
 * mshibili01@gmail.com
 * Find the repository : https://github.com/mshibili/Dialisys-Monitoring-System
 * 
 *  Monitoring Parameters
 * ~~~~~~~~~~~~~~~~~~~~~~~
 *    1. Patient blood temperature.
 *    2. The volume of blood flows through the inlet chamber of dialysis.
 *    3. Blood outflow from the dialysis chamber
 *    4. Bubble formation in blood flow tubes.
 * 
 * Pin Configuration
 * ~~~~~~~~~~~~~~~~~
 *  GSM/GPRS module on 12,3
 *  Buzzer module on D11
 *  IR sensor at A3
 *  Flow sensor at A2
 *  Temperature sensor at A0
 *
 */

#include <LiquidCrystal.h>
LiquidCrystal lcd(4, 5, 6, 7, 8, 9);
#include <SoftwareSerial.h>
#include <WString.h>
SoftwareSerial mySerial(12, 3);

const int DIA_RELAY = 10;       // Relay Pin
const int BUZZER = 11;          // Buzzer Pin
const int BUBBLE = A3;          // IR sensor input Analog pin
const int TEMP = A0;
const int flowsensor = A2;

volatile int flow_frequency;    // Measures flow sensor pulses

//  Calculated litres/hour
float vol = 0.0, l_minute;
unsigned long currentTime;      // Variables to calc. interval
unsigned long cloopTime;

int val = 0;
int TEMPERATURE = 0;
int count = 0;

void flow()                     // Interrupt function
{
  flow_frequency++;             // At each rising edge, the interrupt is called and flow frequency value is updated.
}

void setup()
{
  // -------------Pin_mode Initialization-------------
  Serial.begin(9600);           // Serial for debug Info
  mySerial.begin(9600);         // Serial port for GSM 
  lcd.begin(16, 2);
  pinMode(BUZZER, OUTPUT);
  pinMode(DIA_RELAY, OUTPUT);
  pinMode(BUBBLE, INPUT);
  digitalWrite(BUZZER, LOW);
  digitalWrite(DIA_RELAY, LOW);

  //--------------LCD Intial setup-------------------
  lcd.setCursor(0, 0);
  lcd.print("IoTBASD DIALYSIS");
  lcd.setCursor(0, 1);
  lcd.print("MONITORING SYSTM");
  delay(5000);
  lcd.setCursor(0, 0);
  lcd.print(" BOOTING MODEM  ");
  lcd.setCursor(0, 1);
  lcd.print(" PLEASE WAIT . .");
  delay(7000);
  lcd.setCursor(0, 0);
  lcd.print("  MODEM READY!  ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  delay(2000);
  lcd.setCursor(0, 0);
  lcd.print("DIALYSIS PROCESS");
  lcd.setCursor(0, 1);
  lcd.print("    STARTED!!   ");
  digitalWrite(DIA_RELAY, HIGH);
  delay(2000);

  // ----------------Flow sensor Setup---------------------

  pinMode(flowsensor, INPUT);
  // digitalWrite(flowsensor, HIGH);                                    // Optional Internal Pull-Up
  attachInterrupt(digitalPinToInterrupt(flowsensor), flow, RISING);     // Setup Interrupt
  currentTime = millis();
  cloopTime = currentTime;

  lcd.clear();
}

void loop()
{

  count += 1;
  if (count == 1000)
  {
    count = 0;
    pingserver();
  }

  currentTime = millis();
  // Every second, calculate and print litres/hour
  if (currentTime >= (cloopTime + 1000))
  {
    cloopTime = currentTime;                    // Updates cloopTime
    if (flow_frequency != 0)
    {
                                                // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/Hour
      l_minute = (flow_frequency / 7.5);        // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/min
      // lcd.clear();
      lcd.setCursor(14, 1);
      //  lcd.print("Rate: ");
      lcd.print(l_minute);
      /*
      lcd.print(" L/M");
      l_minute = l_minute/60;
      lcd.setCursor(0,1);
      vol = vol +l_minute;
      lcd.print("Vol:");
      lcd.print(vol);
      lcd.print(" L");
      */
      flow_frequency = 0;                       // Reset Counter
      // Serial.print(l_minute, DEC);           // Print litres/hour
      // Serial.println(" L/Sec");
    }
    else
    {
      // Serial.println(" flow rate = 0 ");
      // lcd.clear();
      lcd.setCursor(14, 1);
      // lcd.print("Rate: ");
      lcd.print(flow_frequency);
      lcd.print(" L/M");
      /*
      lcd.setCursor(0,1);
      lcd.print("Vol:");
      lcd.print(vol);
      lcd.print(" L");
      */
    }
  }

// -----------Temperature Calculation--------------
 
  val = analogRead(TEMP);               // Collect the ADC input val  
  float mv = (val / 1024.0) * 5000;   // mV = (Value/ADC_resolution) * 5V
  TEMPERATURE = mv / 10;              // Calculte temp

  lcd.setCursor(0, 0);
  lcd.print("TEMPERATURE:");          
  lcd.print(TEMPERATURE);
  lcd.print("'F  ");
 
  if (TEMPERATURE > 100)              // If the temperature value is Higher than desired
  {
    lcd.setCursor(0, 0);
    lcd.print("ALERT!!HIGH TEMP");
    lcd.setCursor(0, 1);
    lcd.print("VALUE DETECTED!!");
    digitalWrite(BUZZER, HIGH);       // Trigger buzzer
    delay(3000);
    lcd.setCursor(0, 0);
    lcd.print("DIALYSIS PROCESS");
    lcd.setCursor(0, 1);
    lcd.print("  TERMINATED!!! ");
    digitalWrite(DIA_RELAY, LOW);     // Terminate the dialysis process by cutting down relay signal
    sendtempsms();                    // Trigger temperature alert msg
    digitalWrite(BUZZER, LOW);      
    delay(3000);
    lcd.clear();
  }


  if (digitalRead(BUBBLE) == LOW)     // If bubble sensor output LOW pulse
  {
    lcd.setCursor(0, 1);
    lcd.print("BUBBLE:NO");
  }
  lcd.print(" FLW:");

  if (digitalRead(BUBBLE) == HIGH)    // If bubble sensor output HIGH pulse
  {
    lcd.setCursor(0, 0);
    lcd.print("ALERT! AIRBUBBLE");
    lcd.setCursor(0, 1);
    lcd.print("   DETECTED!!!  ");
    digitalWrite(BUZZER, HIGH);       // Trigger Buzzer
    delay(3000);
    lcd.setCursor(0, 0);
    lcd.print("DIALYSIS PROCESS");
    lcd.setCursor(0, 1);
    lcd.print("  TERMINATED!!! ");
    digitalWrite(DIA_RELAY, LOW);     // Terminate the dialysis process cutting the relay signal

    sendbubblesms();                  // Trigger the bubble detected msg
    digitalWrite(BUZZER, LOW);
    delay(3000);
    lcd.clear();
  }


  delay(10);
}

//----------Temperature Alert MSG-----------
void sendtempsms()
{
  lcd.setCursor(0, 0);
  lcd.print("HIGH TEMPERATURE");
  lcd.setCursor(0, 1);
  lcd.print(" SMS SENDING . .");

  delay(2000);

  lcd.setCursor(0, 0);
  lcd.print(" SMS SENDING TO ");
  lcd.setCursor(0, 1);
  lcd.print("REGISTERED PHNUM");
  delay(2000);
  mySerial.println("AT");                       // Intitilise AT command
  mySerial.write(13);
  mySerial.write(10);
  delay(1000);
  mySerial.println("AT+CMGF=1");                // Selecting the Operating Mode PDU ,CMGF (command name in text: Message Format) 
  mySerial.write(13);
  mySerial.write(10);
  delay(1000);
  mySerial.println("AT+CMGS=\"9847229676\"");   // AT+CMGS command sends an SMS message to a GSM phone.
  mySerial.write(13);
  mySerial.write(10);
  delay(1000);
  mySerial.println("ALERT!! HIGH TEMPERATURE DETECTED IN DIALYSIS PROCESS");
  mySerial.println("DIALYSIS INTERRUPTED");
  mySerial.println("https://iot-project24.000webhostapp.com/dia.html");
  mySerial.write(26);
  mySerial.write(10);
  delay(7000);
  lcd.setCursor(0, 0);
  lcd.print("    SMS SENT    ");
  lcd.setCursor(0, 1);
  lcd.print(" SUCCESSFULLY...");
  delay(3000);
}

//-----------Bubble Alert SMS------------------

void sendbubblesms()
{
  lcd.setCursor(0, 0);
  lcd.print("AIRBUBBLE DETCTD");
  lcd.setCursor(0, 1);
  lcd.print(" SMS SENDING . .");

  delay(2000);

  lcd.setCursor(0, 0);
  lcd.print(" SMS SENDING TO ");
  lcd.setCursor(0, 1);
  lcd.print("REGISTERED PHNUM");
  delay(2000);
  mySerial.println("AT");                     // Initialize AT command
  mySerial.write(13);
  mySerial.write(10);
  delay(1000);
  mySerial.println("AT+CMGF=1");              // Selecting the Operating Mode PDU ,CMGF (command name in text: Message Format)
  mySerial.write(13);
  mySerial.write(10);
  delay(1000);
  mySerial.println("AT+CMGS=\"9847229676\""); // AT+CMGS command sends an SMS message to a GSM phone.
  mySerial.write(13);
  mySerial.write(10);
  delay(1000);
  mySerial.println("ALERT!! AIR BUBBLE DETECTED IN DIALYSIS PROCESS");
  mySerial.println("DIALYSIS INTERRUPTED");
  mySerial.write(26);
  mySerial.write(10);
  delay(7000);
  lcd.setCursor(0, 0);
  lcd.print("    SMS SENT    ");
  lcd.setCursor(0, 1);
  lcd.print(" SUCCESSFULLY...");
  delay(3000);
}

//-------Things speak server ping---------

void pingserver()
{

  lcd.setCursor(0, 0);
  lcd.print("DATA LOGGING TO ");
  lcd.setCursor(0, 1);
  lcd.print("SERVER PLS WAIT ");

  mySerial.println("AT");                           // Intitialize AT command
  delay(1000);
  mySerial.println("AT+CPIN?");                     // Query whether the PIN code is expected by SIM
  delay(1000);
  mySerial.println("AT+CREG?");                     // Displays network registration status. 
  delay(1000);
  mySerial.println("AT+CGATT?");                    // Status of Packet service attach. '0'-device not attached, '1'-device attached.
  delay(1000);
  mySerial.println("AT+CIPSHUT");                   // Close the GPRS PDP context.
  delay(1000);
  mySerial.println("AT+CIPSTATUS");                 // Returns the current connection status
  delay(2000);
  mySerial.println("AT+CIPMUX=0");                  // Disable multi IP connection
  delay(2000);
  mySerial.println("AT+CSTT=\"airtelgprs.com\"");   // Start task and setting the APN,
  delay(2000);
  mySerial.println("AT+CIICR");                     // Start GPRS depending on configuration previously set by CSTT
  delay(3000);
  mySerial.println("AT+CIFSR");                     // Get local IP adress
  delay(2000);
  mySerial.println("AT+CIPSPRT=0");                 // Disable echo prompt ">" after issuing "AT+CIPSEND" command.
  delay(3000);
  mySerial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\""); // start up the connection
  delay(6000);
  mySerial.println("AT+CIPSEND");                   // begin send data to remote server
  delay(4000);
  String str = "GET http://api.thingspeak.com/update?api_key=A393SO49234IH20W&field5=" + String(TEMPERATURE);
  mySerial.println(str);                            // begin send data to remote server
  delay(4000);
  mySerial.println((char)26);                       // sending
  delay(5000);
  // waitting for reply, important! the time is base on the condition of internet
  mySerial.println();
  mySerial.println("AT+CIPSHUT");                   // close the connection
  delay(100);

  lcd.setCursor(0, 0);
  lcd.print(" DATA LOADED TO ");
  lcd.setCursor(0, 1);
  lcd.print("WEB COMPLETED!! ");
  delay(2000);
  lcd.clear();
}
