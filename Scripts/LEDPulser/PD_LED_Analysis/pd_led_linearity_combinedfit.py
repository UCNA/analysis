# pd_led_linearity.py
# Author: Simon Slutsky
# Created: 04/20/2015
# Updated: 05/20/2015

# Plot fitted linearity constants output from pd_led_pmt_combinedfit 
# Useful syntax: carp = data[data['Channel'] == 4]

# usage: python pd_led_linearity_combinedfit.py <show> <save> <LED>

# Update 05/20/2015: combined fit works differently now

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages
from pylab import *
from math import sqrt
#from ROOT import TCanvas
import sys
import matplotlib.dates as dates

sys.path.append('../../') # find RunPlotter.py
from RunPlotter import getTimeForRunlist

def makeQualityCuts(data):
    cutruns = [18746, 18751, 18760, 18761, 
               21288, 21295, 21298, 21312, 21318, 
               21599, 21601, 21602, 
               21674, 21686, 
               21792, # refuses to fit
               21903, 21932, 21933, 
               21777, 21891, 21948, 21911, #runs including a bad cycle

               20782, 20889, 20685, 20287]  # haven't/should investigate by eye

    
    bimodalrange1 = range(21650, 21704)
    bimodalrange2 = range(21827, 21843)
    bimodalrange3 = range(21865, 21878)
    bimodalranges = [bimodalrange1, bimodalrange2, bimodalrange3]
    for bmr in bimodalranges:
        for j in bmr:
            cutruns.append(j)
    
    cutsAll = True
    for i in cutruns:
        _cut = (data['Run']!=i)
        cutsAll = cutsAll & _cut
    _data = data[cutsAll]
    return _data

def checkChiSq(data, run):
    runCut = data['Run'] == run
    chiCut = data['ParName'] == "Chisq/nvars"
    cutCond = runCut & chiCut
    chi_data_cut = data[cutCond]
    retarray = chi_data_cut['ParVal'] != 0.0  # cut runs with chisq = 0
    print retarray
    if retarray[0]:
        return run
    else:
        return 0

def makeChiCuts(data):
    cutlist = []
    runlist = data['Run']
    runlist_short = list()
    for run in runlist:
        runinshort = 0
        for run_short in runlist_short:
            if run == run_short:
                runinshort = 1
        if runinshort == 0:
            runlist_short.append(run)

    for run in runlist_short:
        print "Checking ChiSq for run " + str(run)
        cutrun = checkChiSq(data, run)
        cutlist.append(cutrun)
        
#    _data = [row for row in data if row['Run'] not in cutlist]

    # Black magic, 
    # see http://stackoverflow.com/questions/1962980/selecting-rows-from-a-numpy-ndarray
    _data = data[np.logical_or.reduce([data['Run'] == x for x in cutlist])]

    return _data

def getLEDdata(basedir):
    # import data. 
#    basedir = "/data1/saslutsky/LEDPulser/images_05_20_2015_16way_separate_wavelength_coeff_21274_21328/"
#    basedir = "/data1/saslutsky/LEDPulser/images_05_19_2015_16way_separate_wavelength_coeff_18745_18768/"
#    basedir = "/data1/saslutsky/LEDPulser/images_05_14_2015_16way_separate_wavelength_coeff_21927_21939/"
   # basedir = "/data1/saslutsky/LEDPulser/images_05_22_2015_16way_separate_wavelength_coeff_residuals_21927_21939/"
#    basedir = "/data1/saslutsky/LEDPulser/images_05_26_2015_16way_separate_wavelength_coeff_residuals_21650_21950/"

    data = np.genfromtxt(basedir + "FitResults_Combined.txt", 
                         skip_header=0, 
                         delimiter = "\t", 
                         usecols = range(0,6),
                         names = ['Run','Channel', 'ParName',
                                  'ParVal', 'ParErr', 
                                  'Rmin', 'Rmax', 'Rmin2', 'Rmax2'],
                         dtype = "int, int, S12, float, float, float" )
    return data

if __name__ == "__main__":
    
    showbool = int(sys.argv[1]) # boolean to control plotting
    savebool = int(sys.argv[2]) # boolean to control saving
    datebool = int(sys.argv[3]) # boolean to toggle run#/date for x-axis # DOESN'T WORK

    rcParams['figure.figsize'] = 10, 10     #Set default fig size
    plt.rc('axes', color_cycle=['r', 'g', 'b', 'y'])

#    basedir = "/data1/saslutsky/LEDPulser/images_05_26_2015_16way_separate_wavelength_coeff_residuals_21650_21950/"
    basedir = "/data1/saslutsky/LEDPulser/images_06_10_2015_16way_separate_wavelength_coeff_20254_23173/"
    data = getLEDdata(basedir)

    runlist = data['Run']
