#ifndef MISC_TOOLS_H
#define MISC_TOOLS_H
#include "ns3/network-module.h"
#include "ns3/core-module.h"

#define TOTAL_SIMULATION_TIME 50
#define SERVERS_START_TIME 1.0
#define CLIENTS_START_TIME 2.0

namespace ns3 {

// This is a helper map that holds the node to (IP, MAC) mappings

void setPos (Ptr<Node> n, int x, int y, int z);

void RunIp (Ptr<Node> node, Time at, std::string str);

void AddAddress (Ptr<Node> node, Time at, const char *name, const char *address);

std::string Ipv4AddressToString (Ipv4Address ad);

void StartFlow(Ptr<Node> from, Ptr<Node> to, Time at, uint32_t flowSize, 
				std::map<Ptr<Node>, std::pair<std::string, std::string> > nodeToIpMac,
				int simTime);

std::vector<std::pair<double, uint32_t>> generateFlowsNormalDist(int flowsPerSec,
				 uint32_t minFlowSize, uint32_t maxFlowSize, double variance,
				 int simTime);

std::vector<std::pair<double, uint32_t>> generateFlowsCdf(int flowsPerSec, std::string DistFile, int simTime);

std::tuple<int, int, int, int, int, int> getSourceDestPairs(double sameEdgeProb, double samePodProb, int k, int seed_factor);

}

#endif // MISC_TOOLS_H
