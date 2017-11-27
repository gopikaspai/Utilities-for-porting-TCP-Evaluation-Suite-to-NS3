#include "evalstats.h"

namespace ns3 {

  Stats::Stats (std::string filename)
  {
    this->filename = filename;
  }

  LinkStats::LinkStats (uint32_t bandwidth, std::string filename) : Stats (filename)
  {
    linkFileName.assign (filename);
    qlenFileName.assign (filename);
    percQlenFileName.assign (filename);
    droppedPacketsFileName.assign (filename);
    linkFileName.append ("_l.dat");
    qlenFileName.append ("_q.dat");
    percQlenFileName.append ("_p.dat");
    droppedPacketsFileName.append ("_d.dat");
    file.open (linkFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();
    file.open (qlenFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();
    file.open (percQlenFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();
    file.open (droppedPacketsFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();

    bytesOut = 0;
    avgQueueLength = 0;
    nthSampleInInterval = 0;
    listOfQueueLengths = std::vector<uint32_t> (200);
    this->bandwidth = bandwidth;
  }

  LinkStats::~LinkStats ()
  {
    linkFile << "end";
    linkFile.close ();
    qlenFile << "end";
    qlenFile.close ();
    percQlenFile << "end";
    percQlenFile.close ();
    droppedPacketsFile << "end";
    droppedPacketsFile.close ();
  }

  //dont go by names:this function does way more than reset things every second
  void LinkStats :: SizeReset ()
  {
    double utilization = (double) bytesOut * 8.0 / ( bandwidth * 1000 * 1000 ); //10^6 bandwidth
    linkFile << (Simulator::Now ().GetSeconds ())  << "  " << utilization << std::endl;
    bytesOut = 0;

    qlenFile << (Simulator::Now ().GetSeconds ()) << " " << ((nthSampleInInterval == 0) ? 0 : (avgQueueLength / nthSampleInInterval));
    qlenFile << std::endl;
    avgQueueLength = 0;
    nthSampleInInterval = 0;

    std::sort (listOfQueueLengths.begin (),listOfQueueLengths.end ());
    int length = listOfQueueLengths.size ();
    length = (int) (length * 95) / 100;
    percQlenFile << (Simulator::Now ().GetSeconds ()) << " " << ((length == 0) ? 0 : listOfQueueLengths.at (length));
    percQlenFile << std::endl;
    listOfQueueLengths.clear ();
    
    uint32_t totalPackets = queue -> GetTotalReceivedPackets();
    uint32_t droppedPackets = queue -> GetTotalDroppedPackets();
    droppedPacketsFile << (Simulator::Now ().GetSeconds ())  << "  " ;
    droppedPacketsFile << ((droppedPackets == 0) ? 0 : ( 100.0 * (double)droppedPackets / (double)totalPackets) )<<std::endl;

  }

//Throughput
//Throughput is calculated at network level. Change to Application level is so desired.

  void LinkStats :: AggregateOverInterval (Ptr< const Packet > packet, Ptr< Ipv4 > ipv4, uint32_t interface)
  {
    bytesOut += packet->GetSize ();
  }

//PS : Change the frequency it is being sampled. Use randomVariable to do it.
//Is it proper to poll at each enque event or must it be polled at some other occasion?
// Also Queue length in packets or bytes??

  void LinkStats :: AggregateQueue (Ptr< const Packet> packet)
  {
    avgQueueLength += queue->GetNPackets ();
    nthSampleInInterval++;

    listOfQueueLengths.push_back (queue->GetNPackets ());
  }

//the net device must also  be passed explicitly to allow parking lot topology
//if we are using then parking lot, then the node number must be also explicitly passed, since the file name must be set accordingly
//alternatively, when parking lot is implemented, convert the whole thing to number being passed all the time system.
  void LinkStats :: Install ( Ptr<Node> node)
  {
    ipv4 = node->GetObject<Ipv4L3Protocol> ();
    queue = node->GetDevice (0)->GetObject<PointToPointNetDevice> ()->GetQueue ();

    ipv4->TraceConnectWithoutContext ("Tx", MakeCallback (&LinkStats::AggregateOverInterval, this));
    queue->TraceConnectWithoutContext ("Enqueue", MakeCallback (&LinkStats::AggregateQueue, this));
    
    //*********
    UintegerValue limit;
    queue->GetAttribute("MaxPackets", limit);
    std::cout<<"pp"<<limit.Get()<<"pp";
    for (int i = 1; i <= 10; i++)
      {
        Simulator::Schedule (Seconds (i), &LinkStats::SizeReset,this);
      }

    linkFile.open (linkFileName.c_str (), std::ios_base::app);
    qlenFile.open (qlenFileName.c_str (), std::ios_base::app);
    percQlenFile.open (percQlenFileName.c_str (), std::ios_base::app);
    droppedPacketsFile.open (droppedPacketsFileName.c_str (), std::ios_base::app);
  }


  SocketStats::SocketStats (std::string filename) : Stats (filename)
  {
    cwndFileName.assign (filename);
    rttFileName.assign (filename);
    seqNoFileName.assign (filename);
    cwndFileName.append ("_cw.dat");
    rttFileName.append ("_rtt.dat");
    seqNoFileName.append ("_sq.dat");
    file.open (cwndFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();
    file.open (rttFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();
    file.open (seqNoFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();
  }

  SocketStats::~SocketStats ()
  {
    cwndFile << "end";
    cwndFile.close ();
    rttFile << "end";
    rttFile.close ();
    seqNoFile << "end";
    seqNoFile.close ();
  }

//This method is called whenever the congestion window changes. Logs the new CWND value to file
  void SocketStats::CwndChange (uint32_t oldCwnd, uint32_t newCwnd)
  {
    cwndFile << Simulator::Now ().GetSeconds () << " " << newCwnd << "\n";
  }

  void SocketStats:: RTTChange (Time oldRTT, Time newRTT)
  {
    rttFile << Simulator::Now ().GetSeconds () << " " << (newRTT / 100000) << "\n";
  }

  void SocketStats:: SequenceNumberChange (SequenceNumber32 oldval, SequenceNumber32 newval)
  {
    seqNoFile << Simulator::Now ().GetSeconds () << " " << newval << "\n";
  }

//Since we cannot access the socket at compile time, because the socket is create dynamically by the application when it starts.
//So we have scheduled a call to the following function at the time the app starts.
//Right now the time is hard coded based on the start time set in the simulation.
//TODO: Find how to make the call to this function when the app starts, instead of a hardcoded value.
  void SocketStats:: DynamicSocketFitter (Ptr<Node> cwndNode)
  {
    //use either dynamic cast or GetObject<>() to downcast to derived class
    Ptr<Application> application;
    application = cwndNode->GetApplication (0);

    if (application->GetInstanceTypeId () == TypeId::LookupByName ("ns3::BulkSendApplication"))
      {
        socket = application->GetObject<BulkSendApplication> ()->GetSocket ();
      }
    else if (application->GetInstanceTypeId () == TypeId::LookupByName ("ns3::OnOffApplication"))
      {
        socket = application->GetObject<OnOffApplication> ()->GetSocket ();
      }

    if (socket)
      {
        socket->TraceConnectWithoutContext ("CongestionWindow", MakeCallback (&SocketStats::CwndChange, this));
        socket->TraceConnectWithoutContext ("RTT", MakeCallback (&SocketStats::RTTChange, this));
        //possible options are NextTxSequence, HighestSequence, HighestRxSequence, HighestRxAck
        //choose your destiny
        socket->TraceConnectWithoutContext ("NextTxSequence", MakeCallback (&SocketStats::SequenceNumberChange, this));

      }
  }

  //socket tracer is installed on the nodes having application
  void SocketStats:: Install ( Ptr<Node> cwndNode)
  {
    //TODO:if start time of app is made as an attribute, it can be accessed using the ns3 config system
    Simulator::Schedule (Seconds (1.000001), &SocketStats::DynamicSocketFitter,this, cwndNode); //app start time, ie., socket creation time

    cwndFile.open (cwndFileName.c_str (), std::ios_base::app);
    rttFile.open (rttFileName.c_str (), std::ios_base::app);
    seqNoFile.open (seqNoFileName.c_str (), std::ios_base::app);
  }





  
  SinkStats::SinkStats (std::string filename) : Stats (filename)
  {
    thruFileName.assign (filename);
    thruFileName.append ("_thr.dat");
    file.open (thruFileName.c_str (), std::ios::out);
    file.flush ();
    file.close ();
    
    prevThru = 0;

  }

  SinkStats::~SinkStats ()
  {
    thruFile << "end";
    thruFile.close ();
  }

//find the troughput at the TCP sink
  void SinkStats:: ThroughputFinder ( Ptr<Node> node)
  {
    Ptr<PacketSink> sink = node->GetApplication (0)->GetObject<PacketSink> ();
    throughput = sink->GetTotalRx ();
    thruFile << Simulator::Now ().GetSeconds () << " " << ( throughput - prevThru) * 8 / 1000000.0 << std::endl;
    prevThru = throughput;
  }

//throughput is metered at the packet sink side
  void SinkStats:: Install ( Ptr<Node> thruNode)
  {
    for (int i = 1; i <= 10; i++)
      {
        Simulator::Schedule (Seconds (i), &SinkStats::ThroughputFinder, this, thruNode);
      }

    thruFile.open (thruFileName.c_str (), std::ios_base::app);
  }
 

} //namespcae

