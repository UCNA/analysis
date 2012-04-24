#include "AsymmetryAnalyzer.hh"
#include "BetaSpectrum.hh"
#include "KurieFitter.hh"
#include "GraphicsUtils.hh"
#include <TH2F.h>

std::string AsymmetryAnalyzer::processedLocation = "";	// set this later depending on situtation

/// nominal asymmetry for fit
Double_t asymmetryFitFunc(const Double_t* x, const Double_t* par) {
	return par[0]*plainAsymmetry(x[0],0.5)/A0_PDG;
}

TF1 AsymmetryAnalyzer::asymmetryFit = TF1("asymFit",&asymmetryFitFunc,0,neutronBetaEp,1);
TF1 AsymmetryAnalyzer::averagerFit = TF1("averagerFir","pol0",0,neutronBetaEp);
AnalysisChoice  AsymmetryAnalyzer::anChoice = ANCHOICE_A;

AsymmetryAnalyzer::AsymmetryAnalyzer(OutputManager* pnt, const std::string& nm, const std::string& inflname): OctetAnalyzer(pnt,nm,inflname) {
	for(Side s = EAST; s <= WEST; ++s) {
		qAnodeCal[s] = registerCoreHist("AnodeCal","Anode Calibration Events",50, 0, 8, s, &hAnodeCal[s]);
		
		for(unsigned int t=TYPE_0_EVENT; t<=TYPE_IV_EVENT; t++) {
			for(unsigned int p=0; p<=nBetaTubes; p++) {
				qEnergySpectra[s][p][t] = registerCoreHist(std::string("hEnergy_")+(p<nBetaTubes?itos(p)+"_":"")+"Type_"+itos(t),
														   std::string("Type ")+itos(t)+" Events Energy",
														   100, 0, 1000, s, &hEnergySpectra[s][p][t]);
			}
			if(t>TYPE_III_EVENT) continue;
			TH2F* hPositionsTemplate = new TH2F((std::string("hPos_Type_")+itos(t)).c_str(),
												(std::string("Type ")+itos(t)+" Positions").c_str(),
												200,-65,65,200,-65,65);
			qPositions[s][t] = registerCoreHist(*hPositionsTemplate,s,(TH1**)&hPositions[s][t]);
			delete(hPositionsTemplate);
		}
	}
}

void AsymmetryAnalyzer::fillCoreHists(ProcessedDataScanner& PDS, double weight) {
	Side s = PDS.fSide;
	if(!(s==EAST || s==WEST)) return;
	if(PDS.fPID != PID_BETA) return;
	if(PDS.passesPositionCut(s) && PDS.fType == TYPE_0_EVENT && PDS.getEtrue()>225)
		hAnodeCal[s]->Fill(PDS.mwpcEnergy[s]/PDS.ActiveCal->wirechamberGainCorr(s,PDS.runClock.t[BOTH]),weight);
	if(PDS.passesPositionCut(s) && PDS.fType <= TYPE_IV_EVENT) {
		hEnergySpectra[s][nBetaTubes][PDS.fType]->Fill(PDS.getEtrue(),weight);
		for(unsigned int t=0; t<nBetaTubes; t++)
			hEnergySpectra[s][t][PDS.fType]->Fill(PDS.scints[s].tuben[t].x,weight);
	}
	if(PDS.fType <= TYPE_III_EVENT)
		hPositions[s][PDS.fType]->Fill(PDS.wires[s][X_DIRECTION].center,PDS.wires[s][Y_DIRECTION].center,weight);
}

void AsymmetryAnalyzer::fitAsym(float fmin, float fmax, unsigned int color, bool avg) {
	Stringmap m;
	TF1* fitter = avg?&averagerFit:&asymmetryFit;
	fitter->SetParameter(0,A0_PDG);
	fitter->SetLineColor(color);
	hAsym->Fit(fitter,"Q+","",fmin,fmax);
	m.insert("A0_fit",fitter->GetParameter(0));
	m.insert("dA0",fitter->GetParError(0));
	m.insert("A0_chi2",fitter->GetChisquare());
	m.insert("A0_NDF",fitter->GetNDF());
	m.insert("fitMin",fmin);
	m.insert("fitMax",fmax);
	m.insert("anChoice",itos(anChoice));
	m.insert("method",avg?"average":"fit");
	qOut.insert("asymmetry",m);
}

