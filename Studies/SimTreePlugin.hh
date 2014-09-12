#ifndef SIMTREEANALYZER_HH
#define SIMTREEANALYZER_HH

#include "OctetAnalyzer.hh"
#include "AsymmetryPlugin.hh"
#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>

/// plugin with extra plots for simulated runs
class SimTreePlugin: public OctetAnalyzerPlugin {
public:
	/// constructor
	SimTreePlugin(OctetAnalyzer* OA);
        /// destructor
  ~SimTreePlugin() {SimOut->Print(); SimFile->Write(); SimOut->Delete();}
  /// make simulation tree
  void makeSimTree();
	/// fill from scan data point
	void fillSimTree(ProcessedDataScanner& PDS, double weight);
        virtual void fillCoreHists(ProcessedDataScanner& PDS, double weight);
	/// output plot generation
	//virtual void makePlots();
	

  //void printSimTree() { SimOut->Print(); }
  //Tree to hold simulated data which looks like real analyzed data
  TFile *SimFile;
  TTree *SimOut;
  TBranch *MWPCE, *MWPCW, *Erecon, *Type, *MWPCPosE, *MWPCPosW, *Event;
  Float_t mwpce, mwpcw, erecon;
  Float_t mwpcposE[2], mwpcposW[2];
  Int_t type;
};
#endif
