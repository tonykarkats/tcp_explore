/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Gustavo Carneiro  <gjc@inescporto.pt>
 */
#include "path-hashing-net-device.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/hash-fnv.h"

// Andreas Hack
#include "ns3/custom-switches.h"
#include "ns3/custom-switches-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PathHashingNetDevice");

NS_OBJECT_ENSURE_REGISTERED (PathHashingNetDevice);

TypeId
PathHashingNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PathHashingNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Bridge")
    .AddConstructor<PathHashingNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&PathHashingNetDevice::SetMtu,
                                         &PathHashingNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("EnableLearning",
                   "Enable the learning mode of the Learning Bridge",
                   BooleanValue (true),
                   MakeBooleanAccessor (&PathHashingNetDevice::m_enableLearning),
                   MakeBooleanChecker ())
    .AddAttribute ("ExpirationTime",
                   "Time it takes for learned MAC state entry to expire.",
                   TimeValue (Seconds (300)),
                   MakeTimeAccessor (&PathHashingNetDevice::m_expirationTime),
                   MakeTimeChecker ())
  ;
  return tid;
}


PathHashingNetDevice::PathHashingNetDevice ()
  : m_node (0),
    m_ifIndex (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_channel = CreateObject<PathHashingChannel> ();
}

PathHashingNetDevice::~PathHashingNetDevice()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
PathHashingNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin (); iter != m_ports.end (); iter++)
    {
      *iter = 0;
    }
  m_ports.clear ();
  m_channel = 0;
  m_node = 0;
  NetDevice::DoDispose ();
}

void
PathHashingNetDevice::ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("UID is " << packet->GetUid ());

  Mac48Address src48 = Mac48Address::ConvertFrom (src);
  Mac48Address dst48 = Mac48Address::ConvertFrom (dst);
  std::cout << src48 << "   " << dst48 << "\n";

  if (!m_promiscRxCallback.IsNull ())
    {
      m_promiscRxCallback (this, packet, protocol, src, dst, packetType);
    }

  switch (packetType)
    {
    case PACKET_HOST:
      if (dst48 == m_address)
        {
          m_rxCallback (this, packet, protocol, src);
        }
      break;

    case PACKET_BROADCAST:
    case PACKET_MULTICAST:
      m_rxCallback (this, packet, protocol, src);
      ForwardBroadcast (incomingPort, packet, protocol, src48, dst48);
      break;

    case PACKET_OTHERHOST:
      if (dst48 == m_address)
        {
          m_rxCallback (this, packet, protocol, src);
        }
      else
        {
          ForwardUnicast (incomingPort, packet, protocol, src48, dst48);
        }
      break;
    }
}

uint32_t
PathHashingNetDevice::getHash(Ipv4Address src, Ipv4Address dst, uint16_t srcPort, uint16_t dstPort) {


  Hash::Function::Murmur3 hasher;

  uint32_t srcInt = src.Get();
  uint32_t dstInt = dst.Get();

  char toHash[12];

  memcpy(toHash, &srcInt, 4);
  memcpy(toHash + 4, &dstInt, 4);
  memcpy(toHash + 8, &srcInt, 2);
  memcpy(toHash + 10, &dstPort, 2);

  uint32_t  tuple_hash = hasher.GetHash32 ( toHash, 12);

  NS_LOG_FUNCTION("5-tuple hash is : " << tuple_hash);

  return tuple_hash;
}

uint32_t
PathHashingNetDevice::getHash_2(Ipv4Address src, Ipv4Address dst, uint16_t srcPort, uint16_t dstPort)
{

  Hash::Function::Fnv1a	hasher;

  uint32_t srcInt = src.Get();
  uint32_t dstInt = dst.Get();

  char toHash[12];

  memcpy(toHash, &srcInt, 4);
  memcpy(toHash + 4, &dstInt, 4);
  memcpy(toHash + 8, &srcInt, 2);
  memcpy(toHash + 10, &dstPort, 2);

  uint32_t  tuple_hash = hasher.GetHash32 (toHash, 12);
  NS_LOG_FUNCTION("5-tuple hash is : " << tuple_hash);

  return tuple_hash;
}

