#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <sstream>
#include <vector>

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
#include "ns3/ecmp-net-device.h"
#include "ns3/ipv4.h"
#include "ns3/custom-switches.h"
#include "ns3/custom-switches-helper.h"
#include "ns3/packet.h"
#include "ns3/arp-cache.h"
#include "ns3/ecmp-helper.h"
#include "ns3/traffic-simulation.h"
#include "ns3/traffic-control-module.h"
#include "ns3/utils-switches.h"
#include "ns3/config-store.h"
#include "ns3/dce-module.h"
#include "ns3/arp-cache.h"
#include "ns3/ipv4-linux.h"
#include "misc-tools.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("fat-tree");

void printSysctl(std::string key, std::string value)
{
  std::cout << key << " = " << value;
}

void CheckQueueSize(Ptr<Queue> queue)
{
  uint32_t qSize = StaticCast<DropTailQueue> (queue)->GetNPackets();
  std::cout << "QUEUE SIZE = " << qSize << "\n";
  Simulator::Schedule (Seconds (0.01), &CheckQueueSize, queue);
}

int
main(int argc, char *argv[])
{

//=========== Define parameters based on value of k ===========//
//         The parameters are self-described below (cmd)       //

  int simTime = 10;
  int k = 4;						
  string expType = "cdf";			 
  string expId = "1";
  char dataRate [200] = "1000Mbps";
  uint32_t delay = 50;
  uint64_t queueSize = 200;	
  double probSamePod = 0.3;
  double probSameEdge = 0.2;
  double flowsPerSec = 30;
  uint32_t flowSize = 128000;   // This is for experiments with fixed flow size (e.g incast)
  string distrFile = "";
  
  uint32_t tcpSndBufSize                = 131072;
  uint32_t tcpRcvBufSize                = 131072;

  string tcpTcpNoDelay                    = "1";
  string tcpWindowScaling                 = "1";
  string tcpSack                          = "1";
  string tcpFack                          = "1";
  string tcpDsack                         = "1";
  string tcpTimestamp                     = "1";
  string tcpLowLatency                    = "0";

  string tcpAbortOnOverflow               = "1";
  string tcpSlowStartAfterIdle			  = "1";
  string tcpNoMetricsSave                 = "1";
  string tcpFrto                          = "1";
  string tcpTwReuse                       = "1";
  int tcpReordering                   = 3;
  int tcpEarlyRetrans                 = 2;
  int tcpInitCwnd					  = 2;
  uint64_t tcpMinRto                      = 1000000;
  uint32_t tcpReTxThreshold               = 3;
  uint32_t tcpFinTimeout                  = 30;
  uint32_t tcpRetries1	                  = 3;

  uint32_t flowletGap					= 100;		// Flowlet gap in microseconds
  uint32_t ecnThresh                    = 0;	   // ECN disabled by default
  string dctcpEnable                    = "1";   // DCTCP Support
  int dctcpG                        = 4;			// The actual g value is 1/2^dctcpG
	

  CommandLine cmd;
  
  // Experiment specific parameters
  cmd.AddValue("simTime", "Total simulation time", simTime);
  if (simTime < 2.0) {
    NS_FATAL_ERROR("Total simulation time should be greater than 2.0 seconds");
  }
  cmd.AddValue("expType", "Experiment type (normal, cdf, incast)", expType);
  cmd.AddValue("expId", "Unique experiment ID", expId);
  cmd.AddValue("probSamePod", "Probability of sending to host to the same pod", probSamePod);
  cmd.AddValue("probSameEdge", "Probability of sending to host to the same edge switch", probSameEdge);
  cmd.AddValue("flowsPerSec", "Flows per second", flowsPerSec);
  cmd.AddValue("flowSize", "Size of each iperf flow in bytes", flowSize);
  cmd.AddValue("distrFile", "File with cumulative distribution of flow sizes", distrFile);
  
  // Architecture Specific Parameters
  cmd.AddValue("k", "Ports of Fat-Tree", k);
  cmd.AddValue("dataRate", "Bandwidth of link, used in multiple experiments", dataRate);
  cmd.AddValue("delay", "Delay of link, used in multiple experiments", delay);
  cmd.AddValue("queueSize", "Size of queues in devices.", queueSize);
  
  // TCP Specific Parameters
  cmd.AddValue("tcpSndBufSize", "TCP Initial Send Buffer Size (Bytes)", tcpSndBufSize);
  cmd.AddValue("tcpRcvBufSize", "TCP Initial Receive Buffer Size (Bytes)", tcpRcvBufSize);
  cmd.AddValue("tcpWindowScaling", "Enable or disable the Window Scaling option", tcpWindowScaling);
  cmd.AddValue("tcpSack", "Enable or disable the SACK option", tcpSack);
  cmd.AddValue("tcpFack", "Enable or disable the FACK option", tcpFack);
  cmd.AddValue("tcpDsack", "Enable or disable the DSACK option", tcpDsack);
  cmd.AddValue("tcpReordering", "tcp_reordering value", tcpReordering);
  cmd.AddValue("tcpTimestamp", "Enable or disable the Timestamp option", tcpTimestamp);
  cmd.AddValue("tcpLowLatency", "Enable or disable optimization for Low Latency", tcpLowLatency);
  cmd.AddValue("tcpMinRto", "TCP minimum retransmit timeout value", tcpMinRto);
  cmd.AddValue("tcpReTxThreshold", "TCP Threshold for fast retransmit", tcpReTxThreshold);
  cmd.AddValue("tcpAbortOnOverflow", "TCP Abort on overflow", tcpAbortOnOverflow);
  cmd.AddValue("tcpEarlyRetrans", "TCP Early retransmission", tcpEarlyRetrans);
  cmd.AddValue("tcpFinTimeout", "TCP Early retransmission", tcpFinTimeout);
  cmd.AddValue("tcpFrto", "TCP Early retransmission", tcpFrto);
  cmd.AddValue("tcpNoMetricsSave", "TCP do not cache initssht", tcpNoMetricsSave);
  cmd.AddValue("tcpTwReuse", "Reuse Time-wait connections", tcpTwReuse);
  cmd.AddValue("tcpTcpNoDelay", "Disable Nagle's Algorithm", tcpTcpNoDelay);
  cmd.AddValue("tcpRetries1", "How many times to try to reach host before reporting to net layer", tcpRetries1);
  cmd.AddValue("tcpTcpNoDelay", "Disable Nagle's Algorithm", tcpTcpNoDelay);
  cmd.AddValue("tcpSlowStartAfterIdle", "Return to slow start after connection timeout", tcpSlowStartAfterIdle);
  cmd.AddValue("tcpInitCwnd", "Initical TCP Congestion Window", tcpInitCwnd);
  
  // DCTCP Parameters
  cmd.AddValue("dctcpEnable", "Enable DCTCP", dctcpEnable);
  cmd.AddValue("dctcpG", "g parameter of DCTCP", dctcpG);
  
  // Switch Parameters
  cmd.AddValue("ecnThresh", "Queue Threshold for marking packets with CE", ecnThresh);
  cmd.AddValue("flowletGap", "Flowlet gap in microseconds", flowletGap);
  
  cmd.Parse (argc,argv);

  int num_pod = k;			// Number of pods
  int num_host = (k/2);		// Number of hosts under a switch
  int num_edge = (k/2);		// Number of edge switches in a pod
  int num_bridge = num_edge;	// Number of bridges in a pod
  int num_agg = (k/2);		// Number of aggregation switches in a pod
  int num_group = k/2;		// Number of groups of core switches
  int num_core = (k/2);		// Number of core switches in a group
  int total_host = k*k*k/4;	// Number of hosts in the entire network

  // Initialize other variables
  int i = 0;
  int j = 0;
  int h = 0;
  int seed = 0;

  // Masks for switches
  Ipv4Mask edgeMask = Ipv4Mask("255.255.255.255");
  Ipv4Mask aggMask = Ipv4Mask("255.255.255.0");
  Ipv4Mask coreMask = Ipv4Mask("255.255.0.0");

  // Output some useful information
//  std::cout << "Value of k =  " << k<< "\n";
//  std::cout << "Total number of hosts =  " << total_host << "\n";
//  std::cout << "Type of experiment = " << expType << "\n";
//  std::cout << "Size of queues in switches = " << queueSize << "\n";
//  if (ecnThresh > 0)
//  	std::cout << "Threshold of ECN marking = " << ecnThresh << "\n";
//  else
//  	std::cout << "ECN Disabled!\n";
//  std::cout << "Distribution (if used) is " << distrFile << "\n";
//  std::cout << "Total Simulation Time is " << TOTAL_SIMULATION_TIME << "\n";
  
  // Configure Switches
  Config::SetDefault ("ns3::Queue::MaxPackets", UintegerValue(uint32_t(queueSize)));
  Config::SetDefault("ns3::CsmaNetDevice::ecnThresh", UintegerValue(ecnThresh));
  Config::SetDefault("ns3::EcmpNetDevice::FlowletGap", TimeValue(MicroSeconds(flowletGap)));
  
  // Configure Links
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue (dataRate));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (delay)));
  csma.SetDeviceAttribute("Mtu", UintegerValue(1500));
  csma.SetChannelAttribute("FullDuplex", BooleanValue(true));
  
  // Configure Linux Stack on the hosts
  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute( "FiberManagerType", StringValue ( "UcontextFiberManager" ) );
  std::string kernel_lib = std::string("libsim-linux2.6.36-initcwnd-") + std::to_string(tcpInitCwnd) + std::string(".so");
  dceManager.SetNetworkStack("ns3::LinuxSocketFdFactory", "Library", StringValue (kernel_lib));
  LinuxStackHelper stack;

  // Creation of Node Containers
  EcmpNodeContainer core[num_group];			// NodeContainer for core switches
  for (i=0; i<num_group;i++){
  	core[i].Create (num_core);
  }
  
  EcmpNodeContainer agg[num_pod];				// NodeContainer for aggregation switches
  for (i=0; i<num_pod;i++){
  	agg[i].Create (num_agg);
  }
  
  EcmpNodeContainer edge[num_pod];				// NodeContainer for edge switches
  for (i=0; i<num_pod;i++){
  	edge[i].Create (num_bridge);
  }
  
  NodeContainer host[num_pod][num_bridge];		// NodeContainer for hosts
  NodeContainer tempHosts;
  
  for (i=0; i<k;i++){
    for (j=0;j<num_bridge;j++){
  	  host[i][j].Create (num_host);
  	  // Install appropriate stack and DCE manager on the hosts
      stack.Install(host[i][j]);
  	  dceManager.Install(host[i][j]);
  	  // Here we disable IPV6 so that we don't get any solicitations from the
  	  // hosts every time we bring up a link.
  	  stack.SysctlSet (host[i][j], ".net.ipv6.conf.default.disable_ipv6", "1");
  	  stack.SysctlSet (host[i][j], ".net.ipv6.conf.all.disable_ipv6", "1");
      stack.SysctlSet (host[i][j], ".net.ipv4.congestion_control", "reno");

      // DCTCP Params
      if (dctcpEnable == "1") {
        stack.SysctlSet (host[i][j], ".net.ipv4.tcp_dctcp_enable", "1"); 
        stack.SysctlSet (host[i][j], ".net.ipv4.tcp_ecn", "1");
        stack.SysctlSet (host[i][j], ".net.ipv4.tcp_dctcp_shift_g", std::to_string(dctcpG));
      }

      // TCP Options
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_window_scaling", tcpWindowScaling);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_sack", tcpSack);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_fack", tcpFack);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_dsack", tcpDsack);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_timestamps", tcpTimestamp);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_low_latency", tcpLowLatency);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_slow_start_after_idle", tcpSlowStartAfterIdle);
      
      // TCP Buffer Sizes
      if (tcpRcvBufSize < 87380) {
        NS_FATAL_ERROR("TCP Rcv buffer size should be set larger than the default (87380)");
      }
      else {
        std::string rmemString("4096 87380 ");
        rmemString += std::to_string(tcpRcvBufSize);
        stack.SysctlSet (host[i][j], ".net.ipv4.tcp_rmem", rmemString);
      }

      if (tcpSndBufSize < 65536) {
        NS_FATAL_ERROR("TCP Snd buffer size should be set larger than the default (65536)");
      }
      else {
        std::string wmemString("4096 65536 ");
        wmemString += std::to_string(tcpSndBufSize);
        stack.SysctlSet (host[i][j], ".net.ipv4.tcp_wmem", wmemString);
      }

      // Rest of the parameters
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_abort_on_overflow", tcpAbortOnOverflow);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_early_retrans", std::to_string(tcpEarlyRetrans));
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_fin_timeout", std::to_string(tcpFinTimeout));
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_frto", tcpFrto);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_no_metrics_save", tcpNoMetricsSave);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_tw_reuse", tcpTwReuse);
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_reordering", std::to_string(tcpReordering));
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_retries_1", std::to_string(tcpRetries1));
      stack.SysctlSet (host[i][j], ".net.ipv4.tcp_syncookies", "1");

  	  tempHosts.Add(host[i][j]);
  	}
  }

  // Print out the TCP settings for the hosts
