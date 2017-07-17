/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef CUSTOM_SWITCHES_H
#define CUSTOM_SWITCHES_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
//#include "ns3/test-switch.h"
//#include "ns3/test-switch-helper.h"
#include "ns3/csma-channel.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/mac48-address.h"
#include <vector>

namespace ns3 {

class ECMPNode : public Node
{
    public:
      int attr;

      std::vector<Ptr<NetDevice> > upLinks;
      std::vector<Ptr<NetDevice> > downLinks;
      std::map<Ipv4Address, Ptr<NetDevice> > hostAddressToInterfaceMap;

      //0 for edge switches , 1 for agg switches, 2 for core switches
      int layer;

      // address of switch
      Ipv4Address baseSwitchAddress;

      ECMPNode() : Node() {};
      void ECMPReceiveEdge ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                           const Address &to, NetDevice::PacketType packetType);
      void ECMPReceiveAgg ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                            const Address &to, NetDevice::PacketType packetType);
      void ECMPReceiveCore ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                            const Address &to, NetDevice::PacketType packetType);
      void setSpecialAttribute(int attr);
      void setECMPSwitchAttributes(int layer, Ipv4Address baseSwitchAddress);
      void setHostToIntfAttribute(std::map<Ipv4Address, Ptr<NetDevice> > hostAddressToInterfaceMap);


};

class PathHashingNode : public Node
{
  public:

    PathHashingNode() : Node() {};
    void PathHashingReceive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                         const Address &to, NetDevice::PacketType packetType);
};

}

#endif /* CUSTOM_SWITCHES_H */