bool
PathHashingNetDevice::inTable (Ipv4Address addr) {

  NS_LOG_FUNCTION_NOARGS ();
  uint32_t u_addr = addr.CombineMask(this->mask).Get();
  return this->addressToInterface.count(u_addr);
}

Ptr<NetDevice>
PathHashingNetDevice::getIntf(Ipv4Address addr) {

  NS_LOG_FUNCTION_NOARGS ();
  uint32_t u_addr = addr.CombineMask(this->mask).Get();
  return (this->addressToInterface.at(u_addr));
}

uint32_t
PathHashingNetDevice::reHashToPath(Ipv4Address src, Ipv4Address dst,uint16_t srcPort, uint16_t dstPort) {

  uint32_t totalPaths = this->totalPaths;
  uint32_t pathIndex = this->getHash_2(src, dst, srcPort, dstPort) % totalPaths;

  // std::cout << "Re-hashed into path :" << pathIndex <<"\n";
  NS_LOG_FUNCTION("Re-hashed into path :" << pathIndex);
  return pathIndex;
}

uint32_t
PathHashingNetDevice::hashToPath(Ipv4Address src, Ipv4Address dst,uint16_t srcPort, uint16_t dstPort)
{
  uint32_t totalPaths = this->totalPaths;
  uint32_t pathIndex = this->getHash(src, dst, srcPort, dstPort) % totalPaths;

  NS_LOG_FUNCTION("Hashed into path :" << pathIndex);
  return pathIndex;
}

void
PathHashingNetDevice::UpdatePath(int pathId)
{
  this->pathCounter[pathId]++;
  unsigned long long cntPackets = this->pathCounter[pathId];
  if (cntPackets > this->packetsOnMaxPath) {
    this->packetsOnMaxPath = cntPackets;
    this->maxPath = pathId;
  }

  this->totalPackets++;

  unsigned long long sum = 0;
  for (uint64_t p=0; p<this->totalPaths; p++) {
    sum += this->pathCounter[p];
  }

  NS_LOG_FUNCTION ("Updated path " << pathId
                    << ", Total Packets " << this->totalPackets
                    << ", Packets on Path " << this->pathCounter[pathId]);
  return;
}

void
PathHashingNetDevice::EdgeReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_FUNCTION ("UID is " << packet->GetUid ());

  //std::cout << "\nIn switch with mask: " << this->mask << "\n";
  //packet->Print(std::cout);
  //std::cout << "\n";

  Ptr<Packet> packetCopy = packet->Copy();
  Ipv4Header header;
  TcpHeader tcpHeader;

  packetCopy->RemoveHeader(header);
  packetCopy->RemoveHeader(tcpHeader);

  Ipv4Address ipSrc = header.GetSource();
  Ipv4Address ipDst = header.GetDestination();
  uint16_t srcPort = tcpHeader.GetSourcePort();
  uint16_t dstPort = tcpHeader.GetDestinationPort();

  NS_LOG_FUNCTION("SourceIp: " << ipSrc << ", DstIp: " << ipDst << ", SourcePort: " << srcPort << ", DstPort: " << dstPort);

  Ptr<NetDevice> toSend;
  Ptr<Packet> packetToSend = packet->Copy();
  if (this->inTable(ipDst)) {
    toSend = this->getIntf(ipDst);
  }
  else {
    uint32_t uplinkPorts = this->uplinkPorts;
    uint32_t pathIndex = this->hashToPath(ipSrc, ipDst, srcPort, dstPort);
    this->UpdatePath(pathIndex);

    // if hashed into the max path , re-hash
    if (pathIndex == this->previousMaxPath) {
      if ((this->getHash(ipSrc, ipDst, srcPort, dstPort)%100) > this->offset) {

        NS_LOG_FUNCTION(">>>>>>>>>>>>>>>>>>>>>>>> Device: " << this->m_address
                      << "Hashed to max path of previous window and greater hash than offset, re-hashing to other path!");
        pathIndex = reHashToPath(ipSrc, ipDst, srcPort, dstPort);
      }
    }

    // get port of this switch
    toSend = this->uplinkDevices.Get(pathIndex / uplinkPorts);
    // get port of aggregation switch
    uint8_t aggPort = pathIndex % uplinkPorts;

    // encode port from aggregation switch in ToS FIELDS
    packetToSend->RemoveHeader(header);
    header.SetTos(aggPort);

    packetToSend->AddHeader(header);
    NS_LOG_FUNCTION("Hashed to path : " << pathIndex << ", Uplink port : " << pathIndex << ", Agg Port : " << (int) aggPort);
  }

  // forward the packet accordingly!
  toSend->SendFrom( packetToSend, src, dst, protocol);
  return;
}

