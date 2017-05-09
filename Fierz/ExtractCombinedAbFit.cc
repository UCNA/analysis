/// UCNA includes
#include "G4toPMT.hh"
#include "PenelopeToPMT.hh"
#include "CalDBSQL.hh"
#include "FierzFitter.hh"

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

/// C++ includes
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <limits>
#include <ctime>

/// C includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/// name spaces
using std::setw;
using std::cout;
using namespace TMath;
double infity = std::numeric_limits<double>::infinity();

/// cuts and settings
double KEmin = 0;                 /// min kinetic energy for plots
double KEmax = 800;               /// max kinetic range for plots
int    KEbins=(KEmax-KEmin)/10;   /// number of bins to use fit spectral plots
double fit_min = 120;             /// min kinetic energy for plots
double fit_max = 630;             /// max kinetic range for plots
int fit_bins=(fit_max-fit_min)/10;/// number of bins to use fit spectral plots
double fedutial_cut = 50;         /// radial cut in millimeters TODO!! HARD CODED IN MODEL

/// set up free fit parameters with best guess

#if 0
static TString FIT_TYPE = "AbN";
static const int nPar = 3;
double afp_ratio = 0.40;
TString paramNames[3] = {"A", "b", "N"};
double paramInits[3] = {-0.12, 0.0, 1};
#endif
#if 0
static TString FIT_TYPE = "bN";
static const int nPar = 2;
double afp_ratio = 0.40;
TString paramNames[2] = {"b", "N"};
double paramInits[2] = {0.0, 1000000};
int A_index=-1, b_index=0, N_index=1;
#endif
#if 1
static TString FIT_TYPE = "b";
static const int nPar = 1;
double afp_ratio = 0.40;
TString paramNames[1] = {"b"};
double paramInits[1] = {0};
int A_index=-1, b_index=0, N_index=-1;
#endif
#if 0
static TString FIT_TYPE = "Ab";
static const int nPar = 2;
double afp_ratio = 0.40;
TString paramNames[2] = {"A", "b"};
double paramInits[2] = {1, 0};
int A_index=0, b_index=1, N_index=-1;
#endif

/// path to experiment data files
TString data_dir = "/media/hickerson/boson/Data/OctetAsym_Offic_2010_FINAL/"; 

/// path to Monte Carlo files
TString mc_dir = "/home/xuansun/Documents/SimProcessedFiles/100mill_beta/";
TString mc_sys_dir = "/home/xuansun/Documents/SimProcessedFiles/100mill_both_twiddled/";

/// path to save output plots
//TString plots_dir = "/home/hickerson/Dropbox/Root/";
TString plots_dir = "/home/hickerson/Root/";

/// path to save output root structures 
//TString root_output_dir = "/home/hickerson/Documents/";
TString root_output_dir = "/home/hickerson/Root/";



/// GLOBAL MODELS
//ucna.fidcut2 = fedutial_cut*fedutial_cut;

/// This needs to be static and global for MINUIT to work
UCNAFierzFitter* global_ff = 0;

void combined_chi2(Int_t & /*n*/, Double_t * /*grad*/ , Double_t &chi2, Double_t *p, Int_t /*iflag */  )
{
    assert(global_ff);
    if (FIT_TYPE=="AbN") {
        double A=p[0], b=p[1], N=p[2];
        chi2 = global_ff->combined_chi2(A,b,N);
    }
    else if (FIT_TYPE=="bN") {
        double b=p[0], N=p[1];
        chi2 = global_ff->supersum_chi2(b,N);
    }
    else if (FIT_TYPE=="b") {
        double b=p[0];
        //chi2 = global_ff->supersum_chi2(b,1/(1+b*x_1));
        chi2 = global_ff->supersum_chi2(b,1);
    }
}

