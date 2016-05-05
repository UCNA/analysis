#ifndef FIERZ_FITTER
#define FIERZ_FITTER

/// UCNA includes
#include "G4toPMT.hh"
#include "PenelopeToPMT.hh"
#include "CalDBSQL.hh"

/// ROOT includes
#include <TH1.h>
#include <TLegend.h>
#include <TF1.h>
#include <TVirtualFitter.h>
#include <TList.h>
#include <TStyle.h>
#include <TApplication.h>
#include <TMatrixD.h>
#include <TNtuple.h>
#include <TLeaf.h>
#include <TString.h>
#include <TRandom2.h>

/// c++ includes
#include <iostream>
#include <fstream>
#include <string>

/// c includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using TMath::Sqrt;
using TMath::IsNaN;


///
/// physical constants
///
const double    pi          = 3.1415926535;         /// pi
const double    alpha       = 1/137.035999;         /// fine-structure constant
const double    a2pi        = alpha/(2*pi);         /// alpha over 2pi
const double    m_e         = 510.9988484;          /// electron mass       (14)
const double    m_p         = 938272.046;           /// proton mass         (21)
const double    m_n         = 939565.378;           /// neutron mass        (21)
const double    mu_p        = 2.792846;             /// proton moment       (7)
const double    mu_n        = -1.9130427;           /// neutron moment      (5)
const double    muV         = mu_p - mu_n;          /// nucleon moment      (9)
const double    Q           = 782.344;              /// end point KE        (30)
const double    E0          = m_e + Q;              /// end point E         (30)
const double    lambda      = -1.27590;             /// gA/gV               (450)
const double    M           = 1 + 3*lambda*lambda;  /// matrix element
const double    I_0         = 1.63632;              /// 0th moment          (25)
const double    I_1         = 1.07017;              /// 1st moment          (15)
const double    x_1         = I_1/I_0;              /// first m/E moment    (9)


using namespace std;


struct UCNAhistogram : TH1D {
    int side;
    int spin;
    //TString name;
    //TString title;
    //int bins;
    //double min, max;
    //TH1D* histogram;
    //vector<double> energy;        
    //vector<double> values;        
    //vector<double> errors;

    /*
    UCNAhistogram(int bins, double min, double max) 
      : TH1D(name, title, bins, min, max),
        name(""),
        title(""),
        bins(bins), min(min), max(max)
        //histogram(NULL),
        //energy(bins),        
        //values(bins),        
        //errors(bins)
    {}
    */
       
    UCNAhistogram(TString name, TString title, int bins, double min, double max) 
      : TH1D(name, title, bins, min, max)
        //name(name),
        //title(title),
        //bins(bins), min(min), max(max)
        //histogram(NULL),
        //energy(bins),        
        //values(bins),        
        //errors(bins),
    {
        //histogram = new TH1D(name, title, bins, min, max);
    }

    int fill(TString filename, TString name, TString title);
    int fill(TString filename);
    void save(TString filename, TString name, TString title);
    void save(TString filename);

    bool test_min();
    bool test_min(double min);
    bool test_max();
    bool test_max(double max);
    bool test_range();
    bool test_range(double min, double max);

    double normalize(double min, double max);
    double normalize();
};



struct UCNAmodel {
    TString name;
    TString title;
    int     bins;
    double  min, max;
    TRandom2 rand;

    //TH1D*   raw[2][2];  
    TNtuple* ntuple;     /// another way to store the raw data
    UCNAhistogram* counts[2][2]; // TODO make member not pointer
    UCNAhistogram super_ratio;
    UCNAhistogram super_sum;
    UCNAhistogram asymmetry;
    // Add Yup and Ydown

    /// cuts and settings
    unsigned nToSim = 5e7;			/// how many triggering events to simulate
    double afp_off_prob = 1/1.68; 	/// afp off probability per neutron (0.68/1.68 for on)
    int KEbins = 150;               /// number of bins to use fit spectral plots

