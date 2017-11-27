/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/tmix-helper.h"

#include "ns3/tmix-ns2-style-trace-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/global-route-manager.h"

using namespace ns3;

void AddCvecsToPairs(Ptr<TmixTopology> tmix, bool twosided, double portion, int num_pairs, int cvecs_per_pair)
{
  int i;
  std::vector<TmixTopology::TmixNodePair> leftNodes;
  std::vector<TmixTopology::TmixNodePair> rightNodes;

  Tmix::ConnectionVector cvec;

  Ptr <UniformRandomVariable> rnd=CreateObject<UniformRandomVariable>();

  char filename_in[] = "scratch/inbound.ns";
  char filename_out[] = "scratch/outbound.ns";

  std::ifstream cvecfile_in;
  std::ifstream cvecfile_out;

  cvecfile_in.open(filename_in);
  if(!cvecfile_in) std::exit(0);

  if(twosided)
  {
    cvecfile_out.open(filename_out);
    if(!cvecfile_out) std::exit(0);
  }
 
  for(i=0;i<num_pairs;i++)
  {
     leftNodes.push_back(tmix->NewPair(TmixTopology::LEFT));
  }
  if(twosided)
  {
    for(i=0;i<num_pairs;i++)
    {
     rightNodes.push_back(tmix->NewPair(TmixTopology::RIGHT));
    }
  }
 
  for(i=0;i<num_pairs*cvecs_per_pair;i+=num_pairs)
  {
    for(std::vector<TmixTopology::TmixNodePair>::const_iterator itr = leftNodes.begin(); itr != leftNodes.end(); ++itr)
      {
        if(Tmix::ParseConnectionVector(cvecfile_in, cvec) && (rnd->GetValue() < portion))
          (itr->helper)->AddConnectionVector(cvec);
      }
  }
  if(twosided)
  for(i=0;i<num_pairs*cvecs_per_pair;i+=num_pairs)
  {
    for(std::vector<TmixTopology::TmixNodePair>::const_iterator itr = rightNodes.begin(); itr != rightNodes.end(); ++itr)
      {
        if(Tmix::ParseConnectionVector(cvecfile_in, cvec) && (rnd->GetValue() < portion))
          (itr->helper)->AddConnectionVector(cvec);
      }
  }
}

int 
main (int argc, char *argv[])
{
  bool twosided = false;
  double cvec_portion = 1.0;
  int num_pairs = 3;
  int cvecs_per_pair = 100; 
  bool use_animation = true; 

  CommandLine cmd;

  cmd.AddValue ("two-sided", "Two sided traffic or one sided traffic", twosided);
  cmd.AddValue ("portion", "Portion of file to be used", cvec_portion);
  cmd.AddValue ("num-pairs", "Number of pairs to be used", num_pairs);
  cmd.AddValue ("cvecs-per-pair", "Number of Conn vectors per pair", cvecs_per_pair);
  cmd.AddValue ("animate", "Animation Required?", use_animation);
  cmd.Parse (argc,argv);

  char animFile[] = "tmix-animation.xml";

  InternetStackHelper internet;
  Ptr<TmixTopology> tmix = Create<TmixTopology> (internet);

  AddCvecsToPairs(tmix, twosided, cvec_portion, num_pairs,cvecs_per_pair);

  std::ostream& ns2StyleTraceFile = std::cout;
  Tmix::Ns2StyleTraceHelper ost(tmix, ns2StyleTraceFile);
  ost.Install();

  GlobalRouteManager::BuildGlobalRoutingDatabase();
  GlobalRouteManager::InitializeRoutes();

  AnimationInterface anim (animFile);
  
  if(use_animation==true)
  {
    Ptr<Node> r1=tmix->GetLeftRouter();
    Ptr<Node> r2=tmix->GetRightRouter();

    int pos=1;
    std::vector<TmixTopology::TmixNodePair> leftNodes = tmix->LeftPairs();

    for (std::vector<TmixTopology::TmixNodePair>::const_iterator i = leftNodes.begin(); i != leftNodes.end(); ++i, ++pos)
    {
      anim.SetConstantPosition(i->initiator,0,20+pos*15);
      anim.SetConstantPosition(i->acceptor,175,20+pos*15);
    }

    pos=1;
    std::vector<TmixTopology::TmixNodePair> rightNodes = tmix->RightPairs();
    for (std::vector<TmixTopology::TmixNodePair>::const_iterator i = rightNodes.begin(); i != rightNodes.end(); ++i, ++pos)
    {
      anim.SetConstantPosition(i->initiator,185,20+pos*15);
      anim.SetConstantPosition(i->acceptor,10,20+pos*15);
    }

    anim.SetConstantPosition(r1,50,50);
    anim.SetConstantPosition(r2,150,50);
    anim.SetStartTime(Seconds(0.25));
  }

  Simulator::Stop(Seconds(10));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

