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

#include "ecmp-helper.h"
#include "ns3/log.h"
#include "ns3/ecmp-net-device.h"
#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EcmpHelper");

EcmpHelper::EcmpHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.SetTypeId ("ns3::EcmpNetDevice");
}

void
EcmpHelper::SetDeviceAttribute (std::string n1, const AttributeValue &v1)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_deviceFactory.Set (n1, v1);
}

NetDeviceContainer
EcmpHelper::Install (Ptr<Node> node, SwitchType st, NetDeviceContainer uplinkDevices,
                              std::map<uint32_t, Ptr<NetDevice> > addressToInterface, Ipv4Mask mask, uint32_t seed)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_LOGIC ("**** Install bridge device on node " << node->GetId ());

  NetDeviceContainer devs;
  Ptr<EcmpNetDevice> dev = m_deviceFactory.Create<EcmpNetDevice> ();
  devs.Add (dev);
  node->AddDevice (dev);

  // add all devices of node to the new ecmp-device
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
  dev->seed = seed;

  return devs;
}

NetDeviceContainer
EcmpHelper::Install (std::string nodeName, SwitchType st, NetDeviceContainer uplinkDevices,
                              std::map<uint32_t, Ptr<NetDevice> > addressToInterface, Ipv4Mask mask, uint32_t seed)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Node> node = Names::Find<Node> (nodeName);
  return Install (node, st, uplinkDevices, addressToInterface, mask, seed);
}

} // namespace ns3
