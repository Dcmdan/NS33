#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/command-line.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/rv-battery-model.h"
#include "ns3/energy-source-container.h"
#include "cstdio"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RvModelTest");

FILE *file;

static void
PrintCellInfo (Ptr<RvBatteryModel> es)
{
	std::cout << "At " << Simulator::Now ().GetSeconds () << " Cell voltage: " << es->GetSupplyVoltage () << " V Remaining Capacity: " <<
	  es->GetRemainingEnergy () << " J" << std::endl;
	fprintf(file, "%f \n\r", es->GetRemainingEnergy ());

  if (!Simulator::IsFinished ())
    {
      Simulator::Schedule (Seconds (10),
                           &PrintCellInfo,
                           es);
    }
}

int
main (int argc, char **argv)
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  file = fopen("log.txt", "r+");

  LogComponentEnable ("RvBatteryModel", LOG_LEVEL_INFO);

  Ptr<Node> node = CreateObject<Node> ();

  Ptr<SimpleDeviceEnergyModel> sem = CreateObject<SimpleDeviceEnergyModel> ();
  Ptr<EnergySourceContainer> esCont = CreateObject<EnergySourceContainer> ();
  Ptr<RvBatteryModel> es = CreateObject<RvBatteryModel> ();

  es->SetAlpha(36000);
  es->SetBeta(1);
  es->SetCutoffVoltage(4.0);
  es->SetNumOfTerms(100);
  es->SetOpenCircuitVoltage(4.2);

  esCont->Add (es);
  es->SetNode (node);
  sem->SetEnergySource (es);
  es->AppendDeviceEnergyModel (sem);
  sem->SetNode (node);
  node->AggregateObject (esCont);

  Time now = Simulator::Now ();

  // discharge at 2.33 A for 1700 seconds
  sem->SetCurrentA (1);
  now += Seconds (600);


  PrintCellInfo (es);

  Simulator::Stop (now);
  Simulator::Run ();
  Simulator::Destroy ();
}
