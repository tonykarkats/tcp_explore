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
#include "ecmp-net-device.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/boolean.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/hash.h"
#include "ns3/random-variable-stream.h"

// Andreas Hack
#include "ns3/custom-switches.h"
#include "ns3/custom-switches-helper.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EcmpNetDevice");

NS_OBJECT_ENSURE_REGISTERED (EcmpNetDevice);


TypeId
EcmpNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EcmpNetDevice")
    .SetParent<NetDevice> ()
    .SetGroupName("Bridge")
    .AddConstructor<EcmpNetDevice> ()
    .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&EcmpNetDevice::SetMtu,
                                         &EcmpNetDevice::GetMtu),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("EnableLearning",
                   "Enable the learning mode of the Learning Bridge",
                   BooleanValue (true),
                   MakeBooleanAccessor (&EcmpNetDevice::m_enableLearning),
                   MakeBooleanChecker ())
    .AddAttribute ("ExpirationTime",
                   "Time it takes for learned MAC state entry to expire.",
                   TimeValue (Seconds (300)),
                   MakeTimeAccessor (&EcmpNetDevice::m_expirationTime),
                   MakeTimeChecker ())
    .AddAttribute ("FlowletGap",
                   "Flowlet gap value",
                   TimeValue ( MicroSeconds(100)),
                   MakeTimeAccessor (&EcmpNetDevice::m_flowletGap),
                   MakeTimeChecker ())
    .AddAttribute ("EcmpMode",
                   "ECMP Mode (0 = ECMP, 1 = Random Flowlet Switching)",
                   UintegerValue (0),
                   MakeEnumAccessor (&EcmpNetDevice::m_ecmpMode),
                   MakeUintegerChecker<uint16_t> ())
  ;
  return tid;
}


EcmpNetDevice::EcmpNetDevice ()
  : m_node (0),
    m_ifIndex (0),
    m_flowletGap (MicroSeconds(100))
{
  NS_LOG_FUNCTION_NOARGS ();
  m_channel = CreateObject<EcmpChannel> ();
  m_rand = CreateObject<UniformRandomVariable> ();
  RngSeedManager::SetSeed (9369214);
}

EcmpNetDevice::~EcmpNetDevice()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
EcmpNetDevice::DoDispose ()
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
EcmpNetDevice::ReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
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
EcmpNetDevice::getHash(Ipv4Address src, Ipv4Address dst, uint16_t srcPort, uint16_t dstPort) {

  NS_LOG_FUNCTION_NOARGS ();
  uint32_t srcInt = src.Get();
  uint32_t dstInt = dst.Get();

  char toHash[16];

  memcpy(toHash, &srcInt, 4);
  memcpy(toHash + 4, &dstInt, 4);
  memcpy(toHash + 8, &srcPort, 2);
  memcpy(toHash + 10, &dstPort, 2);
  memcpy(toHash + 12, &this->seed, 4);

  uint32_t  tuple_hash = Hash32 ( toHash, 16);
  NS_LOG_INFO("5-tuple hash is : " << tuple_hash << ", Seed used is " << this->seed);

  return tuple_hash;
}

bool
EcmpNetDevice::inTable (Ipv4Address addr) {

  NS_LOG_FUNCTION_NOARGS ();
  uint32_t u_addr = addr.CombineMask(this->mask).Get();
  return this->addressToInterface.count(u_addr);
}

Ptr<NetDevice>
EcmpNetDevice::getIntf(Ipv4Address addr) {

  uint32_t u_addr = addr.CombineMask(this->mask).Get();
  return (this->addressToInterface.at(u_addr));
}

Ptr<NetDevice>
EcmpNetDevice::hashToUplink(Ipv4Address src, Ipv4Address dst, uint16_t srcPort, uint16_t dstPort) {

  NS_LOG_FUNCTION_NOARGS ();
  uint32_t totalPaths = this->uplinkDevices.GetN();
  uint32_t selectIndex = 0;
  uint16_t hashedTuple = this->getHash(src, dst, srcPort, dstPort);

  if (m_flowletGap == 0)
  {
    selectIndex =  hashedTuple % totalPaths;
  }
  else if (m_flowletGap > 0)
  {
    flowlet_t flowlet_data;
    if (flowlet_table.count(hashedTuple) > 0) {
      // Already existing entry
      flowlet_data = flowlet_table[hashedTuple];
      
      // Check if packet gap is bigger than threhold. In that case select a new random port
      Time now = Simulator::Now();
      if (now - flowlet_data.time > m_flowletGap) {
        //NS_LOG_DEBUG("Flowlet expired in node: " <<  Names::FindName(m_ipv4->GetObject<Node>()));
        //std::cout << "At " << Simulator::Now().GetSeconds() << " Inter packet gap is :" << (now-flowlet_data.time) << "\n";
        selectIndex = m_rand->GetInteger(0, totalPaths-1);
        flowlet_data.time = now;
        flowlet_data.outPort = selectIndex;
        flowlet_table[hashedTuple] = flowlet_data;
      }
      else
      {
        // Select port and update flowlet table
        selectIndex = (flowlet_data.outPort % totalPaths);
        flowlet_data.time = now;
        flowlet_table[hashedTuple] = flowlet_data;
      }
    }
    else
    {
      // New entry
      Time now = Simulator::Now();
      selectIndex = m_rand->GetInteger(0, totalPaths-1);
      flowlet_data.time = now;
      flowlet_data.outPort = selectIndex; 
      flowlet_table[hashedTuple] = flowlet_data;
      
    }
  }
  else
    NS_FATAL_ERROR("ECMP mode not recognized");

  return this->uplinkDevices.Get(selectIndex);
  NS_LOG_INFO("Hashed into interface :" + selectIndex);
}

