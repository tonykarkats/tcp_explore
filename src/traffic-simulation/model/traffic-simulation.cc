/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "traffic-simulation.h"

NS_LOG_COMPONENT_DEFINE ("TrafficSimulation");

namespace ns3 {

int
startStrideTraffic(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort, uint64_t flowSize, std::string filename)
{

  // all hosts need to have tcp servers installed by installServersOnHosts
  // ipToPort argument is the return value of installServersOnHosts
  uint32_t totalNodes = hosts.GetN();
  int totalFlows = 0;

  for (uint32_t i=0; i<totalNodes; i++) {

    Ptr<Node> hostFrom = hosts.Get(i);
    Ptr<Node> hostTo = hosts.Get((i + totalNodes/2)%totalNodes);

    Ipv4Address ipFrom = GetNodeIp(hostFrom);
    Ipv4Address ipTo = GetNodeIp(hostTo);
    uint32_t portTo = ipToPort.at(ipTo.Get());

    NS_LOG_FUNCTION("Scheduling flow with size "<< flowSize <<
                ", from " << ipFrom << " to " << ipTo << ":" << portTo);
	
    BulkSendHelper source ("ns3::TcpSocketFactory",
                        InetSocketAddress (ipTo, portTo), ipFrom, filename);
    source.SetAttribute ("MaxBytes", UintegerValue (flowSize));

    ApplicationContainer sourceApps = source.Install (hostFrom);
    sourceApps.Stop(Seconds(TOTAL_SIM_TIME));
    sourceApps.Start(Seconds(1.0));

    totalFlows++;
  }

  return totalFlows;
}

int
startMultipleStrideTraffic(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort, uint64_t flowSize, int totalFlows, std::string filename)
{

  // all hosts need to have tcp servers installed by installServersOnHosts
  // ipToPort argument is the return value of installServersOnHosts
  uint32_t totalNodes = hosts.GetN();
  int totalScheduledFlows = 0;

  std::cout << "In startMultipleStrideTraffic!\n";
  for (int flow=0; flow<totalFlows; flow++) {
    for (uint32_t i=0; i<totalNodes; i++) {

      Ptr<Node> hostFrom = hosts.Get(i);
      Ptr<Node> hostTo = hosts.Get((i + totalNodes/2 + flow*2)%totalNodes);

      Ipv4Address ipFrom = GetNodeIp(hostFrom);
      Ipv4Address ipTo = GetNodeIp(hostTo);
      uint32_t portTo = ipToPort.at(ipTo.Get());

      NS_LOG_FUNCTION("Scheduling flow with size "<< flowSize <<
                  ", from " << ipFrom << " to " << ipTo << ":" << portTo);
      BulkSendHelper source ("ns3::TcpSocketFactory",
                          InetSocketAddress (ipTo, portTo), ipFrom, filename);
      source.SetAttribute ("MaxBytes", UintegerValue (flowSize));
      ApplicationContainer sourceApps = source.Install (hostFrom);
      sourceApps.Stop(Seconds(TOTAL_SIM_TIME));
      sourceApps.Start(Seconds(0.0));

      totalScheduledFlows++;
    }
  }
  return totalScheduledFlows;
}

int
startRandomTraffic(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort, uint64_t flowSize, int totalFlows, int k, std::string filename)
{
  Ptr<UniformRandomVariable> rndGen = CreateObject<UniformRandomVariable> ();
  uint32_t totalNodes = hosts.GetN();
  int totalScheduledFlows = 0;

  uint32_t hostsPerPod = (k/2) * (k/2);

  for (int flow=0; flow<totalFlows; flow++) {

    for (uint32_t i=0; i<totalNodes; i++) {

      Ptr<Node> hostFrom = hosts.Get(i);
      uint32_t hostPod = i / hostsPerPod;

      uint32_t sendPod = rndGen->GetInteger(0, k-1);
      while (sendPod == hostPod) {
        sendPod = rndGen->GetInteger(0, k-1);
      }

      uint32_t sendSwitch = rndGen->GetInteger(0, k/2-1);
      uint32_t sendHost = rndGen->GetInteger(0, k/2-1);
      uint32_t nodeIndex = sendPod * hostsPerPod + sendSwitch * k/2 + sendHost;

      Ptr<Node> hostTo = hosts.Get(nodeIndex);
      Ipv4Address ipFrom = GetNodeIp(hostFrom);
      Ipv4Address ipTo = GetNodeIp(hostTo);
      uint32_t portTo = ipToPort.at(ipTo.Get());

      NS_LOG_FUNCTION("Scheduling flow with size "<< flowSize <<
                  ", from " << ipFrom << " to " << ipTo << ":" << portTo);
      BulkSendHelper source ("ns3::TcpSocketFactory",
                          InetSocketAddress (ipTo, portTo), ipFrom, filename);
      source.SetAttribute ("MaxBytes", UintegerValue (flowSize));
      ApplicationContainer sourceApps = source.Install (hostFrom);
      sourceApps.Stop(Seconds(TOTAL_SIM_TIME));
      sourceApps.Start(Seconds(0.0));

      totalScheduledFlows++;
    }
  }

  return totalScheduledFlows;
}

int
startRandomTrafficWithDistributions(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort,
  int totalFlows, int k, std::string filename, double avgPacketsPerSec,
  double sameEdgeProb, double samePodProb, std::vector< std::pair<double,unsigned long long> > distribution)
{
  Ptr<UniformRandomVariable> rndGen = CreateObject<UniformRandomVariable> ();
  uint32_t totalNodes = hosts.GetN();
  uint32_t hostsPerPod = (k/2) * (k/2);
  unsigned long long sampleFlowSize;
  int totalScheduledFlows = 0;

  NS_ASSERT(sameEdgeProb < 1.0);
  NS_ASSERT(samePodProb < 1.0);
  NS_ASSERT((sameEdgeProb + samePodProb) <= 1.0);

  for (uint32_t i=0; i<totalNodes; i++) {

    // generator for random inter-arrival times
    std::mt19937 arrivalTime_rng(std::random_device{}());
    std::exponential_distribution<double> interArrivalTime(avgPacketsPerSec);
    double start_time = 0 ;

    // generator for destination switch, pod and host
    std::mt19937 destHost_rng(std::random_device{}());
    std::uniform_real_distribution<double> uniformDistribution(0.0,1.0);

    // generator for destination switch, pod and host
    std::mt19937 flowSize_rng(std::random_device{}());
    std::uniform_real_distribution<double> uniformDistributionFlows(0.0,1.0);

    // for each different node, give a different seed (same across experiments)
    arrivalTime_rng.seed(42*i);
    destHost_rng.seed(42*42*i);
    flowSize_rng.seed(42*42*42*i);

    for (int flow=0; flow<totalFlows; flow++) {
      // generate random number which will tell you where to send
      // same edge, same pod different pod
      double num = uniformDistribution(destHost_rng);
      //std::cout << "Generated :" << num << "\n";

      uint32_t hostPod, hostId, hostEdge, sendPod, sendSwitch, sendHost;
      hostPod = i / hostsPerPod;
      hostId = (i % (k/2)) ;
      hostEdge = (i / (k/2))%(k/2);

      Ptr<Node> hostFrom = hosts.Get(i);

      if (num < sameEdgeProb) {
        //NS_LOG_FUNCTION("Same edge");
        sendPod = hostPod;
        sendSwitch = hostEdge;

        sendHost = rndGen->GetInteger(0, k/2-1);
        while (sendHost == hostId) {
          sendHost = rndGen->GetInteger(0, k/2-1);
        }
      }
      else if (num < (samePodProb + sameEdgeProb)) {
        //NS_LOG_FUNCTION("Same pod");
        sendPod = hostPod;

        sendSwitch = rndGen->GetInteger(0, k/2-1);
        while (sendSwitch == hostEdge) {
          sendSwitch = rndGen->GetInteger(0, k/2-1);
        }

        sendHost = rndGen->GetInteger(0, k/2-1);
      }
      else {
        //NS_LOG_FUNCTION("Different pod");
        sendPod = rndGen->GetInteger(0, k-1);
        while (sendPod == hostPod) {
          sendPod = rndGen->GetInteger(0, k-1);
        }

        sendSwitch = rndGen->GetInteger(0, k/2-1);
        sendHost = rndGen->GetInteger(0, k/2-1);
      }

      uint32_t nodeIndex = sendPod * hostsPerPod + sendSwitch * k/2 + sendHost;

      Ptr<Node> hostTo = hosts.Get(nodeIndex);
      Ipv4Address ipFrom = GetNodeIp(hostFrom);
      Ipv4Address ipTo = GetNodeIp(hostTo);
      uint32_t portTo = ipToPort.at(ipTo.Get());

      sampleFlowSize = getFlowSizeFromDistribution(distribution, uniformDistributionFlows(flowSize_rng));

      BulkSendHelper source ("ns3::TcpSocketFactory",
                      InetSocketAddress (ipTo, portTo), ipFrom, filename);
      source.SetAttribute ("MaxBytes", UintegerValue (sampleFlowSize));
      ApplicationContainer sourceApps = source.Install (hostFrom);
      sourceApps.Stop(Seconds(TOTAL_SIM_TIME));

      // get a random arrival time according to the exponential distribution
      // which simulates the Poisson arrival process model
      start_time += interArrivalTime(arrivalTime_rng);

      NS_LOG_FUNCTION("Scheduling flow with size "<< sampleFlowSize <<
                  ". At " << start_time <<
                  ". From " << ipFrom << " to " << ipTo << ":" << portTo);

      //std::cout << "From : " << ipFrom << ", Flow starting at: " << start_time << ", To " << ipTo << "\n";
      sourceApps.Start(Seconds(start_time));

      totalScheduledFlows++;
    }
  }

  return totalScheduledFlows;
}

int
simulateTrafficForTimePeriod(NodeContainer hosts, std::map<uint32_t, uint32_t> ipToPort,
  int experimentTime, int k, std::string filename, double avgPacketsPerSec,
  double sameEdgeProb, double samePodProb, std::vector< std::pair<double,unsigned long long> > distribution)
{
  Ptr<UniformRandomVariable> rndGen = CreateObject<UniformRandomVariable> ();
  uint32_t totalNodes = hosts.GetN();
  uint32_t hostsPerPod = (k/2) * (k/2);
  unsigned long long sampleFlowSize;
  int totalFlows = 0;

  NS_ASSERT(sameEdgeProb < 1.0);
  NS_ASSERT(samePodProb < 1.0);
  NS_ASSERT((sameEdgeProb + samePodProb) <= 1.0);

  for (uint32_t i=0; i<totalNodes; i++) {

    // generator for random inter-arrival times
    std::mt19937 arrivalTime_rng(std::random_device{}());
    std::exponential_distribution<double> interArrivalTime(avgPacketsPerSec);
    double start_time = 0 ;

    // generator for destination switch, pod and host
    std::mt19937 destHost_rng(std::random_device{}());
    std::uniform_real_distribution<double> uniformDistribution(0.0,1.0);

    // generator for destination switch, pod and host
    std::mt19937 flowSize_rng(std::random_device{}());
    std::uniform_real_distribution<double> uniformDistributionFlows(0.0,1.0);

    // for each different node, give a different seed (same across experiments)
    arrivalTime_rng.seed(42*i);
    destHost_rng.seed(42*42*i);
    flowSize_rng.seed(42*42*42*i);

    while(start_time < experimentTime) {
      // generate random number which will tell you where to send
      // same edge, same pod different pod
      double num = uniformDistribution(destHost_rng);
      //std::cout << "Generated :" << num << "\n";

      uint32_t hostPod, hostId, hostEdge, sendPod, sendSwitch, sendHost;
      hostPod = i / hostsPerPod;
      hostId = (i % (k/2)) ;
      hostEdge = (i / (k/2))%(k/2);

      Ptr<Node> hostFrom = hosts.Get(i);

      if (num < sameEdgeProb) {
        //NS_LOG_FUNCTION("Same edge");
        sendPod = hostPod;
        sendSwitch = hostEdge;

        sendHost = rndGen->GetInteger(0, k/2-1);
        while (sendHost == hostId) {
          sendHost = rndGen->GetInteger(0, k/2-1);
        }
      }
      else if (num < (samePodProb + sameEdgeProb)) {
        //NS_LOG_FUNCTION("Same pod");
        sendPod = hostPod;

        sendSwitch = rndGen->GetInteger(0, k/2-1);
        while (sendSwitch == hostEdge) {
          sendSwitch = rndGen->GetInteger(0, k/2-1);
        }

        sendHost = rndGen->GetInteger(0, k/2-1);
      }
      else {
        //NS_LOG_FUNCTION("Different pod");
        sendPod = rndGen->GetInteger(0, k-1);
        while (sendPod == hostPod) {
          sendPod = rndGen->GetInteger(0, k-1);
        }

        sendSwitch = rndGen->GetInteger(0, k/2-1);
        sendHost = rndGen->GetInteger(0, k/2-1);
      }

      uint32_t nodeIndex = sendPod * hostsPerPod + sendSwitch * k/2 + sendHost;

      Ptr<Node> hostTo = hosts.Get(nodeIndex);
      Ipv4Address ipFrom = GetNodeIp(hostFrom);
      Ipv4Address ipTo = GetNodeIp(hostTo);
      uint32_t portTo = ipToPort.at(ipTo.Get());

      sampleFlowSize = getFlowSizeFromDistribution(distribution, uniformDistributionFlows(flowSize_rng));

      BulkSendHelper source ("ns3::TcpSocketFactory",
                      InetSocketAddress (ipTo, portTo), ipFrom, filename);
      source.SetAttribute ("MaxBytes", UintegerValue (sampleFlowSize));
      ApplicationContainer sourceApps = source.Install (hostFrom);
      sourceApps.Stop(Seconds(TOTAL_SIM_TIME));

      // get a random arrival time according to the exponential distribution
      // which simulates the Poisson arrival process model
      start_time += interArrivalTime(arrivalTime_rng);

      NS_LOG_FUNCTION("Scheduling flow with size "<< sampleFlowSize <<
                  ". At " << start_time <<
                  ". From " << ipFrom << " to " << ipTo << ":" << portTo);

      //std::cout << "From : " << ipFrom << ", Flow starting at: " << start_time << ", To " << ipTo << "\n";
      sourceApps.Start(Seconds(start_time));

      totalFlows++;
    }
  }

  return totalFlows;
}

unsigned long long
getFlowSizeFromDistribution (std::vector< std::pair<double,unsigned long long> > distribution, double sampledNumber)
{
  // NS_LOG_FUNCTION(sampledNumber);
  NS_ASSERT_MSG(sampledNumber <= 1.0, "Provided sampled number is bigger than 1.0!");

  // iterating over the distribution until we choose a size
  // just like in the python code
  for (uint32_t i=0; i<distribution.size(); i++) {
    double prob = distribution[i].first;
    unsigned long long size = distribution[i].second;

    if (sampledNumber <= prob) {
      return size;
    }
  }

  NS_ASSERT_MSG(false, "Should have chossen a flow size by now!");
  // this is for compilers sake
  return 1500;
}

std::vector< std::pair<double,unsigned long long> >
getDistribution(std::string distributionFile) {

  std::vector< std::pair<double,unsigned long long> > cumulativeDistribution;
  std::ifstream infile(distributionFile);

  NS_ASSERT_MSG(infile, "Please provide a valid file for reading the flow size distribution!");
  double cumulativeProbability;
  int size;
  while (infile >> size >> cumulativeProbability)
  {
    cumulativeDistribution.push_back(std::make_pair(cumulativeProbability, size));
  }

  for(uint32_t i = 0; i < cumulativeDistribution.size(); i++)
  {
    NS_LOG_FUNCTION(cumulativeDistribution[i].first << " " << cumulativeDistribution[i].second);
  }

  return cumulativeDistribution;
}

std::vector< std::pair<double,unsigned long long> >
getDistribution(std::string distributionFile, uint32_t packetSize) {

  std::vector< std::pair<double,unsigned long long> > cumulativeDistribution;
  std::ifstream infile(distributionFile);

  NS_ASSERT_MSG(infile, "Please provide a valid file for reading the flow size distribution!");
  double cumulativeProbability;
  int size;
  while (infile >> size >> cumulativeProbability)
  {
    cumulativeDistribution.push_back(std::make_pair(cumulativeProbability, size * packetSize));
  }

  for(uint32_t i = 0; i < cumulativeDistribution.size(); i++)
  {
    NS_LOG_FUNCTION(cumulativeDistribution[i].first << " " << cumulativeDistribution[i].second);
  }

  return cumulativeDistribution;
}

std::map< uint32_t, uint32_t>
installServersOnHosts(NodeContainer hosts) {

  uint32_t baseServerPort = 50000;
  std::map<uint32_t, uint32_t> addrToPort;

  for (uint32_t i=0; i<hosts.GetN(); i++) {

    Ptr<Node> node = hosts.Get(i);
    Ptr<Ipv4> ip = node->GetObject<Ipv4> ();

    NS_ASSERT(ip !=0);
    ObjectVectorValue interfaces;
    ip->GetAttribute("InterfaceList", interfaces);
    for(ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End (); j ++)
    {
      Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
      NS_ASSERT(ipIface != 0);
      Ptr<NetDevice> device = ipIface->GetDevice();
      NS_ASSERT(device != 0);
      Ipv4Address ipAddr = ipIface->GetAddress (0).GetLocal();

      // ignore localhost interface...
      if (ipAddr == Ipv4Address("127.0.0.1")) {
        continue;
      }

      NS_LOG_FUNCTION("Installing server on host " << ipAddr <<
                  " at port " << baseServerPort);
      PacketSinkHelper sink ("ns3::TcpSocketFactory",
                                   InetSocketAddress (Ipv4Address::GetAny (), baseServerPort));
      ApplicationContainer apps = sink.Install(node);

      apps.Stop(Seconds(TOTAL_SIM_TIME));
      apps.Start(Seconds(0.0));

      // insert address port mapping into map
      addrToPort[ipAddr.Get()] = baseServerPort;
      baseServerPort++;
    }
  }

  return addrToPort;
}

}
