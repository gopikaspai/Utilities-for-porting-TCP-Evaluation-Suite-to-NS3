#ifndef STATS_TCP_H
#define STATS_TCP_H

#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"

#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"

#include "create-graph.h"

namespace ns3 {

class Stats : public Object
{
protected:
  std::string filename;
  std::ofstream file;
public:
  Stats (std::string filename);
  //virtual ~Stats ();
};


class LinkStats : public Stats
{
private:
  uint32_t bytesOut;
  uint32_t bandwidth;
  uint32_t avgQueueLength;
  uint32_t nthSampleInInterval;

  Ptr<Ipv4L3Protocol> ipv4;
  Ptr<Queue> queue;
//TODO: Very Very Important: change to list or make vector scalable
  std::vector<uint32_t> listOfQueueLengths;

  std::string linkFileName, qlenFileName, percQlenFileName, droppedPacketsFileName; //files for link util, queue len, and percentile Q len
  std::ofstream linkFile,qlenFile,percQlenFile, droppedPacketsFile;

public:
  LinkStats (uint32_t bandwidth, std::string filename);

  ~LinkStats ();

  //dont go by names:this function does way more than reset things every second
  void SizeReset ();
  
//Throughput
//Throughput is calculated at network level. Change to Application level is so desired.

  void AggregateOverInterval (Ptr< const Packet > packet, Ptr< Ipv4 > ipv4, uint32_t interface);
 

//PS : Change the frequency it is being sampled. Use randomVariable to do it.
//Is it proper to poll at each enque event or must it be polled at some other occasion?
// Also Queue length in packets or bytes??

  void AggregateQueue (Ptr< const Packet> packet);

//the net device must also  be passed explicitly to allow parking lot topology
//if we are using then parking lot, then the node number must be also explicitly passed, since the file name must be set accordingly
//alternatively, when parking lot is implemented, convert the whole thing to number being passed all the time system.
  void Install ( Ptr<Node> node);

};

class SocketStats : public Stats
{
  Ptr<Socket> socket;
  std::string cwndFileName,rttFileName,seqNoFileName;
  std::ofstream cwndFile,rttFile,seqNoFile; 
 
public:

  SocketStats (std::string filename);

  ~SocketStats ();

//This method is called whenever the congestion window changes. Logs the new CWND value to file
  void CwndChange (uint32_t oldCwnd, uint32_t newCwnd);

  void RTTChange (Time oldRTT, Time newRTT);
  
  void SequenceNumberChange (SequenceNumber32 oldval, SequenceNumber32 newval);
  
//Since we cannot access the socket at compile time, because the socket is create dynamically by the application when it starts.
//So we have scheduled a call to the following function at the time the app starts.
//Right now the time is hard coded based on the start time set in the simulation.
//TODO: Find how to make the call to this function when the app starts, instead of a hardcoded value.
  void DynamicSocketFitter (Ptr<Node> cwndNode);
  
  //socket tracer is installed on the nodes having application
  void Install ( Ptr<Node> cwndNode);
};


class SinkStats : public Stats
{
  std::string thruFileName;
  std::ofstream thruFile;
  double prevThru,  throughput; 

public:
  
  SinkStats (std::string filename);

  ~SinkStats ();

//find the troughput at the TCP sink
  void ThroughputFinder ( Ptr<Node> node);


//throughput is metered at the packet sink side
  void Install ( Ptr<Node> thruNode);
 
};

} //namespcae
#endif
