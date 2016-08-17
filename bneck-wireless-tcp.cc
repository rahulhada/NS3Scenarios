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
 *
 *
 * Author : Rahul Hada <hada.rahul@gmail.com>
 */

 /*
 * Network Topology
 *
 *  LeftWifi 10.1.3.0
 *                       AP1				AP2
 * *    . . .   *    *    *					 *   *    *    	    *
 * |    		|    |    |             	 |   |    | . . .   |
 * n10   		n6   n7   n0 -------------- n1   n2   n3       n10
 *                           point-to-point
 *										     RightWifi 10.1.2.0
 *	This example scenario is used to analyze the performance of TCP Variants for the
 *	above topology.In this we are basically measuring the Throughput and change in
 *	congestion window size.
 *  User Defined Variables
 *  Number of Station Nodes on Right --> 10
 *  Number of Station Nodes on Left  --> 10
 *  To modify the number of station nodes you can use nRightWifi and nLeftWifi variables.
 *  Using command line:-
 *  ./waf --run "scratch/bneck-wireless-tcp --nRightWifi=2"
 *  ./waf --run "scratch/bneck-wireless-tcp --nLeftWifi=2"
 *  .
 *  .
 *  ./waf --run "scratch/bneck-wireless-tcp --nRightWifi=10"
 *  ./waf --run "scratch/bneck-wireless-tcp --nLeftWifi=10"
 *  To modify the TCP variants (NewReno,Tahoe,Westwood etc.) we can use tcpType variable.
 *  Using command line:-
 *  ./waf --run "scratch/bneck-wireless-tcp --tcpType="
 *
 *  The scenario suggested my Anjali Chaure <anjali.chaure91@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/config-store.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Bneck-Wireless-TCP");

void changeWindow(const uint32_t oldValue,uint32_t newValue)
{
	NS_LOG_INFO("cwnd-size:"<<Simulator::Now().GetSeconds()<<"  "<<newValue);
}
void flowControl(const uint32_t oldValue,uint32_t newValue)
{
	NS_LOG_INFO("flow:"<<Simulator::Now().GetSeconds()<<"  "<<newValue);
}
void traceRTO(const Time oldValue,Time newValue)
{
	NS_LOG_INFO(Simulator::Now().GetSeconds()<<"  "<<newValue.GetSeconds());
}
void stateChange(const TcpSocket::TcpStates_t oldvalue,const TcpSocket::TcpStates_t newvalue)
{
	NS_LOG_INFO("State--Change"<<"--"<<Simulator::Now().GetSeconds()<<"--"<<newvalue);
}
void seqChange(SequenceNumber32 oldSeq, SequenceNumber32 newSeq)
{
	NS_LOG_INFO("Old SeqNo:"<<Simulator::Now().GetSeconds()<<oldSeq.GetValue()<<"--"<<newSeq.GetValue());
}
void highSeqChange(SequenceNumber32 oldSeq, SequenceNumber32 newSeq)
{
	NS_LOG_INFO(Simulator::Now().GetSeconds()<<"----"<<"NewSeqNo:"<<newSeq.GetValue()<<"----"<<"Old SeqNo:"<<oldSeq.GetValue());
}
class TestApp:public Application
{
public:
		TestApp();
		virtual ~TestApp();
		void Setup(Ptr<Socket> socket,Address address , uint32_t packetSize,uint32_t nPackets,DataRate datarate);
private:
		virtual void StartApplication(void);
		virtual void StopApplication(void);

		void ScheduleTx(void);
		void SendPacket(void);

		Ptr<Socket> 	m_socket;
		Address 		m_peer;
		uint32_t        m_packetSize;
		uint32_t        m_nPackets;
		DataRate        m_dataRate;
		EventId         m_sendEvent;
		bool            m_running;
		uint32_t        m_packetsSent;
};

TestApp::TestApp()
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

TestApp::~TestApp()
{
	m_socket=0;
}

void TestApp::Setup(Ptr<Socket> socket,Address address,uint32_t packetSize,uint32_t nPackets,DataRate datarate)
{
	m_socket=socket;
	m_peer=address;
	m_packetSize=packetSize;
	m_nPackets=nPackets;
	m_dataRate=datarate;
}

void TestApp::StartApplication(void)
{
	m_running=true;
	m_packetsSent=0;
	m_socket->Bind();
	m_socket->Connect(m_peer);
	SendPacket();

}
void TestApp::StopApplication(void)
{
	m_running=false;
	if(m_sendEvent.IsRunning())
	{
		Simulator::Cancel(m_sendEvent);
	}
	if(m_socket)
	{
		m_socket->Close();
	}
}

void TestApp::SendPacket(void)
{
	Ptr<Packet> packet=Create<Packet> (m_packetSize);
	m_socket->Send(packet);
	if(++m_packetsSent<m_nPackets)
	{
		ScheduleTx();
	}
}
void TestApp::ScheduleTx(void)
{
	if(m_running)
	{
		Time tNext(Seconds(m_packetSize*8/static_cast<double> (m_dataRate.GetBitRate())));
		m_sendEvent=Simulator::Schedule(tNext,&TestApp::SendPacket,this);
	}
}

int
main (int argc, char *argv[])
{
  LogComponentEnable ("Bneck-Wireless-TCP", LOG_LEVEL_INFO);
  uint32_t nRightWifi = 10;
  uint32_t nLeftWifi = 10;
  std::string tcpType = "NewReno";

  bool cwndTraceEnable = false;
  bool rwndTraceEnable =false;
  bool rtoTraceEnable = false;
  bool stateTraceEnable = false;
  bool nextTSeqTraceEnable = false;
  bool highSeqTraceEnable = true;
  bool enableAnim=false;

  CommandLine cmd;
  cmd.AddValue ("nRightWifi", "Number of right Wifi Station Nodes", nRightWifi);
  cmd.AddValue ("nLeftWifi", "Number of wifi STA devices", nLeftWifi);

  cmd.AddValue("cwndTraceEnable","Enable the Congestion Window Trace",cwndTraceEnable);
  cmd.AddValue("rwndTraceEnable","Enable the RWND Trace",rwndTraceEnable);
  cmd.AddValue("rtoTraceEnable","Enable the RTO Trace",rtoTraceEnable);
  cmd.AddValue("stateTraceEnable","Enable the State Change Trace",stateTraceEnable);
  cmd.AddValue("nextTSeqTraceEnable","Enable the Next Tx Sequence Trace",nextTSeqTraceEnable);
  cmd.AddValue("highSeqTraceEnable","Enable the High Sequence Trace",highSeqTraceEnable);

  cmd.AddValue("tcpType","Change TCP variant(NewReno,Reno,Tahoe,Westwood etc.)",tcpType);
  cmd.AddValue("enableAnim","Enable/Disable Animation",enableAnim);
  cmd.Parse (argc,argv);
  Config::SetDefault("ns3::TcpL4Protocol::SocketType",TypeIdValue(TypeId::LookupByName("ns3::Tcp" + tcpType)));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",UintegerValue (10));

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer rightWifiNodes,ap2WifiNodes;
  rightWifiNodes.Create (nRightWifi);
  ap2WifiNodes.Add(p2pNodes.Get(1));

  NodeContainer leftWifiNodes,ap1WifiNodes;
  leftWifiNodes.Create (nLeftWifi);
  ap1WifiNodes = p2pNodes.Get (0);


  NetDeviceContainer rightWifiDevices,ap2WifiDevices;
  NetDeviceContainer leftWifiDevices,ap1WifiDevices;

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	 YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();
  Ssid ssid;
  ssid=Ssid ("Right-Wifi");
      mac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));

 /*
  * Install PHY , MAC on right wifi station nodes
  */
  rightWifiDevices=wifi.Install(phy,mac,rightWifiNodes);

  mac.SetType ("ns3::ApWifiMac",
                     "Ssid", SsidValue (ssid));
  /*
   * Install PHY, MAC on right access point node
   */
  ap2WifiDevices=wifi.Install(phy,mac,ap2WifiNodes);

  ssid=Ssid ("Left-Wifi");
  mac.SetType ("ns3::StaWifiMac",
                     "Ssid", SsidValue (ssid),
                     "ActiveProbing", BooleanValue (false));
  /*
   * Install PHY , MAC on left station nodes
   */
  leftWifiDevices=wifi.Install(phy,mac,leftWifiNodes);

  mac.SetType ("ns3::ApWifiMac",
                       "Ssid", SsidValue (ssid));
  /*
   * Install PHY , MAC on left access point node
   */
  ap1WifiDevices=wifi.Install(phy,mac,ap1WifiNodes);


  MobilityHelper mobility;

  /*
   * Set and install the mobility model and position allocator for left station nodes
   */
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (leftWifiNodes);
 /*
  * Set and install the mobility model and position allocator for right station nodes
  */
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (150.0),
                                   "MinY", DoubleValue (50.0),
                                   "DeltaX", DoubleValue (5.0),
                                   "DeltaY", DoubleValue (10.0),
                                   "GridWidth", UintegerValue (3),
                                   "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
		  	  	  	  	  	  	   "Bounds", RectangleValue (Rectangle (100,300,40,80)));
  mobility.Install(rightWifiNodes);

  /*
   * Set and install the mobility model and position allocator for
   * left/right access points
   */
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (35.0),
                                   "MinY", DoubleValue (40.0),
                                   "DeltaX", DoubleValue (60.0),
                                   "DeltaY", DoubleValue (150.0),
                                   "GridWidth", UintegerValue (2),
                                   "LayoutType", StringValue ("RowFirst"));
  mobility.Install (p2pNodes);

  InternetStackHelper stack;
  OlsrHelper olsr;
  stack.SetRoutingHelper(olsr);
  stack.Install (rightWifiNodes);
  stack.Install (leftWifiNodes);
  stack.Install(p2pNodes);

 /*
  * Assign the IP address to the first interface of each access points
  */
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  address.Assign (p2pDevices);

  /*
   * Assign IP address to right wifi station nodes
   * Assign IP address to right access point
   */
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer rightInterfaces;
  rightInterfaces=address.Assign(rightWifiDevices);
  address.Assign(ap2WifiDevices);
  /*
   * Assign IP address to left wifi station nodes
   */
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer leftInterfaces;
  leftInterfaces=address.Assign(leftWifiDevices);
  /*
   * Assign IP address to left access point
   */
  address.Assign(ap1WifiDevices);

  /*
   *
   */
  Address anyAddress,sinkAddress;
  uint16_t sinkPort=6000;
  anyAddress=InetSocketAddress(Ipv4Address::GetAny(),sinkPort);
  sinkAddress=InetSocketAddress(rightInterfaces.GetAddress(1),sinkPort);

  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", anyAddress);
  ApplicationContainer sinkApp=packetSinkHelper.Install(rightWifiNodes.Get(1));
  sinkApp.Start(Seconds(30.0));
  sinkApp.Stop(Seconds(40.0));

  /*
   * Create TestApp using TCP socket and install it
   * on one of the left and right wifi station nodes
   */
  Ptr<TestApp> app = CreateObject<TestApp> ();
  Ptr<Socket> ns3TcpSocket =Socket::CreateSocket(leftWifiNodes.Get(1),TcpSocketFactory::GetTypeId());
  app->Setup(ns3TcpSocket, sinkAddress, 1200, 1000, DataRate ("1Mbps"));
  leftWifiNodes.Get(1)->AddApplication(app);
  app->SetStartTime(Seconds(35.0));
  app->SetStopTime(Seconds(40.0));
  /*
   * Tracing of TCP Data like cwnd , rwnd , rto etc.
   */
  if(cwndTraceEnable)
	  ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow",MakeCallback(&changeWindow));
  else if(rwndTraceEnable)
  {
	  NS_LOG_INFO("Trace:RWND");
	  ns3TcpSocket->TraceConnectWithoutContext("RWND",MakeCallback(&flowControl));
  }
  else if(rtoTraceEnable)
  {
	  NS_LOG_INFO("Trace:RTO");
	  ns3TcpSocket->TraceConnectWithoutContext("RTO",MakeCallback(&traceRTO));// Not Working
  }
  else if(stateTraceEnable)
  {
	  NS_LOG_INFO("Trace:State Change");
	  ns3TcpSocket->TraceConnectWithoutContext("State",MakeCallback(&stateChange));
  }
  else if(nextTSeqTraceEnable)
  {
	  NS_LOG_INFO("Trace:Next Seq");
	  ns3TcpSocket->TraceConnectWithoutContext("NextTxSequence",MakeCallback(&seqChange));
  }

  else if(highSeqTraceEnable)
  {
	  NS_LOG_INFO("Trace:Highest Seq");
	  ns3TcpSocket->TraceConnectWithoutContext("HighestSequence",MakeCallback(&highSeqChange));
  }

  else
	  NS_LOG_INFO("NO Trace");
  /*
   * Create UDP echo application and install it on one
   * of the left and right wifi station node
   */


  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install(rightWifiNodes.Get(2));
  serverApps.Start (Seconds (30.0));
  serverApps.Stop (Seconds (40.0));

  UdpEchoClientHelper echoClient(rightInterfaces.GetAddress(2),9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (.001)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (leftWifiNodes.Get (2));
  clientApps.Start (Seconds (35.0));
  clientApps.Stop (Seconds (40.0));
  /*
   * Enable/Disable Animation output
   */
  if(enableAnim)
  {
	  AnimationInterface anim("bneck-wireless-tcp.xml");
	  anim.EnablePacketMetadata(true);
  }
  Simulator::Stop (Seconds (40.0));

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();
  Simulator::Run ();
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  	  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  	  	  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
  	  	   {
  	  	  	 Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
  	  	  	 /*
  	  	  	  * Calculate the throughtput and other network parameters
  	  	  	  * between TCP source/client (10.1.3.2) and
  	  	  	  * TCP destination/server (10.1.2.2).
  	  	  	  */
  	  	       if ((t.sourceAddress=="10.1.3.2" && t.destinationAddress == "10.1.2.2"))
  	  	       {
  	  	            std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
  	  	            std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
  	  	            std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
  	  	            std::cout<<"First Time"<<i->second.timeFirstRxPacket.GetSeconds()<<"\n";
  	  	            std::cout<<"Last Time"<<i->second.timeLastTxPacket.GetSeconds()<<"\n";
  	  	        	std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
  	  	        }
  	  	   }
  Simulator::Destroy ();
  return 0;
}