#cut bad runs
    print "Making Quality Cuts"
    data = makeQualityCuts(data)   # cut runs found bad by eye
    print "Making Chi^2 cuts"
    data = makeChiCuts(data)       # cut runs with chisq == 0.0

    # open output file
    outputfile = PdfPages(basedir + "FitResultsCombined_" + 
                          str(min(runlist)) + "_" +
                          str(max(runlist)) + ".pdf")
    p1file = PdfPages(basedir + "FitResultsCombined_" + 
                          str(min(runlist)) + "_" +
                          str(max(runlist)) + "_p1.pdf")
    

    print data
    npars = 4   #p0, p1, p2, nlambda for each tube
    marks = 8
    parnames = ["p0", "p1", "p2", "nlambda"]
    pdparnames = ["PDq1", "PDq2", "PDq3"]

    axes = list()
    figures = list()
    p1Figures = list()
   
    pd_data_cut = list()
    print "making pdpar cuts"
    for k in range(0, len(pdparnames)):
        _pd_data_cut = data[data['ParName'] == pdparnames[k]]
        pd_data_cut.append(_pd_data_cut)

    tmpFig1, (ax0, ax1, ax2) = plt.subplots(nrows = 3, sharex = True, sharey = False)
    pdaxes = [ax0, ax1, ax2]
    figures.append(tmpFig1)

    for k in range(0, len(pdparnames)): 
        if datebool:
            pdtimelist = getTimeForRunlist(pd_data_cut[k]['Run'])
            pdaxes[k].xaxis.set_major_formatter(dates.DateFormatter("%m/%d/%y"))
        
        if not datebool:
            pdaxes[k].errorbar(pd_data_cut[k]['Run'], 
                               pd_data_cut[k]['ParVal'], 
                               yerr=pd_data_cut[k]['ParErr'], 
                               linestyle='None', marker='o',
                               markersize=marks, color = 'Black')
            xmin, xmax = min(pd_data_cut[k]['Run']), max(pd_data_cut[k]['Run']) 
            pdaxes[0].set_xlim([xmin - 0.5, xmax + 0.5])

        if datebool:
            pdaxes[k].errorbar(pdtimelist, 
                               pd_data_cut[k]['ParVal'], 
                               yerr=pd_data_cut[k]['ParErr'], 
                               linestyle='None', marker='o',
                               markersize=marks, color = 'Black')
        
        pdaxes[k].set_ylabel("q" + str(k+1))
        pdaxes[0].set_title("PD nonlinearity parameters")
        if not datebool:
            pdaxes[2].set_xlabel('Run Number')
        if datebool:
            pdaxes[2].set_xlabel('Time')

    for i in range(0,8): # 8 channels 
        data_cut = list()
        for j in range (0, npars): 
            # Select the channel and parameter, will plot vs run #
            cutChannel = data['Channel'] == int(i)
            cutParm = data['ParName'] == parnames[j]
            cutErr = data['ParErr'] < abs(data['ParVal'])
            cutCond = cutChannel & cutParm & cutErr
            #cutCond = cutChannel & cutParm
            _data_cut = data[cutCond]    
            #            print _data_cut
            data_cut.append(_data_cut) # npars arrays of ParName (for all runs)    return _data_cut

        ## Prepare Plots

        tmpFig0, (tmpAx0, tmpAx1, tmpAx2, tmpAx3) = plt.subplots(nrows=4, sharex = True, sharey=False)
        p1Fig, p1Ax = plt.subplots(nrows = 1)
        axes = [tmpAx0, tmpAx1, tmpAx2, tmpAx3]
        figures.append(tmpFig0)
        p1Figures.append(p1Fig)
        _chan = ""
        if i < 4:
            _chan = "E" 
            _chan = _chan + str(i)
        if i >3 and i < 8:
            _chan = "W" 
            _chan = _chan + str(i-4)
        if i == 10:
            _chan = "PD" 
        for j in range(0, npars): 
            if not datebool:
                axes[j].errorbar(data_cut[j]['Run'], 
                                 data_cut[j]['ParVal'], 
                                 yerr=data_cut[j]['ParErr'], 
                                 linestyle='None', marker='^',
                                 markersize=marks, label=_chan, color = 'Black')
                xmin, xmax = min(data_cut[j]['Run']), max(data_cut[j]['Run']) 
                axes[0].set_xlim([xmin - 0.5, xmax + 0.5])
                axes[3].set_xlabel('Run Number')
                p1Ax.set_xlim([xmin - 0.5, xmax + 0.5])
                 
            if datebool:
                timelist = getTimeForRunlist(data_cut[j]['Run'])
                axes[j].xaxis.set_major_formatter(dates.DateFormatter("%m/%d/%y"))
                axes[j].errorbar(timelist,
                                 data_cut[j]['ParVal'], 
                                 yerr=data_cut[j]['ParErr'], 
                                 linestyle='None', marker='^',
                                 markersize=marks, label=_chan, color = 'Black')
                axes[3].set_xlabel('Time')
        
            axes[j].set_ylabel("p" + str(j))

        p1Ax.errorbar(data_cut[1]['Run'], 
                             data_cut[1]['ParVal'], 
                             yerr=data_cut[1]['ParErr'], 
                             linestyle='None', marker='^',
                             markersize=marks, label=_chan, color = 'Black')

        axes[3].set_ylabel("$\eta_{\lambda}$")        
        axes[0].set_title(_chan)
        

        p1Ax.set_ylabel("p" + str(1))
        p1Ax.set_title(_chan)
        p1Ax.set_xlabel('Run Number')

    if savebool: 
        for f in range (0, 8):
            outputfile.savefig(figures[f])
            p1file.savefig(p1Figures[f])
    
    outputfile.close()
    p1file.close()
    if showbool:
        plt.show(block=True)     #block=True keeps the plot window open when in interactive mode 
