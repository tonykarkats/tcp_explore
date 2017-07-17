/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "custom-switches-helper.h"

namespace ns3 {

void
  EcmpNodeContainer::Create (uint32_t n)
  {
    for (uint32_t i = 0; i < n; i++)
      {
        m_nodes.push_back (CreateObject<ECMPNode> ());
      }
  }

void
PathHashingNodeContainer::Create (uint32_t n)
{
  for (uint32_t i = 0; i < n; i++)
  {
    m_nodes.push_back (CreateObject<PathHashingNode> ());
  }
}

}