int cl = 14;
void output_matrix(TString title, TMatrixD matrix, TString colNames[], TString rowNames[])
{
    cout<<"\n "<<title;
    int cols = matrix.GetNcols();
    int rows = matrix.GetNrows();
    if (cols>1) {
        cout<<"\n     ";
        for (int i=0; i<cols; i++)
            cout<<setw(cl)<<colNames[i];
    }
    cout<<"\n";
    for (int i=0; i<cols; i++) {
        cout<<"    "<<rowNames[i];
        for (int j=0; j<rows; j++)
            cout<<setw(cl)<<matrix[i][j];
        cout<<"\n";
    }
}

void output_matrix(TString title, TMatrixD matrix)
{
    output_matrix(title, matrix, paramNames, paramNames);
}

/// FITTING 
TF1* fit(UCNAFierzFitter &ff) {
    /// Set up the fit functions parameters.
    TF1* asymmetry_func = new TF1("asymmetry_fit_func", &asymmetry_fit_func, fit_min, fit_max, nPar);
    asymmetry_func->SetParameters(paramInits);
    for (int i=0; i<nPar; i++)
        asymmetry_func->SetParName(i, paramNames[i]);

    /*TF1 supersum_func("supersum_fit_func", &supersum_fit_func, fit_min, fit_max, nPar);
    supersum_func.SetParameters(paramInits);
    for (int i=0; i<nPar; i++)
        supersum_func->SetParName(i, paramNames[i]);*/

    /// Actually do the combined fitting.
    global_ff = &ff; /// TODO cannot be called in a member
    TMatrixD cov(nPar,nPar);
    ff.combined_fit(cov, asymmetry_func, /*supersum_func,*/ &combined_chi2); /// TODO no member
    //ff.compute_fit(&asymmetry_func);
    global_ff = 0;

    double *p = asymmetry_func->GetParameters();
    if (FIT_TYPE=="AbN") {
        assert(nPar == 3);
        double A=p[0], b=p[1], N=p[2];
        ff.compute_fit(A,b,N);
    }
    else if (FIT_TYPE=="bN") {
        assert(nPar == 2);
        double b=p[0], N=p[1];
        ff.compute_fit(-0.12,b,N);
    }
    else if (FIT_TYPE=="b") {
        assert(nPar == 1);
        double b=p[0];
        //ff.compute_fit(-0.12,b,1/(1+b*x_1));
        ff.compute_fit(-0.12,b,1);
    }

    /// Look up sizes
    double Nsim_data = ff.data.super_sum.GetEntries();
    double Nall_data = ff.data.super_sum.GetEffectiveEntries(KEmin, KEmax);
    double Nfit_data = ff.data.super_sum.GetEffectiveEntries(fit_min, fit_max);
    double Nfit_vector = ff.vector.super_sum.GetEffectiveEntries(fit_min, fit_max);
    //double Nfit_axial = ff.axial.super_sum.GetEffectiveEntries(fit_min, fit_max);
    double Nfit_fierz = ff.fierz.super_sum.GetEffectiveEntries(fit_min, fit_max);
  
    /// Set up reasonable guesses 
    double A = -0.12;
    double b = 0;
    //double N = fit_entries;
    //double N = Nfit_data*Nfit_vector/Sqrt(Nfit_data*Nfit_data + Nfit_vector*Nfit_vector);
    double N = Nfit_data*Nfit_vector/(Nfit_data + Nfit_vector);
    //double N = Nfit_data + Nfit_vector;
    //double N = Nfit_data;
    double ex_cos = 0.5; //evaluate_expected_cos_theta(fit_min,fit_max);
    double ec2 = ex_cos*ex_cos; //evaluate_expected_cos_theta(fit_min,fit_max);


    /// PRINT OUT REPORT OF FITS, CORRELATIONS AND ERRORS

    /// Output the data and cut info.
    cout<<setprecision(5);
    cout<<" ENERGY RANGE:\n";
    cout<<"    Full energy range is "<<KEmin<<" - "<<KEmax<<" keV.\n";
    cout<<"    Fit energy range cut is "<<fit_min<<" - "<<fit_max<<" keV.\n";
    cout<<"    Number of actual counts in data is "<<int(Nsim_data)<<".\n";
    cout<<"    Effective number of counts in full energy range is "<<int(Nall_data)<<".\n";
    cout<<"    Effective number of counts in fit energy range cut is "<<int(Nfit_data)<<".\n";
    cout<<"    Efficiency of energy cut is "<<int(Nfit_data/Nall_data*1000)/10<<"%.\n";
    cout<<"    Effective N "<<int(N*100)/100<<".\n";
    
    /// Set all expectation values for this range.
    const int nSpec = 4;
    TMatrixD ex(nSpec,nSpec);
    TString colNames[nSpec], rowNames[nSpec];
    for (int m=0; m<nSpec; m++) {       /// beta exponent
        colNames[m].Form("(v/c)^%d",m);
        for (int n=0; n<nSpec; n++) {  /// m_e/E exponent
            rowNames[n].Form("(m/E)^%d",n); 
            ex[m][n] = evaluate_expected_fierz(m,n,fit_min,fit_max);
            //cout<<"Expected value for beta exponent: "<<m
            //    <<" and m/E exponent "<<n<<" is "<<ex[m][n]<<".\n";
        }
    }

    output_matrix("APPROXIMATE ANALYTICAL EXPECTATION MATRIX", ex, colNames, rowNames);
    
    /// Calculate the predicted inverse covariance matrix for this range.
    TMatrixD est_cov_inv(nPar,nPar);
    for (int i=0; i<nPar; i++)
        for (int j=0; j<nPar; j++)
            est_cov_inv[i][j] = 0;

    if (A_index >= 0)
        est_cov_inv[A_index][A_index] += N*ec2*ex[2][0];

    if (b_index >= 0) {
        est_cov_inv[b_index][b_index] += N*(ex[0][2] - ex[0][1]*ex[0][1]);
        if (A_index >= 0) {
            est_cov_inv[A_index][b_index] = 
            est_cov_inv[b_index][A_index] = -N*A*ec2*ex[2][1];
            est_cov_inv[b_index][b_index] += N*A*A*ec2*ex[2][2];
        }
    }

    if (N_index >= 0) {
        est_cov_inv[N_index][N_index] = N;
        if (A_index >= 0) {
            est_cov_inv[A_index][N_index] =
            est_cov_inv[N_index][A_index] = N*ex[0][1];
        }
    }

    /*  
    if (b_index > -1)
        est_cov_inv[0][0] = N*ex_cos*ex[2][0];
    if (nPar > 1) {
        est_cov_inv[1][0] = 
        est_cov_inv[0][1] = -N*A*ex_cos*ex[2][1];
        //est_cov_inv[1][1] =  N*(A*A*ex[2][2]/4 + ex[0][2] - ex[0][1]*ex[0][1]);
        est_cov_inv[1][1] = N*(A*A*ex_cos*ex[2][2] + ex[0][2] - ex[0][1]*ex[0][1]);
    }
    if (nPar > 2) {
        est_cov_inv[1][2] =
        est_cov_inv[2][1] = N*ex[0][1];
        est_cov_inv[2][2] = N;
    } */

    /// Output the fit covariance matrix from the fit.
    output_matrix("FIT COVARIANCE MATRIX", cov);

    /// Compute the estimated covariance matrix by inverse.
    double det = 0;
    TMatrixD est_cov = est_cov_inv;
    output_matrix("ESTIMATED COVARIANCE MATRIX", est_cov.Invert(&det));

    /// Output the estimated covariance details.
    TMatrixD ratio_cov(nPar,nPar);
    for (int i=0; i<nPar; i++)
        for (int j=0; j<nPar; j++)
            ratio_cov[i][j] = est_cov[i][j]/cov[i][j];
    output_matrix("ESTIMATED/ACTUAL COVARIANCE RATIOS", ratio_cov);

    /// Output the fit covariance inverse matrix time 2.
    TMatrixD cov_inv = cov;
    output_matrix("FIT HALF CHI^2 DERIVATIVE MATRIX", cov_inv.Invert(&det));

    /// Compute the estimated covariance matrix by inverse.
    output_matrix("ESTIMATED HALF CHI^2 DERIVATIVE MATRIX", est_cov_inv);

    /// Output the estimated covariance details.
    TMatrixD ratio_cov_inv(nPar,nPar);
    for (int i=0; i<nPar; i++)
        for (int j=0; j<nPar; j++)
            ratio_cov_inv[i][j] = est_cov_inv[i][j]/cov_inv[i][j];
    output_matrix("ESTIMATED/ACTUAL HALF CHI^2 DERIVATIVE RATIOS", ratio_cov_inv);

#if 0
    /// Compute independent standard errors.
    cout<<"\n FOR UNCOMBINED FITS:\n";
    cout<<"    Expected independent statistical sigma and error:\n";
    cout<<"    "<<setw(1)<<" " 
                <<setw(cl)<<"value"
                <<setw(cl)<<"sigma" 
                <<setw(cl+1)<<"error\n";
    for (int i=0; i<nPar; i++) {
        TString name = paramNames[i];
        mc_dir+"SimAnalyzed_Beta_7.root",
        double value = asymmetry_func->GetParameter(i);
        double sigma = 1/Sqrt(est_cov_inv[i][i]);
        double error = 100*sigma/value;
        cout<<"    "<<setw(1) <<name
                    <<setw(cl)<<value
                    <<setw(cl)<<sigma
                    <<setw(cl-1)<<error<<"%\n";
    }
#endif

    /// Compare predicted and actual standard errors.
    cout<<"\n FOR COMBINED FITS:\n";
    cout<<"    Actual and estimated combined statistical sigmas and errors:\n";
    cout<<"    "<<setw(1)<<" "
                <<setw(cl)<<"value" 
                <<setw(3*cl/2)<<"sigma (act./est)"
                <<setw(3*cl/2)<<"factor (act./est.)"
                <<setw(3*cl/2)<<"error (act./est.)" 
                <<setw(cl+1)<<"est./actual\n";
    for (int i=0; i<nPar; i++) {
        TString param = paramNames[i];
        double scale = Sqrt(N);
        double value = asymmetry_func->GetParameter(i);
        double sigma = Sqrt(cov[i][i]);
        double error = sigma*100/value;
        double factor = scale*sigma;
        double est_sigma = Sqrt(est_cov[i][i]);
        double est_error = est_sigma*100/value;
        double est_factor = scale*est_sigma;
        double ratio = est_sigma/sigma;
        cout<<"    "<<setw(1)   <<param 
                    <<setw(cl)  <<value 
                    <<setw(cl-1)<<sigma<<"/"
                    <<setw(cl/2)<<est_sigma 
                    <<setw(cl-1)<<factor<<"/"
                    <<setw(cl/2)<<est_factor 
                    <<setw(cl-2)<<error <<"%/"
                    <<setw(cl/2-1)<<est_error<<"%"
                    <<setw(cl)  <<ratio<<"\n";
    }

    /// Compare predicted and actual correlations.
    if (nPar > 1)
        cout<<"\n CORRELATIONS FACTORS FOR COMBINED FITS:\n"
            <<"    "<<setw(8)<<" "
                    <<setw(cl)<<"actual"
                    <<setw(cl)<<"estimate" 
                    <<setw(cl)<<"act. sq(N)"
                    <<setw(cl)<<"est. sq(N)" 
                    <<setw(cl+1)<<"est./actual\n";
    for (int i=0; i<nPar; i++) {
        TString name_i = paramNames[i];
        for (int j = i+1; j<nPar; j++) {
            TString name_j = paramNames[j];
            TString cor_name = "cor("+name_i+","+name_j+")";
            double cor_ij = cov[j][i]/Sqrt(cov[i][i]*cov[j][j]);
            double est_cor_ij = est_cov[j][i]/Sqrt(est_cov[i][i]*est_cov[j][j]);
            double ratio = est_cor_ij/cor_ij;
            cout<<"    "<<setw(8)<<cor_name
                        <<setw(cl)<<cor_ij
                        <<setw(cl)<<est_cor_ij
                        <<setw(cl)<<cor_ij*Sqrt(N)
                        <<setw(cl)<<est_cor_ij*Sqrt(N)
                        <<setw(cl)<<ratio<<"\n";
        }
    }
    return asymmetry_func;
}