void
PathHashingNetDevice::AggReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION ("UID is " << packet->GetUid ());

  // std::cout << "\nIn switch with mask: " << this->mask << "\n";
  // packet->Print(std::cout);
  // std::cout << "\n";

  Ptr<Packet> packetCopy = packet->Copy();
  Ipv4Header header;
  TcpHeader tcpHeader;

  packetCopy->RemoveHeader(header);
  packetCopy->RemoveHeader(tcpHeader);

  Ipv4Address ipSrc = header.GetSource();
  Ipv4Address ipDst = header.GetDestination();
  uint16_t srcPort = tcpHeader.GetSourcePort();
  uint16_t dstPort = tcpHeader.GetDestinationPort();

  NS_LOG_FUNCTION("SourceIp: " << ipSrc << ", DstIp: " << ipDst << ", SourcePort: " << srcPort << ", DstPort: " << dstPort);

  Ptr<NetDevice> toSend;
  if (this->inTable(ipDst)) {
    toSend = this->getIntf(ipDst);
  }
  else {

    // packet needs to go the the core
    // uplink port was set by edge switch
    // in the ToS field
    uint8_t uplinkPort = header.GetTos();
    toSend = this->uplinkDevices.Get(uplinkPort);
    NS_LOG_FUNCTION("Got ToS field, will send to port with index : " << (int) uplinkPort);
  }

  // forward the packet accordingly!
  Ptr<Packet> packetToSend = packet->Copy();
  toSend->SendFrom( packetToSend, src, dst, protocol);

  return;
}

void
PathHashingNetDevice::CoreReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_FUNCTION ("UID is " << packet->GetUid ());

  // std::cout << "\nIn switch with mask: " << this->mask << "\n";
  // packet->Print(std::cout);
  // std::cout << "\n";

  Ptr<Packet> packetCopy = packet->Copy();
  Ipv4Header header;
  TcpHeader tcpHeader;

  packetCopy->RemoveHeader(header);
  packetCopy->RemoveHeader(tcpHeader);

  Ipv4Address ipSrc = header.GetSource();
  Ipv4Address ipDst = header.GetDestination();
  uint16_t srcPort = tcpHeader.GetSourcePort();
  uint16_t dstPort = tcpHeader.GetDestinationPort();

  NS_LOG_FUNCTION("SourceIp: " << ipSrc << ", DstIp: " << ipDst << ", SourcePort: " << srcPort << ", DstPort: " << dstPort);

  Ptr<NetDevice> toSend;

  // in core switches we can only go down-link,
  // no uplink to hash to!
  NS_ASSERT (this->inTable(ipDst) == 1);
  toSend = this->getIntf(ipDst);

  // forward the packet accordingly!
  Ptr<Packet> packetToSend = packet->Copy();
  toSend->SendFrom( packetToSend, src, dst, protocol);

  return;
}

