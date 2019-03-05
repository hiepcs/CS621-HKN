#include "ns3/netanim-module.h"
#include "ns3/core-module.h"
#include "ns3/global-route-manager.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/bridge-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("P2P");

/*
 * 
 * this is the architecture
 * n0-----r0-----r1-----n1
 *    L0     L1     L2
 * L0 -> 8Mbps
 * L1 -> 1 to 10 Mbps
 * L2 -> 8Mbps
 * n0 client, n1 server
 * 
 */

int main(int argc, char *argv[])
{
	Time::SetResolution (Time::NS);
	LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  // LogComponentEnable ("PointToPointNetDevice", LOG_LEVEL_ALL);
  CommandLine cmd;
  cmd.Parse (argc, argv);

  double transmission_times[2] = {};

  /* Configuration. */
  std::cout << "Enter Compression Link Capacity (xMbps) : ";
  std::string link_capacity = "8Mbps";
  std::cin >> link_capacity;

  /* Build nodes. */
  NodeContainer client;
  client.Create (1);
  NodeContainer router_0;
  router_0.Create (1);
  NodeContainer router_1;
  router_1.Create (1);
  NodeContainer server;
  server.Create (1);

  /* Build link. */
  PointToPointHelper p2p_client_r0;
  p2p_client_r0.SetDeviceAttribute ("DataRate", StringValue("8Mbps"));
  p2p_client_r0.SetChannelAttribute ("Delay", StringValue("1ms"));
  PointToPointHelper p2p_r0_r1;
  p2p_r0_r1.SetDeviceAttribute ("DataRate", StringValue(link_capacity));
  p2p_r0_r1.SetChannelAttribute ("Delay", StringValue("1ms"));
  PointToPointHelper p2p_r1_server;
  p2p_r1_server.SetDeviceAttribute ("DataRate", StringValue("8Mbps"));
  p2p_r1_server.SetChannelAttribute ("Delay", StringValue("1ms"));

  /* Build link net device container. */
  NetDeviceContainer ndc_client_r0 = p2p_client_r0.Install (client.Get(0), router_0.Get(0));
  NetDeviceContainer ndc_r0_r1 = p2p_r0_r1.Install (router_0.Get(0), router_1.Get(0));
  NetDeviceContainer ndc_r1_server = p2p_r1_server.Install (server.Get(0), router_1.Get(0));

  Ptr<PointToPointNetDevice> p2p_r0 = DynamicCast<PointToPointNetDevice>(ndc_r0_r1.Get(0));
  p2p_r0->SetCompress(true);

  Ptr<PointToPointNetDevice> p2p_r1 = DynamicCast<PointToPointNetDevice>(ndc_r0_r1.Get(1));
  p2p_r1->SetCompress(true);

  Ptr<PointToPointNetDevice> p2p_server = DynamicCast<PointToPointNetDevice>(ndc_r1_server.Get(1));

  /* Install the IP stack. */
  InternetStackHelper internetStackH;
  internetStackH.Install (client);
  internetStackH.Install (server);
  internetStackH.Install (router_0);
  internetStackH.Install (router_1);

  /* IP assign. */
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_client_r0 = ipv4.Assign (ndc_client_r0);
  ipv4.SetBase ("10.0.1.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_r0_r1 = ipv4.Assign (ndc_r0_r1);
  ipv4.SetBase ("10.0.2.0", "255.255.255.0");
  Ipv4InterfaceContainer iface_ndc_r1_server = ipv4.Assign (ndc_r1_server);

  /* Generate Route. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /* Generate Application. */
  uint16_t port_udpEcho_0 = 9;
  UdpServerHelper server_udpEcho_0 (port_udpEcho_0);
  ApplicationContainer server_app = server_udpEcho_0.Install (server.Get(0));
  server_app.Start (Seconds (1.0));
  server_app.Stop (Seconds (3500.0));
  Time interPacketInterval_udpEcho_0 = MilliSeconds(50);

  int packet_size = 1100;
  p2p_r0->SetPacketSize(packet_size);
  p2p_r1->SetPacketSize(packet_size);

  UdpClientHelper udp_client_0 (iface_ndc_r1_server.GetAddress(0), 9);
  udp_client_0.SetAttribute ("MaxPackets", UintegerValue (6000));
  udp_client_0.SetAttribute ("Interval", TimeValue (interPacketInterval_udpEcho_0));
  udp_client_0.SetAttribute ("PacketSize", UintegerValue (1100));
  udp_client_0.SetAttribute ("Entropy", BooleanValue(true));
  ApplicationContainer client_app = udp_client_0.Install (client.Get (0));
  client_app.Start (Seconds (2.0));
  client_app.Stop (Seconds (6002.0));

	AnimationInterface anim ("p2p.xml");
	anim.SetConstantPosition (client.Get(0), 0.0, 0.0);
  anim.SetConstantPosition (router_0.Get(0), 10.0, 10.0);
  anim.SetConstantPosition (router_1.Get(0), 20.0, 20.0);
  anim.SetConstantPosition (server.Get(0), 30.0, 30.0);
  
  // AsciiTraceHelper ascii;
  AsciiTraceHelper ascii;
  p2p_client_r0.EnableAsciiAll (ascii.CreateFileStream ("client.tr"));
  p2p_client_r0.EnablePcap ("client", ndc_client_r0.Get(0), false, false);

  p2p_r1_server.EnableAsciiAll (ascii.CreateFileStream ("router_0.tr"));
  p2p_r0_r1.EnablePcap ("router_0", ndc_r0_r1.Get(0), false, false);

  p2p_r1_server.EnableAsciiAll (ascii.CreateFileStream ("router_1.tr"));
  p2p_r0_r1.EnablePcap ("router_1", ndc_r0_r1.Get(1), false, false);

  p2p_r1_server.EnableAsciiAll (ascii.CreateFileStream ("server.tr"));
  p2p_r1_server.EnablePcap ("server", ndc_r1_server.Get(0), false, false);

  Simulator::Run ();
  Simulator::Destroy ();

  std::cout << "::::: RECEIVED : " << server_udpEcho_0.GetPacketCount() << ":::::  TIME DIFF : " << server_udpEcho_0.GetTimeDiff() << "ms\n";
  transmission_times[0] = server_udpEcho_0.GetTimeDiff();

  for (int i=0; i < int(sizeof(transmission_times)/sizeof(*transmission_times)); i++) {
    std::cout << transmission_times[i] << '\n';
  }
  return 0;
}