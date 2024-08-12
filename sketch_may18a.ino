//libraries
#include <SFE_BMP180.h> //read data from BMP180 sensor and perform calculations to obtain temperature and pressure values.
#include <Wire.h>  //is used to communicate with I2C/TWI devices like BMP180
#include <ESP8266WiFi.h> //is a set of APIs for connecting the ESP8266 to a wireless network, create a web server, access HTTP and HTTPS 
#include <ESP8266WebServer.h>  //web server library 
#include <NTPClient.h>  //create an NTP client 
#include <WiFiUdp.h>    //get time

ESP8266WebServer server(80); //Server on port 80 // enables the ESP8266 to act as a web

#define ALTITUDE 40.0 // Altitude of APU in meters
#define rainDigital D3

SFE_BMP180 bmp; //creates objects for BMP180 library
double T, P, p0, a; //several variables are created and initialized 
int rainDigitalVal = digitalRead(rainDigital); //D0 (Digital Output) pin of rain sensor connected at D3 Pin
int B;
int LDR = A0; //LDR Pin connected at A0 Pin
char status;
String formattedTime,currentDate,currentMonthName; 
const long utcOffsetInSeconds = 28800;   //Replace with GMT offset (seconds) of msia //GMT +8 = 28800 

//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

// Define NTP Client to get date and time from an NTP server 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

WiFiClient client;  //Creates a client that can connect to to a specified internet IP address and port
String apiKey = "L43ODFKOR08XFNYW"; //  The Write API key from ThingSpeak
const char *ssid =  "cp"; //my wifi network name
const char *pass =  "xxxx"; //my network password
const char* ts_server = "api.thingspeak.com";

