#include <WiFi.h>                                                                                                                                             // needed to connect to WiFi
#include <WebServer.h>                                                                                                                                        // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)
#include <WebSocketsServer.h>                                                                                                                                 // needed for instant communication between client and server through Websockets
#include <ArduinoJson.h>                                                                                                                                      // needed for JSON encapsulation (send multiple variables with one string)
#include "AD9833.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifndef ESP32
#error ESP32 only example, please select appropriate board
#endif

#define SCREEN_WIDTH 128                                                                                                                                      // OLED display width, in pixels
#define SCREEN_HEIGHT 64                                                                                                                                      // OLED display height, in pixels
#define OLED_RESET     -1                                                                                                                                     // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C                                                                                                                                   //< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


                                                                                                                                                              // SSID and password of Wifi connection:
const char* ssid = "Itzz_Infinity";
const char* password = "123456789";

                                                                                                                                                              // The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
String webpage ="<!DOCTYPE html><html lang=en><head><meta charset=UTF-8><meta name=viewport content='width=device-width, initial-scale=1.0'><title>ESP32sigGen_V6</title><style>body{background-color:#f0f0f0bb;color:#333;font-family:Arial,sans-serif;display:flex;flex-direction:column;justify-content:center;align-items:center;overflow:hidden}output{position:absolute;background-image:linear-gradient(#444444,#999999);width:70px;height:30px;text-align:center;color:white;border-radius:10px;display:inline-block;font:bold 15px/30px Georgia;bottom:175%;left:0}output:after{content:\"\";position:absolute;width:0;height:0;border-top:10px solid #999;border-left:5px solid transparent;border-right:5px solid transparent;top:100%;left:50%;margin-left:-5px;margin-top:-1px}form{position:relative;margin:50px;width:1200px;margin-left:40rem}#numberInput{padding:10px;font-size:16px}button{padding:10px 20px;margin:10px;font-size:16px;cursor:pointer;background-color:#34db58;color:#fff;border-radius:5px}#submit{background-color:#3498db;color:#fff}footer.footright{color:#aaa;font-family:Consolas,'courier new';text-align:right;margin-top:0}footer.footleft{color:#aaa;font-family:Consolas,'courier new';text-align:left;margin-top:50px}.label{margin-top:1rem;display:flex;flex-direction:row}</style></head><body><h1 class=a>ESP32 Signal generator</h1><input type=number id=numberInput placeholder='Enter a positive number' class=a><br><div class=slider><form><input type=range name=foo min=0 max=7 oninput=UpdateSlider(this.value) style=width:50%><output for=foo onforminput='value = labels[foo.valueAsNumber];'></output></form></div><button id=submit class=a>Set Frequency</button><br><br><br><label for=setWAVE class=a>Wave Selector</label><br><div class=\"label a\"><button id=setSINE onclick=updateWave(1)>SINE</button><button id=setTRIANGLE onclick=updateWave(2)>TRIANGLE</button><button id=setSQUARE onclick=updateWave(0)>SQUARE</button></div><footer div class=footleft id=temp class=a>© Copyright to Infinite Solutions Inc.</div></footer><footer div class=footright class=a>Designed & developed by Infinity</div></footer></body><script>const labels=['x1Hz','x10Hz','x100Hz','x1kHz','x10kHz','x100kHz','x1MHz','x10MHz'];var wav=0;function updateWave(newWave){wav=newWave;console.log(\"Updated wav:\",wav);}function modifyOffset(){var el,newPoint,newPlace,offset,siblings,k;width=this.offsetWidth;newPoint=(this.value-this.getAttribute('min'))/(this.getAttribute('max')-this.getAttribute('min'));offset=-1;if(newPoint<0){newPlace=0;}else if(newPoint>1){newPlace=width;}else{newPlace=width*newPoint+offset;offset-=newPoint;}siblings=this.parentNode.childNodes;for(var i=0;i<siblings.length;i++){sibling=siblings[i];if(sibling.id==this.id){k=true;}if((k==true)&&(sibling.nodeName=='OUTPUT')){outputTag=sibling;}}outputTag.style.left=newPlace+'px';outputTag.style.marginLeft=offset+'%';outputTag.innerHTML=labels[this.value];}function modifyInputs(){var inputs=document.getElementsByTagName('input');for(var i=0;i<inputs.length;i++){if(inputs[i].getAttribute('type')=='range'){inputs[i].onchange=modifyOffset;if('fireEvent'in inputs[i]){inputs[i].fireEvent('onchange');}else{var evt=document.createEvent('HTMLEvents');evt.initEvent('change',false,true);inputs[i].dispatchEvent(evt);}}}}modifyInputs();function UpdateSlider(value){multiplyBy(10**value);}function multiplyBy(multiplier){var inputValue=document.getElementById('numberInput').value;if(!isNaN(inputValue)&&inputValue>=0){var result=inputValue*multiplier;document.getElementById('numberInput').value=result;}else{alert('Please enter a valid positive number.');}}var Socket;document.getElementById('submit').addEventListener('click',sendToServer);function init(){Socket=new WebSocket('ws://'+window.location.hostname+':81/');Socket.onmessage=function(event){processCommand(event);};}function sendToServer(){var inputValue=document.getElementById('numberInput').value;var freqValue=parseInt(inputValue,10);var waveInt=parseInt(wav,10);var data={'freqdata':{'Freq':freqValue,'wav':waveInt}};if(!isNaN(freqValue)&&freqValue>=0){Socket.send(JSON.stringify(data));console.log(data);}else{alert('Please enter a valid positive number.');}}window.onload=function(event){init();}</script></html>";

                                                                                                                                                              // The JSON library uses static memory, so this will need to be allocated:
                                                                                                                                                              // -> in the video I used global variables for "doc_tx" and "doc_rx", however, I now changed this in the code to local variables instead "doc" -> Arduino documentation recomends to use local containers instead of global to prevent data corruption

                                                                                                                                                              // We want to periodically send values to the clients, so we need to define an "interval" and remember the last time we sent data to the client (with "previousMillis")
