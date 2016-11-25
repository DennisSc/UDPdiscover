#include <AltSoftSerial\AltSoftSerial.h>
//#include <SoftwareSerial.h>

// AltSoftSerial always uses these pins:
//
// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Teensy 3.0 & 3.1  21        20         22
// Teensy 2.0         9        10       (none)
// Teensy++ 2.0      25         4       26, 27
// Arduino Uno        9         8         10
// Arduino Leonardo   5        13       (none)
// Arduino Mega      46        48       44, 45
// Wiring-S           5         6          4
// Sanguino          13        14         12


#define DEBUG false
#define interval 29000 //interval (in ms) for UDP broadcast beacons - 29 seconds
#define interval2 1200000 //interval in ms for Wifi reset - 20 minutes
#define nodeID 9
#define BCaddress "192.168.0.255"
#define SSID "yourSSID"
#define PSK "password"


//SoftwareSerial esp8266(10,11); 
AltSoftSerial esp8266;

// timer counters for UDP beacon
long oldtime = 0;
long newtime = 0;

// timer counters for Wifi reset
long oldtime2 = 0;
long newtime2 = 0;


void setup()
{
	
	Serial.begin(9600);
	esp8266.begin(9600); // baud rate might be different

	pinMode(4, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);
	digitalWrite(4, HIGH);
	digitalWrite(5, HIGH);
	digitalWrite(6, HIGH);
	digitalWrite(7, HIGH);

	while (!WifiReset());

	
}

void loop()
{
	if (esp8266.available()) // check if the esp is sending something via uart 
	{

		delay(300); //let uart buffer fill up

		if (esp8266.find("+IPD,"))
		{

			int connID = esp8266.parseInt(); //parse first integer - this is the connection id


			esp8266.find("GET "); //advance cursor to start of URL

			char URLchar;
			URLchar = esp8266.read();  //read at least one char into URL (the dash sign)     

			String URL = "";
			while (URLchar != ' ' && URLchar != '=') //if there are more chars, read them into URL as well
			{
				URL += URLchar;
				URLchar = (esp8266.read());
			}

			char URLpin;
			if (URLchar == '=')
			{
				URLpin = esp8266.read();
			}

			if (DEBUG)
			{
				Serial.print("URL = ");
				Serial.println(URL);
			}

			String Path;
			Path = "/";

			String Param;
			Param = "/?toggle";

			if (DEBUG)
			{
				Serial.print("search string is: ");
				Serial.println(Path);

				Serial.print("Match detected: ");
				Serial.println(Path.equals(URL));
			}

			if (Param.equals(URL))
			{
				int iPin = int(URLpin) - 48;
				digitalWrite(iPin, (digitalRead(iPin) ^ 1));

				String Pinstring = "<html><meta http-equiv='refresh' content='1; url=/'/>";
				Pinstring += "Pin ";
				Pinstring += URLpin;
				Pinstring += " toggled... redirecting</html>";

				//sendHTTPline("<html><meta http-equiv='refresh' content='0; url = / '/>", 200, connID);
				sendHTTPline(Pinstring, 400, connID);
				
				if (DEBUG)
				{
					Serial.print("Pin ");
					Serial.print(iPin);
					Serial.println(" detected.");
				}
				
			}


			else if (Path.equals(URL))
			{
				int wait = 180;
				
				String webpage = "<html><h3>DSC micro webserver</h3>type: wireless<br>node ID: ";
				webpage += nodeID;
				webpage += "<hr><br>";
				sendHTTPline(webpage, wait+170, connID);
				
				//sendHTTPline("<br>hallo test 123 &auml; &deg;</html>", 300, connID);
			
				String Pinstring = "Pin 4 status: ";
				Pinstring += (digitalRead(4)^1);
				Pinstring += " &gt; <a href=\"/?toggle=4\">toggle</a><br><br>";
				sendHTTPline(Pinstring, wait, connID);

				Pinstring = "Pin 5 status: ";
				Pinstring += (digitalRead(5)^1);
				Pinstring += " &gt; <a href=\"/?toggle=5\">toggle</a><br><br>";
				sendHTTPline(Pinstring, wait, connID);
				
				Pinstring = "Pin 6 status: ";
				Pinstring += (digitalRead(6)^1);
				Pinstring += " &gt; <a href=\"/?toggle=6\">toggle</a><br><br>";
				sendHTTPline(Pinstring, wait, connID);
				
				Pinstring = "Pin 7 status: ";
				Pinstring += (digitalRead(7)^1);
				Pinstring += " &gt; <a href=\"/?toggle=7\">toggle</a><br><br></html>";
				sendHTTPline(Pinstring, wait, connID);
				
			}

			else
			{

				//sendHTTPline("HTTP/1.0 404 not found", 1000, connID);
				sendHTTPline("<html><h3>oops - page not found</h3>(HTTP 404)</html>", 200, connID);
			
			}

			String cipsend;
			cipsend = "AT+CIPCLOSE=";
			cipsend += connID;
			cipsend += "\r\n";

			sendData(cipsend, 200, DEBUG);
						
		
		}
	}
	newtime = millis(); //compare timer
	if (newtime > (oldtime + interval)) //if 5 seconds have elapsed
	{
		oldtime = newtime; //reset timer
		String UDPstring = "";
		UDPstring += "#DSC-arduinoreply-";
		UDPstring += nodeID;
		sendUDP(UDPstring, 200, 4);

	}


	newtime2 = millis(); //compare timer
	if (newtime2 > (oldtime2 + interval2)) //if 30 minutes have elapsed
	{
		oldtime2 = newtime2; //reset timer
		while (!WifiReset());

	}

}