void setup() {
  Serial.begin(115200);  //initialize the Serial communication at baud rate 115200
  delay(10); //wait 10 milliseconds 
  bmp.begin();  //initializes the BMP180 sensor library.
  Wire.begin(); //initializes the I2C communication protocol on the ESP8266 board
  WiFi.begin(ssid, pass);  //connect to network with ssid and pass
  pinMode(rainDigital,INPUT);


  //initialize serial monitor, wait until wifi connection successful 
  while (WiFi.status() != WL_CONNECTED) {  
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected\n");

  // Initialize a NTPClient to get time and time 
  timeClient.begin();

  //Once connect to wifi successfully, IP address was assigned to ESP8266, it also was printed
  Serial.print("IP Address of network: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //To handle incoming HTTP requests, we must specify which code should be executed when a specific URL is accessed. So, on() function is used. 
  //handle_OnConnect() will be called when a server receives an HTTP request on the root (/) path
  //server.onNotFound() method: //if the client requests a URL that isn’t specified with server.on() . It should give a 404 error (Page Not Found) as a response.
  server.on("/", handle_OnConnect); 
  server.onNotFound(handle_NotFound); 

  server.begin();  //start server 
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient(); //handle the incoming HTTP requests from a Web Client

  timeClient.update(); //current date and time from NTP server

  time_t epochTime = timeClient.getEpochTime(); //returns the epoch time function (number of seconds that have elapsed since January 1, 1970);
  struct tm *ptm = gmtime ((time_t *)&epochTime); //use epoch time function along with a time structure to get the date.
  
  int monthDay = ptm->tm_mday;

  int currentMonth = ptm->tm_mon+1;

  String currentMonthName = months[currentMonth-1];

  int currentYear = ptm->tm_year+1900;
  currentDate = String(monthDay) + "-" + String(currentMonth)+ "-" + String(currentYear)  ;
  Serial.print("Date: ");
  Serial.print(currentDate);

  formattedTime  = timeClient.getFormattedTime(); //returns a String with the time formatted like HH:MM:SS;
  Serial.print("\t\t Time: ");
  Serial.println(formattedTime );
  delay(1000);

  //BMP180 sensor
  Serial.println();
  Serial.print("Provided altitude: ");
  Serial.print(ALTITUDE, 0);
  Serial.print(" meters\n");

  status =  bmp.startTemperature(); //start temp measurement
  if (status != 0) { //returns a non-zero value
    delay(status); //wait for at least 4.5 milliseconds
    status = bmp.getTemperature(T); //read temp value and store it in the variable T
    if (status != 0) {
      Serial.print("Temperature: ");
      Serial.print(T, 2);
      Serial.println(" °C");

      //start pressure measurement 
      // We provide an oversampling value as parameter, which can be between 0 to 3. A value of 3 provides a high resolution, but also a longer delay between measurements  
      status = bmp.startPressure(3); 
      if (status != 0) {
        delay(status);
        status = bmp.getPressure(P, T);  //read pressurevalue and store it in the variable P  // variable T is passed coz the pressure calculation is dependent on the temperature.
        if (status != 0) {
          Serial.print("Absolute pressure: ");
          Serial.print(P, 2); //2 means two decimal places 
          Serial.println(" mb");

          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and the current altitude.
          // This number is usually used in weather reports.

          p0 = bmp.sealevel(P, ALTITUDE);
          Serial.print("Relative (sea-level) pressure: ");
          Serial.print(p0, 2);
          Serial.println(" mb");

          // On the other hand, if we want to determine the altitude from the pressure reading,
          // use the altitude function along with a baseline pressure (sea-level or other).

          a = bmp.altitude(P, p0);
          Serial.print("Computed altitude: ");
          Serial.print(a, 0);
          Serial.println(" meters");

        }
        else Serial.println("error receiving pressure measurement !!!\n");
      }
      else Serial.println("error starting pressure measurement !!!\n");
    }
    else Serial.println("error receiving temperature measurement !!!\n");
  }
  else Serial.println("error starting temperature measurement !!!\n");

  //rain sensor
  if (rainDigitalVal == 1 || rainDigitalVal == 0){
    Serial.print("Rain: ");
    Serial.print(rainDigitalVal);  //0 means raining, 1 means not raining
    Serial.println("\t [0 means raining, 1 means not raining]");
  } 
  else Serial.println("error receiving rainfall measurement !!!\n");

  //LDR
  B = analogRead(LDR); //Read Analog values and store in B variable
  Serial.print("Light luminosity: ");
  Serial.print(B);
  Serial.println(" cd"); //“Candela”, as known as cd, is the unit of Luminous Intensity
  delay(500);

  //Connects to a specified server address and port.
  if (client.connect(ts_server, 80)) { 
    String postStr = apiKey;  //API key is needed to authenticate and allow access to the ThingSpeak channels.
    postStr += "&field1=";   //This line concatenates the string &field1= to the postStr. 
    postStr += String(T, 2); //This line appends the value of the BMP180 sensor T to the postStr string.
    postStr += "&field2=";
    postStr += String(P, 2);
    postStr += "&field3=";
    postStr += String(p0, 2);
    postStr += "&field4=";
    postStr += String(a, 0);
    postStr += "&field5=";
    postStr += String(rainDigitalVal);
    postStr += "&field6=";
    postStr += String(B);
    postStr += "\r\n\r\n\r\n\r\n"; //add some additional blank lines 

    client.print("POST /update HTTP/1.1\n");  //specifies the path and version of the HTTP protocol to be used.
    client.print("Host: api.thingspeak.com\n");  //specifies the host name of the ThingSpeak server.
    client.print("Connection: close\n");  //specifies that the connection will be closed after the message has been sent.
    client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");  //is the name of the header that contains the ThingSpeak API key. The API key is concatenated to the string
    client.print("Content-Type: application/x-www-form-urlencoded\n");  //specifies the format of the data being sent. In this case, it is URL-encoded data.
    client.print("Content-Length: ");  
    client.print(postStr.length());  //specifies the length of the message body in bytes
    client.print("\n\n\n\n");
    client.print(postStr);  //sends the message body to the server

    Serial.println("");
    Serial.println("Data Send to Thingspeak");
    Serial.println("Waiting...");
    Serial.println("***************************************************\n");
  }
  client.stop();  //close the client 
  delay(1000);  // Pause for 1 second.
}

//If handleClient() detects a request from a Web Client.If successfully connects, it directs the sketch to the handle_OnConnect() function.
//the first parameter we pass to the send method is the code 200 (one of the HTTP status codes), which corresponds to the OK response.
//next, specify the content type as “text/html”
//next, pass the SendHTML() custom function, which generates a dynamic HTML page with the data that measured by sensors.
void handle_OnConnect() {
  server.send(200, "text/html", SendHTML(T, P, p0, a, rainDigitalVal, B, formattedTime,currentDate,currentMonthName)); //returns the data to the client 
}

//If handleClient() detects a request from a Web Client.If there is an error with the connection, it directs to the handle_NotFound() function.
void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}


//show the HTML Web Page according to the data we have collected.
//Whenever the ESP8266 web server receives a request from a web client, the sendHTML() function generates a web page. 
//It simply concatenates HTML code into a long string and returns to the server.send() function. 
//The function uses the results that measured by sensors as a parameter to generate HTML content dynamically.
String SendHTML(double T, double P, double p0, double a, int rainAnalogVal , int B, String formattedTime, String currentDate, String currentMonthName ) {
  String ptr = "<!DOCTYPE html>"; //were sending HTML code.
  ptr += "<html>";
  ptr += "<head>";
  ptr += "<title>Weather Monitoring System</title>";  //title of page
  ptr += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"; //makes the web page responsive, ensuring that it looks good on all devices
  ptr += "<link href='https://fonts.googleapis.com/css?family=Open+Sans:300,400,600' rel='stylesheet'>";
  ptr += "<style>"; //style the overall appearance of the web page.
  ptr += "html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #444444;}"; //defined content to be shown as an block, center-aligned.
  ptr += "body{margin: 0px;} ";
  ptr += "h1 {margin: 50px auto 30px;} ";
  ptr += ".side-by-side{display: table-cell;vertical-align: middle;position: relative;}";
  ptr += ".text{font-weight: 600;font-size: 19px;width: 200px;}";
  ptr += ".reading{font-weight: 300;font-size: 50px;padding-right: 25px;}";
  ptr += ".T .reading{color: #F29C1F;}";
  ptr += ".P .reading{color: #3B97D3;}";
  ptr += ".p0 .reading{color: #FF0000;}";
  ptr += ".a .reading{color: #955BA5;}";
  ptr += ".rainDigitalVal .reading{color: #F29C1F;}";
  ptr += ".B .reading{color: #F29C1F;}";
  ptr += ".superscript{font-size: 17px;font-weight: 600;position: absolute;top: 10px;}";
  ptr += ".subscript{font-size: 17px;font-weight: 600;position: relative;color: #808080;}";
  ptr += ".data{padding: 10px;}";
  ptr += ".container{display: table;margin: 0 auto;}";
  ptr += ".icon{width:65px}";
  ptr += "</style>";
  ptr += "</head>";
  ptr += "<body>";
  ptr += "<h1>Weather Monitoring System</h1>"; //the heading of the web page is set.
  ptr +="<h2>";
  ptr +=(String)currentDate;
  ptr +="&nbsp &nbsp &nbsp"; //add space between currentDate and formattedTime
  ptr +=(String)formattedTime;
  ptr +="</h2>";
  ptr += "<div class='container'>"; //container is used to put the content 

  ptr += "<div class='data temperature'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 19.438 54.003'height=54.003px id=Layer_1 version=1.1 viewBox='0 0 19.438 54.003'width=19.438px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M11.976,8.82v-2h4.084V6.063C16.06,2.715,13.345,0,9.996,0H9.313C5.965,0,3.252,2.715,3.252,6.063v30.982";
  ptr += "C1.261,38.825,0,41.403,0,44.286c0,5.367,4.351,9.718,9.719,9.718c5.368,0,9.719-4.351,9.719-9.718";
  ptr += "c0-2.943-1.312-5.574-3.378-7.355V18.436h-3.914v-2h3.914v-2.808h-4.084v-2h4.084V8.82H11.976z M15.302,44.833";
  ptr += "c0,3.083-2.5,5.583-5.583,5.583s-5.583-2.5-5.583-5.583c0-2.279,1.368-4.236,3.326-5.104V24.257C7.462,23.01,8.472,22,9.719,22";
  ptr += "s2.257,1.01,2.257,2.257V39.73C13.934,40.597,15.302,42.554,15.302,44.833z'fill=#F29C1F /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Temperature</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (double)T;
  ptr += "<span class='superscript'>°C</span></div>";
  ptr += "</div>";

  ptr += "<div class='data absolute pressure'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
  ptr += "c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
  ptr += "c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
  ptr += "c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
  ptr += "C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
  ptr += "c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
  ptr += "C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#1D267D /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Absolute Pressure</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (double)P;
  ptr += "<span class='superscript'>mb</span></div>";
  ptr += "</div>";

  ptr += "<div class='data relative pressure'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 40.542 40.541'height=40.541px id=Layer_1 version=1.1 viewBox='0 0 40.542 40.541'width=40.542px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M34.313,20.271c0-0.552,0.447-1,1-1h5.178c-0.236-4.841-2.163-9.228-5.214-12.593l-3.425,3.424";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293c-0.391-0.391-0.391-1.023,0-1.414l3.425-3.424";
  ptr += "c-3.375-3.059-7.776-4.987-12.634-5.215c0.015,0.067,0.041,0.13,0.041,0.202v4.687c0,0.552-0.447,1-1,1s-1-0.448-1-1V0.25";
  ptr += "c0-0.071,0.026-0.134,0.041-0.202C14.39,0.279,9.936,2.256,6.544,5.385l3.576,3.577c0.391,0.391,0.391,1.024,0,1.414";
  ptr += "c-0.195,0.195-0.451,0.293-0.707,0.293s-0.512-0.098-0.707-0.293L5.142,6.812c-2.98,3.348-4.858,7.682-5.092,12.459h4.804";
  ptr += "c0.552,0,1,0.448,1,1s-0.448,1-1,1H0.05c0.525,10.728,9.362,19.271,20.22,19.271c10.857,0,19.696-8.543,20.22-19.271h-5.178";
  ptr += "C34.76,21.271,34.313,20.823,34.313,20.271z M23.084,22.037c-0.559,1.561-2.274,2.372-3.833,1.814";
  ptr += "c-1.561-0.557-2.373-2.272-1.815-3.833c0.372-1.041,1.263-1.737,2.277-1.928L25.2,7.202L22.497,19.05";
  ptr += "C23.196,19.843,23.464,20.973,23.084,22.037z'fill=#6C9BCF /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Relative Pressure</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (double)p0;
  ptr += "<span class='superscript'>mb</span></div>";
  ptr += "</div>";

  ptr += "<div class='data altitude'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
  ptr += "c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
  ptr += "c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
  ptr += "C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#BFCCB5 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
  ptr += "c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
  ptr += "C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
  ptr += "C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#87CBB9 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Altitude</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (double)a;
  ptr += "<span class='superscript'>m</span></div>";
  ptr += "</div>";

  ptr += "<div class='data rainfall'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 29.235 40.64'height=40.64px id=Layer_1 version=1.1 viewBox='0 0 29.235 40.64'width=29.235px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><path d='M14.618,0C14.618,0,0,17.95,0,26.022C0,34.096,6.544,40.64,14.618,40.64s14.617-6.544,14.617-14.617";
  ptr += "C29.235,17.95,14.618,0,14.618,0z M13.667,37.135c-5.604,0-10.162-4.56-10.162-10.162c0-0.787,0.638-1.426,1.426-1.426";
  ptr += "c0.787,0,1.425,0.639,1.425,1.426c0,4.031,3.28,7.312,7.311,7.312c0.787,0,1.425,0.638,1.425,1.425";
  ptr += "C15.093,36.497,14.455,37.135,13.667,37.135z'fill=#05BFDB /></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Rainfall</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)rainDigitalVal;
  ptr += "<span class='subscript'>[0: rainy, 1: sunny]</span></div>";  //0 means raining, 1 means not raining
  ptr += "</div>";

  ptr += "<div class='data light luminosity'>";
  ptr += "<div class='side-by-side icon'>";
  ptr += "<svg enable-background='new 0 0 58.422 40.639'height=40.639px id=Layer_1 version=1.1 viewBox='0 0 58.422 40.639'width=58.422px x=0px xml:space=preserve xmlns=http://www.w3.org/2000/svg xmlns:xlink=http://www.w3.org/1999/xlink y=0px><g><path d='M58.203,37.754l0.007-0.004L42.09,9.935l-0.001,0.001c-0.356-0.543-0.969-0.902-1.667-0.902";
  ptr += "c-0.655,0-1.231,0.32-1.595,0.808l-0.011-0.007l-0.039,0.067c-0.021,0.03-0.035,0.063-0.054,0.094L22.78,37.692l0.008,0.004";
  ptr += "c-0.149,0.28-0.242,0.594-0.242,0.934c0,1.102,0.894,1.995,1.994,1.995v0.015h31.888c1.101,0,1.994-0.893,1.994-1.994";
  ptr += "C58.422,38.323,58.339,38.024,58.203,37.754z'fill=#FEE8B0 /><path d='M19.704,38.674l-0.013-0.004l13.544-23.522L25.13,1.156l-0.002,0.001C24.671,0.459,23.885,0,22.985,0";
  ptr += "c-0.84,0-1.582,0.41-2.051,1.038l-0.016-0.01L20.87,1.114c-0.025,0.039-0.046,0.082-0.068,0.124L0.299,36.851l0.013,0.004";
  ptr += "C0.117,37.215,0,37.62,0,38.059c0,1.412,1.147,2.565,2.565,2.565v0.015h16.989c-0.091-0.256-0.149-0.526-0.149-0.813";
  ptr += "C19.405,39.407,19.518,39.019,19.704,38.674z'fill=#F97B22 /></g></svg>";
  ptr += "</div>";
  ptr += "<div class='side-by-side text'>Light Luminosity</div>";
  ptr += "<div class='side-by-side reading'>";
  ptr += (int)B;
  ptr += "<span class='superscript'>cd</span></div>";
  ptr += "</div>";

  ptr += "</div>";
  ptr += "</body>";
  ptr += "</html>";
  return ptr;
}
