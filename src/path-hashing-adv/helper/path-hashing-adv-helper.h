/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Gustavo Carneiro <gjc@inescporto.pt>
 */
#ifndef PATH_HASHING_ADV_HELPER_H
#define PATH_HASHING_ADV_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"
#include <string>
#include <map>

namespace ns3 {

enum SwitchType
{
  EDGE_SWITCH = 1,
  AGG_SWITCH = 2,
  CORE_SWITCH = 3,
};

class Node;
class AttributeValue;

/**
 * \ingroup bridge
 * \brief Add capability to bridge multiple LAN segments (IEEE 802.1D bridging)
 */
class PathHashingAdvHelper
{
public:
  /*
   * Construct a PathHashingAdvHelper
   */
  PathHashingAdvHelper ();
  /**
   * Set an attribute on each ns3::BridgeNetDevice created by
   * PathHashingAdvHelper::Install
   *
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   */
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);
  /**
   * This method creates an ns3::BridgeNetDevice with the attributes
   * configured by PathHashingAdvHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * bridge.
   *
   * \param node The node to install the device in
   * \param c Container of NetDevices to add as bridge ports
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (Ptr<Node> node, SwitchType st, NetDeviceContainer uplinkDevices,
                                std::map<uint32_t, Ptr<NetDevice>> addressToInterface,
                                Ipv4Mask mask, int totalPaths, double timeWindow);
  /**
   * This method creates an ns3::BridgeNetDevice with the attributes
   * configured by PathHashingAdvHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * bridge.
   *
   * \param nodeName The name of the node to install the device in
   * \param c Container of NetDevices to add as bridge ports
   * \returns A container holding the added net device.
   */
  NetDeviceContainer Install (std::string nodeName, SwitchType st, NetDeviceContainer uplinkDevices,
                                std::map<uint32_t, Ptr<NetDevice>> addressToInterface,
                                Ipv4Mask mask, int totalPaths, double timeWindow);
private:
  ObjectFactory m_deviceFactory; //!< Object factory
};

} // namespace ns3


#endif /* PATH_HASHING_HELPER_H */
