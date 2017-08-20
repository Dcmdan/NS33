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

#include "li-ion-battery-model.h"

#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"

#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LiIonBatteryModel");

NS_OBJECT_ENSURE_REGISTERED (LiIonBatteryModel);

TypeId
LiIonBatteryModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LiIonBatteryModel")
    .SetParent<EnergySource> ()
    .SetGroupName ("Energy")
    .AddConstructor<LiIonBatteryModel> ()
    .AddAttribute ("LiIonBatteryModelInitialEnergyJ",
                   "Initial energy stored in basic energy source.",
                   DoubleValue (31752.0),  // in Joules
                   MakeDoubleAccessor (&LiIonBatteryModel::SetInitialEnergy,
                                       &LiIonBatteryModel::GetInitialEnergy),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("LiIonEnergyLowBatteryThreshold",
                   "Low battery threshold for LiIon energy source.",
                   DoubleValue (0.10), // as a fraction of the initial energy
                   MakeDoubleAccessor (&LiIonBatteryModel::m_lowBatteryTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("InitialCellVoltage",
                   "Initial (maximum) voltage of the cell (fully charged).",
                   DoubleValue (4.05), // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::SetInitialSupplyVoltage,
                                       &LiIonBatteryModel::GetSupplyVoltage),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NominalCellVoltage",
                   "Nominal voltage of the cell.",
                   DoubleValue (3.6),  // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::m_eNom),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ExpCellVoltage",
                   "Cell voltage at the end of the exponential zone.",
                   DoubleValue (3.6),  // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::m_eExp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RatedCapacity",
                   "Rated capacity of the cell.",
                   DoubleValue (2.45),   // in Ah
                   MakeDoubleAccessor (&LiIonBatteryModel::m_qRated),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("NomCapacity",
                   "Cell capacity at the end of the nominal zone.",
                   DoubleValue (1.1),  // in Ah
                   MakeDoubleAccessor (&LiIonBatteryModel::m_qNom),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ExpCapacity",
                   "Cell Capacity at the end of the exponential zone.",
                   DoubleValue (1.2),  // in Ah
                   MakeDoubleAccessor (&LiIonBatteryModel::m_qExp),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("InternalResistance",
                   "Internal resistance of the cell",
                   DoubleValue (0.083),  // in Ohms
                   MakeDoubleAccessor (&LiIonBatteryModel::m_internalResistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TypCurrent",
                   "Typical discharge current used to fit the curves",
                   DoubleValue (2.33), // in A
                   MakeDoubleAccessor (&LiIonBatteryModel::m_typCurrent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ThresholdVoltage",
                   "Minimum threshold voltage to consider the battery depleted.",
                   DoubleValue (3.3), // in Volts
                   MakeDoubleAccessor (&LiIonBatteryModel::m_minVoltTh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("PeriodicEnergyUpdateInterval",
                   "Time between two consecutive periodic energy updates.",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&LiIonBatteryModel::SetEnergyUpdateInterval,
                                     &LiIonBatteryModel::GetEnergyUpdateInterval),
                   MakeTimeChecker ())
    .AddTraceSource ("RemainingEnergy",
                     "Remaining energy at BasicEnergySource.",
                     MakeTraceSourceAccessor (&LiIonBatteryModel::m_remainingEnergyJ),
                     "ns3::TracedValueCallback::Double")
	 .AddAttribute ("RvBatteryModelAlphaValue",
			 	 	 	 "RV battery model alpha value.",
					    DoubleValue (35220.0),
					    MakeDoubleAccessor (&LiIonBatteryModel::m_alpha),
					    MakeDoubleChecker<double> ())
	 .AddAttribute ("RvBatteryModelBetaValue",
					    "RV battery model beta value.", DoubleValue (0.637),
					    MakeDoubleAccessor (&LiIonBatteryModel::SetBeta,
					                        &LiIonBatteryModel::GetBeta),
					    MakeDoubleChecker<double> ())
  ;
  return tid;
}

LiIonBatteryModel::LiIonBatteryModel ()
  : m_drainedCapacity (0.0),
    m_lastUpdateTime (Seconds (0.0))
{
	m_previousLoad = -1.0;
	m_lastUpdateAlpha = 0;
	m_lastSampleTime = Simulator::Now ();
	m_timeStamps.push_back (m_lastSampleTime);
  NS_LOG_FUNCTION (this);
}

LiIonBatteryModel::~LiIonBatteryModel ()
{
  NS_LOG_FUNCTION (this);
}

void
LiIonBatteryModel::SetBeta (double beta)
{
  NS_LOG_FUNCTION (this << beta);
  NS_ASSERT (beta >= 0);
  m_beta = beta;
}

double
LiIonBatteryModel::GetBeta (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beta;
}

void
LiIonBatteryModel::SetInitialEnergy (double initialEnergyJ)
{
  NS_LOG_FUNCTION (this << initialEnergyJ);
  NS_ASSERT (initialEnergyJ >= 0);
  m_initialEnergyJ = initialEnergyJ;
  // set remaining energy to be initial energy
  m_remainingEnergyJ = m_initialEnergyJ;
}

double
LiIonBatteryModel::GetInitialEnergy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_initialEnergyJ;
}

void
LiIonBatteryModel::SetInitialSupplyVoltage (double supplyVoltageV)
{
  NS_LOG_FUNCTION (this << supplyVoltageV);
  m_eFull = supplyVoltageV;
  m_supplyVoltageV = supplyVoltageV;
}

double
LiIonBatteryModel::GetSupplyVoltage (void) const
{
  NS_LOG_FUNCTION (this);
  return m_supplyVoltageV;
}

void
LiIonBatteryModel::SetEnergyUpdateInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_energyUpdateInterval = interval;
}

Time
LiIonBatteryModel::GetEnergyUpdateInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_energyUpdateInterval;
}

double
LiIonBatteryModel::GetRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  return m_remainingEnergyJ;
}

double
LiIonBatteryModel::GetEnergyFraction (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  return m_remainingEnergyJ / m_initialEnergyJ;
}

void
LiIonBatteryModel::DecreaseRemainingEnergy (double energyJ)
{
  NS_LOG_FUNCTION (this << energyJ);
  NS_ASSERT (energyJ >= 0);
  m_remainingEnergyJ -= energyJ;

  // check if remaining energy is 0
  if (m_supplyVoltageV <= m_minVoltTh)
    {
      HandleEnergyDrainedEvent ();
    }
}

void
LiIonBatteryModel::IncreaseRemainingEnergy (double energyJ)
{
  NS_LOG_FUNCTION (this << energyJ);
  NS_ASSERT (energyJ >= 0);
  m_remainingEnergyJ += energyJ;
}

void
LiIonBatteryModel::UpdateEnergySource (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LiIonBatteryModel:Updating remaining energy at node #" <<
                GetNode ()->GetId ());

  // do not update if simulation has finished
  if (Simulator::IsFinished ())
    {
      return;
    }

  m_energyUpdateEvent.Cancel ();

  CalculateRemainingEnergy ();

  m_lastUpdateTime = Simulator::Now ();

  if (m_remainingEnergyJ <= m_lowBatteryTh * m_initialEnergyJ)
    {
      HandleEnergyDrainedEvent ();
      return; // stop periodic update
    }

  m_energyUpdateEvent = Simulator::Schedule (m_energyUpdateInterval,
                                             &LiIonBatteryModel::UpdateEnergySource,
                                             this);
}

/*
 * Private functions start here.
 */
void
LiIonBatteryModel::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  UpdateEnergySource ();  // start periodic update
}