void sendHTTPline(String input, int delay, int connID)
{
	input += "\r\n";
	String cipsend = "AT+CIPSEND=";
	cipsend += connID;
	cipsend += ",";
	cipsend += (input.length() - 1);
	cipsend += "\r\n";

	sendData(cipsend, delay, DEBUG);
	sendData(input, delay, DEBUG);
	//esp8266.println("\r\n");



}

void sendUDP(String input, int deley, int connID)
{
	String cipsend = "AT+CIPSTART=";
	cipsend += connID;
	cipsend += ",\"UDP\",\"";
	cipsend += BCaddress;
	cipsend += "\",10815,55555,2";
	cipsend += "\r\n";
	sendData(cipsend, deley, DEBUG);
	//esp8266.println("\r\n");
	//delay(500); //let uart buffer fill up

	cipsend = "AT+CIPSEND=";
	cipsend += connID;
	cipsend += ",";
	cipsend += (input.length());
	cipsend += "\r\n";

	sendData(cipsend, deley, DEBUG);
	sendData(input, deley, DEBUG);
	//esp8266.println("\r\n");

	cipsend = "AT+CIPCLOSE=";
	cipsend += connID;
	cipsend += "\r\n";

	sendData(cipsend, 200, DEBUG);


}



String sendData(String command, const int timeout, boolean debug)
{
	String response = "";

	esp8266.print(command); // send the read character to the esp8266

	long int time = millis();

	while ((time + timeout) > millis())
	{
		while (esp8266.available())
		{

			// The esp has data so display its output to the serial window 
			char c = esp8266.read(); // read the next character.
			response += c;
		}
	}

	if (debug)
	{
		Serial.print(response);
	}

	return response;
}

bool WifiReset()
{
	sendData("AT+RST\r\n", 2000, DEBUG); // reset module
	sendData("AT+CWMODE=3\r\n", 500, DEBUG); // configure as softAP (server mode)

	String CWJAPstring = "";
	CWJAPstring += "AT+CWJAP=\"";
	CWJAPstring += SSID;
	CWJAPstring += "\",\"";
	CWJAPstring += PSK;
	CWJAPstring += "\"\r\n";
	sendData(CWJAPstring, 5000, DEBUG);

	sendData("AT+CIFSR\r\n", 500, DEBUG); // check for ip address
	sendData("AT+CIPMUX=1\r\n", 500, DEBUG); // configure for multiple connections
	sendData("AT+CIPSERVER=1,80\r\n", 500, DEBUG); // enable listener on port 80
	return true;
}