void
PathHashingNetDevice::resetWindow ()
{
  unsigned long long total = this->totalPackets;
  this->previousMaxPath = this->maxPath;

  this->packetsOnPreviousMaxPath = this->pathCounter[this->maxPath];

  if (total >= this->totalPaths) {
    unsigned long long equalShare = total / this->totalPaths;
    if (this->packetsOnPreviousMaxPath > equalShare) {
        this->offset = 100 / (this->packetsOnPreviousMaxPath / equalShare);
    }
    else {
      this->offset = 100;
    }
  }
  else {
      this->offset = 100;
  }

  NS_LOG_FUNCTION("Resetting path counters for switch with addr " << this->m_address
                  << ", at time : " << Simulator::Now().GetSeconds()
                  << ", Max Pathid : "<< this->previousMaxPath
                  << ", Total packets : " << this->totalPackets
                  << ", Packets on max path : " << this->packetsOnPreviousMaxPath
                  << ", Total Paths : " << this->totalPaths
                  << ", offset : " << this->offset);

  // some assertions to ensure our logic is correct
  NS_ASSERT(this->previousMaxPath == this->maxPath);
  NS_ASSERT(this->packetsOnMaxPath == this->pathCounter[this->maxPath]);
  NS_ASSERT(this->packetsOnMaxPath == this->packetsOnPreviousMaxPath);
  unsigned long long sum = 0;

  for (uint64_t p=0; p<this->totalPaths; p++) {
    sum += this->pathCounter[p];
  }

  NS_ASSERT(this->totalPackets == sum);

  this->totalPackets = 0;
  this->packetsOnMaxPath = 0;
  memset(this->pathCounter, 0, this->totalPaths * sizeof(*this->pathCounter));

  // assert that counters for paths are zeroed
  for (uint32_t i=0; i<this->totalPaths; i++)
    NS_ASSERT(this->pathCounter[i] == 0);

  Simulator::Schedule(Seconds(this->timeWindow), &PathHashingNetDevice::resetWindow, this);

}

void
PathHashingNetDevice::ForwardUnicast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                 uint16_t protocol, Mac48Address src, Mac48Address dst)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("LearningBridgeForward (incomingPort=" << incomingPort->GetInstanceTypeId ().GetName ()
                                                       << ", packet=" << packet << ", protocol="<<protocol
                                                       << ", src=" << src << ", dst=" << dst << ")");

  Learn (src, incomingPort);
  Ptr<NetDevice> outPort = GetLearnedState (dst);
  if (outPort != NULL && outPort != incomingPort)
    {
      NS_LOG_LOGIC ("Learning bridge state says to use port `" << outPort->GetInstanceTypeId ().GetName () << "'");
      outPort->SendFrom (packet->Copy (), src, dst, protocol);
    }
  else
    {
      NS_LOG_LOGIC ("No learned state: send through all ports");
      for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
           iter != m_ports.end (); iter++)
        {
          Ptr<NetDevice> port = *iter;
          if (port != incomingPort)
            {
              NS_LOG_LOGIC ("LearningBridgeForward (" << src << " => " << dst << "): "
                                                      << incomingPort->GetInstanceTypeId ().GetName ()
                                                      << " --> " << port->GetInstanceTypeId ().GetName ()
                                                      << " (UID " << packet->GetUid () << ").");
              port->SendFrom (packet->Copy (), src, dst, protocol);
            }
        }
    }
}

void
PathHashingNetDevice::ForwardBroadcast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
                                   uint16_t protocol, Mac48Address src, Mac48Address dst)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("LearningBridgeForward (incomingPort=" << incomingPort->GetInstanceTypeId ().GetName ()
                                                       << ", packet=" << packet << ", protocol="<<protocol
                                                       << ", src=" << src << ", dst=" << dst << ")");
  Learn (src, incomingPort);

  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
       iter != m_ports.end (); iter++)
    {
      Ptr<NetDevice> port = *iter;
      if (port != incomingPort)
        {
          NS_LOG_LOGIC ("LearningBridgeForward (" << src << " => " << dst << "): "
                                                  << incomingPort->GetInstanceTypeId ().GetName ()
                                                  << " --> " << port->GetInstanceTypeId ().GetName ()
                                                  << " (UID " << packet->GetUid () << ").");
          port->SendFrom (packet->Copy (), src, dst, protocol);
        }
    }
}

void PathHashingNetDevice::Learn (Mac48Address source, Ptr<NetDevice> port)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_enableLearning)
    {
      LearnedState &state = m_learnState[source];
      state.associatedPort = port;
      state.expirationTime = Simulator::Now () + m_expirationTime;
    }
}

