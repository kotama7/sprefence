using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace SprefenceServer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private PortContent m_portContent;
        private SerialPort m_serialPort;
        private WebServer m_webServer;

        public MainWindow()
        {
            InitializeComponent();
            m_portContent = new PortContent();
            Port.DataContext = m_portContent;
            m_serialPort = new SerialPort();
            m_webServer = new WebServer(LogBox, SynchronizationContext.Current);
        }

        private void Disconnect_Click(object sender, RoutedEventArgs e)
        {
            m_serialPort.Close();
            LogBox.Text += "\nDisconnect : " + m_portContent.Ports[Port.SelectedIndex];
            m_webServer.Close();
            Port.IsEnabled = true;
            Search.IsEnabled = true;
            if (m_portContent.Ports.Any())
            {
                Connect.IsEnabled = true;
            }
            Disconnect.IsEnabled = false;
        }

        private void Search_Click(object sender, RoutedEventArgs e)
        {
            m_portContent.Ports.Clear();
            string[] ports = SerialPort.GetPortNames();
            foreach (string port in ports)
            {
                m_portContent.Ports.Add(port);
            }

            Port.SelectedIndex = -1;
            if(ports.Any())
            {
                Connect.IsEnabled = true;
            }
        }

        private void Connect_Click(object sender, RoutedEventArgs e)
        {
            if (Port.SelectedIndex < 0)
            {
                LogBox.Text += "\nError : Portが指定されていません．";
                return;
            }

            try
            {
                m_serialPort.BaudRate = 115200;
                m_serialPort.Parity = Parity.None;
                m_serialPort.DataBits = 8;
                m_serialPort.StopBits = StopBits.One;
                m_serialPort.Handshake = Handshake.None;
                m_serialPort.PortName = m_portContent.Ports[Port.SelectedIndex];
                m_serialPort.ReadBufferSize = 1024 * 1024;
                m_serialPort.ReadTimeout = 1000;
                m_serialPort.ReceivedBytesThreshold = 8 * 75;
                m_serialPort.DiscardNull = false;
                m_serialPort.Open();
                while (m_serialPort.BytesToRead > 0)
                {
                    m_serialPort.ReadByte();
                }
                LogBox.Text += "Connect : " + m_portContent.Ports[Port.SelectedIndex];
                Search.IsEnabled = false;
                Connect.IsEnabled = false;
                Port.IsEnabled = false;
                Disconnect.IsEnabled = true;
                m_webServer.Open(m_serialPort);
            }
            catch(Exception err)
            {
                LogBox.Text += "\nFailed : " + err.ToString();
            }
        }
    }
}
