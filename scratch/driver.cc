#include "ns3/driver.h"

using namespace ns3;

int main ()
{
  Driver r1;

  //Set all the available( or required) TCPs
  //Parameters are set in the files tcpeval.conf and tcpeval.run
  
  r1.SetTCPs ("tahoe");
  r1.SetTCPs ("reno");

  r1.Run ();
}

