/* 
   Web control program for OSBee Shield
   This demo creates a simple web server using the 
   Arduino Ethernet Shield. The web page presents
   three buttons. Clicking each button opens / closes
   the corresponding valve.
   
   !!! IMPORTANT !!!
   As the Ethernet Shield uses pin D10, please leave
   this pin out on the OSBee Shield. This will disable
   Port D functionality, so you will only have 3 zones.
   
   http://bee.opensprinkler.com
   
   License: Creative Commons CC-SA 3.0
   
   Written by : Jonathan Goldin (Rayshobby LLC)
   info@rayshobby.net
   
*/

#include <OSBee.h>
#include <SPI.h>
#include <Ethernet.h>

OSBee osb;

byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,177);

EthernetServer server(80);
#define STATIONCOUNT 3

char buf[32];
int index;
byte old_station_bits = 0;
byte station_bits = 0;

void setup() {
  
  Serial.begin(9600);
  // these are optional parameters
  //osb.setVoltage(13.0);
  //osb.setPulseLength(25);
  //osb.setDutyCycle(33);
  
  // must call the begin() function for proper initialization
  osb.begin();

  Serial.println(osb.getBattVoltage());
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Server is at: ");
  Serial.println(Ethernet.localIP());  
}

void loop() {
  EthernetClient client = server.available();
  if(client){
    boolean foundGet = false;
    boolean getEnded = false;
    boolean currentLineIsBlank = true;
    while(client.connected()){
      if(client.available()){
        char c = client.read();
        
        if(c == '/' && !getEnded){
          index = 0;
          foundGet = true;
        }
        if(foundGet && c == ' '){
          foundGet = false;
          getEnded = true;
          parseGetRequest();
          if(index > 1){
            gotoHomePage(client);
            break;
          }
        }
        if(foundGet){
          buf[index] = c;
          index++;
        }
        if(c == '\n' && currentLineIsBlank){
          printHeader(client);
          client.println("<script>");
          client.print("var sbits=");
          client.print((int)station_bits);
          client.println(";");
          addFunctions(client);
          client.println("</script>");          
          addSetForm(client);
          for(int i = 0; i < STATIONCOUNT; i++){
            addButton(client,i);
          }
          printFooter(client);
          break;
        }
        if(c == '\n'){
          currentLineIsBlank = true;
        }
        else if(c != '\r'){
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }
  
  // apply station bits
  if (station_bits != old_station_bits) {
    byte b;
    for(int i=0; i<STATIONCOUNT; i++) {
      b = ((station_bits >> i) & 1);
      if (b != ((old_station_bits >> i) & 1)) {
        if (b) osb.open(i);
        else osb.close(i);
      }
    }
    old_station_bits = station_bits;
  }
}  

void printHeader(EthernetClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");  // the connection will be closed after completion of the response
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
}

void printFooter(EthernetClient client){
  client.println("</html>");
}

void addButton(EthernetClient client,int idx){
  client.print("<input type=button value='Station ");
  client.print(idx);
  client.print("' id=s");
  client.print(idx);
  client.print(" style='width:160px;height:100px;font-size:24px;background-color:");
  if(station_bits & (1<<idx)){
    client.print("Green");
  } else {
    client.print("Red");
  }  
  client.print("' onClick=sf(");
  client.print(idx);
  client.println(")>");
  
}

void addSetForm(EthernetClient client){
  client.println("<form name=set action=set method=get><input type=hidden name=i><input type=hidden name=v></form>");
}

void addFunctions(EthernetClient client){
  client.println("function sf(i){set.elements[0].value=i;set.elements[1].value=1-((sbits>>i)&1);set.submit()}");
}

void gotoHomePage(EthernetClient client){
  printHeader(client);
  client.println("<script>window.location = '/';</script>");
  printFooter(client);
}

void parseGetRequest() {
  int idx=-1;
  bool on=false;
  if(index > 1){
    for(int i = index; i<32; i++){
        buf[i] = ' ';
    }
    String s = String(buf);
    int firstEquals = s.indexOf('=');
    int secondEquals = s.indexOf('=',firstEquals+1);
    idx = buf[firstEquals+1]-'0';
    if(buf[secondEquals+1] == '0'){
      on = false;
    } else {
      on = true;
    }
  }
  if(idx>-1 && idx<8){
    if(on){
      old_station_bits = station_bits;
      station_bits |= (1<<idx);
    } else {
      old_station_bits = station_bits;
      station_bits &= ~(1<<idx);
    }
  }
}
