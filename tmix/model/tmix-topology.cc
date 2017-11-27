/* -*- Mode:C++; c-file-style:''gnu''; indent-tabs-mode:nil; -*- */

/* 
 * Copyright 2012, Old Dominion University
 * Copyright 2012, University of North Carolina at Chapel Hill
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution.
 *    3. The name of the author may not be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * M.C. Weigle, P. Adurthi, F. Hernandez-Campos, K. Jeffay, and F.D. Smith, 
 * Tmix: A Tool for Generating Realistic Application Workloads in ns-2, 
 * ACM Computer Communication Review, July 2006, Vol 36, No 3, pp. 67-76.
 *
 * Contact: Michele Weigle (mweigle@cs.odu.edu)
 * http://www.cs.odu.edu/inets/Tmix
 */

#include "tmix-topology.h"
#include "ns3/node-container.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"
#include "ns3/ipv4-global-routing-helper.h"

NS_LOG_COMPONENT_DEFINE("TmixTopology");

namespace ns3
{

/**
 * Returns the Ipv4 address assigned to the tmix node's net device.
 *
 * Out parameters: tmixDevice, routerDevice, address
 */
void
ConnectNodeToRouter(Ptr<Node> routerNode, Ptr<Node> tmixNode, Ptr<
    PointToPointNetDevice>& tmixDevice,
    Ptr<PointToPointNetDevice>& routerDevice, Ipv4AddressHelper &addresses,
    Ipv4Address& address)
{
  Ptr<PointToPointChannel> channel = CreateObject<PointToPointChannel> ();

  routerDevice = CreateObject<PointToPointNetDevice> ();
  routerDevice->SetAddress(Mac48Address::Allocate());
  routerDevice->Attach(channel);
  routerNode->AddDevice(routerDevice);
  addresses.Assign(NetDeviceContainer(routerDevice));

  tmixDevice = CreateObject<PointToPointNetDevice> ();
  tmixDevice->SetAddress(Mac48Address::Allocate());
  tmixDevice->Attach(channel);
  tmixNode->AddDevice(tmixDevice);
  address = addresses.Assign(NetDeviceContainer(tmixDevice)).GetAddress(0);

  // Increment the network number for the next node
  addresses.NewNetwork();

  // Set additional topology delays, unrelated to DelayBox/Tmix
  // TODO: make these not hard-coded
  channel->SetAttribute("Delay", TimeValue(MicroSeconds(100)));

  tmixDevice->SetAttribute("DataRate", DataRateValue(DataRate("1000Mbps")));
    {
      Ptr<DropTailQueue> queue = CreateObject<DropTailQueue> ();
      queue->SetMode(Queue::QUEUE_MODE_PACKETS);
      queue->SetAttribute("MaxPackets", UintegerValue(200));
      tmixDevice->SetQueue(queue);
    }
  routerDevice->SetAttribute("DataRate", DataRateValue(DataRate("1000Mbps")));
    {
      Ptr<DropTailQueue> queue = CreateObject<DropTailQueue> ();
      queue->SetMode(Queue::QUEUE_MODE_PACKETS);
      queue->SetAttribute("MaxPackets", UintegerValue(200));
      routerDevice->SetQueue(queue);
    }
}

/**
 * Out parameters: device, routerAddress
 */
void
ConnectRouter(Ptr<Node> router, Ptr<PointToPointChannel> channel,
    Ptr<DelayBox> delayBox, Ptr<DelayBoxPointToPointNetDevice>& device,
    Ipv4AddressHelper& addresses, Ipv4Address& routerAddress)
{
  device = CreateObject<DelayBoxPointToPointNetDevice> ();
  device->SetAddress(Mac48Address::Allocate());
  device->SetDelayBox(delayBox);
  device->Attach(channel);
  router->AddDevice(device);
  routerAddress = addresses.Assign(NetDeviceContainer(device)).GetAddress(0);

  // Set additional topology delays, unrelated to DelayBox
  // This is to more precisely duplicate the behavior of ns-2 Tmix
  // TODO: make these not hard-coded
  device->SetAttribute("DataRate", DataRateValue(DataRate("1000Mbps")));
    {
      Ptr<DropTailQueue> queue = CreateObject<DropTailQueue> ();
      queue->SetMode(Queue::QUEUE_MODE_PACKETS);
      queue->SetAttribute("MaxPackets", UintegerValue(200));
      device->SetQueue(queue);
    }
}

TmixTopology::TmixNodePair&
TmixTopology::NewPair(InitiatorSide side)
{
  TmixNodePair pair;

  pair.initiator = CreateObject<Node> ();
  pair.acceptor = CreateObject<Node> ();

  m_internet.Install(pair.initiator);
  m_internet.Install(pair.acceptor);

  ConnectNodeToRouter((side == LEFT)?m_leftRouter:m_rightRouter, pair.initiator, pair.initiatorDevice,
      pair.routerInitiatorDevice, (side == LEFT) ? m_leftAddresses
          : m_rightAddresses, pair.initiatorAddress);
  NS_LOG_LOGIC(pair.initiatorAddress << " assigned to INITIATOR");

  ConnectNodeToRouter((side == RIGHT)?m_leftRouter:m_rightRouter, pair.acceptor, pair.acceptorDevice,
      pair.routerAcceptorDevice, (side == RIGHT) ? m_leftAddresses
          : m_rightAddresses, pair.acceptorAddress);
  NS_LOG_LOGIC(pair.acceptorAddress << " assigned to ACCEPTOR");

  if (side == LEFT)
    {
      m_nodeTypeOfAddress[pair.initiatorAddress] = LEFT_INITIATOR;
      m_nodeTypeOfAddress[pair.acceptorAddress] = RIGHT_ACCEPTOR;
    }
  else
    {
      m_nodeTypeOfAddress[pair.initiatorAddress] = RIGHT_INITIATOR;
      m_nodeTypeOfAddress[pair.acceptorAddress] = LEFT_ACCEPTOR;
    }

  pair.helper = Create<TmixHelper> (m_delayBox, pair.initiator,
      pair.initiatorAddress, pair.acceptor, pair.acceptorAddress);

  if (side == LEFT)
    {
      m_leftPairs.push_back(pair);
      return m_leftPairs.back();
    }
  else
    {
      m_rightPairs.push_back(pair);
      return m_rightPairs.back();
    }
}

TmixTopology::NodeType
TmixTopology::NodeTypeByAddress(Ipv4Address address)
{
  std::map<Ipv4Address, NodeType>::const_iterator i = m_nodeTypeOfAddress.find(
      address);
  if (i == m_nodeTypeOfAddress.end())
    {
      return INVALID;
    }
  else
    {
      return i->second;
    }
}

TmixTopology::TmixTopology(const InternetStackHelper& internet) :
  m_internet(internet)
{
  m_centerChannel = CreateObject<PointToPointChannel> ();

  // Set additional topology delays, unrelated to DelayBox
  // This is to more precisely duplicate the behavior of ns-2 Tmix
  // TODO: make these not hard-coded
  m_centerChannel->SetAttribute("Delay", TimeValue(MicroSeconds(100)));

  m_delayBox = CreateObject<DelayBox> ();

  m_leftRouter = CreateObject<Node> ();
  m_rightRouter = CreateObject<Node> ();
  m_internet.Install(m_leftRouter);
  m_internet.Install(m_rightRouter);

  m_leftAddresses.SetBase("10.1.0.0", "255.255.255.252");
  m_rightAddresses.SetBase("10.2.0.0", "255.255.255.252");

  Ipv4AddressHelper routerAddresses;
  routerAddresses.SetBase("10.0.0.0", "255.255.255.252");
  ConnectRouter(m_leftRouter, m_centerChannel, m_delayBox, m_leftRouterDevice,
      routerAddresses, m_leftRouterAddress);
  NS_LOG_LOGIC(m_leftRouterAddress << " assigned to LEFT_ROUTER");
  ConnectRouter(m_rightRouter, m_centerChannel, m_delayBox,
      m_rightRouterDevice, routerAddresses, m_rightRouterAddress);
  NS_LOG_LOGIC(m_rightRouterAddress << " assigned to RIGHT_ROUTER");

  m_nodeTypeOfAddress[m_leftRouterAddress] = LEFT_ROUTER;
  m_nodeTypeOfAddress[m_rightRouterAddress] = RIGHT_ROUTER;
}

}
