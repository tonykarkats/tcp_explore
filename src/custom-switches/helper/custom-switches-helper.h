/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef CUSTOM_SWITCHES_HELPER_H
#define CUSTOM_SWITCHES_HELPER_H

#include "ns3/custom-switches.h"

namespace ns3 {

class EcmpNodeContainer : public NodeContainer {

  public:
     EcmpNodeContainer() : NodeContainer() {};
     void Create (uint32_t n);
};

class PathHashingNodeContainer : public NodeContainer {

  public:
     PathHashingNodeContainer() : NodeContainer() {};
     void Create (uint32_t n);
};

}

#endif /* CUSTOM_SWITCHES_HELPER_H */
