//Arduino + Modem sms relay controller
//Tested on Arduino Nano v3 (CH340) + SIM800L (Z1435) + 4-channel 5Vdc relay module
//Version 1.0.30704
//2023 Alexey Smirnov

/*
Avaliable actions:
1) Switch output pin ON
2) Switch output pin OFF
3) Switch output pin once ON and OFF with delay

Avaliable sms commands:
1) RON + [DIGIT] - switch on relay [NUMBER], e.g. RON1 switches on relay 1 (first pin number in array below)
2) ROFF + [DIGIT] - switch off, same as RON
3) RBL + [DIGIT] - blink with adjustable delay
4) STATUS - sends sms with relay status, e.g. 1:OFF 2:OFF 3:OFF 4:ON
5) HELP - sends sms with avaliable commands, e.g. RON,ROFF,RBL + [1..4]; STATUS; HELP
Text of RON, ROFF and RBL commands is adjustable in User variables section.
Commands in sms are not case sensitive: RON1 = rOn1 = ron1, etc.
But command syntax remains the same: TEXT + DIGIT.
*/

#include <SoftwareSerial.h>

// User variables
String phonesAllowed[] = {"+12025550137 ","+12025550168"}; //Allowed phone numbers, first one is used for welcome
String countryCode = "+1"; //Need this to find string with phone number
int phoneLenght = 10; //You need to adjust this if you have more than 10 digits after country code

bool welcomeOn = true; //True - send sms when system is ready to receive messages, false - off
String welcomeMsg = "Ready"; //Welcome message text

int relays[] = {5,6,7,8}; //Relay pins
int relayOff = 1; // HIGH = Off change to 0 for opposite action
int relayOn = 0; // LOW = On change to 1 for opposite action

String cmdROn = "RON"; //Command to switch relay on (uppercase only)
String cmdROff = "ROFF"; //Command to switch relay off (uppercase only)
String cmdRBl = "RBL"; //Command to switch relay on and off with delay (uppercase only)
int blinkDelay = 1000; //Delay for blinking function in millisec

int led = 13; //Led pin (switches on when busy)
int mdmRst = 4; //Modem reset pin

bool debug = true; //Debug output to serial (true - on, false - off)

// System variables
int phoneLen = countryCode.length() + phoneLenght;
int phonesQty = sizeof(phonesAllowed) / sizeof(phonesAllowed[0]); //Number of items in phonesAllowed array
int relaysQty = sizeof(relays) / sizeof(relays[0]); //Number of items in relays array
char incomingByte; //Modem reply var
String inputString,message,msg,msgBody,msgPhone,msgOut; //String vars
unsigned int msgPhoneStart,index; //Unsigned int vars
int relayNo,relayCmd,msgHeaderLen; //Int vars
SoftwareSerial GSM(3,2);  // Rx,Tx pins of Arduino (Tx, Rx for SIM800)
void(* resetFunc) (void) = 0; //Software reset function

// SETUP //
void setup(){
    if(debug){
        Serial.begin(9600);
        Serial.println("Init serial");
    }

    pinMode(mdmRst, OUTPUT);
    digitalWrite(led, LOW); // Initial state of reset pin
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW); // Initial state of the led
    if(debug){
        Serial.println("Init pin[RST]:" + String(mdmRst));
        Serial.println("Init pin[LED]:" + String(led));
    }
    for(int x = 0; x < relaysQty; x++){ //Relay pins init
        if(debug){Serial.println("Init pin[REL]:" + String(relays[x]));}
        pinMode(relays[x], OUTPUT);
        digitalWrite(relays[x], relayOff);
    }

    modemRst();

    GSM.begin(9600);

    while(!GSM.available()){
        GSM.println("AT");
        delay(1000); 
        if(debug){Serial.println("Connecting...");}
    }

    if(debug){Serial.println("Connected!");}
    delay(10000);
    GSM.println("AT+CMGF=1");  //Set SMS to Text Mode 
    delay(1000);  
    GSM.println("AT+CNMI=1,2,0,0,0");  //Procedure to handle newly arrived messages(command name in text: new message indications to TE) 
    delay(1000);
    GSM.println("AT+CMGL=\"REC UNREAD\""); //Read Unread Messages
    delay(1000);
    if(welcomeOn){sendSMS(phonesAllowed[0],welcomeMsg);} //Send welcome message
}

// LOOP //
void loop(){  
  if(GSM.available()){
      delay(100);

      while(GSM.available()){
        incomingByte = GSM.read();
        inputString += incomingByte; //Catchin new messages on modem serial
        }
        
        digitalWrite(led, HIGH); 
        
        delay(10);      
        
        if (inputString.indexOf("+CMT:") > -1){ //Checking for new message marker
          message = inputString;
          if(debug){Serial.println("Parsing message");}
          msgParse(message); //Proceed parsing
        }
        else if (inputString.indexOf("ERROR") > -1){ //Catching errors
            resetFunc(); //Software reset if error found
        }
        else if (inputString.indexOf("+CLIP:") > -1){
            GSM.println("ATH");
        }
        else {
            //No action
        }

        if(debug){Serial.println(inputString);}
        
        digitalWrite(led, LOW); 
        
        delay(50);

        if (inputString.indexOf("OK") == -1){ //Delete messages
        GSM.println("AT+CMGDA=\"DEL ALL\"");

        delay(1000);}

        inputString = "";
        message = "";
  }
}

// FUNCTIONS //

int allowedNumberCheck(String number){
  int i = 0;
  for(int x=0; x < phonesQty; x++){
    if(phonesAllowed[x] == number){
      i = i + 1;
    }
  }
  return i;
}

