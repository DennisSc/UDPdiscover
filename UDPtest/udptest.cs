using System;
using System.Net;
using System.Net.Sockets;
using System.Text;



class Program
{
    
    private static void SendDiscover(string text)
    {
        Socket s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);

        s.EnableBroadcast = true;

        //IPAddress broadcast = IPAddress.Parse("255.255.255.255");

        byte[] sendbuf = Encoding.ASCII.GetBytes(text);
        IPEndPoint ep = new IPEndPoint(IPAddress.Broadcast, 10815);

        s.SendTo(sendbuf, ep);

        Console.WriteLine("Message sent to the broadcast address");

    }

    static void Main(string[] args)
    {
        SendDiscover(args[0]);
        //SendDiscover("hello");
        
    }
}