/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "utils-switches.h"

NS_LOG_COMPONENT_DEFINE ("Utils");

namespace ns3 {

#define PRINT_TIME_THRES 15

// prints the time in seconds and re-schedules to run
// in one second
void
PrintTime() {
  uint32_t seconds =  Simulator::Now().GetSeconds();

  if (seconds < PRINT_TIME_THRES) {
    std::cout << "Simulator at " << seconds << " Second\n";
    Simulator::Schedule(Seconds(1.0), &PrintTime);
  }
}


//XXX Does not work for linux stack
void
PopulateArpCache (NodeContainer NodeList)
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds(3600 * 24));

  for (uint32_t i = 0; i < NodeList.GetN (); ++i)
  {
    Ptr<Node> node = NodeList.Get(i);
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
      Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
      Ipv4Address ipAddr = ipIface->GetAddress (0).GetLocal();

      // ignore localhost interface...
      if (ipAddr == Ipv4Address("127.0.0.1")) {
        continue;
      }

      ArpCache::Entry * entry = arp->Add(ipAddr);
      entry->SetMacAddresss (addr);

      NS_LOG_INFO("MAC: " << addr << " ");
      NS_LOG_INFO( "IP: " << ipAddr << "\n");
    }
  }

  for (uint32_t i = 0; i < NodeList.GetN (); ++i)
    {
      Ptr<Node> node = NodeList.Get(i);
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


        ipIface->SetArpCache(arp);
        // ignore localhost interface...
        if (ipAddr == Ipv4Address("127.0.0.1")) {
          continue;
        }

      }
  }
}


//XXX Fix to play with Ipv4Linux
Ipv4Address
GetNodeIp(Ptr<Node> node)
{
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
    else {
      return ipAddr;
    }
  }

  return Ipv4Address("127.0.0.1");
}

// Function to create address string from numbers
//
char * toString(int a,int b, int c, int d)
{

	int first = a;
	int second = b;
	int third = c;
	int fourth = d;

	char *address =  new char[30];
	char firstOctet[30], secondOctet[30], thirdOctet[30], fourthOctet[30];
	//address = firstOctet.secondOctet.thirdOctet.fourthOctet;

	bzero(address,30);

	snprintf(firstOctet,10,"%d",first);
	strcat(firstOctet,".");
	snprintf(secondOctet,10,"%d",second);
	strcat(secondOctet,".");
	snprintf(thirdOctet,10,"%d",third);
	strcat(thirdOctet,".");
	snprintf(fourthOctet,10,"%d",fourth);


	strcat(thirdOctet,fourthOctet);
	strcat(secondOctet,thirdOctet);
	strcat(firstOctet,secondOctet);
	strcat(address,firstOctet);

	return address;
}

}
