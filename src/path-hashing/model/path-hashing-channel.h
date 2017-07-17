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
#ifndef PATH_HASHING_CHANNEL_H
#define PATH_HASHING_CHANNEL_H

#include "ns3/net-device.h"
#include "ns3/channel.h"
#include <vector>

namespace ns3 {

/**
 * \ingroup bridge
 *
 * \brief Virtual channel implementation for bridges (BridgeNetDevice).
 *
 * Just like BridgeNetDevice aggregates multiple NetDevices,
 * PathHashingChannel aggregates multiple channels and make them appear as
 * a single channel to upper layers.
 */
class PathHashingChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  PathHashingChannel ();
  virtual ~PathHashingChannel ();

  /**
   * Adds a channel to the bridged pool
   * \param bridgedChannel  the channel to add to the pool
   */
  void AddChannel (Ptr<Channel> bridgedChannel);

  // virtual methods implementation, from Channel
  virtual uint32_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

private:

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  PathHashingChannel (const PathHashingChannel &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  PathHashingChannel &operator = (const PathHashingChannel &);

  std::vector< Ptr<Channel> > m_bridgedChannels; //!< pool of bridged channels

};

} // namespace ns3

#endif /* PATH_HASHING_CHANNEL_H */