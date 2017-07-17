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

#include "ns3/log.h"
#include "ecmp-channel.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EcmpChannel");

NS_OBJECT_ENSURE_REGISTERED (EcmpChannel);

TypeId
EcmpChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EcmpChannel")
    .SetParent<Channel> ()
    .SetGroupName("Bridge")
    .AddConstructor<EcmpChannel> ()
  ;
  return tid;
}

EcmpChannel::EcmpChannel ()
  : Channel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

EcmpChannel::~EcmpChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (std::vector< Ptr<Channel> >::iterator iter = m_bridgedChannels.begin ();
       iter != m_bridgedChannels.end (); iter++)
    {
      *iter = 0;
    }
  m_bridgedChannels.clear ();
}


void
EcmpChannel::AddChannel (Ptr<Channel> bridgedChannel)
{
  m_bridgedChannels.push_back (bridgedChannel);
}

uint32_t
EcmpChannel::GetNDevices (void) const
{
  uint32_t ndevices = 0;
  for (std::vector< Ptr<Channel> >::const_iterator iter = m_bridgedChannels.begin ();
       iter != m_bridgedChannels.end (); iter++)
    {
      ndevices += (*iter)->GetNDevices ();
    }
  return ndevices;
}


Ptr<NetDevice>
EcmpChannel::GetDevice (uint32_t i) const
{
  uint32_t ndevices = 0;
  for (std::vector< Ptr<Channel> >::const_iterator iter = m_bridgedChannels.begin ();
       iter != m_bridgedChannels.end (); iter++)
    {
      if ((i - ndevices) < (*iter)->GetNDevices ())
        {
          return (*iter)->GetDevice (i - ndevices);
        }
      ndevices += (*iter)->GetNDevices ();
    }
  return NULL;
}


} // namespace ns3