    double KEmin = 50;              /// min kinetic energy for plots
    double KEmax = 650;             /// max kinetic range for plots
    double KEmin_A = 120;           /// min kinetic energy for asymmetry fit
    double KEmax_A = 670;           /// max kinetic range for asymmetry fit
    double KEmin_b = 120;           /// min kinetic energy for Fierz fit
    double KEmax_b = 670;           /// max kinetic range for Fierz fit
    double fedutial_cut = 50;       /// radial cut in millimeters 
    double fidcut2 = 50*50;         /// mm^2 radial cut

    /// set up free fit parameters with best guess
    static const int nPar = 3;
    TString paramNames[3] = {"A", "b", "N"};
    double paramInits[3] = {-0.12, 0, 1e1};

    /*
    UCNAmodel(int bins, double min, double max) 
      : name(""), title(""), 
        bins(bins), min(min), max(max),
        //raw({{NULL,NULL},{NULL,NULL}}),
        super_ratio(bins, min, max),
        super_sum(bins, min, max),
        asymmetry(bins, min, max) {
        for (int side = 0; side < 2; side++)
            for (int spin = 0; spin < 2; spin++)
                counts[side][spin] = 0;
        ntuple = new TNtuple("mc_ntuple", "MC NTuple", "s:load:energy");
    }
    */

    UCNAmodel(TString name, TString title, int bins, double min, double max) 
      : name(name), title(title),
        bins(bins), min(min), max(max),
        rand(0),
        super_ratio(name+"_super_ratio",title+" Super Ratio",bins,min,max),
        super_sum(name+"_super_sum",title+" Super Sum",bins,min,max),
        asymmetry(name+"_asymmetry",title+" Asymmetry",bins,min,max)
    {
        for (int side = 0; side < 2; side++) {
            TString sub_name = name;
            TString sub_title = title;
            if (not side) {
                sub_name += "_E";
                sub_title += " East";
            } else {
                sub_name += "_W";
                sub_title += " West";
            }
            for (int spin = 0; spin < 2; spin++) {
                if (not spin) {
                    sub_name += "_off";
                    sub_title += " AFP Off";
                } else {
                    sub_name += "_on";
                    sub_title += " AFP On";
                }
                counts[side][spin] = new UCNAhistogram(sub_name, sub_title, bins, min, max);
            }
        }
        ntuple = new TNtuple("mc_ntuple", "MC NTuple", "s:load:energy");
    }

    double asymmetry_chi2(double A, double b);
    int fill(TString filename, TString name, TString title);
    void save(TString filename, TString title, TString name);
    void save(TString filename);

    /// accessing data
    bool test_counts();
    bool test_construction();
    void get_counts(int bin, double n[2][2]);
    void get_counts(int bin, double n[2][2], double e[2][2]);

    /// compute super sum
    double compute_super_sum(double n[2][2]);
    double compute_super_sum(double n[2][2], double e[2][2], 
                             double& S, double& error);
    double compute_super_sum(int bin, double& count, double& error);
    double compute_super_sum(int bin);
    TH1D& compute_super_sum();
    TH1D& compute_super_sum(double min, double max, 
                            int& min_bin, int& max_bin);
    TH1D& compute_super_sum(int min_bin, int max_bin);
    #if 0
    /// compute super ratio
    double compute_super_ratio(double n[2][2]);
    double compute_super_ratio(double n[2][2], double e[2][2], 
                               double& S, double& error);
    double compute_super_ratio(int bin, double& count, double& error);
    double compute_super_ratio(int bin);
    TH1D& compute_super_ratio();
    TH1D& compute_super_ratio(double min, double max, 
                             int& min_bin, int& max_bin);
    TH1D& compute_super_ratio(int min_bin, int max_bin);
    #endif

    /// compute asymmetry (and super ratio)
    double compute_asymmetry(double n[2][2]);
    double compute_asymmetry(double n[2][2], double e[2][2], 
                             double& S, double& error);
    double compute_asymmetry(int bin, double& count, double& error);
    double compute_asymmetry(int bin);
    TH1D& compute_asymmetry();
    TH1D& compute_asymmetry(double min, double max, 
                            int& min_bin, int& max_bin);
    TH1D& compute_asymmetry(int min_bin, int max_bin);
};