Ptr<NetDevice> PathHashingNetDevice::GetLearnedState (Mac48Address source)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_enableLearning)
    {
      Time now = Simulator::Now ();
      std::map<Mac48Address, LearnedState>::iterator iter =
        m_learnState.find (source);
      if (iter != m_learnState.end ())
        {
          LearnedState &state = iter->second;
          if (state.expirationTime > now)
            {
              return state.associatedPort;
            }
          else
            {
              m_learnState.erase (iter);
            }
        }
    }
  return NULL;
}

uint32_t
PathHashingNetDevice::GetNBridgePorts (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports.size ();
}

Ptr<NetDevice>
PathHashingNetDevice::GetBridgePort (uint32_t n) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports[n];
}

void
PathHashingNetDevice::AddBridgePort (Ptr<NetDevice> bridgePort, SwitchType st)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (bridgePort != this);

  if (!Mac48Address::IsMatchingType (bridgePort->GetAddress ()))
    {
      NS_FATAL_ERROR ("Device does not support eui 48 addresses: cannot be added to bridge.");
    }
  if (!bridgePort->SupportsSendFrom ())
    {
      NS_FATAL_ERROR ("Device does not support SendFrom: cannot be added to bridge.");
    }
  if (m_address == Mac48Address ())
    {
      m_address = Mac48Address::ConvertFrom (bridgePort->GetAddress ());
    }

  NS_LOG_DEBUG ("RegisterProtocolHandler for " << bridgePort->GetInstanceTypeId ().GetName ());
  switch (st)
  {
    case(EDGE_SWITCH):
      m_node->RegisterProtocolHandler (MakeCallback (&PathHashingNetDevice::EdgeReceiveFromDevice, this),
                                        0, bridgePort, true);
      break;
    case(AGG_SWITCH):
      m_node->RegisterProtocolHandler (MakeCallback (&PathHashingNetDevice::AggReceiveFromDevice, this),
                                        0, bridgePort, true);
      break;
    case(CORE_SWITCH):
      m_node->RegisterProtocolHandler (MakeCallback (&PathHashingNetDevice::CoreReceiveFromDevice, this),
                                        0, bridgePort, true);
      break;
  }
  m_ports.push_back (bridgePort);
  m_channel->AddChannel (bridgePort->GetChannel ());
  NS_LOG_DEBUG ("Finished adding port!");
}

void
PathHashingNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ifIndex = index;
}

uint32_t
PathHashingNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel>
PathHashingNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_channel;
}

void
PathHashingNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_address = Mac48Address::ConvertFrom (address);
}

Address
PathHashingNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool
PathHashingNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_mtu = mtu;
  return true;
}

uint16_t
PathHashingNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}


bool
PathHashingNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


void
PathHashingNetDevice::AddLinkChangeCallback (Callback<void> callback)
{}


bool
PathHashingNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


Address
PathHashingNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
PathHashingNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
PathHashingNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
  return multicast;
}


bool
PathHashingNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool
PathHashingNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


bool
PathHashingNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
PathHashingNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  Mac48Address dst = Mac48Address::ConvertFrom (dest);

  // try to use the learned state if data is unicast
  if (!dst.IsGroup ())
    {
      Ptr<NetDevice> outPort = GetLearnedState (dst);
      if (outPort != NULL)
        {
          outPort->SendFrom (packet, src, dest, protocolNumber);
          return true;
        }
    }

  // data was not unicast or no state has been learned for that mac
  // address => flood through all ports.
  Ptr<Packet> pktCopy;
  for (std::vector< Ptr<NetDevice> >::iterator iter = m_ports.begin ();
       iter != m_ports.end (); iter++)
    {
      pktCopy = packet->Copy ();
      Ptr<NetDevice> port = *iter;
      port->SendFrom (pktCopy, src, dest, protocolNumber);
    }

  return true;
}


Ptr<Node>
PathHashingNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}


void
PathHashingNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = node;
}


bool
PathHashingNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


void
PathHashingNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void
PathHashingNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool
PathHashingNetDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address PathHashingNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

} // namespace ns3