void
LiIonBatteryModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  BreakDeviceEnergyModelRefCycle ();  // break reference cycle
}


void
LiIonBatteryModel::HandleEnergyDrainedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LiIonBatteryModel:Energy depleted at node #" <<
                GetNode ()->GetId ());
  NotifyEnergyDrained (); // notify DeviceEnergyModel objects
}


void
LiIonBatteryModel::CalculateRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  double totalCurrentA = CalculateTotalCurrent ();

  Time duration = Simulator::Now () - m_lastUpdateTime;
  double calculatedAlpha = Discharge (totalCurrentA * 1000, Simulator::Now ());
  NS_ASSERT (duration.GetSeconds () >= 0);
  // energy = current * voltage * time
  //double energyToDecreaseJ = totalCurrentA * m_supplyVoltageV * duration.GetSeconds ();
  double energyToDecreaseJ = (calculatedAlpha - m_lastUpdateAlpha) * m_supplyVoltageV;
  if (m_remainingEnergyJ < energyToDecreaseJ) 
    {
      m_remainingEnergyJ = 0; // energy never goes below 0
    } 
  else 
    {
      m_remainingEnergyJ -= energyToDecreaseJ;
      m_drainedCapacity = (calculatedAlpha / 3600);
    }  
  //m_drainedCapacity += (totalCurrentA * duration.GetSeconds () / 3600);
  // update the supply voltage
  m_supplyVoltageV = GetVoltage (totalCurrentA);
  m_lastUpdateAlpha = calculatedAlpha;
  NS_LOG_DEBUG ("LiIonBatteryModel:Remaining energy = " << m_remainingEnergyJ);
}