struct UCNAEvent {
    double EdepQ;
    double Edep;
    double MWPCEnergy;
    double ScintPos;
    double MWPCPos;
    double time;
    double primMomentum;
};

struct UCNAFierzFitter {
    int bins;
    double min;
    double max;

    UCNAmodel data;         /// Measured foreground data to fit
    UCNAmodel bg;           /// Measured background data to remove
    UCNAmodel sm;           /// Standard Model vector Monte Carlo spectrum
    UCNAmodel axial;        /// Axial vector Monte Carlo spectrum
    UCNAmodel fierz;        /// Fierz (Scaler + tensor) Monte Carlo spectrum
    UCNAmodel fit;          /// Vector + axial + Fierz Monte Carlo best fit

    UCNAFierzFitter(int bins, double min, double max)
      : bins(bins), min(min), max(max),
        data("ucna_data_", "UCNA data", bins, min, max),
        bg("ucna_bg_", "UCNA background", bins, min, max),
        sm("ucna_sm_", "Standard Model Monte Carlo", bins, min, max),
        axial("ucna_axial_", "Axial-vector Monte Carlo", bins, min, max),
        fierz("ucna_fierz_", "Fierz Monte Carlo", bins, min, max),
        fit("ucna_fit_", "Standard Model + Fierz best fit", bins, min, max) 
    {}

    //void combined_chi2(Int_t & /*nPar*/, Double_t * /*grad*/ , Double_t &fval, Double_t *p, Int_t /*iflag */);
    double asymmetry_chi2(double A, double b);
    double supersum_chi2(double b, double N);
    double combined_chi2(double A, double b, double N);

    //TF1* combined_fit(TH1D* asymmetry, TH1D* super_sum, TMatrixD &cov, TF1 *func);
    TF1* combined_fit(TMatrixD &cov, TF1 *func,  
        void (*)(Int_t&, Double_t*, Double_t&, Double_t*, Int_t));
        /*
        fierz.super_sum.histogram = 0;
        sm.super_sum.histogram = 0;
        for (int side = 0; side < 2; side++)
            for (int spin = 0; spin < 2; spin++) {
                fierz.raw[side][spin] = 0;
                sm.raw[side][spin] = 0;
            }
        fierz.super_sum.histogram = new TH1D("fierz_histogram", "", bins, min, max);
        sm.super_sum.histogram = new TH1D("standard_model_histogram", "", bins, min, max);
        for (int side = 0; side < 2; side++)
            for (int spin = 0; spin < 2; spin++) {
                fierz.raw[side][spin] = new TH1D("fierz_super_sum", "", bins, min, max);
                sm.raw[side][spin] = new TH1D("standard_model_super_sum", "", bins, min, max);
            }
    }
            */

    /*
    double evaluate(double *x, double*p) {
        double rv = 0;
        rv += p[0] * sm_histogram->GetBinContent(sm_histogram->FindBin(x[0]));        
        rv += p[1] * fierz_histogram->GetBinContent(fierz_histogram->FindBin(x[0]));
        return rv;
    }

    void normalize(TH1D* hist) {
        hist->Scale(1/(hist->GetBinWidth(2)*hist->Integral()));
    }

    void normalize(TH1D* hist, double min, double max) {
		int _min = hist->FindBin(min);
		int _max = hist->FindBin(max);
        hist->Scale(1/(hist->GetBinWidth(2)*hist->Integral(_min, _max)));
    }
    */
};


/*
double random(double min, double max) 
{
    double p = rand();
    return min + (max-min)*p/RAND_MAX;
}
*/


/// beta spectrum with little b term
double fierz_beta_spectrum(const double *val, const double *par) ;
/*
{
	const double K = val[0];                    /// kinetic energy
	if (K <= 0 or K >= Q)
		return 0;                               /// zero outside range

	const double b = par[0];                    /// Fierz parameter
	const int n = par[1];                    	/// Fierz exponent
	const double E = K + m_e;                   /// electron energy
	const double e = Q - K;                     /// neutrino energy
	const double p = sqrt(E*E - m_e*m_e);       /// electron momentum
	const double x = pow(m_e/E,n);              /// Fierz term
	const double f = (1 + b*x)/(1 + b*x_1);     /// Fierz factor
	const double k = 1.3723803E-11/Q;           /// normalization factor
	const double P = k*p*e*e*E*f*x;             /// the output PDF value

	return P;
}
*/


