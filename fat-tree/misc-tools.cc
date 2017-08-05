#include "misc-tools.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/dce-module.h"
#include "ns3/random-variable-stream.h"

NS_LOG_COMPONENT_DEFINE("misc-tools");

namespace ns3 {

void setPos (Ptr<Node> n, int x, int y, int z)
{
  Ptr<ConstantPositionMobilityModel> loc = CreateObject<ConstantPositionMobilityModel> ();
  n->AggregateObject (loc);
  Vector locVec2 (x, y, z);
  loc->SetPosition (locVec2);
}

/* Runs the linux 'ip' command with the given arguments

   @param node The node to run the command on.
   @param at The simulator time at which to run the command.
   @param str The arguments to the 'ip' command.
 */
void RunIp (Ptr<Node> node, Time at, std::string str)
{
  NS_LOG_INFO(at << " :" << str << "\n");
  DceApplicationHelper process;
  ApplicationContainer apps;
  process.SetBinary ("ip");
  process.SetStackSize (1 << 30);
  process.ResetArguments ();
  process.ParseArguments (str.c_str ());
  apps = process.Install (node);
  apps.Start (at);
}

/* Sets up the IP address of a host.

   @param node The node to set the IP.
   @param at The simulator time at which to set the IP.
   @param name The name of the interface to set up.
   @param address The address to set.
*/
void AddAddress (Ptr<Node> node, Time at, const char *name, const char *address)
{
  std::ostringstream oss;
  oss << "-f inet addr add " << address << " dev " << name;
  RunIp (node, at, oss.str ());
}

std::string Ipv4AddressToString (Ipv4Address ad)
{
  std::ostringstream oss;
  ad.Print (oss);
  return oss.str ();
}

/* Starts an iperf flow for using DCE

   @param from The source host.
   @param to The destination host.
   @param flowSize The flow size in bytes.
   @param nodeToIpMac Map with IP-MAC bindings.
   @param simTime The total simulation time.
*/
void StartFlow(Ptr<Node> from, Ptr<Node> to, Time at, uint32_t flowSize,
			  std::map<Ptr<Node>, std::pair<std::string, std::string> > nodeToIpMac, int 			   simTime)
{

  if (flowSize % 128000 != 0) {
  	NS_FATAL_ERROR("Flowsize has to be multiple of 128K");
  }

  std::string ipFrom = nodeToIpMac[from].first;
  std::string ipTo = nodeToIpMac[to].first;

  //std::cout << at << " : Starting flow of size " << flowSize << " from " << ipFrom << " to " << ipTo << "\n";

  DceApplicationHelper dce;
  ApplicationContainer apps;
  dce.SetStackSize(1 << 30);
  dce.SetBinary("iperf");
  dce.ResetArguments();
  dce.ResetEnvironment();
  dce.AddArgument("-c");
  dce.AddArgument(ipTo);
  dce.AddArgument("-n");
  dce.AddArgument(std::to_string(flowSize));
  apps = dce.Install (from);
  apps.Start (at);
  apps.Stop (Seconds(simTime));

  return;

}

/* Generates flows with sizes drawn from a uniform distribution.
   Flow arrival times are drawn from an exponential distibution.

   @param flowsPerSec The flow interarrival rate fot the exponential distribution.
   @param minFlowSize Minimum flow size.
   @param maxFlowSize Maximum flow size.
   @param variance Variance of the flow sizes.
   @param simTime Total simulation time.
*/

std::vector<std::pair<double, uint32_t>> generateFlowsNormalDist(int flowsPerSec, uint32_t minFlowSize, uint32_t maxFlowSize, double variance, int simTime)
{

	std::vector<std::pair<double, uint32_t>> flows;
	std::exponential_distribution<double> inter_arrival_time(flowsPerSec);
	std::normal_distribution<double> flowSizeDist((minFlowSize + maxFlowSize) / 2.0, variance);

	std::mt19937 rng(std::random_device{}());
	rng.seed(42);
	double time = CLIENTS_START_TIME + 0.5;
	// Stop generating flows for the last 5 seconds of the total time
	// in order to let all the flows finish inside the simulation window
	while (time < simTime - 5.0) {
		uint32_t size;
		do 
			size = std::round(flowSizeDist(rng));
		while (size < minFlowSize || size > maxFlowSize);
		//std::cout << "Generating flow at " << time << " with size = " << size << "\n";
		flows.push_back(std::pair<double,uint32_t>(time, size));
		time += inter_arrival_time(rng);
	}
	return flows;

}

/* Generates flows according to a Cumulative Distribution Function from a file.

   @param flowsPerSec The flow interarrival rate for the exponential distribution.
   @param distFile The file containing the CDF of the flow sizes.
   @param simTime The total simulation time.
*/
std::vector<std::pair<double, uint32_t>> generateFlowsCdf(int flowsPerSec, std::string distFile, int simTime)
{
	std::vector<std::pair<double, uint32_t>> flows;
	std::exponential_distribution<double> inter_arrival_time(flowsPerSec);
	std::mt19937 rng(std::random_device{}());
	rng.seed(42);

	// Parse CDF file
	std::vector<std::pair<uint32_t, double>> cdf;
	std::ifstream cdfFile(distFile);
	uint32_t size;
	double prob;
	while (cdfFile >> size >> prob) {
		cdf.push_back(std::pair<uint32_t,double>(size, prob));
	}

	std::uniform_real_distribution<double> uniformDistribution(0.0, 1.0);
	double time = CLIENTS_START_TIME + 0.5;
	while (time < simTime - 5.0) {
		// Sample a flow size from the CDF
		double num = uniformDistribution(rng);
		for (int i=0; i<cdf.size(); i++) {
			double prob = cdf[i].second;
			uint32_t size = cdf[i].first;
			if (num <= prob) {
				flows.push_back(std::pair<double, uint32_t>(time, size));
				break;
			}
		}
		time += inter_arrival_time(rng);
		
	}
	
	return flows;
}


/* Create source-destination pairs of hosts in a uniformly at random fashion.

   @param sameEdgeProb Probability that source and destination are on the same edge switch.
   @param samePodProb Probability that source and destination are on the same pod.
   @param k The fat-tree's k parameter.
   @param seed_factor Seed factor for calculating the seed for the RNG.
*/
std::tuple<int, int, int, int, int, int> getSourceDestPairs(double sameEdgeProb, double samePodProb, int k, int seed_factor) {

	int num_pod = k;
	int num_edge = (k/2);
	int num_host = (k/2);

	int src_pod, src_edge, src_host;
	int dst_pod, dst_edge, dst_host;

	std::mt19937 rng(std::random_device{}());
	Ptr<UniformRandomVariable> rndGen = CreateObject<UniformRandomVariable> ();

	// Uniformly sample source host
	src_pod = rndGen->GetInteger(0, k-1);
	src_edge = rndGen->GetInteger(0, k/2 - 1);
	src_host = rndGen->GetInteger(0, k/2 - 1);

	// The same seed is used for the same generated flows so that the experiments
	// can be reproducible.	
	rng.seed(42*seed_factor);
	// Now choose the destination host based on the probabilities	
	std::uniform_real_distribution<double> uniformDistribution(0.0,1.0);
	double num = uniformDistribution(rng);
	if (num < sameEdgeProb) {
		// Same Edge
		dst_pod = src_pod;
		dst_edge = src_edge;
		dst_host = rndGen->GetInteger(0, k/2-1);
		while (dst_host == src_host)
			dst_host = rndGen->GetInteger(0, k/2-1);
	}
	else if (num < sameEdgeProb + samePodProb) {
		// Same Pod
		dst_pod = src_pod;
		dst_edge = rndGen->GetInteger(0, k/2-1);
		while (dst_edge == src_edge)
			dst_edge = rndGen->GetInteger(0, k/2-1);
		dst_host = rndGen->GetInteger(0, k/2-1);
	}
	else {
		// Different Pod
        dst_pod = rndGen->GetInteger(0, k-1);
        while (dst_pod == src_pod) {
            dst_pod = rndGen->GetInteger(0, k-1);
        }

        dst_edge = rndGen->GetInteger(0, k/2-1);
        dst_host = rndGen->GetInteger(0, k/2-1);
	}	

	//std::cout << "(" << src_pod << ", " << src_edge << ", " << src_host
	//		   << ") -> (" << dst_pod << ", " << dst_edge << ", " << dst_host << ")\n";
	return std::tuple<int, int, int, int, int, int>(src_pod, src_edge, src_host, dst_pod, dst_edge, dst_host);
}

}
