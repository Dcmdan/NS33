#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/energy-module.h"
#include "ns3/config-store-module.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TwoWifiNodes");

int
main (int argc, char *argv[])
{
  CommandLine cmd;

  cmd.Parse (argc,argv);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("RvBatteryModel", LOG_LEVEL_DEBUG);

  std::string phyMode ("DsssRate1Mbps");

  NodeContainer wifiNodes;
  wifiNodes.Create (2);

  YansWifiChannelHelper channel;
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  channel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  Ptr<YansWifiChannel> wifiChannelPtr = channel.Create ();

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.Set ("RxGain", DoubleValue (-10));
  phy.Set ("TxGain", DoubleValue (-1));
  phy.Set ("CcaMode1Threshold", DoubleValue (0.0));
  phy.SetChannel (wifiChannelPtr);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  WifiMacHelper mac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
		  	  	  	  	  	  	  	  	  "DataMode", StringValue (phyMode),
										  "ControlMode", StringValue (phyMode));
  mac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer wifiDevices;
  wifiDevices = wifi.Install (phy, mac, wifiNodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (20.0),
                                 "DeltaY", DoubleValue (40.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiNodes);

  RvBatteryModelHelper rvModelHelper;
  rvModelHelper.Set ("RvBatteryModelAlphaValue", DoubleValue (35220));
  rvModelHelper.Set ("RvBatteryModelBetaValue", DoubleValue (0.637));
  rvModelHelper.Set ("RvBatteryModelLowBatteryThreshold", DoubleValue (0.0));
  // install source
  EnergySourceContainer sources = rvModelHelper.Install (wifiNodes);

  WifiRadioEnergyModelHelper radioEnergyHelper;
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (96.0 / 1000));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (29.5 / 1000));
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (wifiDevices, sources);

  InternetStackHelper stack;
  stack.Install (wifiNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiInterfaces =  address.Assign (wifiDevices);

  UdpEchoServerHelper echoServer (28);

  ApplicationContainer serverApps = echoServer.Install (wifiNodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (wifiInterfaces.GetAddress (0), 28);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024*2));

  ApplicationContainer clientApps =
    echoClient.Install (wifiNodes.Get (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
