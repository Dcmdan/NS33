/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Andrea Sacco
 *
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
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/command-line.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/li-ion-battery-model.h"
#include "ns3/energy-source-container.h"
#include "cstdio"

using namespace ns3;

FILE *file;

/**
 * In this simple example, we show how to create and drain energy from a
 * LiIonEnergySource.
 * We make a series of discharge calls to the energy source class with
 * different current drain and duration until all the energy is depleted
 * from the cell.
 *
 * Every 20 seconds it is printed out the actual cell voltage to verify
 * that it follows the discharge curve of the datasheet [1].
 *
 * At the end of the example it is verified that after the energy depletion
 * call, the cell voltage is below the threshold voltage.
 *
 * References:
 * [1] Panasonic CGR18650DA Datasheet, http://www.panasonic.com/industrial/includes/pdf/Panasonic_LiIon_CGR18650DA.pdf
 */

static void
PrintCellInfo (Ptr<LiIonBatteryModel> es)
{
  std::cout << "At " << Simulator::Now ().GetSeconds () << " Cell voltage: " << es->GetSupplyVoltage () << " V Remaining Capacity: " <<
  es->GetRemainingEnergy () << " J" << std::endl;
	fprintf(file, "%f \n\r", es->GetRemainingEnergy ());

  if (!Simulator::IsFinished ())
    {
      Simulator::Schedule (Seconds (20),
                           &PrintCellInfo,
                           es);
    }
}

int
main (int argc, char **argv)
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  file = fopen("log.txt", "w+");

    // uncomment below to see the energy consumption details
  // LogComponentEnable ("LiIonEnergySource", LOG_LEVEL_DEBUG);

  Ptr<Node> node = CreateObject<Node> ();

  Ptr<SimpleDeviceEnergyModel> sem = CreateObject<SimpleDeviceEnergyModel> ();
  Ptr<EnergySourceContainer> esCont = CreateObject<EnergySourceContainer> ();
  Ptr<LiIonBatteryModel> es = CreateObject<LiIonBatteryModel> ();
  esCont->Add (es);
  es->SetNode (node);
  sem->SetEnergySource (es);
  es->AppendDeviceEnergyModel (sem);
  sem->SetNode (node);
  node->AggregateObject (esCont);

  Time now = Simulator::Now ();

  sem->SetCurrentA (1);
  now += Seconds (1800);


  // discharge at 4.66 A for 628 seconds
  Simulator::Schedule (now,
                       &SimpleDeviceEnergyModel::SetCurrentA,
                       sem,
                       0);
  now += Seconds (600);

  Simulator::Schedule (now,
                       &SimpleDeviceEnergyModel::SetCurrentA,
                       sem,
                       1);
  now += Seconds (1800);

  PrintCellInfo (es);

  Simulator::Stop (now);
  Simulator::Run ();
  Simulator::Destroy ();
}
