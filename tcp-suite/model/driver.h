/*
  Remove unsolicited presence of Params in the comments.
  Better yet, remove all such comments.

  This code parses two files: tcpeval.conf and tcpeval.run. They are analogs of the *_def.tcl and vary_*.sh files in NS2 TCP eval suite.
  The config values are fed into objects of derived class of Params. They may be passed to other modules.
  The run values are such that one attribute, say bandwidth is varied each time. Changes are done to the Params objects as needed.
*/

#ifndef DRIVER_H
#define DRIVER_H

#include <sstream>
#include <cstdlib>

#include "evalstats.h"
#include "create-graph.h"

#include "ns3/core-module.h"
/*
  these headers aren't 'as such' neceesary for actual simulation. remove them and test.*/

#include "ns3/applications-module.h"
#include "ns3/internet-module.h"


namespace ns3 {

class Driver
{
  std::string type;  //represents the varying factor in the simulation.
  std::list<double> variables;
  std::list<std::string> TCPs;

  bool IsInteger (std::string value)
  {
    for (std::string::const_iterator sit = value.begin (); sit != value.end (); ++sit)
      {
        if (!std::isdigit (*sit))
          {
            return false;
          }
      }
    return true;
  }

  //the following method is necessary because std::to_string() seems too new to work(c++11)
  //and std::itoa isn't standard library, so.
  template <typename T>
  std::string to_string (const T& data)
  {
    std::ostringstream conv;
    conv << data;
    return conv.str ();
  }

  /*
    This function parses the file "tcpeval.conf" in scratch folder.
    Then it feeds the values it reads into Params objects.
  */
  void ParseConfigfile ()
  {
    std::ifstream file;
    std::string key, value;

    file.open ("scratch/tcpeval.conf", std::ios::in);
    if (!file.is_open ())
      {
        //TODO:Add log message
        std::exit (0);
      }
    //ensure PlotGraph, and other classes which have attributes are registered
    NS_OBJECT_ENSURE_REGISTERED (PlotGraph);
    while ((file >> key) && (key != "end"))
      {
        file >> value;
        std::cout << key << " " << value << std::endl;
        Config::SetDefault (key, StringValue (value));

      }
  }

  /*
    This method reads the tcpeval.run file.
    It feeds the type of variation into the string "type".
    It feeds the various values of the variation into the list "variables".
  */
  void ParseRunfile ()
  {
    std::ifstream file;
    std::string val;

    file.open ("scratch/tcpeval.run", std::ios::in);
    if (!file.is_open ())
      {
        //TODO: as usual, add log messages
        std::exit (0);
      }

    file >> val;
    type = val;
    if ((type != "BANDWIDTH")&&(type != "RTT")&&(type != "FTP"))
      {
        std::cout << "Please check manual(not yet done, I know) for usage details";
        std::exit (0);
      }
    file >> val;
    while (val != "end" )
      {
        variables.push_back (std::atof (val.c_str ()));
        file >> val;
      }

    file.close ();
  }

  TypeId getTcpTypeId (std::string name)
  {
    if (name.compare ("tahoe"))
      {
        return TcpTahoe::GetTypeId ();
      }
    else if (name.compare ("reno"))
      {
        return TcpReno::GetTypeId ();
      }
    else
      {
        return TcpReno::GetTypeId ();
      }
  }

public:
  Driver ()
  {

  }

  void SetTCPs (std::string tcp)
  {
    TCPs.push_back (tcp);
  }

  void Run ()
  {
    std::list<std::string>::iterator tcpIter;
    std::list<double>::iterator varIter;

    ParseRunfile ();
    ParseConfigfile ();

    for (tcpIter = TCPs.begin (); tcpIter != TCPs.end (); ++tcpIter)
      {
        for (varIter = variables.begin (); varIter != variables.end (); ++varIter)
          {
            //this is where we call the topology creator, traffic generator and graph installer. the next few/many lines are place_holders
            Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (getTcpTypeId (*tcpIter)));

            //SIMULATION code goes here

            std::string filename = "a-test";

            filename.append (std::string ("-") + *tcpIter);
            filename.append (std::string ("-") + type);
            filename.append (std::string ("-") + to_string<uint32_t> (*varIter));

            double btnkDelay = 0.1;
            double btnkBw;

            if (type == "BANDWIDTH")
              {
                btnkBw = (*varIter);
              }
            std::cout << btnkBw;
            PointToPointHelper routerToRouter;
            routerToRouter.SetDeviceAttribute ("DataRate", StringValue (to_string<double> (btnkBw) + std::string ("Mbps")));
            routerToRouter.SetChannelAttribute ("Delay", StringValue ((to_string<double> (btnkDelay)).append ("ms")));

            PointToPointHelper routerToNode;
            routerToNode.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
            routerToNode.SetChannelAttribute ("Delay", StringValue ("0.1ms"));

            PointToPointDumbbellHelper dumb (3, routerToNode, 3, routerToNode,
                                             routerToRouter);

            InternetStackHelper stack;
            dumb.InstallStack (stack);

            dumb.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.3.0", "255.255.255.0"),
                                      Ipv4AddressHelper ("10.2.4.0", "255.255.255.0"),
                                      Ipv4AddressHelper ("10.3.4.0", "255.255.255.0"));

            BulkSendHelper bulk ("ns3::TcpSocketFactory",
                                 InetSocketAddress (dumb.GetRightIpv4Address (1), 8));
            ApplicationContainer bulksrc = bulk.Install (dumb.GetLeft (1));
            bulk.SetAttribute ("MaxBytes", UintegerValue (0));

            PacketSinkHelper sink ("ns3::TcpSocketFactory",
                                   InetSocketAddress (dumb.GetRightIpv4Address (1), 8));
            ApplicationContainer bulkdest = sink.Install (dumb.GetRight (1));

            Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

            bulksrc.Start (Seconds (1.0));
            bulksrc.Stop (Seconds (7.0));

            //some old code ends here

            //install on all required middle nodes. PS*1
            Ptr<Node> left = dumb.GetLeft ();
            Ptr<LinkStats> linkstats = CreateObject<LinkStats>(btnkBw,filename);
            linkstats->Install (left);

            //install on node to check the TCP congestion window

            Ptr<Node> cwndNode = dumb.GetLeft (1);
            Ptr<SocketStats> socketstats = CreateObject<SocketStats>(filename);
            socketstats->Install (cwndNode);
            
            Ptr<Node> thruNode = dumb.GetRight(1); //dest node
            Ptr<SinkStats> sinkstats = CreateObject<SinkStats>(filename);
            sinkstats->Install (thruNode);

            Simulator::Stop (Seconds (10.0));
            Simulator::Run ();
            Simulator::Destroy ();

            //SIMULATION code ends here

          }
      }
      
    Ptr<PlotGraph> p1 = CreateObject<PlotGraph> ();
    for (tcpIter = TCPs.begin (); tcpIter != TCPs.end (); ++tcpIter)
      {
        for (varIter = variables.begin (); varIter != variables.end (); ++varIter)
          {
            //run another loop to iterate through the different nodes in parking lot. PS*1
            //and another for the different flows
            std::string filename = "a-test";

            filename.append (std::string ("-") + *tcpIter);
            filename.append (std::string ("-") + type);
            filename.append (std::string ("-") + to_string<uint32_t> (*varIter));
            p1->CreateGraphs (filename);
          }
      }
  }
};

} //namespace ns3
#endif