void msgParse (String msg){
    msgBody = ""; //Setting to null
    msgPhone = "";

    msg.toUpperCase(); //Preprocessing
    msg.replace("\r","");
    msg.replace("\n","");
    
    unsigned int msgPhoneStart = msg.indexOf("\"" + countryCode); //Searching for phone number
    msgPhone = msg.substring(msgPhoneStart + 1, msgPhoneStart + phoneLen + 1); //Extracting ph no
    msgPhone.trim(); //Trimming unexpected spaces
    
    int check = allowedNumberCheck(msgPhone);
    if (check > 0) {
        int msgHeaderLen = msg.lastIndexOf("\"") + 1; //Searching for sessage body
        msgBody = msg; //Transfering data to local var (may need full msg later)
        msg = ""; //Setting to null
        msgBody.remove(0, msgHeaderLen); //Cutting header
        msgBody.trim(); //Trimming unexpected spaces

        if(debug){Serial.println("msgParse input: " + msgBody);} //Printing to serial
        if(debug){Serial.println("msgParse output: " + msgPhone);}
        
        if (msgBody.indexOf(cmdROn) == 0 || msgBody.indexOf(cmdROff) == 0 || msgBody.indexOf(cmdRBl) == 0){ //Dont know how to make this shorter
            unsigned int index;
            bool error = false;
            if (msgBody.indexOf(cmdROn) == 0){
                index = msgBody.indexOf(cmdROn) + cmdROn.length();
                relayNo = msgBody.substring(index, index + 1).toInt();
                relayCmd = 1;
            }
            else if (msgBody.indexOf(cmdROff) == 0){
                index = msgBody.indexOf(cmdROff) + cmdROff.length();
                relayNo = msgBody.substring(index, index + 1 ).toInt();
                relayCmd = 0;
            }
            else if (msgBody.indexOf(cmdRBl) == 0){
                index = msgBody.indexOf(cmdRBl) + cmdRBl.length();
                relayNo = msgBody.substring(index, index + 1 ).toInt();
                relayCmd = 2;
            }
            else {
                //Unknown command action
                if(debug){Serial.println("msgParse: Unknown command: " + msgBody);}
                error = true;
            }
            if (error){
                msgOut = "Unknown command. Send HELP for help.";
            }
            else {
                relayCtl(relayNo,relayCmd); //Call to relay control function
                delay(250);
                msgOut = relayStat(relayNo); //Call to relay status function
            }
        }
        else if (msgBody.indexOf("STATUS") == 0){
            msgOut = relayStat(0);//Call to relay status function
        }
        else if (msgBody.indexOf("HELP") == 0){
            msgOut = cmdROn + "," + cmdROff + "," + cmdRBl + "[1.." + String(relaysQty) + "]; STATUS; HELP"; //Send sms with help
        }
        else {
            //Unknown command action
            if(debug){Serial.println("msgParse: Unknown command: " + msgBody);}
            msgOut = "Unknown command. Send HELP for help.";
        }
        sendSMS(msgPhone,msgOut);
    }
    else {
        if(debug){Serial.println("msgParse output: Unknown sender " + msgPhone);}
    }
}

//Relay control
void relayCtl (int relayNo, int relayCmd){
    Serial.println("relayControl input: " + String(relayNo) + String(relayCmd));
    if (relayNo > 0 && relayNo <= relaysQty){
        int target = relayNo - 1; // Adjusting relay no to pin position in array
        int targetPin = relays[target];
        if (relayCmd == 0){ // 0 - off
            digitalWrite(targetPin, relayOff);
        }
        else if (relayCmd == 1){ // 1 - on
            digitalWrite(targetPin, relayOn);
        }
        else if (relayCmd == 2){ // 2 - blink
            digitalWrite(targetPin, relayOn);
            delay(blinkDelay);
            digitalWrite(targetPin, relayOff);
        }
        else {
            //Unknown command action
            if(debug){Serial.println("relayControl: Unknown command: " + String(relayNo) + String(relayCmd));}
        }
    }
}

//Relay status
String relayStat(int rNo){
    String relStat = "";
    if (rNo == 0){
        for(int x = 0; x < relaysQty; x++){
            if(digitalRead(relays[x]) == relayOff){
                relStat += String(x + 1) + ":Off "; //if pin is high -- it's turned off
            }
            else {
                relStat += String(x + 1) + ":ON ";
            }
        }
    }
    if (rNo > 0 && rNo <= relaysQty){
        int target = rNo - 1; // 
        int targetPin = relays[target];
        if(digitalRead(targetPin) == relayOff){
            relStat = String(rNo) + ":Off";
        }
        else {
            relStat = String(rNo) + ":ON";
        }
    }
    relStat.toUpperCase();
    relStat.trim();
    return relStat;
}

//Send reply sms
void sendSMS(String phone, String message){
    if(debug){Serial.println("sendSMS: To:[" + phone + "], Text:[" + message + "]");} //Debug output
    GSM.println("AT+CMGS=\"" + phone + "\"");
    delay(50);
    GSM.print(message);
    delay(50);
    GSM.write(26); //Ctrl+Z after message input
    delay(50);
    GSM.println("\r\n"); //Doesn't work without this
    delay(100);
    GSM.println("AT+CMGDA=\"DEL ALL\""); //Delete all sms to save memory
}

void modemRst() {
    digitalWrite(mdmRst, HIGH);
    delay(250);
    digitalWrite(mdmRst, LOW);
    delay(1000);
}
