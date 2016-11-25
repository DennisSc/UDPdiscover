using System;
using System.Net;
using System.Net.Sockets;
using System.Text;

public class UDPListener
{
    private const int listenPort = 10815;

    private static void StartListener(string text)
    {
        bool done = false;

        UdpClient listener = new UdpClient(listenPort);
        IPEndPoint groupEP = new IPEndPoint(IPAddress.Any, listenPort);

        try
        {
            while (!done)
            {
                Console.WriteLine("Waiting for UDP packet");
                byte[] bytes = listener.Receive(ref groupEP);
                string receivedstring = Encoding.ASCII.GetString(bytes, 0, bytes.Length);
                Console.WriteLine("Received packet from {0} : {1}\n",
                    groupEP.ToString(),
                    receivedstring);
                if (receivedstring == text)
                {
                    done = true;
                }
            }

        }
        catch (Exception e)
        {
            Console.WriteLine(e.ToString());
        }
        finally
        {
            listener.Close();
        }
    }

    public static int Main(string[] args)
    {
        StartListener(args[0]);

        return 0;
    }
}