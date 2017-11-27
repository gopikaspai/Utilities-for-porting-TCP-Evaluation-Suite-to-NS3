#ifndef CREATE_GRAPH_H
#define CREATE_GRAPH_H

#include "ns3/core-module.h"
#include "ns3/stats-module.h"

#include <fstream>

#include "ns3/boolean.h"
#include "ns3/type-id.h"

namespace ns3 {

/*Graph Creation Part*/

class PlotGraph : public Object
{
  bool m_createLinkUtilGraph;
  bool m_createQueueLengthGraph;
  bool m_createPercentileQueueGraph;
  bool m_createCwndGraph;
  bool m_createRttGraph;
  bool m_createSeqNoGraph;
  bool m_createThroughputGraph;
  bool m_createPacketDropGraph;

public:
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PlotGraph")
      .SetParent<Object> ()
      .SetGroupName ("TcpEvalSuite")
      .AddConstructor<PlotGraph> ()
      .AddAttribute ("CreateLinkUtilGraph",
                     "blah..........................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createLinkUtilGraph),
                     MakeBooleanChecker ())
      .AddAttribute ("CreateQueueLengthGraph",
                     "blah................................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createQueueLengthGraph),
                     MakeBooleanChecker ())
      .AddAttribute ("CreatePercentileQueueGraph",
                     "blah...............................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createPercentileQueueGraph),
                     MakeBooleanChecker ())
      .AddAttribute ("CreateCwndGraph",
                     "blah...............................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createCwndGraph),
                     MakeBooleanChecker ())
      .AddAttribute ("CreateRttGraph",
                     "blah...............................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createRttGraph),
                     MakeBooleanChecker ())
      .AddAttribute ("CreateSeqNoGraph",
                     "blah...............................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createSeqNoGraph),
                     MakeBooleanChecker ())
      .AddAttribute ("CreateThroughputGraph",
                     "blah...............................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createThroughputGraph),
                     MakeBooleanChecker ())
      .AddAttribute ("CreatePacketDropGraph",
                     "blah...............................",
                     BooleanValue (true),
                     MakeBooleanAccessor (&PlotGraph::m_createPacketDropGraph),
                     MakeBooleanChecker ())
      ;
    return tid;
  }

  PlotGraph ()
  {

  }

  enum Logarithm
  {
    NON_LOGARITHM_GRAPH, LOGARITHM_GRAPH
  };

  struct PlotDetails
  {
    PlotDetails ()
    {
      xAxis = std::string ("x-axis");
      yAxis = std::string ("y-axis");
      xLog = NON_LOGARITHM_GRAPH;
      yLog = NON_LOGARITHM_GRAPH;
    }

    std::string xAxis;
    std::string yAxis;
    Logarithm xLog;
    Logarithm yLog;
  };

  void CreatePlot (Gnuplot2dDataset dataset, std::string name, PlotDetails pd)
  {
    std::string plotFileName = name + ".plt";

    Gnuplot plot (name);
    plot.SetTitle (name);

    plot.SetTerminal ("png");
    plot.SetLegend (pd.xAxis.c_str (), pd.yAxis.c_str ());

    if (pd.xLog == LOGARITHM_GRAPH)
      {
        plot.AppendExtra ("set logscale x 10");
      }
    if (pd.yLog == LOGARITHM_GRAPH)
      {
        plot.AppendExtra ("set logscale y 10");
      }

    dataset.SetTitle (name);
    dataset.SetStyle (Gnuplot2dDataset::LINES_POINTS);

    plot.AddDataset (dataset);
    std::ofstream plotFile (plotFileName.c_str ());

    plot.GenerateOutput (plotFile);
    plotFile.close ();
  }

  void FeedData (Gnuplot2dDataset& dataset, std::string fileName, bool flattenTop = false)
  {
    static double oldX = 0;

    std::ifstream file;
    file.open (fileName.c_str (), std::ios::in);
    if (!file.is_open ())
      {
        std::exit (0);                 //add a error msg

      }
    std::string xVal, yVal;
    //uint32_t len;

    file >> xVal >> yVal;
    while (xVal != "end" )
      {
        //len = xVal.length ();
        //xVal = xVal.substr (0,len - 2);
        //flattening of graph only for link stats
        if (flattenTop) 
          {
            dataset.Add ((double) oldX ,std::atof (yVal.c_str ()));
            oldX = std::atof (xVal.c_str ());
          }
        dataset.Add (std::atof (xVal.c_str ()),std::atof (yVal.c_str ()));
        file >> xVal >> yVal;
      }
    file.close ();

  }

  void CreateGraphs (std::string fileName)
  {
    Gnuplot2dDataset dataset[10];
    PlotDetails pd[10];

    if (m_createLinkUtilGraph)
      {
        FeedData (dataset[0],fileName.c_str () + std::string ("_l.dat"), true); //flattop is true
        pd[0].xAxis = "time";
        pd[0].yAxis = "utilisation";
        CreatePlot (dataset[0],fileName.c_str () + std::string ("_l"),pd[0]);
      }
    if (m_createQueueLengthGraph)
      {
        FeedData (dataset[1],fileName.c_str () + std::string ("_q.dat"), true);
        pd[1].xAxis = "time";
        pd[1].yAxis = "queue length";
        CreatePlot (dataset[1],fileName.c_str () + std::string ("_q"),pd[1]);
      }
    if (m_createPercentileQueueGraph)
      {
        FeedData (dataset[2],fileName.c_str () + std::string ("_p.dat"), true);
        pd[2].xAxis = "time";
        pd[2].yAxis = "percentile queue length";
        CreatePlot (dataset[2],fileName.c_str () + std::string ("_p"),pd[2]);
      }
    if (m_createCwndGraph)
      {
        FeedData (dataset[3],fileName.c_str () + std::string ("_cw.dat"));  //flattop is false by default
        pd[3].xAxis = "time";
        pd[3].yAxis = "congestion window";
        //pd[3].yLog = LOGARITHM_GRAPH;
        CreatePlot (dataset[3],fileName.c_str () + std::string ("_cw"),pd[3]);
      }
    if (m_createRttGraph)
      {
        FeedData (dataset[4],fileName.c_str () + std::string ("_rtt.dat"));
        pd[4].xAxis = "time";
        pd[4].yAxis = "RTT in micro seconds";
        CreatePlot (dataset[4],fileName.c_str () + std::string ("_rtt"),pd[4]);
      }
    if (m_createSeqNoGraph)
      {
        FeedData (dataset[5],fileName.c_str () + std::string ("_sq.dat"));
        pd[5].xAxis = "time";
        pd[5].yAxis = "Sequence Number";
        CreatePlot (dataset[5],fileName.c_str () + std::string ("_sq"),pd[5]);
      }
    if (m_createThroughputGraph)
      {
        FeedData (dataset[6],fileName.c_str () + std::string ("_thr.dat"));
        pd[6].xAxis = "time";
        pd[6].yAxis = "ThroughPut";
        CreatePlot (dataset[6],fileName.c_str () + std::string ("_thr"),pd[6]);
      }
    if (m_createPacketDropGraph)
      {
        FeedData (dataset[7],fileName.c_str () + std::string ("_d.dat"));
        pd[7].xAxis = "time";
        pd[7].yAxis = "packet drop rate";
        CreatePlot (dataset[7],fileName.c_str () + std::string ("_d"),pd[7]);
      }
  }

};

} //namespace
#endif
