/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c)
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
 */

#include "path-hashing-adv-helper.h"
#include "ns3/log.h"
#include "ns3/path-hashing-adv-net-device.h"
#include "ns3/node.h"
#include "ns3/names.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PathHashingAdvHelper");

PathHashingAdvHelper::PathHashingAdvHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.SetTypeId ("ns3::PathHashingAdvNetDevice");
}

void
PathHashingAdvHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.Set (n1, v1);
}

NetDeviceContainer
PathHashingAdvHelper::Install (Ptr<Node> node, SwitchType st, NetDeviceContainer uplinkDevices,
                              std::map<uint32_t, Ptr<NetDevice>> addressToInterface,
                              Ipv4Mask mask, int totalPaths,  double timeWindow)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_LOGIC ("**** Install bridge device on node " << node->GetId ());

  NetDeviceContainer devs;
  Ptr<PathHashingAdvNetDevice> dev = m_deviceFactory.Create<PathHashingAdvNetDevice> ();
  devs.Add (dev);
  node->AddDevice (dev);

  // add all devices of node to the new path-hashing-device
  for (uint32_t i=0; i < node->GetNDevices(); i++)
  {
    Ptr<NetDevice> devToAdd = node->GetDevice(i);
    if (devToAdd == dev)
      continue;

    NS_LOG_LOGIC ("**** Add BridgePort "<< devToAdd);
    dev->AddBridgePort (devToAdd, st);
  }

  // add uplink interfaces, used for hashing to up-path :)
  for (NetDeviceContainer::Iterator i = uplinkDevices.Begin (); i != uplinkDevices.End (); ++i)
    {
      NS_LOG_LOGIC ("**** Add BridgePort "<< *i);
      dev->uplinkDevices.Add(*i);
    }

  // used to simulate routing tables
  dev->addressToInterface = addressToInterface;
  dev->mask = mask;

  // used to keep count of paths
  dev->totalPaths = (unsigned long long) totalPaths;
  dev->uplinkPorts = (int) dev->uplinkDevices.GetN();

  if (st == SwitchType::EDGE_SWITCH) {
    dev->pathCounter = new unsigned long long[totalPaths]();
    dev->packetsOnMaxPath = 0;
    dev->maxPath = 0;
    dev->timeWindow = timeWindow;
    dev->totalPackets = 0;
    Simulator::Schedule(Seconds(dev->timeWindow), &PathHashingAdvNetDevice::resetWindow, dev);
  }

  return devs;
}

NetDeviceContainer
PathHashingAdvHelper::Install (std::string nodeName, SwitchType st, NetDeviceContainer uplinkDevices,
                              std::map<uint32_t, Ptr<NetDevice>> addressToInterface,
                              Ipv4Mask mask, int totalPaths, double timeWindow)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node, st, uplinkDevices, addressToInterface, mask, totalPaths, timeWindow);
}

} // namespace ns3
