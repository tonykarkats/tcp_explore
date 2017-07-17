/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef UTILS_H
#define UTILS_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4.h"
#include "ns3/packet.h"
#include "ns3/arp-cache.h"

namespace ns3 {

void
PopulateArpCache (NodeContainer NodeList);

char *
toString(int a,int b, int c, int d);

Ipv4Address
GetNodeIp(Ptr<Node> node);

void
PrintTime();

}

#endif /* UTILS_H */