int interval = 1000;                                                                                                                                          // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0;                                                                                                                             // we use the "millis()" command for time reference and this will output an unsigned long

                                                                                                                                                              // Initialization of webserver and websocket
WebServer server(80);                                                                                                                                         // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);                                                                                                            // the websocket uses port 81 (standard port for websockets

                                                                                                                                                              //  HSPI uses default   SCLK=14, MISO=12, MOSI=13, SELECT=15
                                                                                                                                                              //  VSPI uses default   SCLK=18, MISO=19, MOSI=23, SELECT=5
SPIClass * myspi = new SPIClass(VSPI);
AD9833 AD(5, myspi);
                                                                                                                                                              //  AD9833 AD(15, 13, 14); //  SW SPI

float dispFreq = 0.0;                                                                                                                                         // Variable to store formatted frequency value
                                                                                                                                                              // sdata = mosi 23
                                                                                                                                                              // sclk = sck 18
                                                                                                                                                              // fsync = slave select 5
void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);
    WiFi.begin(ssid, password);                                                                                                                               // start WiFi interface
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));                                                                               // print SSID to the serial interface for debugging

 if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);                                                                                                                                                  // Don't proceed, loop forever
  }
 display.clearDisplay();
  while (WiFi.status() != WL_CONNECTED) {                                                                                                                     // wait until WiFi is connected
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());                                                                                                                             // show IP address that the ESP32 has received from router

  display.setTextSize(2);                                                                                                                                     // Draw 1X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("IP:");
  display.print(WiFi.localIP());
  display.display();                                                                                                                                          // Show initial text
  delay(100);

  
  server.on("/", []() {                                                                                                                                       // define here wat the webserver needs to do
    server.send(200, "text/html", webpage);                                                                                                                   //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();                                                                                                                                             // start server
  
  webSocket.begin();                                                                                                                                          // start websocket
  webSocket.onEvent(webSocketEvent);                                                                                                                          // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"

  int setFREQ = 10000;
  
  AD.begin();
  AD.setFrequency(setFREQ, 0);                                                                                                                                //  1000 Hz.

  AD.setWave(AD9833_SQUARE1);
  Serial.println(AD.getWave());
}


void loop()
{
    server.handleClient();                                                                                                                                    // Needed for the webserver to handle all clients
  webSocket.loop();                                                                                                                                           // Update function for the webSockets 
  
  unsigned long now = millis();                                                                                                                               // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  if ((unsigned long)(now - previousMillis) > interval) {                                                                                                     // check if "interval" ms has passed since last time the clients were updated
     previousMillis = now;                                                                                                                                    // reset previousMillis
  }
}
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {                                                                              // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  display.clearDisplay();
  display.setTextSize(1);                                                                                                                                     // Draw 1X-scale text
  display.setTextColor(SSD1306_WHITE);
  
  
  switch (type) {                                                                                                                                              // switch on the type of information sent
    case WStype_DISCONNECTED:                                                                                                                                  // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                                                                                                                                     // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
                                                                                                                                                               // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:                                                                                                                                          // if a client has sent data, then type == WStype_TEXT
                                                                                                                                                               // try to decipher the JSON string received
      StaticJsonDocument<100> doc;                                                                                                                             // create a JSON container
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      else {
        int freqdata_Freq = doc["freqdata"]["Freq"];
        int freqdata_wav = doc["freqdata"]["wav"]; 
        Serial.println("Received Waveform info from user: " + String(num));
        Serial.println("Frequency: " + String(freqdata_Freq) + " Hz");     
        Serial.println("Waveform: " + String(freqdata_wav));
        AD.setFrequency(freqdata_Freq, 0);
        switch (freqdata_wav) {
          case 1:
            AD.setWave(AD9833_SINE);
            Serial.println("Sine Wave Selected");
            display.clearDisplay();
            display.setCursor(0,0);
            display.print("Wave:");
            display.setTextSize(2);
            display.print("Sine");
            display.display(); 
          break;
      
          case 2:
            AD.setWave(AD9833_TRIANGLE);
              Serial.println("Triangle Wave Selected");
              display.clearDisplay();
              display.setCursor(0,0);
              display.print("Wave:");
              display.setTextSize(2);
              display.print("Triangle");
              display.display(); 
              break;
      
          case 0:
            AD.setWave(AD9833_SQUARE1);
            Serial.println("Square Wave Selected");
            display.clearDisplay();
            display.setCursor(0,0);
            display.print("Wave:");
            display.setTextSize(2);
            display.print("Square");
            display.display(); 
           break;
      
          default:
            Serial.println("Some Error Occured");
            display.clearDisplay();
            display.setCursor(0,0);
            display.println("Some Error Occured");
            display.display(); 
        }
        display.setTextSize(1);                                                                                                                                    // Draw 1X-scale text
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,20);
        display.print("Freq:");
        display.setTextSize(2);                                                                                                                                    // Calculate the frequency in MHz, KHz, or Hz
          if (freqdata_Freq / 1000000 > 0.9) {
            dispFreq = (float)freqdata_Freq / 1000000;                                                                                                             // Calculate frequency in MHz
            Serial.print("Frequency: ");
            Serial.print(dispFreq, 2);                                                                                                                             // Print with 2 decimal places
            Serial.println(" MHz");
            display.print(dispFreq);
            display.print("MHz");
          } else if (freqdata_Freq / 1000 > 0.9) {
            dispFreq = (float)freqdata_Freq / 1000;                                                                                                                // Calculate frequency in KHz
            Serial.print("Frequency: ");  
            Serial.print(dispFreq);                                                                                                                                // Print with 2 decimal places
            Serial.println(" KHz");
            display.print(dispFreq);
            display.print("kHz");
          } else { 
            dispFreq = (float)freqdata_Freq;                                                                                                                       // Frequency is less than 1 Hz
            Serial.print("Frequency: ");                                                                  
            Serial.print(dispFreq, 2);                                                                                                                             // Print with 2 decimal places
            Serial.println(" Hz");
            display.print(dispFreq);
            display.print("Hz");
        }
        display.display();
      }
      Serial.println("");
      break;
  }
}

//  -- END OF FILE --
