#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/global-route-manager.h"
#include "ns3/netanim-module.h"
#include "ns3/applications-module.h"

#include <cerrno>

#include "ns3/tmix.h"
#include "ns3/tmix-helper.h"
#include "ns3/delaybox-net-device.h"
#include "ns3/tmix-topology.h"
#include "ns3/tmix-ns2-style-trace-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TrafficTest");
std::string animFile = "traffic-test.xml";              //Name of the animation file

void
SetTmixPairOptions (TmixTopology::TmixNodePair &pair)
{
  pair.helper->SetLossless (true);
}

//Adds connection vectors to the pair passed as argument
void
ReadAndAddCvecs (TmixTopology::TmixNodePair& pair, std::istream& cvecfile)
{
  SetTmixPairOptions (pair);
  Tmix::ConnectionVector cvec;
  while (Tmix::ParseConnectionVector (cvecfile, cvec))
    {
      pair.helper->AddConnectionVector (cvec);
    }
}

/*  General function to create traffic.
                Takes as parameters the topology and the number of pairs to be created (currently for dumbbell topology)
*/
void
Traffic (Ptr<TmixTopology>& tmix, int nPairs, TmixTopology::InitiatorSide side, char* trafficType)
{

  std::ifstream cvecfile;
  char fileName[100] = "scratch/tmix-inbound.ns";
//	char fileName2[100]="scratch/tmix-inbound.ns";			//Uncomment if two way tmix traffic is needed

// Check for traffic type

  if (!(std::strcmp (trafficType,"Tmix")))
    {
// If tmix, use tmix-inbound.ns (Same as the one Provided by UNC for NS2)
      std::strcpy (fileName,"scratch/tmix-inbound.ns");
//		std::strcpy(fileName2,"scratch/tmix-inbound.ns");			//Uncomment if two way tmix traffic is needed
    }
  else if (!(std::strcmp (trafficType,"HTTP")))
    {
// If HTTP, use http-inbound.ns (Yet to be generated. NS3 does not have a HTTP Application yet; Need to figure out a way to generate http-inbound)
// Currently, we are using the same tmix-inbound.ns file until we find a way to generate cvecs for HTTP packets
      std::strcpy (fileName,"scratch/http-inbound.ns");
    }
  else if (!(std::strcmp (trafficType,"FTP")))
    {

// For FTP, we use Bulk Send Application over TCP, with Max Packet size set to 0 (Infinite Traffic). We install it on both sides, similar to how it was implemented in TCP Suite for NS2
      for (int i = 0; i < nPairs; i++)
        {
          TmixTopology::TmixNodePair pair = tmix->NewPair (side);

          uint16_t port1 = 40;
          BulkSendHelper bulkhelp ("ns3::TcpSocketFactory",InetSocketAddress (pair.acceptorAddress, port1));
          bulkhelp.SetAttribute ("MaxBytes", UintegerValue (0));
          ApplicationContainer bulkapp = bulkhelp.Install (pair.initiator);
          PacketSinkHelper packetsinkhelp ("ns3::TcpSocketFactory",InetSocketAddress (pair.acceptorAddress,port1));
          ApplicationContainer bulkdest = packetsinkhelp.Install (pair.acceptor);

          bulkapp.Start (Seconds (1.0));
          bulkapp.Stop (Seconds (3.0));

          uint16_t port2 = 41;
          BulkSendHelper bulkhelp2 ("ns3::TcpSocketFactory",InetSocketAddress (pair.initiatorAddress, port2));
          bulkhelp2.SetAttribute ("MaxBytes", UintegerValue (0));
          ApplicationContainer bulkapp2 = bulkhelp2.Install (pair.acceptor);
          PacketSinkHelper packetsinkhelp2 ("ns3::TcpSocketFactory",InetSocketAddress (pair.initiatorAddress,port2));
          ApplicationContainer bulkdest2 = packetsinkhelp2.Install (pair.initiator);

          bulkapp2.Start (Seconds (1.0));
          bulkapp2.Stop (Seconds (3.0));

        }
      return;
    }
  else if (!(std::strcmp (trafficType,"Voice")))
    {
// We use On Off application with the specifications as mentioned in the NS2 TCP Suite manual to generate voice traffic
// Needs to be called twice for voice traffic (once with RIGHT and once with LEFT)
      for (int i = 0; i < nPairs; i++)
        {
          TmixTopology::TmixNodePair pair = tmix->NewPair (side);
          uint16_t port = 40;
          OnOffHelper onoffhelp ("ns3::UdpSocketFactory",InetSocketAddress (pair.acceptorAddress, port));
          onoffhelp.SetAttribute ("PacketSize",UintegerValue (172));
          onoffhelp.SetAttribute ("DataRate", DataRateValue (DataRate ("80kb/s")));
          onoffhelp.SetAttribute ("OffTime",StringValue ("ns3::ConstantRandomVariable[Constant=1.35]"));
          ApplicationContainer onoffapp = onoffhelp.Install (pair.initiator);

          PacketSinkHelper packetSinkhelp ("ns3::UdpSocketFactory",InetSocketAddress (pair.acceptorAddress, port));
          ApplicationContainer onoffdest = packetSinkhelp.Install (pair.acceptor);

          onoffapp.Start (Seconds (1.0));
          onoffapp.Stop (Seconds (3.0));
        }
      return;
    }
  else if (!(std::strcmp (trafficType,"Streaming")))
    {
// We use On Off application with offTime set to 0. In essence, this mimics CBR Traffic
      for (int i = 0; i < nPairs; i++)
        {
          TmixTopology::TmixNodePair pair = tmix->NewPair (side);
          uint16_t port = 40;
          OnOffHelper onoffhelp ("ns3::UdpSocketFactory",InetSocketAddress (pair.acceptorAddress, port));
// onoffhelp.SetAttribute("PacketSize",UintegerValue(172));
// onoffhelp.SetAttribute("DataRate", DataRateValue (DataRate("80kb/s")));
          onoffhelp.SetAttribute ("OnTime",StringValue ("ns3::ConstantRandomVariable[Constant=10]"));
          onoffhelp.SetAttribute ("OffTime",StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
          ApplicationContainer onoffapp = onoffhelp.Install (pair.initiator);

          PacketSinkHelper packetSinkhelp ("ns3::UdpSocketFactory",InetSocketAddress (pair.acceptorAddress, port));
          ApplicationContainer onoffdest = packetSinkhelp.Install (pair.acceptor);

          onoffapp.Start (Seconds (1.0));
          onoffapp.Stop (Seconds (3.0));
        }
      return;
    }
  else
    {
      std::cout << "Error! Traffic type not found!" << std::endl;
      exit (1);
    }

// For HTTP and Tmix
  cvecfile.open (fileName);
  if (!cvecfile)
    {
      std::cerr << "Error opening " << fileName << ": " << strerror (errno) << std::endl;
      exit (1);
    }
  cvecfile.close ();

  for (int i = 0; i < nPairs; i++)
    {
      cvecfile.open (fileName);
      TmixTopology::TmixNodePair pair = tmix->NewPair (side);
      ReadAndAddCvecs (pair,cvecfile);
      cvecfile.close ();
    }

/* // Uncomment if two way tmix traffic is needed
        if(!std::strcmp(trafficType,"Tmix"))
  {
          cvecfile.open(fileName2);
          if (!cvecfile)
    {
          std::cerr << "Error opening "<< fileName2 <<": " << strerror(errno) <<std::endl;
          exit(1);
                }
    cvecfile.close();
        }

        if(!std::strcmp(trafficType,"Tmix"))
        {
                for(int i=0;i<nPairs;i++)
                {
                  cvecfile.open(fileName2);
                  TmixTopology::TmixNodePair pair = tmix->NewPair( side == TmixTopology::LEFT ? TmixTopology::RIGHT : TmixTopology::LEFT);
                  ReadAndAddCvecs(pair,cvecfile);
                  cvecfile.close();
                }
        }
*/
}


int
main (int argc, char** argv)
{
	
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  LogComponentEnable ("TrafficTest", LOG_LEVEL_WARN);
  LogComponentEnable ("Tmix", LOG_LEVEL_WARN);
  LogComponentEnable ("TmixHelper", LOG_LEVEL_WARN);
  LogComponentEnable ("TmixTopology", LOG_LEVEL_WARN);

  std::ifstream cvecfileA;
  std::ifstream cvecfileB;

  if (argc != 2)
    {
      std::cerr
      << "Usage: appname traffic-type[Tmix/HTTP/FTP/Voice/Streaming]\n" << std::endl;
      exit (1);
    }

  CommandLine cmd;
  cmd.Parse (argc, argv);
  InternetStackHelper internet;
// Creating the skeleton topology, with 2 routers
  Ptr<TmixTopology> tmix = Create<TmixTopology> (internet); 

// Create Traffic method
  Traffic (tmix,3,TmixTopology::LEFT,argv[1]);

// To generate trace file
  std::ostream& ns2StyleTraceFile = std::cout;
  Tmix::Ns2StyleTraceHelper ost (tmix, ns2StyleTraceFile);

// This generates additional nodes to balance number of nodes on LEFT and RIGHT sides, if not already balanced;
  ost.Install ();


  GlobalRouteManager::BuildGlobalRoutingDatabase ();
  GlobalRouteManager::InitializeRoutes ();


// Netanim code
  Ptr<Node> r1 = tmix->GetLeftRouter ();
  Ptr<Node> r2 = tmix->GetRightRouter ();

// Fetches vector contaning all nodes with initiator as LEFT side
  std::vector<TmixTopology::TmixNodePair> leftNodes = tmix->LeftPairs ();
  
  animFile = std::string(argv[1]) + std::string("Animation.xml");
  AnimationInterface anim (animFile);
  int pos = 1;

// Iterate throught the vector and assign positions to each node pair
  for (std::vector<TmixTopology::TmixNodePair>::const_iterator i = leftNodes.begin (); i != leftNodes.end (); ++i, ++pos)
    {
      anim.SetConstantPosition (i->initiator,0,20 + pos * 15);
      anim.SetConstantPosition (i->acceptor,175,20 + pos * 15);
    }

  pos = 1;
// Fetches vector contaning all nodes with initiator as RIGHT side
  std::vector<TmixTopology::TmixNodePair> rightNodes = tmix->RightPairs ();

// Iterate throught the vector and assign positions to each node pair
  for (std::vector<TmixTopology::TmixNodePair>::const_iterator i = rightNodes.begin (); i != rightNodes.end (); ++i, ++pos)
    {
      anim.SetConstantPosition (i->initiator,185,20 + pos * 15);
      anim.SetConstantPosition (i->acceptor,10,20 + pos * 15);
    }

//Set router Positions
  anim.SetConstantPosition (r1,50,50);
  anim.SetConstantPosition (r2,150,50);
  anim.SetStartTime (Seconds (0.25));

// Netanim code ends

  NS_LOG_INFO ("Starting simulation");
  Simulator::Stop (Seconds (10));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
