# Run Beta Spectrum Study over all Beta runs
# typical command: 
# ./BetaEnergyScaleStudy /data4/saslutsky/OfficialReplayData_06_2014/hists/spec_23171.root
# use standard output to dump to a text file

#### USAGE: python BetaEnergyScaleBatch.py >>  txt.txt

## Simon Slutsky 06/09/2014

import os
import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit
import sys
sys.path.append('/home/saslutsky/UCNA/UCNAReplay_052214/UCNA/Scripts/LEDPulser/PD_LED_Analysis')

from pd_led_pmt_gain import *             # make LED corrections
from led_bi_compare import *
from RunLogUtilities import *

def createBetaRunList(filename):
    sourcerunlist = list()
    f = open(filename, "r")
    beta_types = ["A2","A5","A7","A10","B2","B5","B7","B10"]
    for line in f:     
        for beta_type in beta_types:
            if line.find(beta_type) > 0:
                if line[0] == "*":       # only take "good" runs
                    run = line[1:6]      # parse run list
                    sourcerunlist.append(run)
    
    return sourcerunlist

def createBetaRunCycleList(filename):
    # creates list of lists, a list of run #s for each "@cycle"
    f = open(filename, "r")
    beta_types = ["A2","A5","A7","A10","B2","B5","B7","B10"]

    cyclelist = list()
    indivcyclelist = list()
    cycleindex = -1
    for line in f:
        if line.find("@cycle") > -1:    # cycle over. Save old cycle and reset
     #       print "Cycle " + str(cycleindex) + ":"
     #       print indivcyclelist
            cycleindex += 1
            cyclelist.append(indivcyclelist)
            indivcyclelist = list()
        for beta_type in beta_types:
            if line.find(beta_type) > 0:
                if line[0] == "*":       # only take "good" runs
                    run = line[1:6]      # parse run list
                    indivcyclelist.append(run)
                    
  #  print "Cycle " + str(cycleindex) + ":"
  #  print indivcyclelist
    cyclelist.append(indivcyclelist)     # get the last cycle
    cyclelist = [x for x in cyclelist if x]   # clean out empty arrays
    return cyclelist

