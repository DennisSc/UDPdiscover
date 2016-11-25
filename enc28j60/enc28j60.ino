#include <Ethercard\EtherCard.h>
#include <IPAddress.h>
#include <DHT11\dht11.h>


dht11 DHT11;
#define DHT11PIN A1

float humi = 0;
float temp = 0;
float dewp;

const long interval = 5000;
long oldtime = 0;
long newtime = 0;

#define led 5
#define udpport 10815
#define nodeID 333

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/mygw values below)

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe };

#if STATIC

// ethernet interface ip address
static byte myip[] = { 192, 168, 119, 222 };

// ethernet interface gateway address
static byte mygw[] = { 192, 168, 119, 2 };

// ethernet interface dns server address
static byte mydns[] = { 192, 168, 119, 120 };

// ethernet interface subnet mask
static byte mymask[] = { 255, 255, 255, 0 };
#endif

byte Ethernet::buffer[1000];
BufferFiller bfill;


// called when a ping comes in (replies to it are automatic)
static void gotPinged(byte* ptr) {
	ether.printIp(">>> ping from: ", ptr);
	digitalWrite(led, (digitalRead(led) ^ 1));

}


void setup()
{
	pinMode(led, OUTPUT);
	digitalWrite(led, HIGH);

	pinMode(4, OUTPUT);
	digitalWrite(4, HIGH);
	pinMode(6, OUTPUT);
	digitalWrite(6, HIGH);
	pinMode(7, OUTPUT);
	digitalWrite(7, HIGH);


  /* add setup code here */
	Serial.begin(9600);

	

	if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
	{
		Serial.println("Failed to access Ethernet controller"); 
	}
	else
	{
#if STATIC
		ether.staticSetup(myip, mygw, mydns, mymask);
#else
		if (!ether.dhcpSetup())
		{
			Serial.println("DHCP failed");
		}
#endif
		ether.printIp("IP:  ", ether.myip);
		ether.printIp("GW:  ", ether.gwip);
		ether.printIp("DNS: ", ether.dnsip);
		ether.printIp("Netmask: ", ether.netmask);

	}
	
	// call this to report others pinging us
	ether.registerPingCallback(gotPinged);
	ether.udpServerListenOnPort(&udpComm, udpport);
	
}


void udpComm(word port, byte ip[4], const char *data, word len)
{
	IPAddress src(ip[0], ip[1], ip[2], ip[3]);
	/*Serial.println(src);
	Serial.println(port);
	Serial.println(data);
	Serial.println(len);
	*/
	if (!strncmp("#DSC-arduinosearch", data, len) != 0)
	{
		Serial.print("received UDP discover packet from ");
		Serial.println(src);
		
		delay(300); // give the control center some time to open socket listener after sending broadcast

		String UDPresponse = "#DSC-arduinoreply-"; // generate response string
		UDPresponse += nodeID;
		int UDPlen = UDPresponse.length();

		Serial.print("sending UDP response \""); //show response
		Serial.print(UDPresponse);
		Serial.print("\" with length ");
		Serial.println(UDPlen);

		char UDPresp[22];  //convert response string to char array
		UDPresponse.toCharArray(UDPresp, UDPlen+1);
		//Serial.println(UDPresp);

		ether.sendUdp(UDPresp, UDPlen, 55555, ip, udpport); //send response
		Serial.println("done.");
	}
}


static word http_Unauthorized() {
	bfill = ether.tcpOffset();
	bfill.emit_p(PSTR(
		"HTTP/1.1 401 Unauthorized\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<h3>401 Unauthorized</h3>\r\n\r\n"));
	return bfill.position();
}

static word http_redirect(){
	bfill = ether.tcpOffset();
	bfill.emit_p(PSTR(
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Pragma: no-cache\r\n"
		"\r\n"
		"<meta http-equiv='refresh' content='1; url = / '/>"
		"Pin toggled... <br>redirecting\r\n\r\n"
		));
	return bfill.position();
}

/*
static word http_ok() {
	bfill = ether.tcpOffset();
	bfill.emit_p(PSTR(
		"HTTP/1.0 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Pragma: no-cache\r\n\r\n"));
	return bfill.position();
}*/