void
EcmpNetDevice::EdgeReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("UID is " << packet->GetUid ());

  //std::cout <<"EDGE  ";
  //std::cout << Simulator::Now().GetNanoSeconds() << ": ";
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

  Ptr<NetDevice> toSend;
  NS_LOG_INFO("SourceIp: " << ipSrc << ", DstIp: " << ipDst);
  NS_LOG_INFO("SourcePort: " << srcPort << ", DstPort: " << dstPort);

  if (this->inTable(ipDst)) {
    toSend = this->getIntf(ipDst);
  }
  else {
    toSend = this->hashToUplink(ipSrc, ipDst, srcPort, dstPort);
  }
  // forward the packet accordingly!
  Ptr<Packet> packetToSend = packet->Copy();
  toSend->SendFrom( packetToSend, src, dst, protocol);

  return;
}

void
EcmpNetDevice::AggReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("UID is " << packet->GetUid ());

  //std::cout <<"AGG  ";
  //std::cout << Simulator::Now().GetNanoSeconds() << ": ";
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

  Ptr<NetDevice> toSend;
  NS_LOG_INFO("SourceIp: " << ipSrc << ", DstIp: " << ipDst);
  NS_LOG_INFO("SourcePort: " << srcPort << ", DstPort: " << dstPort);

  if (this->inTable(ipDst)) {
    toSend = this->getIntf(ipDst);
  }
  else {
    toSend = this->hashToUplink(ipSrc, ipDst, srcPort, dstPort);
  }

  // forward the packet accordingly!
  Ptr<Packet> packetToSend = packet->Copy();
  toSend->SendFrom( packetToSend, src, dst, protocol);

  return;
}
void
EcmpNetDevice::CoreReceiveFromDevice (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                                    Address const &src, Address const &dst, PacketType packetType)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG ("UID is " << packet->GetUid ());

  //std::cout << "CORE\n";
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

  Ptr<NetDevice> toSend;
  NS_LOG_INFO("SourceIp: " << ipSrc << ", DstIp: " << ipDst);
  NS_LOG_INFO("SourcePort: " << srcPort << ", DstPort: " << dstPort);

  // in core switches we can only go down-link,
  // no uplink to hash to, assert that
  NS_ASSERT (this->inTable(ipDst) == 1);
  toSend = this->getIntf(ipDst);

  // forward the packet accordingly!
  Ptr<Packet> packetToSend = packet->Copy();
  toSend->SendFrom( packetToSend, src, dst, protocol);

  return;
}

void
EcmpNetDevice::ForwardUnicast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
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
EcmpNetDevice::ForwardBroadcast (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet,
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

void EcmpNetDevice::Learn (Mac48Address source, Ptr<NetDevice> port)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_enableLearning)
    {
      LearnedState &state = m_learnState[source];
      state.associatedPort = port;
      state.expirationTime = Simulator::Now () + m_expirationTime;
    }
}

Ptr<NetDevice> EcmpNetDevice::GetLearnedState (Mac48Address source)
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
EcmpNetDevice::GetNBridgePorts (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports.size ();
}


Ptr<NetDevice>
EcmpNetDevice::GetBridgePort (uint32_t n) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports[n];
}

void
EcmpNetDevice::AddBridgePort (Ptr<NetDevice> bridgePort, SwitchType st)
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
      m_node->RegisterProtocolHandler (MakeCallback (&EcmpNetDevice::EdgeReceiveFromDevice, this),
                                        0, bridgePort, true);
      break;
    case(AGG_SWITCH):
      m_node->RegisterProtocolHandler (MakeCallback (&EcmpNetDevice::AggReceiveFromDevice, this),
                                        0, bridgePort, true);
      break;
    case(CORE_SWITCH):
      m_node->RegisterProtocolHandler (MakeCallback (&EcmpNetDevice::CoreReceiveFromDevice, this),
                                        0, bridgePort, true);
      break;
  }
  m_ports.push_back (bridgePort);
  m_channel->AddChannel (bridgePort->GetChannel ());
  NS_LOG_DEBUG ("Finished adding port!");
}

void
EcmpNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_ifIndex = index;
}

uint32_t
EcmpNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel>
EcmpNetDevice::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_channel;
}

void
EcmpNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_address = Mac48Address::ConvertFrom (address);
}

Address
EcmpNetDevice::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool
EcmpNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_mtu = mtu;
  return true;
}

uint16_t
EcmpNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}


bool
EcmpNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


void
EcmpNetDevice::AddLinkChangeCallback (Callback<void> callback)
{}


bool
EcmpNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


Address
EcmpNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
EcmpNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
EcmpNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
  return multicast;
}


bool
EcmpNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool
EcmpNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


bool
EcmpNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION_NOARGS ();
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
EcmpNetDevice::SendFrom (Ptr<Packet> packet, const Address& src, const Address& dest, uint16_t protocolNumber)
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
EcmpNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}


void
EcmpNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = node;
}


bool
EcmpNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}


void
EcmpNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxCallback = cb;
}

void
EcmpNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_promiscRxCallback = cb;
}

bool
EcmpNetDevice::SupportsSendFrom () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address EcmpNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

} // namespace ns3