/// MAIN APPLICATION
int main(int argc, char *argv[])
{
    //TApplication app("Extract Combined A+b Fitter", &argc, argv);
    srand( time(NULL) );    /// set this to make random or repeatable

    UCNAFierzFitter ucna("monte_carlo", "Monte Carlo UCNA", KEbins, KEmin, KEmax, fit_bins, fit_min, fit_max);
    UCNAFierzFitter fake("fake", "Fake", KEbins, KEmin, KEmax, fit_bins, fit_min, fit_max);
    UCNAFierzFitter real("real", "2010 Data", KEbins, KEmin, KEmax, fit_bins, fit_min, fit_max);

    /// LOAD 2010 UCNA DATA

    /// Load the files that already contain data super histogram.
    bool use_real_data = false;
    if (use_real_data) {
        for (int side=EAST; side<=WEST; side++) {
            for (int afp=EAST; afp<=WEST; afp++) {
                TString s = side? "W":"E", a = afp? "On":"Off";
                TString title = "2010 final official "+s+" afp "+a;
                TString name = "hTotalEvents_"+s+"_"+a+";1";
                int entries = real.data.counts[side][afp]->fill(data_dir+"OctetAsym_Offic.root", name, title);
                if (entries) {
                    cout<<"Status: Number of entries in "<<(side? "west":"east")
                        <<" side with afp "<<a<<" is "<<entries<<".\n";
                }
                else
                    cout<<"Error: found no events in "<<title<<".\n";
            }
        }
    }

    /*
    /// LOAD PRECOMPUTED HISTOGRAMS AND OVERWRITE 
    /// Load the files that already contain data asymmetry histogram.
    ucna.data.asymmetry.fill(
        data_dir+"Range_0-1000/CorrectAsym/CorrectedAsym.root",
        "hAsym_Corrected_C",
        "2010 final official asymmetry");
    /// Load the files that already contain data super histogram.
    ucna.data.super_sum.fill(
        data_dir+"OctetAsym_Offic.root",
        "Total_Events_SuperSum",
        "2010 final official supersum");
    /// Load Monte Carlo simulated Standard Model events
    ucna.sm.fill(mc_dir+"SimAnalyzed_Beta_7.root",
               "SimAnalyzed",
               "Monte Carlo Standard Model beta spectrum");
    /// Load Monte Carlo simulated Fierz events
    ucna.fierz.fill(mc_dir+"SimAnalyzed_Beta_fierz_7.root",
                  "SimAnalyzed",
                  "Monte Carlo Fierz beta spectrum");
    //fit(ucna);
    //display(ucna);
    */

    /// Find file paths from environment
    //if (getenv("SIM_PROC_MC_DIR"))
    //    mc_dir = getenv("SIM_PROC_MC_DIR");
    if (getenv("SIM_MC_DIR"))
        mc_dir = getenv("SIM_MC_DIR");
    //if (getenv("SIM_PROC_SYS_DIR"))
    //    mc_sys_dir = getenv("SIM_PROC_SYS_DIR");
    if (getenv("SIM_PROC_DIR"))
        mc_sys_dir = getenv("SIM_PROC_DIR");

    /// Default filenames.
    TString ucna_vector_filename = mc_dir+"SimAnalyzed_Beta_12.root";
    TString ucna_fierz_filename = mc_dir+"SimAnalyzed_Beta_fierz_12.root";
    //TString ucna_axial_filename = mc_dir+"SimAnalyzed_Beta_Axial_12.root";
    TString fake_vector_filename = mc_dir+"SimAnalyzed_Beta_14.root";
    TString fake_fierz_filename = mc_dir+"SimAnalyzed_Beta_fierz_14.root";
    //TString fake_axial_filename = mc_dir+"SimAnalyzed_Beta_Axial_13.root";
    TString plot_filebase = plots_dir;
    if (FIT_TYPE == "b" or FIT_TYPE == "bN") {
        if (argc < 5) {
            ucna_vector_filename = mc_dir+"SimAnalyzed_Beta_"+argv[1]+".root";
            ucna_fierz_filename = mc_dir+"SimAnalyzed_Beta_Fierz_"+argv[1]+".root";
            fake_vector_filename = mc_dir+"SimAnalyzed_Beta_"+argv[2]+".root";
            fake_fierz_filename = mc_dir+"SimAnalyzed_Beta_Fierz_"+argv[2]+".root";
        }
        if (argc == 3)
            plot_filebase = plots_dir+"output_"+argv[2]+"_";
        if (argc == 4)
            plot_filebase = plots_dir+argv[3];
        else {
            cout<<"Only 3 args is supported right now.\n";
            cout<<"Usage: "<<argv[0]<<" <ucna file number | base> <fake file number | twiddled-n> <output plot dir>\n";
            exit(1);
        }
        if (argc == 5) {
            ucna_vector_filename = mc_dir+argv[1]+".root";
            ucna_fierz_filename = mc_dir+argv[2]+".root";
            fake_vector_filename = mc_dir+argv[3]+".root";
            fake_fierz_filename = mc_dir+argv[4]+".root";
        }
        cout<<"MC vector file: "<<ucna_vector_filename<<".\n";
        cout<<"MC Fierz file: "<<ucna_fierz_filename<<".\n";
        cout<<"Fake vector file: "<<fake_vector_filename<<".\n";
        cout<<"Fake Fierz file: "<<fake_fierz_filename<<".\n";
        cout<<"Plot files: "<<plot_filebase<<".\n";
    }
    else {
        cout<<"Can't run with more than one parameter right now.\n";
        exit(1);
    }

    int min1 = 11, max1 = 20, min2 = 21, max2 = 30;
    ucna.vector.fill(
        //mc_dir+"SimAnalyzed_Beta_*.root", 10, 11,
        ucna_vector_filename, min1, max1,
        "SimAnalyzed",
        "Standard Model vector current", afp_ratio);

    /*
    ucna.axial.fill(
        mc_dir+"SimAnalyzed_Beta_9.root",
        ucna_axial_filename,
        "SimAnalyzed",
        "Standard Model axial-vector current", afp_ratio);
    */

    /// Load Monte Carlo simulated Fierz events
    ucna.fierz.fill(
        ucna_fierz_filename, 
        min1, max1,
        "SimAnalyzed",
        "Fierz current", afp_ratio); // TODO this is suppressing the errors

    /// LOAD FAKE DATA FROM MONTE CARLO
    /// Load Monte Carlo simulated Standard Model vector current as fake events
    //TString fake_dir = mc_sys_dir;
    fake.vector.fill(
        fake_vector_filename, 
        min2, max2,
        "SimAnalyzed",
        "Standard Model vector current", afp_ratio);

    /// Load Monte Carlo simulated Standard Model axial-vector current as fake events
    /* fake.axial.fill(
        fake_axial_filename,
        "SimAnalyzed",
        "Axial-vector Standard Model Monte Carlo beta spectrum", afp_ratio); */

    /// Load Monte Carlo simulated Fierz as fake events
    fake.fierz.fill(
        fake_fierz_filename, 
        min2, max2,
        "SimAnalyzed",
        "Fierz current", afp_ratio); // TODO this is suppressing the errors

    /* fake.data.fill(
        mc_dir+"SimAnalyzed_Beta_9.root",
        "SimAnalyzed",
        "Monte Carlo Standard Model beta spectrum", afp_ratio, A, b);
    fake.data.asymmetry.fill(
        data_dir+"Range_0-1000/CorrectAsym/CorrectedAsym.root",
        "hAsym_Corrected_C",
        "2010 final official asymmetry"); */

    /// Pick fake values of asymmetry and Fierz terms
    double A = -0.12;
    double b = 0.00;
    double N = 1;

    /// generate fake signal curve from different simulated spectra
    fake.compute_fit(A,b,N);
    ucna.data = fake.fit;
    ucna.data.super_sum.snapshot();

    /// Load the files that already contain data super histogram.
    ucna.data.super_sum.fill(
        data_dir+"OctetAsym_Offic.root",
        "Total_Events_SuperSum",
        "2010 final official supersum");

    ucna.vector.super_sum.snapshot();
    //ucna.axial.super_sum.snapshot();
    ucna.fierz.super_sum.snapshot();

    fake.vector.super_sum.snapshot();
    //fake.axial.super_sum.snapshot();
    fake.fierz.super_sum.snapshot();

    real.vector.super_sum.snapshot();
    //real.axial.super_sum.snapshot();
    real.fierz.super_sum.snapshot();

    TF1* fit_func = fit(ucna);
    if (not fit_func) {
        cout<<" Error: No fitting function returned.\n";
        exit(1);
    }

    ucna.data.super_sum.snapshot();
    ucna.fit.super_sum.snapshot();
    ucna.display(plot_filebase);

    /// extract most critical results from fit functions and report
    ofstream rofs;
    // time_t t = time(0);   // get time now
    // struct tm * now = localtime(&t);
    //tm = *localtime(&t);
    // char buffer[256];
    // strftime(buffer, sizeof(buffer), "-%a-%b-%d-%H:%M:%S-%Y-", tm);
    //string time = put_time(&tm, "-%a-%b-%d-%H:%M:%S-%Y-");
    //filename+="report-"+(now->tm_year + 1900)+"-"
    //        +(now->tm_mon+1)+"-"+now->tm_mday+"-"+now->tm_hour+":"+now->tm_min;
    string outfilename;
    if (argc == 1)
        outfilename = plots_dir+"report.dat";
    else 
        outfilename = plot_filebase;
    rofs.open(outfilename, std::fstream::app | std::fstream::out);
    cout << " FINAL REPORT\n";
    cout<<setw(cl)<<"chi^2"<<setw(cl)<<"ndf"<<setw(cl)<<"chi^2/ndf"<<setw(cl)<<"p(chi2,ndf)";
    for (int i=0; i<nPar; ++i) {  
        TString par_name = fit_func->GetParName(i);
        cout<<setw(cl)<<par_name
            <<setw(cl-5)<<par_name<<" err."
            <<setw(cl-3)<<par_name<<" ex"
            <<setw(cl-8)<<par_name<<" ex err.";
    }
    cout<<"\n";

    double chi2 = fit_func->GetChisquare(); /// Get the chi squared for this fit
    double ndf = fit_func->GetNDF();        /// Get the degrees of freedom in fit
    double prob = Prob(chi2,ndf);           /// Get the probability of this chi2 with this ndf
    
    cout<<setw(cl)<<chi2<<setw(cl)<<ndf
        <<setw(cl)<<chi2/ndf<<setw(cl)<<prob;
    rofs<<setw(cl)<<chi2<<setw(cl)<<ndf
        <<setw(cl)<<chi2/ndf<<setw(cl)<<prob;

    for (int i=0; i<nPar; ++i) {  
        double param = fit_func->GetParameter(i);
        double error = fit_func->GetParError(i);
        cout<<setw(cl)<<param<<setw(cl)<<error
            <<setw(cl)<<param*prob<<setw(cl)<<error*prob;
        rofs<<setw(cl)<<param<<setw(cl)<<error
            <<setw(cl)<<param*prob<<setw(cl)<<error*prob;
    }
    cout<<"\n";
    rofs<<"\n";
    rofs.close();

    //app.Run();
    return 0;
}



