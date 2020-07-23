/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/packet-sink.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SixthScriptExample");

// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |    ns-3 TCP    |    |    ns-3 TCP    |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
//
// We want to look at changes in the ns-3 TCP congestion window.  We need
// to crank up a flow and hook the CongestionWindow attribute on the socket
// of the sender.  Normally one would use an on-off application to generate a
// flow, but this has a couple of problems.  First, the socket of the on-off
// application is not created until Application Start time, so we wouldn't be
// able to hook the socket (now) at configuration time.  Second, even if we
// could arrange a call after start time, the socket is not public so we
// couldn't get at it.
//
// So, we can cook up a simple version of the on-off application that does what
// we want.  On the plus side we don't need all of the complexity of the on-off
// application.  On the minus side, we don't have a helper, so we have to get
// a little more involved in the details, but this is trivial.
//
// So first, we create a socket and do the trace connect on it; then we pass
// this socket into the constructor of our simple application which we then
// install in the source node.
// ===========================================================================
//
class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_peer;
  uint32_t        m_packetSize;
  uint32_t        m_nPackets;
  DataRate        m_dataRate;
  EventId         m_sendEvent;
  bool            m_running;
  uint32_t        m_packetsSent;
};

MyApp::MyApp ()
  : m_socket (0),
    m_peer (),
    m_packetSize (0),
    m_nPackets (0),
    m_dataRate (0),
    m_sendEvent (),
    m_running (false),
    m_packetsSent (0)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId (void)
{
  static TypeId tid = TypeId ("MyApp")
    .SetParent<Application> ()
    .SetGroupName ("Tutorial")
    .AddConstructor<MyApp> ()
    ;
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  SendPacket ();
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_socket->Send (packet);

  if (++m_packetsSent < m_nPackets)
    {
      ScheduleTx ();
    }
}

void
MyApp::ScheduleTx (void)
{
  if (m_running)
    {
      Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
      m_sendEvent = Simulator::Schedule (tNext, &MyApp::SendPacket, this);
    }
}

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  NS_LOG_UNCOND (Simulator::Now ().GetSeconds () << "\t" << newCwnd);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << newCwnd << std::endl;
}
//destination tcp port
uint16_t sinkPort = 8080;
Ptr<FlowMonitor> monitor;
FlowMonitorHelper flowmon;

static void
packetDrop(Ptr<OutputStreamWrapper> stream){
   
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    uint32_t packets=0;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
    {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

        if (t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.2")&&t.destinationPort==sinkPort )
        {
           
            packets+=iter->second.lostPackets;
        }
    }
  
  	*stream->GetStream() << Simulator::Now().GetSeconds() << " "  << packets << std::endl;
	Simulator::Schedule(Seconds(0.0001), &packetDrop, stream); // Schedule next sample
}
Ptr<PacketSink> AllSinks[6];

static void
Databytes(Ptr<OutputStreamWrapper> stream)
{int totalVal=0;
   // totalVal = tcpSink->GetTotalRx();

    for(int i=0; i<6; i++)
    {
        totalVal += AllSinks[i]->GetTotalRx();
    }

    *stream->GetStream()<<Simulator::Now ().GetSeconds ()<<" " <<totalVal<<std::endl;

    Simulator::Schedule(Seconds(0.0001),&Databytes, stream);
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
      double CBRtiming[5][2]={{200,1800},{400,1800},{600,1200},{800,1400},{1000,1600}};//{start time,end time}

  //  std::string tcpTypes[5]={"ns3::TcpNewReno","ns3::TcpHybla","ns3::TcpWestwood","ns3::TcpScalable","ns3::TcpVegas"};
//for(int i=0;i<5;i++){
std::string mystr=argv[1];

   AsciiTraceHelper asciiTraceHelper;
  std::string a1="./Result/"+mystr+"cwnd.cwnd";
  std::string a2="./Result/"+mystr+"drop"+".drop";
  std::string a3="./Result/"+mystr+"Output_data_bytes.txt";

  Ptr<OutputStreamWrapper> filecwnd = asciiTraceHelper.CreateFileStream (a1);
  Ptr<OutputStreamWrapper> filedrop = asciiTraceHelper.CreateFileStream (a2);
  Ptr<OutputStreamWrapper> filedatabytes = asciiTraceHelper.CreateFileStream (a3);

  //creating nodes
  NodeContainer nodes;
  nodes.Create (2);
  std::cout<<mystr<<std::endl;
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (mystr));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));
  pointToPoint.SetQueue ("ns3::DropTailQueue","MaxSize", StringValue ("2500B"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.00001));
  devices.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  
  Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
  //create sinkapp(sentination tcp) at n1
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), sinkPort));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (1));
  sinkApps.Start (Seconds (0.));
  sinkApps.Stop (Seconds (1.8));
 AllSinks[0] = DynamicCast<PacketSink> (sinkApps.Get (0));
  Ptr<Socket> ns3TcpSocket = Socket::CreateSocket (nodes.Get (0), TcpSocketFactory::GetTypeId ());