/// beta spectrum with expected x^-n and beta^m
double beta_spectrum(const double *val, const double *par) ;
/*
{
	const double K = val[0];                    	///< kinetic energy
	if (K <= 0 or K >= Q)
		return 0;                               	///< zero beyond endpoint

	const double m = par[0];                    	///< beta exponent
	const double n = par[1];                    	///< Fierz exponent
	const double E = K + m_e;                   	///< electron energy
	const double B = pow(1-m_e*m_e/E/E,(1+m)/2);  	///< beta power factor
	const double x = E / m_e;                   	///< reduced electron energy
	const double y = (Q - K) / m_e;             	///< reduced neutrino energy
	const double z = pow(x,2-n);          			///< Fierz power term
	const double k = 1.3723803E-11/Q;           	///< normalization factor
	const double P = k*B*z*y*y;             		///< the output PDF value

	return P;
}
*/


double evaluate_expected_fierz(double m, double n, double min, double max, double) ;
/*
{
    TH1D *h1 = new TH1D("beta_spectrum_fierz", "Beta spectrum with Fierz term", integral_size, min, max);
    TH1D *h2 = new TH1D("beta_spectrum", "Beta Spectrum", integral_size, min, max);
	for (int i = 0; i < integral_size; i++)
	{
		double K = min + double(i)*(max-min)/integral_size;
		double par1[2] = {m, n};
		double par2[2] = {0, 0};
		double y1 = beta_spectrum(&K, par1);
		double y2 = beta_spectrum(&K, par2);
		h1->SetBinContent(K, y1);
		h2->SetBinContent(K, y2);
	}
	double rv = h1->Integral(0, integral_size) / h2->Integral(0, integral_size);
    delete h1;
    delete h2;
    return rv;
}
*/


double evaluate_expected_fierz(double min, double max, double) ;
/*
{
	return evaluate_expected_fierz(0, 1, min, max, integral_size);
}
*/



/*
void compute_fit(TH1D* histogram, TF1* fierz_fit) 
{
	// compute chi squared
    double chisq = fierz_fit->GetChisquare();
    double N = fierz_fit->GetNDF();
	char A_str[1024];
	sprintf(A_str, "A = %1.3f #pm %1.3f", fierz_fit->GetParameter(0), fierz_fit->GetParError(0));
	char b_str[1024];
	sprintf(b_str, "b = %1.3f #pm %1.3f", fierz_fit->GetParameter(1), fierz_fit->GetParError(1));
	char chisq_str[1024];
    printf("Chi^2 / ( N - 1) = %f / %f = %f\n", chisq, N-1, chisq/(N-1));
	sprintf(chisq_str, "#frac{#chi^{2}}{n-1} = %f", chisq/(N-1));

	// draw the ratio plot
	histogram->SetStats(0);
    histogram->Draw();

	// draw a legend on the plot
    TLegend* ratio_legend = new TLegend(0.3,0.85,0.6,0.65);
    ratio_legend->AddEntry(histogram, "Asymmetry data", "l");
    ratio_legend->AddEntry(fierz_fit, "fit", "l");
    ratio_legend->AddEntry((TObject*)0, A_str, "");
    ratio_legend->AddEntry((TObject*)0, b_str, "");
    ratio_legend->AddEntry((TObject*)0, chisq_str, "");
    ratio_legend->SetTextSize(0.03);
    ratio_legend->SetBorderSize(0);
    ratio_legend->Draw();
}
*/


double asymmetry_fit_func(double *x, double *par);
double fierzratio_fit_func(double *x, double *par);
double GetEntries(TH1D* histogram, double min, double max);



#endif // FIERZ_FITTER