void AsymmetryAnalyzer::endpointFits() {
	const float fitStart = 250;
	const float fitEnd = 750;	
	for(Side s = EAST; s <= WEST; ++s) {		
		for(AFPState afp = AFP_OFF; afp <= AFP_ON; ++afp) {
			for(unsigned int t=0; t<=nBetaTubes; t++) {
				float_err ep = kurieIterator((TH1F*)qEnergySpectra[s][t][TYPE_0_EVENT].fgbg[afp].h[1],
											 800., NULL, neutronBetaEp, fitStart, fitEnd);
				Stringmap m;
				m.insert("fitStart",fitStart);
				m.insert("fitEnd",fitEnd);
				m.insert("afp",afp);
				m.insert("side",ctos(sideNames(s)));
				m.insert("tube",t);
				m.insert("type",TYPE_0_EVENT);
				m.insert("endpoint",ep.x);
				m.insert("dendpoint",ep.err);
				m.insert("counts",qEnergySpectra[s][t][TYPE_0_EVENT].fgbg[afp].h[1]->Integral());
				m.display("--- ");
				qOut.insert("kurieFit",m);
			}
		}
	}
}

void AsymmetryAnalyzer::anodeCalFits() {
	for(Side s = EAST; s <= WEST; ++s) {		
		for(AFPState afp = AFP_OFF; afp <= AFP_ON; ++afp) {
			TF1 fLandau("landauFit","landau",0,15);
			fLandau.SetLineColor(2+2*s);
			int fiterr = qAnodeCal[s].fgbg[afp].h[1]->Fit(&fLandau,"Q");
			Stringmap m;
			m.insert("afp",afp);
			m.insert("side",ctos(sideNames(s)));
			m.insert("fiterr",itos(fiterr));
			m.insert("height",fLandau.GetParameter(0));
			m.insert("d_height",fLandau.GetParError(0));
			m.insert("mpv",fLandau.GetParameter(1));
			m.insert("d_mpv",fLandau.GetParError(1));
			m.insert("sigma",fLandau.GetParameter(2));
			m.insert("d_sigma",fLandau.GetParError(2));
			qOut.insert("anodeCalFit",m);
		}
	}	
}

void AsymmetryAnalyzer::calculateResults() {
	// build total spectra based on analysis choice
	quadHists qTotalSpectrum[2];
	for(Side s = EAST; s <= WEST; ++s) {
		qTotalSpectrum[s] = cloneQuadHist(qEnergySpectra[s][nBetaTubes][TYPE_0_EVENT], "hTotalEvents", "All Events Energy");
		if(!(anChoice == ANCHOICE_A || anChoice == ANCHOICE_B || anChoice == ANCHOICE_C || anChoice == ANCHOICE_D))
			qTotalSpectrum[s] *= 0;	// analysis choices without Type 0 events
		if(anChoice == ANCHOICE_A || anChoice == ANCHOICE_B || anChoice == ANCHOICE_C)
			qTotalSpectrum[s] += qEnergySpectra[s][nBetaTubes][TYPE_I_EVENT];
		if(anChoice == ANCHOICE_A || anChoice == ANCHOICE_C)
			qTotalSpectrum[s] += qEnergySpectra[s][nBetaTubes][TYPE_II_EVENT];
		if(anChoice == ANCHOICE_A || anChoice == ANCHOICE_C)
			qTotalSpectrum[s] += qEnergySpectra[s][nBetaTubes][TYPE_III_EVENT];
	}
	// calculate SR and SS
	hAsym = (TH1F*)calculateSR("Total_Events_SR",qTotalSpectrum[EAST],qTotalSpectrum[WEST]);
	hSuperSum = (TH1F*)calculateSuperSum("Total_Events_SuperSum",qTotalSpectrum[EAST],qTotalSpectrum[WEST]);
	for(EventType tp = TYPE_0_EVENT; tp <= TYPE_II_EVENT; ++tp)
		hEvtSS[tp] = (TH1F*)calculateSuperSum(std::string("SuperSum_Type_")+itos(tp),
											  qEnergySpectra[EAST][nBetaTubes][tp],
											  qEnergySpectra[WEST][nBetaTubes][tp]);
	// perform data fits
	fitAsym(200,675,1,true);	// match Robby's analysis
	fitAsym(50,800,7);
	fitAsym(225,675,6);
	endpointFits();
	anodeCalFits();
	makeRatesSummary();
}