//  std::cout << " TCP Settings on the hosts\n";
//  std::cout << " -------------------------\n";
//  LinuxStackHelper::SysctlGet(host[0][0].Get(0), Seconds (0.1),
//  ".net.ipv4.tcp_wmem", &printSysctl);
//  LinuxStackHelper::SysctlGet(host[0][0].Get(0), Seconds (0.1),
//  ".net.ipv4.tcp_rmem", &printSysctl);
//  LinuxStackHelper::SysctlGet(host[0][0].Get(0), Seconds (0.1),
//  ".net.ipv4.tcp_frto", &printSysctl);
//  LinuxStackHelper::SysctlGet(host[0][0].Get(0), Seconds (0.1),
//  ".net.core.rmem_max", &printSysctl);
//  LinuxStackHelper::SysctlGet(host[0][0].Get(0), Seconds (0.1),
//  ".net.ipv4.tcp_moderate_rcvbuf", &printSysctl);


  // Containers for useful devices, used to build up the Fat-Tree
  // appropriately
  NetDeviceContainer hostSw[num_pod][num_bridge];
  NetDeviceContainer edgeUplinks[num_pod][num_bridge];
  NetDeviceContainer aggUplinks[num_pod][num_bridge];
  NetDeviceContainer coreUpLinks[num_core][num_group];

  NetDeviceContainer edgeDevices[num_pod][num_bridge];
  NetDeviceContainer aggDevices[num_pod][num_bridge];
  NetDeviceContainer coreDevices[num_core][num_group];

  std::map<uint32_t, Ptr<NetDevice> > routesEdges[num_pod][num_edge];
  std::map<uint32_t, Ptr<NetDevice> > routesAggs[num_pod][num_agg];
  std::map<uint32_t, Ptr<NetDevice> > routesCores[num_group][num_core];
  
  // Node to (IP, MAC) mappings (Each node has only one interface)
  std::map<Ptr<Node>, std::pair<std::string, std::string> > nodeToIpMac;

  // Connect edge switches to hosts

  char *mask = toString(255,0,0,0);
  char *addr;

  // Connect edge switches with hosts
  for (i=0; i<num_pod; i++) {
    for (j=0; j<num_edge; j++) {
      for (h=0; h<num_host; h++){
        NetDeviceContainer link1 = csma.Install(NodeContainer (edge[i].Get(j), host[i][j].Get(h)));
        hostSw[i][j].Add(link1.Get(1));
        edgeDevices[i][j].Add(link1.Get(0));
        

        // Install appropriate IP to the host
        addr = toString(10, i, j, h + 2);
        std::string addrWithMask = std::string(addr) + "/8";
        AddAddress(host[i][j].Get(h), Seconds(0.1), "sim0", addrWithMask.c_str());
        RunIp(host[i][j].Get(h), Seconds(0.2), "link set sim0 up arp on");
        
        // Also save the MAC of the host for the ARP cache
        Mac48Address mac = Mac48Address::ConvertFrom(link1.Get(1)->GetAddress ());
        ostringstream os;
        os << mac;
        std::string macAddr = os.str();
        nodeToIpMac[host[i][j].Get(h)] = std::pair<std::string, std::string>(addr, macAddr);
        //std::cout << "host[" << i << "][" << j << "][ " << h << "]"  << " MAC = " << macAddr
        //			 << " , IP = " << addrWithMask  << "\n";
        
        // Create route rules for this switch
        // this map will be later added to the ecmp-bridge device
        // and used for mapping to interfaces. Sadly, we can not use the
        // Ipv4Address as key because we would need to extend it
        routesEdges[i][j][Ipv4Address(addr).Get()] = link1.Get(0);
      }
    }
  }