static word homePage(const char* something, float humi, float temp, float dewp) {
	long t = millis() / 1000;
	word h = t / 3600;
	byte m = (t / 60) % 60;
	byte s = t % 60;
	
	bfill = ether.tcpOffset();
	bfill.emit_p(PSTR(
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Pragma: no-cache\r\n"
		"\r\n"
		"<meta http-equiv='refresh' content='5'/>"
		"<title>DSCmicro server</title>"
		"<h3>DSC micro webserver</h3>"
		"Type: cable<br>node ID: $D<hr>"
		"System uptime: $D$D:$D$D:$D$D<br><br>"
		//"&Uuml;bergebener String: $S<br>"
		"Digital Pin 4 Status: $D<br>"
		"Digital Pin 5 Status: $D<br>"
		"Digital Pin 6 Status: $D<br>"
		"Digital Pin 7 Status: $D<br>"
		//"Digital Pin 8 Status: $D<br>"
		"<style type = \"text/css\">"
		"BUTTON.btn1{ height:26px; width:60px; font-size:12px; text-align:center; }"
		"</style>"
		"<br><form action=\" / \" method=\"get\">"
		"<button class=\"btn1\" name=\"toggle\" type=\"submit\" value=\"4\">4</button><br>"
		"<button class=\"btn1\" name=\"toggle\" type=\"submit\" value=\"5\">5</button><br>"
		"<button class=\"btn1\" name=\"toggle\" type=\"submit\" value=\"6\">6</button><br>"
		"<button class=\"btn1\" name=\"toggle\" type=\"submit\" value=\"7\">7</button><br>"
		//"<button name=\"toggle\" type=\"submit\" value=\"8\">toggle Pin 8</button>"
		"</form>"
		//"<br>Luftfeuchtigkeit: $T %<br>"
		//"Temperatur: $T&deg; C<br>"
		"Luftfeuchtigkeit: $D %<br>"
		"Temperatur: $D&deg; C<br>"
		"Taupunkt: $T&deg; C"
		),
		nodeID,
		h / 10, h % 10, m / 10, m % 10, s / 10, s % 10, 
		//something, 
		(digitalRead(4)^1),
		(digitalRead(5)^1),
		(digitalRead(6)^1),
		(digitalRead(7)^1),
		//digitalRead(8),
		//humi, 
		//temp,
		int(humi), 
		int(temp),
		dewp);
	return bfill.position();
}

double dewPointFast(double celsius, double humidity)
{
	double a = 17.271;
	double b = 237.7;
	double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
	double Td = (b * temp) / (a - temp);
	return Td;
}



void loop()
{
	
	newtime = millis(); //compare timer
	if (newtime > (oldtime + interval)) //if 5 seconds have elapsed
	{
		oldtime = newtime; //reset timer
		
		DHT11.read(DHT11PIN); //read dht11 sensor + calculate values
		humi = (float)DHT11.humidity;
		temp = (float)DHT11.temperature;
		dewp = dewPointFast(temp, humi);
		//Serial.print("Luftfeuchtigkeit (%):\t");
		//Serial.println(humi, 2);
		//Serial.print("Temperatur (\260C):\t");
		//Serial.println(temp, 2);


	}

	// check for ethernet input
	word len = ether.packetReceive();
	word pos = ether.packetLoop(len);


	if (pos) // check if valid tcp data is received
	{
		delay(1);
		bfill = ether.tcpOffset();
		char *data = (char *)Ethernet::buffer + pos; //load input to array "data"

		if (strncmp("GET /", data, 5) != 0) //if its no HTTP GET request
		{
			ether.httpServerReply(http_Unauthorized()); //then send 401
		}
		else //if valid GET request is found
		{
			data += 5; //jump to URL
			if (data[0] == ' ') // if URL is empty/root
			{
				ether.httpServerReply(homePage("", humi, temp, dewp)); // send homepage data
			}
			else if (!strncmp("?toggle=", data, 8)) //if URL contains switch value
			{
				data += 8;
				int URLpin = data[0] - '0';
				Serial.println(URLpin);
				if ((URLpin >= 4) && (URLpin <= 7))
				{
					digitalWrite(URLpin, (digitalRead(URLpin) ^ 1)); //toggle pin
				}
				DHT11.read(DHT11PIN); //read sensor
				//Serial.print("Luftfeuchtigkeit (%):\t");  //debug
				humi = (float)DHT11.humidity;
				//Serial.println(humi, 2); //debug
				//Serial.print("Temperatur (\260C):\t");  //debug
				temp = (float)DHT11.temperature;
				//Serial.println(temp, 2); //debug

				digitalRead(URLpin);
				ether.httpServerReply(http_redirect()); // return to main page
			}
			else //if no valid URL was found
			{
				ether.httpServerReply(http_Unauthorized()); //send 401
			}

		}
	}
}

	