double
LiIonBatteryModel::GetVoltage (double i) const
{
  NS_LOG_FUNCTION (this << i);

  // integral of i in dt, drained capacity in Ah
  double it = m_drainedCapacity;

  // empirical factors
  double A = m_eFull - m_eExp;
  double B = 3 / m_qExp;

  // slope of the polarization curve
  double K = std::abs ( (m_eFull - m_eNom + A * (std::exp (-B * m_qNom) - 1)) * (m_qRated - m_qNom) / m_qNom);

  // constant voltage
  double E0 = m_eFull + K + m_internalResistance * m_typCurrent - A;

  double E = E0 - K * m_qRated / (m_qRated - it) + A * std::exp (-B * it);

  // cell voltage
  double V = E - m_internalResistance * i;

  NS_LOG_DEBUG ("Voltage: " << V << " with E: " << E);

  return V;
}

double
LiIonBatteryModel::Discharge (double load, Time t)
{
  NS_LOG_FUNCTION (this << load << t);

  // record only when load changes
  if (load != m_previousLoad)
   {
	   m_load.push_back (load);
      m_previousLoad = load;
      m_timeStamps[m_timeStamps.size () - 1] = m_lastSampleTime;
      m_timeStamps.push_back (t);
    }
  else
    {
      if  (!m_timeStamps.empty())
      {
        m_timeStamps[m_timeStamps.size () - 1] = t;
      }
    }

  m_lastSampleTime = t;

  // calculate alpha for new t
  NS_ASSERT (m_load.size () == m_timeStamps.size () - 1); // size must be equal
  double calculatedAlpha = 0.0;
  if (m_timeStamps.size () == 1)
    {
      // constant load
      calculatedAlpha = m_load[0] * RvModelAFunction (t, t, Seconds (0.0),
                                                      m_beta);
    }
  else
    {
      // changing load
      for (uint64_t i = 1; i < m_timeStamps.size (); i++)
        {
          calculatedAlpha += m_load[i - 1] * RvModelAFunction (t, m_timeStamps[i],
                                                               m_timeStamps[i - 1],
                                                               m_beta);
        }
    }

  return calculatedAlpha;
}

double
LiIonBatteryModel::RvModelAFunction (Time t, Time sk, Time sk_1, double beta)
{
  NS_LOG_FUNCTION (this << t << sk << sk_1 << beta);

  // everything is in minutes
  double firstDelta = (t.GetSeconds () - sk.GetSeconds ()) / 60;
  double secondDelta = (t.GetSeconds () - sk_1.GetSeconds ()) / 60;
  double delta = (sk.GetSeconds () - sk_1.GetSeconds ()) / 60;

  double sum = 0.0;
  for (int m = 1; m <= 10; m++)
    {
      double square = beta * beta * m * m;
      sum += (std::exp (-square * (firstDelta)) - std::exp (-square * (secondDelta))) / square;
    }
  return delta + 2 * sum;
}

} // namespace ns3