//TODO: Fix indentation :)
//=========== Connect aggregate switches to edge switches ===========//
//
  for (i=0; i<num_pod; i++) {
    for (j=0; j<num_agg; j++) {
      for (h=0; h<num_edge; h++) {
        NetDeviceContainer link1 = csma.Install(NodeContainer(agg[i].Get(j), edge[i].Get(h)));
        edgeUplinks[i][h].Add(link1.Get(1));
        aggDevices[i][h].Add(link1.Get(0));
        edgeDevices[i][h].Add(link1.Get(1));
        addr = toString(10, i, h, 0);
        Ipv4Address ipAddr = Ipv4Address(addr);
        routesAggs[i][j][ipAddr.Get()] = link1.Get(0);
      }
    }
  }

  // Now create devices for ecmp-bridge switches :)
  for (i=0;i<num_pod;i++)
    for (j=0;j<num_edge;j++) {
      Ptr<Node> edgeNode = edge[i].Get(j);
      EcmpHelper ecmpBridge;
      ecmpBridge.Install ( edgeNode, SwitchType::EDGE_SWITCH, edgeUplinks[i][j], routesEdges[i][j], edgeMask, seed++);
    }

//=========== Connect core switches to aggregate switches ===========//
//

	for (i=0; i<num_group; i++){
		for (j=0; j < num_core; j++){
			for (h=0; h < num_pod; h++){
				NetDeviceContainer link1 = csma.Install(NodeContainer(core[i].Get(j), agg[h].Get(i)));
                aggUplinks[h][i].Add(link1.Get(1));
                aggDevices[h][i].Add(link1.Get(1));

        addr = toString(10, h, 0, 0);
        Ipv4Address ipAddr = Ipv4Address(addr);
        routesCores[i][j][ipAddr.Get()] = link1.Get(0);
			}
		}
	}
  for (i=0;i<num_pod;i++)
    for (j=0; j<num_agg; j++) {
      Ptr<Node> aggNode = agg[i].Get(j);
      EcmpHelper ecmpBridge;
      ecmpBridge.Install(aggNode, SwitchType::AGG_SWITCH, aggUplinks[i][j], routesAggs[i][j], aggMask, seed++);
    }

  for (i=0;i<num_group;i++)
    for (j=0;j<num_core;j++) {
      Ptr<Node> coreNode = core[i].Get(j);
      EcmpHelper ecmpBridge;
      ecmpBridge.Install(coreNode, SwitchType::CORE_SWITCH, coreUpLinks[i][j], routesCores[i][j], coreMask, seed++);
    }


	//Packet::EnablePrinting ();
	//Packet::EnableChecking ();

  // Populate the ARP Cache. For now every host holds the mappings for all the other hosts.
  // TODO: This seems ridiculous for a datacenter environment, but seems to be the only solution,
  // since our switches are L2.

  //std::cout << "NODEIOMAC LENGTH = " << nodeToIpMac.size() << "\n";
  for (int i=0; i < num_pod; i++) {
    for (int j=0; j < num_edge; j++) {
      for (int h=0; h < num_host; h++) {
		int k=0;
		for (auto item : nodeToIpMac) {
        	ostringstream ipCommand;
			std::string nodeIp = item.second.first;
			std::string nodeMac = item.second.second;
        	ipCommand << "neigh add " << nodeIp <<
						 " lladdr " << nodeMac <<
						 " dev sim0";
			RunIp(host[i][j].Get(h), Seconds(0.5), ipCommand.str());
			k++;
		}
      }
    }
  }

  DceApplicationHelper dce;
  ApplicationContainer apps;
  dce.SetStackSize (1 << 30);

  // TESTING. Simple iperf from host[1][1] to host[2][0]
  //std::cout << "Starting iperf from " << ipFrom << " to " << ipTo << "\n";
	for (i=0;i<num_pod;i++) {
    	for (j=0;j<num_edge;j++) {
		  	for (h=0;h<num_host; h++) {
  				dce.SetBinary ("iperf");
  				dce.ResetArguments();
				dce.ResetEnvironment();
				dce.AddArgument ("-s");
				//dce.AddArgument ("-i");
				//dce.AddArgument ("-D");
				//dce.AddArgument ("1");
				//dce.AddArgument ("--time");
				//dce.AddArgument (std::to_string(experimentTime));	
  				apps = dce.Install (host[i][j].Get(h));
  				apps.Start (Seconds(SERVERS_START_TIME));
  				apps.Stop (Seconds (simTime));
			}
		}
	}


  //========= Traffic Simulation ===========// 

  if (expType == "normal") {
    std::vector<std::pair<double, uint32_t>> flows;
    flows = generateFlowsNormalDist(flowsPerSec, 10000, 100000, 10000, simTime);
    std::cout << "Generated a total of " << flows.size() << " flows\n";
    int seed_factor = 1;
    for (auto flow : flows) {
      // Randomly pick up a source-destination pair
	  auto srcDst = getSourceDestPairs(0.2, 0.3, k, seed_factor);
	  //XXX: For now, iperf supports granularity of 128K only
  	  StartFlow(host[std::get<0>(srcDst)][std::get<1>(srcDst)].Get(std::get<2>(srcDst)),
			  host[std::get<3>(srcDst)][std::get<4>(srcDst)].Get(std::get<5>(srcDst)),
			  Seconds(CLIENTS_START_TIME + flow.first), 128000, nodeToIpMac, simTime);
	  seed_factor++;
    }
  }
  else if (expType == "cdf") {
    std::vector<std::pair<double, uint32_t>> flows;
    flows = generateFlowsCdf(flowsPerSec, distrFile, simTime);
	std::cout << "Generated " << flows.size() << " flows!\n";

    int seed_factor = 1;
    for (auto flow : flows) {
      // Randomly pick up a source-destination pair
	  auto srcDst = getSourceDestPairs(0.2, 0.3, k, seed_factor);
	  //XXX: For now, iperf supports granularity of 128K only
  	  StartFlow(host[std::get<0>(srcDst)][std::get<1>(srcDst)].Get(std::get<2>(srcDst)),
			  host[std::get<3>(srcDst)][std::get<4>(srcDst)].Get(std::get<5>(srcDst)),
			  Seconds(CLIENTS_START_TIME + flow.first), flow.second, nodeToIpMac, simTime);
	  seed_factor++;
    }
  }
  else if (expType == "incast") {
    // Here host[0][0][0] is going to act as the server and all the other hosts
    // will be sending flows to it starting at the same time.
    for (i=0;i<num_pod;i++) {
      for (j=0;j<num_edge;j++) {
        for (h=0;h<num_host; h++) {
          if (i+j+h != 0) {
            StartFlow(host[i][j].Get(h), host[0][0].Get(0), Seconds(CLIENTS_START_TIME+(i+j+h)*0.0001), flowSize, nodeToIpMac, simTime);
          }

        }
      }
	}
  }
  else if (expType == "test") {
  //std::cout << "Server ID " << host[0][0].Get(0)->GetId() << "\n";

  // Test flow
    //for (int i=0; i<2; i++) {
      StartFlow(host[0][0].Get(0), host[1][1].Get(1), Seconds(CLIENTS_START_TIME + i*0.01), 1280000, nodeToIpMac, simTime);
   // }
  }
  else {
    NS_FATAL_ERROR("Unrecognized experiment type!\n");
  }
  
  std::cout << "Starting Simulation.. "<<"\n";
  Packet::EnablePrinting();
  //PrintTime();
  
  // Add Queue Monitor
  //std::cout << "MAX QUEUE SIZE = " << queue->GetMaxPackets() << "\n";
  //Simulator::ScheduleNow (&CheckQueueSize, queue);

  Simulator::Stop (Seconds(simTime));

  //Pcap Tracing
  //csma.EnablePcapAll("fat-tree");

  Simulator::Run ();
  std::cout << "Simulation finished "<<"\n";

  //Output queue statistics to file
  ofstream queueStatsFile;
  queueStatsFile.open("queue_stats.out");
  queueStatsFile << "QUEUE STATISTICS FOR EXPERIMENT " << expId << "\n---------------------------\n";

  queueStatsFile << " -- EDGE QUEUE STATISTICS --\n";
  uint32_t droppedPackets = 0;
  uint32_t receivedPackets = 0;
  for (int i=0; i<num_pod; i++) {
    for (int j=0; j<num_edge; j++) {
      for (int h=0; h<k; h++) {
        Ptr<CsmaNetDevice> nd = StaticCast<CsmaNetDevice> (edgeDevices[i][j].Get (h));
        Ptr<Queue> queue = nd->GetQueue ();
		droppedPackets += queue->GetTotalDroppedPackets();
        receivedPackets += queue->GetTotalReceivedPackets();
        queueStatsFile << "EDGE (" << i << " ," << j << ") PORT " << h << ": Recv/Dropped : " << queue->GetTotalDroppedPackets() << "/" << queue->GetTotalReceivedPackets() << "\n";
      }
    }
  }

  queueStatsFile << " -- AGG QUEUE STATISTICS --\n";
  for (int i=0; i<num_pod; i++) {
    for (int j=0; j<num_edge; j++) {
      for (int h=0; h<k; h++) {
        Ptr<CsmaNetDevice> nd = StaticCast<CsmaNetDevice> (aggDevices[i][j].Get (h));
        Ptr<Queue> queue = nd->GetQueue ();
		droppedPackets += queue->GetTotalDroppedPackets();
        receivedPackets += queue->GetTotalReceivedPackets();
        queueStatsFile << "AGG (" << i << " ," << j << ") PORT " << h << ": Recv/Dropped : " << queue->GetTotalDroppedPackets() << "/" << queue->GetTotalReceivedPackets() << "\n";
      }
    }
  }

  queueStatsFile << "\n\nTOTAL RECEIVED PACKETS = " << receivedPackets << "\n";
  queueStatsFile << "TOTAL DROPPED PACKETS = " << droppedPackets;
  queueStatsFile.close();


  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  return 0;
}
