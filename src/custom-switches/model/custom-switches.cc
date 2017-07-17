/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "custom-switches.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CustomSwitch");

void
ECMPNode::ECMPReceiveEdge ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                   const Address &to, NetDevice::PacketType packetType)
{
  NS_LOG_INFO("In ECMPReceiveEdge, with base address " << this->baseSwitchAddress);
  NS_LOG_INFO(", src: " << Mac48Address::ConvertFrom(from) << ", to: " << Mac48Address::ConvertFrom(to) <<" , packet type :" << packetType);

  // /std::cout << "In ECMPReceiveEdge \n";
}

void
ECMPNode::ECMPReceiveAgg ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                   const Address &to, NetDevice::PacketType packetType)
{
  NS_LOG_INFO("In ECMPReceiveAgg, with base address " << this->baseSwitchAddress);
}

void
ECMPNode::ECMPReceiveCore ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                   const Address &to, NetDevice::PacketType packetType)
{
  NS_LOG_INFO("In ECMPReceiveCore, with base address " << this->baseSwitchAddress);
}

void ECMPNode::setSpecialAttribute(int attr)
{
  this->attr = attr;
}

void ECMPNode::setECMPSwitchAttributes(int layer, Ipv4Address addr)
{
  this->layer = layer;
  this->baseSwitchAddress = addr;
}

void ECMPNode::setHostToIntfAttribute( std::map<Ipv4Address, Ptr<NetDevice> > hostAddressToInterfaceMap)
{
  this->hostAddressToInterfaceMap = hostAddressToInterfaceMap;
}

void PathHashingNode::PathHashingReceive ( Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol, const Address &from,
                     const Address &to, NetDevice::PacketType packetType)
{
  NS_LOG_INFO("In PathHashingReceive!\n");
}

}
