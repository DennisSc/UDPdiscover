using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Collections.Generic;
using System.Diagnostics;


class Program
{
    private const int listenPort = 10815;
    
    struct UDPResponse
    {
               
        public IPAddress Address;
        public int Number;

        public UDPResponse(IPAddress IP, int ID)
        {
            Address = IP;
            Number = ID;
        }
    }


    private static void StartListener(string text)
    {
        bool done = false;

        UdpClient listener = new UdpClient(listenPort);
        listener.Client.ReceiveTimeout = 5000;
        IPEndPoint groupEP = new IPEndPoint(IPAddress.Any, listenPort);
        List<UDPResponse> responseList; 
        responseList = new List<UDPResponse>();
        
        Stopwatch s = new Stopwatch();
        s.Start();
        
        try
        {
            while (!done)
            {
                //Console.WriteLine("timespan {0}\n", TimeSpan.FromSeconds(5));
                //Console.WriteLine("s.elapsed {0}\n", s.Elapsed);
                if (s.Elapsed < TimeSpan.FromSeconds(5))
                {
                    Console.WriteLine("Waiting for replies");
                    byte[] bytes = listener.Receive(ref groupEP);
                    string remoteIPstring = groupEP.ToString();
                    string[] split = remoteIPstring.Split(new Char[] { ':' });
                    IPAddress remoteip = IPAddress.Parse(split[0]);
                    string receivedstring = Encoding.ASCII.GetString(bytes, 0, bytes.Length);
                    Console.WriteLine("\nReceived packet from {0}: {1}\n",
                            remoteip, receivedstring);

                    if (receivedstring.Length > (text.Length + 1))  //enough data received?
                    {
                        if (receivedstring.Substring(0, text.Length) == text)  //answer packet matches search expression?
                        {
                            
                            string receivedNumber = receivedstring.Substring((text.Length + 1),
                                (receivedstring.Length - text.Length - 1)); //get ID from reply data
                            bool validNumber = true;
                            
                            foreach(char c in receivedNumber) //check if receivedNumber contains only numbers
                            {
                                if (!char.IsNumber(c)) { validNumber = false; }
                            }
                            
                            if (validNumber)
                            {
                                UDPResponse resp = new UDPResponse(remoteip, int.Parse(receivedNumber));
                                responseList.Add(resp); //add discovered ID to list
                                Console.WriteLine("detected ID: {0}\n", receivedNumber); //show ID
                            }
                            else
                            {
                                Console.WriteLine("detected invalid character in ID: {0}\n", receivedNumber); //show error
                            }
                        }
                    }
                }
                else { done = true; }
            }
        }
        catch (Exception e)
        {
            if (e.HResult != -2147467259) // if exception is no socket timeout which is expected
            {
                Console.WriteLine(e.ToString()); // show thrown message
            }
        }
        finally   //clean up + show results
        {
            s.Stop();
            listener.Close();
            Console.WriteLine("\nfound {0} responses from the following IPs:\n", responseList.Count); //show list
            foreach (UDPResponse response in responseList)
            {
                Console.WriteLine("IP: {0} replied with ID: {1}", response.Address.ToString(), response.Number.ToString());
            }
        }
         
    }

    private static void SendDiscover(string text)
    {
        Socket s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
        s.EnableBroadcast = true;

        //IPAddress broadcast = IPAddress.Parse("255.255.255.255");

        byte[] sendbuf = Encoding.ASCII.GetBytes(text);
        IPEndPoint ep = new IPEndPoint(IPAddress.Broadcast, listenPort);

        s.SendTo(sendbuf, ep);

        Console.WriteLine("\ndiscover packet was sent to broadcast address {0}.\n", IPAddress.Broadcast.ToString());
        s.Close();
    }
    
    static void Main(string[] args)
    {
        SendDiscover(args[0]);
        //SendDiscover("hello");
        StartListener(args[1]);
        //StartListener("reply");
    
    }
}