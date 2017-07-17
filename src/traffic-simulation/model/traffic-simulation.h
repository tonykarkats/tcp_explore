/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef TRAFFIC_SIMULATION_H
#define TRAFFIC_SIMULATION_H

#include "ns3/flow-monitor-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/bridge-net-device.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/csma-module.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/netanim-module.h"
#include "ns3/ipv4.h"
#include "ns3/packet.h"
#include "ns3/arp-cache.h"
#include "ns3/utils-switches.h"
#include "ns3/random-variable-stream.h"
#include <fstream>

namespace ns3 {

#define TOTAL_SIM_TIME 1000.0

std::map< uint32_t, uint32_t>
installServersOnHosts(NodeContainer hosts);

// Starts stride traffic between hosts, each host i sends traffic to host
// i + hosts/2 of size flowSize. Internally uses BulkSendApplication
int startStrideTraffic(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort, uint64_t flowSize, std::string filename);
int startMultipleStrideTraffic(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort, uint64_t flowSize, int numFlows, std::string filename);
int startRandomTraffic(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort, uint64_t flowSize, int numFlows, int k, std::string filename);
int startRandomTrafficWithDistributions(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort,
  int numFlows, int k, std::string filename,
  double poissonRatePerSec, double sameRackProb, double samePodProb,
  std::vector< std::pair<double,unsigned long long> > distribution);

int
simulateTrafficForTimePeriod(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort,
    int experimentTime, int k, std::string filename, double avgPacketsPerSec,
    double sameEdgeProb, double samePodProb, std::vector< std::pair<double,unsigned long long> > distribution);

// generates a vector of pairs representing the distribution
// it assumes that the file contains multiple lines where of the form "a b"
// where a is the flow size and b is the cumulative probability
std::vector< std::pair<double,unsigned long long> > getDistribution(std::string distributionName);

// generates a vector of pairs representing the distribution, but, size of flows in distribution
// will be calculated by packet size and number of packets
// it assumes that the file contains multiple lines where of the form "a b"
// where a is the flow size and b is the cumulative probability
std::vector< std::pair<double,unsigned long long> > getDistribution(std::string distributionName);

// given a distribution of vec<pair<double, int>> and a generator it returns a size
unsigned long long
getFlowSizeFromDistribution (std::vector< std::pair<double,unsigned long long> > distribution, double sampledNumber);

}
#endif /* TRAFFIC_SIMULATION_H */