void AsymmetryAnalyzer::makePlots() {
	hAsym->SetMinimum(-0.10);
	hAsym->SetMaximum(0.0);
	hAsym->Draw();
	printCanvas("Asymmetry");
	
	hSuperSum->Draw();
	printCanvas("SuperSum");
	
	drawQuadSides(qAnodeCal[EAST], qAnodeCal[WEST], true, "AnodeCal");
	
	for(unsigned int t=TYPE_0_EVENT; t<=TYPE_II_EVENT; t++)
		drawQuadSides(qEnergySpectra[EAST][nBetaTubes][t], qEnergySpectra[WEST][nBetaTubes][t], true, "Energy");
	
	// positions
	if(depth <= 0) {
		for(Side s = EAST; s <= WEST; ++s) {
			for(unsigned int t=TYPE_0_EVENT; t<=TYPE_II_EVENT; t++) {
				qPositions[s][t].setDrawMinimum(0);
				drawQuad(qPositions[s][t],"Positions","COL");
			}
		}
	}
}

void AsymmetryAnalyzer::compareMCtoData(RunAccumulator& OAdata, float simfactor) {
	// re-cast to correct type
	AsymmetryAnalyzer& dat = (AsymmetryAnalyzer&)OAdata;
	
	hAsym->SetLineColor(4);
	dat.hAsym->SetLineColor(2);
	hAsym->Draw("HIST E1");
	dat.hAsym->Draw("SAME HIST E1");
	printCanvas("DataComparison/Asymmetry");
	
	hSuperSum->Scale(1.0/simfactor);
	drawHistoPair(dat.hSuperSum,hSuperSum);
	printCanvas("DataComparison/SuperSum");
	for(EventType tp = TYPE_0_EVENT; tp <= TYPE_II_EVENT; ++tp) {
		hEvtSS[tp]->Scale(1.0/simfactor);
		drawHistoPair(dat.hEvtSS[tp],hEvtSS[tp]);
		printCanvas(std::string("DataComparison/SuperSum_Type_")+itos(tp));
	}
	
	for(unsigned int t=TYPE_0_EVENT; t<=TYPE_II_EVENT; t++) {
		std::vector<TH1*> hToPlot;
		for(Side s = EAST; s <= WEST; ++s) {		
			for(AFPState afp = AFP_OFF; afp <= AFP_ON; ++afp) {
				qEnergySpectra[s][nBetaTubes][t].fgbg[afp].h[true]->SetMarkerColor(2+2*s);
				qEnergySpectra[s][nBetaTubes][t].fgbg[afp].h[true]->SetMarkerStyle(22+4*afp);
				//qEnergySpectra[s][nBetaTubes][t].fgbg[afp].h[true]->Scale(1.0/simfactor);
				hToPlot.push_back(qEnergySpectra[s][nBetaTubes][t].fgbg[afp].h[true]);
				dat.qEnergySpectra[s][nBetaTubes][t].fgbg[afp].h[true]->SetMarkerColor(2+2*s);
				dat.qEnergySpectra[s][nBetaTubes][t].fgbg[afp].h[true]->SetMarkerStyle(20+4*afp);
				hToPlot.push_back(dat.qEnergySpectra[s][nBetaTubes][t].fgbg[afp].h[true]);
			}
		}
		drawSimulHistos(hToPlot,"HIST P");
		printCanvas(std::string("DataComparison/Type_")+itos(t));
	}
}