//create source app
  Ptr<MyApp> app = CreateObject<MyApp> ();
  app->Setup (ns3TcpSocket, sinkAddress, 100, 10000, DataRate ("10Mbps"));
  nodes.Get (0)->AddApplication (app);
  app->SetStartTime (Seconds (0.));
  app->SetStopTime (Seconds (1.8));
  uint16_t cbrport = 9000;
    for(int j=1;j<6;j++)
    {   ApplicationContainer cbrsource;
        ApplicationContainer cbrrcvr;
        //create cbrrcvr
        PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), cbrport+j));
        cbrrcvr = sink.Install (nodes.Get (1));
        cbrrcvr.Start (MilliSeconds(0));
        cbrrcvr.Stop (MilliSeconds(1800));
        AllSinks[j] = DynamicCast<PacketSink> (cbrrcvr.Get (0));
       // Sinks[j]=DynamicCast<PacketSink> (cbrrcvr.Get (0));
       // cbrSinks[j-1] = DynamicCast<PacketSink> (cbrrcvr.Get (0));

        //create cbr source
        OnOffHelper onoff("ns3::UdpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), cbrport+j));
        onoff.SetAttribute("OnTime",  StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onoff.SetAttribute ("DataRate", StringValue ("250Kbps"));
       
        onoff.SetAttribute ("StartTime", TimeValue (MilliSeconds (CBRtiming[j-1][0])));
        onoff.SetAttribute ("StopTime", TimeValue (MilliSeconds (CBRtiming[j-1][1])));
        cbrsource.Add(onoff.Install(nodes.Get(0)));
    }
 //trace for congestion window..
  ns3TcpSocket->TraceConnectWithoutContext ("CongestionWindow", MakeBoundCallback (&CwndChange, filecwnd));

 //flow Monitor
   monitor = flowmon.InstallAll();
  Simulator::Schedule(MilliSeconds (0.1), &packetDrop, filedrop);
  Simulator::Schedule(Seconds(0.0001),&Databytes, filedatabytes);

  Simulator::Stop (Seconds (1.8));
  Simulator::Run ();

uint16_t cbr1port=9001;
    uint16_t cbr2port=9002;
    uint16_t cbr3port=9003;
    uint16_t cbr4port=9004;
    uint16_t cbr5port=9005;
    //ipv4classifier:Classifies packets by looking at their IP and TCP/UDP headers.
    //From these packet headers, a tuple (source-ip, destination-ip, protocol, source-port, destination-port) 
    //is created(called FiveTuple), and a unique flow identifier is assigned for each different tuple combination

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
    //GetFlowStats() Retrieve all collected the flow statistics.
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
    std::cout<<"FLOW MONTOR"<<std::endl;
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
    {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

        if (t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.2")&&t.destinationPort==sinkPort)
        {
            std::cout<<"FTP"<<std::endl;
            std::cout<<"Tx Packets = " << iter->second.txPackets<<std::endl;
            std::cout<<"Tx Bytes=" << iter->second.txBytes << std::endl;
            std::cout<<"Rx Packets = " << iter->second.rxPackets<<std::endl;
            std::cout<<"Rx Bytes=" << iter->second.rxBytes << std::endl;
            std::cout<<"Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"<<std::endl;
        }
        if (t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.2")&&t.destinationPort==cbr1port)
        {
            std::cout<<"CBR 1"<<std::endl;
            std::cout<<"Tx Packets = " << iter->second.txPackets<<std::endl;
            std::cout<<"Tx Bytes=" << iter->second.txBytes << std::endl;
            std::cout<<"Rx Packets = " << iter->second.rxPackets<<std::endl;
            std::cout<<"Rx Bytes=" << iter->second.rxBytes << std::endl;
            std::cout<<"Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"<<std::endl;
        }
        if (t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.2")&&t.destinationPort==cbr2port)
        {
            std::cout<<"CBR 2"<<std::endl;
            std::cout<<"Tx Packets = " << iter->second.txPackets<<std::endl;
            std::cout<<"Tx Bytes=" << iter->second.txBytes << std::endl;
            std::cout<<"Rx Packets = " << iter->second.rxPackets<<std::endl;
            std::cout<<"Rx Bytes=" << iter->second.rxBytes << std::endl;
            std::cout<<"Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"<<std::endl;
        }
        if (t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.2")&&t.destinationPort==cbr3port)
        {   std::cout<<"CBR 3"<<std::endl;
            std::cout<<"Tx Packets = " << iter->second.txPackets<<std::endl;
            std::cout<<"Tx Bytes=" << iter->second.txBytes << std::endl;
            std::cout<<"Rx Packets = " << iter->second.rxPackets<<std::endl;
            std::cout<<"Rx Bytes=" << iter->second.rxBytes << std::endl;
            std::cout<<"Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"<<std::endl;
        }
        if (t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.2")&&t.destinationPort==cbr4port)
        {
            std::cout<<"CBR 4"<<std::endl;
           std::cout<<"Tx Packets = " << iter->second.txPackets<<std::endl;
            std::cout<<"Tx Bytes=" << iter->second.txBytes << std::endl;
            std::cout<<"Rx Packets = " << iter->second.rxPackets<<std::endl;
            std::cout<<"Rx Bytes=" << iter->second.rxBytes << std::endl;
            std::cout<<"Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"<<std::endl;
        }
        if (t.sourceAddress == Ipv4Address("10.1.1.1") && t.destinationAddress == Ipv4Address("10.1.1.2")&&t.destinationPort==cbr5port)
        {
            std::cout<<"CBR 5"<<std::endl;
            std::cout<<"Tx Packets = " << iter->second.txPackets<<std::endl;
            std::cout<<"Tx Bytes=" << iter->second.txBytes << std::endl;
            std::cout<<"Rx Packets = " << iter->second.rxPackets<<std::endl;
            std::cout<<"Rx Bytes=" << iter->second.rxBytes << std::endl;
            std::cout<<"Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps"<<std::endl;
            std::cout<<"\n"<<std::endl;
        }
    }
    std::string a4="./Result/"+mystr+".flowmon";
    monitor->SerializeToXmlFile(a4, true, true);
  
  Simulator::Destroy ();
  return 0;
}