def getSTDDevByCycle_forTube(filename, tubeIn, doLEDcorr = 0, norm = 1):
    cyclelist = createBetaRunCycleList("../../../Aux/UCNA Run Log 2012.txt")
    BetaData = np.genfromtxt(filename, delimiter = "\t", 
                         names = ['Run', 'e0', 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7'])
    LEDData = ReadLEDFile()

    endpointslist = list()
    stddevlist = list()
    runlist = BetaData['Run']
    endpoints = BetaData['e' + str(tubeIn)]
    # make corrections by tube
    if doLEDcorr > 0:
        corrdata = makeLEDcorr(tubeIn, BetaData, LEDData)
        runlist   = np.asarray( zip(*corrdata)[0] )
        endpoints = np.asarray( zip(*corrdata)[1] )
    if norm > 0:
        endpoints = np.asarray( normalize_array_to_avg(endpoints) ) 
    
    # get endpoints for each cycle
    for k in range(len(cyclelist)):       # iterate over run cycles
        print "Cycle " + str(k)
        endpoints_in_cycle = list()
        for cyclerun in cyclelist[k]:     # iterate over runs in a given cycle
            cyclerunindex = np.where(runlist==int(cyclerun))[0]  #array returns 2 values, take 1st
        #                print cyclerunindex
            if cyclerunindex:   # check if run is still on runlist after LEDcorr
                endpoints_in_cycle.append(endpoints[cyclerunindex])
        stddev = findNormedSTDDev(endpoints_in_cycle)
        stddevlist.append(stddev)
        print stddev
        #endpointslist.append(endpoints_in_cycle) # full list of endpoints for all cycle
        
    return stddevlist

def compareSTDDevs(filename, tubeIn):
#    uncorr_list = getSTDDevByCycle_forTube(filename, tubeIn, 0, 1)
#    corr_list = getSTDDevByCycle_forTube(filename, tubeIn, 1, 1)
    uncorr_list = getSTDDevByCycle_forTube(filename, tubeIn, 0, 0)
    corr_list = getSTDDevByCycle_forTube(filename, tubeIn, 1, 0)

    diff = [u - c for u, c in zip(uncorr_list, corr_list)]

    return diff
            
def findSTDDev(arrayIn):
    l = len(arrayIn)
    m = mean(arrayIn)
    aa = [(a-m)*(a-m) for a in arrayIn]
    s = sum(aa)
    if l > 1:
        std = sqrt( s/(l-1) )
    else: std = -1
        
    return std

def findNormedSTDDev(arrayIn):
    stddev = findSTDDev(arrayIn)
    normedstd = stddev/mean(arrayIn)
    return normedstd

def index_value_utility(array, searchvalue):
    # small utility to handle error when searching array for value
    try:
        index_value = array.index(searchvalue)
    except ValueError:
        index_value = -1

    return index_value

def getBetaData(filename = "/home/saslutsky/UCNA/UCNAReplay_052214/UCNA/Scripts/LEDPulser/BetaSpectrumFitting/BetaLinEndpoints.txt"):
    data = np.genfromtxt(filename, delimiter = "\t", 
                         names = ['Run', 'e0', 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7'])
    return data
    
def makeBetaEndpointPlot(tubeIn, retarrays = 1, norm = 1, truncate = 1, filename = "/home/saslutsky/UCNA/UCNAReplay_052214/UCNA/Scripts/LEDPulser/BetaSpectrumFitting/BetaLinEndpoints.txt", plotit = 1):
    data = getBetaData(filename)
    runlist = data['Run']
    endpoints = data['e' + str(tubeIn)]
    
    if truncate:
        trunclists = truncate_runs_before(runlist, endpoints, 20976.0)
        runlist   = trunclists[0]
        endpoints = trunclists[1]
    if norm:
        endpoints = normalize_array_to_avg(endpoints)
    
    #    pltlabel = ("W" if tubeIn/4 else "E") + str(tubeIn%4) 
    if plotit: 
        plt.errorbar(runlist, endpoints, linestyle = "None", 
                     marker = ('v' if tubeIn/4 else 'o'), markersize = 6)
#    plt.errorbar(runlist, endpoints, linestyle = "None", marker = ('v' if tubeIn/4 else 'o'),
#                 markersize = 6, label = pltlabel )
    ax = plt.gca()

    if retarrays:        # return the actual data
        return runlist, endpoints
    else:                # return a plot
        return ax

    return -1

def truncate_runs_before(runlist, endpointlist, trunc_run = 20976.0):
    if len(runlist) != len(endpointlist):
        print "Incommensurate arrays in truncate_runs_before"
        return -1

    truncrunlist = list()
    truncendpointlist = list()
    for i in range(len(runlist)):
#        print runlist[i]
        if float(runlist[i]) >= trunc_run:
#            print runlist[i] - trunc_run
            truncrunlist.append(float(runlist[i]))
            truncendpointlist.append(float(endpointlist[i]))
    return truncrunlist, truncendpointlist
    

def makeAllBetaEndpointPlots(filename, doLEDcorr = 0, norm = 1, showb=0, lines=1):
    data = np.genfromtxt(filename, delimiter = "\t", 
                  names = ['Run', 'e0', 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7'])
    LEDData = ReadLEDFile()

    fig = plt.figure()
    plt.subplot(111)
    colors = ('b', 'g', 'r', 'c', 'm', 'y', 'k')
    runlist = data['Run']
    for i in range(8):
        endpoints = data['e' + str(i)]
        if doLEDcorr > 0:
            corrdata = makeLEDcorr(i, data, LEDData)
            runlist = zip(*corrdata)[0]
            endpoints = zip(*corrdata)[1]
        if norm > 0:
            endpoints = normalize_array_to_avg(endpoints)
        pltlabel = ("W" if i/4 else "E") + str(i%4) 
        plt.errorbar(runlist, endpoints, color = colors[i%4], linestyle = "None", 
                     marker = ('v' if i/4 else 'o') ,  markersize = 6, label = pltlabel )
        ax = plt.gca()
    ax.set_xlabel("Run Number")
    rawstring = "Beta Linearized Endpoints"
    corrstring = "LED Corrected Beta Linearized Endpoints"
    plt.legend(title = rawstring if not doLEDcorr else corrstring, loc = 2)
    ax.set_ylabel(rawstring if not doLEDcorr else corrstring + " (ADC Counts)")
    
    if lines:
        segmentlist = identifySourceSegments()
        vlinelist = [run - 1 for run in segmentlist]  # otherwise vlines overlap markers :-p
        ax.vlines(vlinelist, 0.0, 2.0, linestyles = 'dashed')
        
#    ax.set_ylim(0, 2000)
#    ax.set_xlim(20400, 23200)
    if showb:
        plt.show()
    
    return fig

def linear(x, m, b):
    return (m * x) + b

def fitToLine(xdata, ydata):
    popt, pcov = curve_fit(linear, np.asarray(xdata), np.asarray(ydata), p0 = [-1e-2, 1])
    return popt, pcov

def convertToList(arrayIn):
    arrayOut = list()
    for i in range(len(arrayIn)):
        arrayOut.append(arrayIn[i][0])
    return arrayOut

def truncatelists(runlist, array2, cutoff):
    #    truncate lists before a cutoff
    if len(runlist) == len(array2):
        trunc_runs = list()
        trunc_array = list()
        for i in range(len(runlist)):
            if runlist[i] > cutoff:
                trunc_runs.append(runlist[i])
                trunc_array.append(array2[i])
        return trunc_runs, trunc_array
    else:
        print "Incommensurate lists"
        return -1

def plotLinFit(fitres, colorIn):
    xdata = np.linspace(21300, 23200, 50)
    ydata = linear(xdata, fitres[0], fitres[1])
    plt.errorbar(xdata, ydata, color = colorIn, linewidth = 4)
    return 0
    
def makeCompareBetaEndpointPlots(filename, tubeIn, showb=0, norm=1, onlyuncorr=0):
    data = np.genfromtxt(filename, delimiter = "\t", 
                  names = ['Run', 'e0', 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7'])
    LEDData = ReadLEDFile()

    fig = plt.figure()
    plt.subplot(111)
    runlist = data['Run']
    endpoints = data['e' + str(tubeIn)]
    if norm == 1:
        endpoints = normalize_array_to_avg(endpoints)
    pltlabel = ("W" if tubeIn/4 else "E") + str(tubeIn%4) 
    plt.errorbar(runlist, endpoints, color = 'b', linestyle = "None", 
                     marker = 'o' ,  markersize = 5, label =  pltlabel + ' uncorr')
    if onlyuncorr == 1:
        return fig
    
    print "Fitting uncorr"
    #endpoints_array = convertToList(endpoints)
    trunc_runs, trunc_endpoints = truncatelists(runlist, endpoints, 21300)
#    fitres = fitToLine(runlist, endpoints_array)
    fitres, fitcov = fitToLine(trunc_runs, trunc_endpoints)
    print " ---------------------------------- " 
    print fitres, fitcov
    print " ---------------------------------- " 
    plotLinFit(fitres, 'c')
    ax = plt.gca()
    

    # make corrections
    corrdata = makeLEDcorr(tubeIn, data, LEDData)
    runlist = zip(*corrdata)[0]
    endpoints = zip(*corrdata)[1]
    if norm == 1:
        endpoints = normalize_array_to_avg(endpoints)
    plt.errorbar(runlist, endpoints, color = 'r', linestyle = "None", 
                 marker = 'o' ,  markersize = 5, label = pltlabel + ' corr' )
    
    print "Fitting corr"
    endpoints_list = convertToList(endpoints)
    trunc_runs, trunc_endpoints = truncatelists(runlist, endpoints_list, 21300)
    fitrescorr, fitcovcorr = fitToLine(trunc_runs, trunc_endpoints)
    print " ---------------------------------- " 
    print fitrescorr, fitcovcorr
    print " ---------------------------------- " 
    plotLinFit(fitrescorr, 'm')

    ax.set_xlabel("Run Number")
    ax.set_ylim([0.5, 1.5])
#    rawstring = "Beta Linearized Endpoints"
#    corrstring = "LED Corrected Beta Linearized Endpoints"
    plt.legend(loc = 2)
    ax.set_ylabel("Normalized ADC Counts")
    
    ax = plt.gca()
#    ax.text(0.4, 0.05, "Slopes:\n uncorr = " + str(fitres[0]) + "\n corr = " + str(fitrescorr[0]),
    ax.text(0.4, 0.05, "Slopes:\n uncorr = " + '%s' % float('%.2g' % fitres[0]) + "\n corr = " + '%s' % float('%.2g' % fitrescorr[0]),  transform=ax.transAxes, color='k', fontsize=18)
    f = open("./Figures_Summary/fitfile.txt", "a")
    f.write(pltlabel + "\t" + '%s' % float('%.2g' % fitres[0]) + "\t" + '%s' % float('%.2g' % fitrescorr[0]) + "\n")
    f.close()
    
    if showb:
        plt.show()
    
    return fig

def compareAllTubesEndpointPlots(filename, showb=0, save = 0):
    for i in range(8):
        makeCompareBetaEndpointPlots(filename, i, showb)
        if save:
            plt.savefig("./Figures_Summary/compareTubes_" + str(i) + "_fits.png")
    return 0

def testLEDcorr(tubeIn, filename = "BetaLinEndpoints.txt"): 
#compare LED corr on BetaData with and without PD correction
    LEDData = ReadLEDFile()
    BetaData = np.genfromtxt(filename, delimiter = "\t", 
                             names = ['Run', 'e0', 'e1', 'e2', 'e3', 'e4', 'e5', 'e6', 'e7'])

    corr   = makeLEDcorr(tubeIn, BetaData, LEDData, 1)
    uncorr = makeLEDcorr(tubeIn, BetaData, LEDData, 0)

    corr_runs   = zip(*corr)[0]
    corr_vals   = zip(*corr)[1]
    corr_vals   = normalize_array_to_avg(corr_vals)
    uncorr_runs = zip(*uncorr)[0]
    uncorr_vals = zip(*uncorr)[1]
    uncorr_vals = normalize_array_to_avg(uncorr_vals)
    
    f, (ax1, ax2) = plt.subplots(2)
    
    fig_corr = ax1.errorbar(corr_runs, corr_vals, color = 'r', linestyle = "None", 
                 marker = 'o' ,  markersize = 5, label = 'corr' )
    fig_uncorr = ax2.errorbar(uncorr_runs, uncorr_vals, color = 'b', linestyle = "None", 
                 marker = 'o' ,  markersize = 5, label = 'uncorr' )

    ax1.set_ylim(0.5, 1.5)
    ax2.set_ylim(0.5, 1.5)
    
    plt.show()

    return 0

def testLEDPDcorr(tubeIn):   # compare LED Data, with and without PD correction
    LEDData = ReadLEDFile()
    LEDData_tube = LEDData[LEDData['tube'] == tubeIn] # Select data for given tube
    LEDData_tube_corr = process_LED_PD_corr(LEDData_tube)
    
    fig, (ax1, ax2) = plt.subplots(2)
    ax1.errorbar( LEDData_tube_corr['Run'], LEDData_tube_corr['Mu'], color = 'r', linestyle = "None", 
                 marker = 'o' ,  markersize = 5, label = 'corr' )
    ax2.errorbar( LEDData_tube['Run'], LEDData_tube['Mu'], color = 'b', linestyle = "None", 
                 marker = 'o' ,  markersize = 5, label = 'uncorr' )
    plt.show()
    return fig


def makeLEDcorr(tubeIn, BetaData, LEDData, PDcorr = 1):
    # divide Beta Data by LEDData for tube.
    LEDData_tube = LEDData[LEDData['tube'] == tubeIn] # Select data for given tube
    if PDcorr:
        LEDData_tube = process_LED_PD_corr(LEDData_tube)    # from led_bi_compare.py
    mutual_runs = list()
    corrected_endpoints = list()
    beta_runlist = BetaData['Run']
    for run in beta_runlist:
        if run > 20400:
            LED_response = LEDData_tube[LEDData_tube['Run'] == run]['Mu']
            Beta_endpoint = BetaData[BetaData['Run'] == run]['e' + str(tubeIn)]
            if abs(LED_response) > 0:
                corr = Beta_endpoint/LED_response
                corrected_endpoints.append(corr)
                mutual_runs.append(run)
#            else:
#                print "LED_response 0 for run " + str(run)
                
    retdata = zip(mutual_runs, corrected_endpoints)
    return retdata

def processKurieFit(runnum):
  basestring  = "/home/saslutsky/UCNA/UCNAReplay_052214/UCNA/UCNAnalyzer"
  runstring   =  basestring + " k " + str(runnum) + " x"
#  print runstring
  os.system(runstring)

  return 0

def processAllBetaKurieFits(runlist):
    for run in runlist:
        processKurieFit(run)

    return 0

def clean_up_txt(txtfile):
    readfile = open(txtfile, 'r')
    outfile = open("BetaKurieEndpoints_clean.txt", 'w')
    print readfile
    for line in readfile:
        if line == "\n":
            continue
        if line.find(">") > -1:
            continue
#        print line[0:5]
        outfile.write(line)
    readfile.close()
    outfile.close()
    return readfile
    
if __name__ == "__main__":
    basepath = "/home/saslutsky/UCNA/UCNAReplay_052214/UCNA/"
    filepath = "/Aux/UCNA Run Log 2012.txt"
    print sys.argv[1]

    sourcerunlist = createBetaRunList(basepath+filepath)
        
    if sys.argv[1] == '0':
        
        basecmdstring = "./BetaEnergyScaleStudy /data4/saslutsky/OfficialReplayData_06_2014/hists/spec_"
        #    basecmdstring  = "./pd_led_pmt_analysis "
        
        for run in sourcerunlist:
            cmdstring = basecmdstring + str(run) + ".root"
            #        cmdstring = basecmdstring + str(run)
            
            print cmdstring
            #os.system(cmdstring)

    if sys.argv[1] == '1':
        processAllBetaKurieFits(sourcerunlist)


