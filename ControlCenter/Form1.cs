using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.Runtime.InteropServices;
//using Microsoft.Win32;

namespace ControlCenter
{
    public partial class Form1 : Form
    {   
        public Form1()
        {
            InitializeComponent();
            DisableClickSounds();  //disable webbrowser click sound
            
            // Tell backgroundWorker to support cancellations
            this.backgroundWorker1.WorkerSupportsCancellation = true;

            // Tell backgroundWorker to report progress
            this.backgroundWorker1.WorkerReportsProgress = true;
        }

        private const int listenPort = 10815; //port for UDP discovery
        private const int BroadcastInterval = 5000; //interval in ms for sending broadcast discovery packets
        
        private bool BCrunning = false;

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

        List<UDPResponse> responseList;

        //crazy stuff to disable webbrowser click sounds
        // (thanks to stackexchange)
        const int FEATURE_DISABLE_NAVIGATION_SOUNDS = 21;
        const int SET_FEATURE_ON_PROCESS = 0x00000002;

        [DllImport("urlmon.dll")]
        [PreserveSig]
        [return: MarshalAs(UnmanagedType.Error)]
        static extern int CoInternetSetFeatureEnabled(
            int FeatureEntry,
            [MarshalAs(UnmanagedType.U4)] int dwFlags,
            bool fEnable);
        
        static void DisableClickSounds()
        {
            CoInternetSetFeatureEnabled(
                FEATURE_DISABLE_NAVIGATION_SOUNDS,
                SET_FEATURE_ON_PROCESS,
                true);
        }
        //end crazy stuff


        private List<UDPResponse> StartListener(String text)
        {
            List<UDPResponse> responseList = new List<UDPResponse>();
            IPEndPoint groupEP = new IPEndPoint(IPAddress.Any, listenPort);
            SendDiscover("#DSC-arduinosearch");
               
            UdpClient listener = new UdpClient(listenPort);
            listener.Client.ReceiveTimeout = BroadcastInterval;
                
            Stopwatch s = new Stopwatch();
            s.Start();
                
            

            while (BCrunning) //loop forever here
            {

                if (!s.IsRunning) // restart timer if necessary
                {
                    s = new Stopwatch();
                    s.Start();
                
                }
              
                if (s.Elapsed < TimeSpan.FromSeconds(5))  // timespan needs to be smaller than BroadcastInterval to work properly
                {
                    try //try to get data from UDP listener and process it
                    {
                        byte[] bytes = listener.Receive(ref groupEP);
                        string remoteIPstring = groupEP.ToString();
                        string[] split = remoteIPstring.Split(new Char[] { ':' });
                        IPAddress remoteip = IPAddress.Parse(split[0]);
                        string receivedstring = Encoding.ASCII.GetString(bytes, 0, bytes.Length);
                        //MessageBox.Show(remoteIPstring, receivedstring, MessageBoxButtons.OK);

                        if (receivedstring.Length > (text.Length + 1))  //enough data received?
                        {
                            if (receivedstring.Substring(0, text.Length) == text)  //answer packet matches search expression?
                            {

                                string receivedNumber = receivedstring.Substring((text.Length + 1),
                                    (receivedstring.Length - text.Length - 1)); //get ID from reply data
                                bool validNumber = true;

                                foreach (char c in receivedNumber) //check if receivedNumber contains only numbers
                                {
                                    if (!char.IsNumber(c)) { validNumber = false; }
                                }

                                if (validNumber)
                                {
                                    UDPResponse resp = new UDPResponse(remoteip, int.Parse(receivedNumber));
                                    responseList.Add(resp); //add discovered ID to list
                                    string URLstring = "http://" + (resp.Address.ToString()) + "/";
                                    //MessageBox.Show(URLstring, "calculated URL:", MessageBoxButtons.OK);
                                    if (!listBox1.Items.Contains(URLstring))
                                    {
                                        listBox1.Items.Add(URLstring);
                                    }

                                }
                                else
                                {
                 
                                }
                            }
                        }



                    }
                    catch (Exception exc)
                    {
                        if (exc.HResult != -2147467259) // if exception is no socket timeout which is expected
                        {
                            MessageBox.Show(exc.ToString(), "error", MessageBoxButtons.OK);
                        }
                    }
                    finally
                    {
                        
                    }
                
                } // end if
                else  // if listener ran into timeout, we end up here
                {
                    listener.Close();
                    s.Stop();
            
                    SendDiscover("#DSC-arduinosearch");
                    listener = new UdpClient(listenPort);
                    listener.Client.ReceiveTimeout = BroadcastInterval;
            
                }
                
            }  // break detected; BCrunning was set to false via button2
            listener.Close();
            s.Stop();
            return responseList;
        }

                
        private static void SendDiscover(string text)
        {
            Socket s = new Socket(AddressFamily.InterNetwork, SocketType.Dgram, ProtocolType.Udp);
            s.EnableBroadcast = true;

            //IPAddress broadcast = IPAddress.Parse("255.255.255.255");

            byte[] sendbuf = Encoding.ASCII.GetBytes(text);
            IPEndPoint ep = new IPEndPoint(IPAddress.Broadcast, listenPort);

            s.SendTo(sendbuf, ep);

            //Console.WriteLine("\ndiscover packet was sent to broadcast address {0}.\n", IPAddress.Broadcast.ToString());
            s.Close();
            
        }


        private void button1_Click(object sender, EventArgs e)
        {
            button1.Enabled = false;
            button2.Enabled = true;
            BCrunning = true;
            this.backgroundWorker1.RunWorkerAsync();
            
            //webBrowser1.Url = new Uri(URLstring, UriKind.Absolute); 
            //webBrowser1.Update();
        }


        private void button2_Click(object sender, EventArgs e)
        {
            BCrunning = false;
            this.backgroundWorker1.CancelAsync();
            
        }    


        private void webBrowser1_DocumentCompleted(object sender, WebBrowserDocumentCompletedEventArgs e)
        {
            label2.Text = "OK";
        
        }

        private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            string curItem = listBox1.SelectedItem.ToString();
            label2.Text = "loading webpage...";
            webBrowser1.AllowNavigation = true;
            webBrowser1.Url = new Uri(curItem, UriKind.Absolute);
        
        }

        private void webBrowser1_Navigated(object sender, WebBrowserNavigatedEventArgs e)
        {
        
        }

        private void backgroundWorker1_DoWork(object sender, DoWorkEventArgs e)
        {
            listBox1.Items.Clear();
            listBox1.Refresh();
            //MessageBox.Show("listbox cleared", "message", MessageBoxButtons.OK);
            label3.Text = "discovering devices...";
            this.backgroundWorker1.WorkerSupportsCancellation = true;
            responseList = StartListener("#DSC-arduinoreply");
        
        }

        private void backgroundWorker1_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            foreach (UDPResponse response in responseList)
            {
                string URLstring = "http://" + (response.Address.ToString()) + "/";
                //MessageBox.Show(URLstring, "calculated URL:", MessageBoxButtons.OK);
                if (!listBox1.Items.Contains(URLstring))
                {
                    listBox1.Items.Add(URLstring);
                }
            }

        }

        private void backgroundWorker1_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
         
            button1.Enabled = true;
            button2.Enabled = false;
            label3.Text = "discovery stopped.";
        }

        private void webBrowser1_Navigating(object sender, WebBrowserNavigatingEventArgs e)
        {
                label2.Text = "loading webpage...";
        
        }


        
    }
}
