# pd_led_linearity.py
# Author: Simon Slutsky
# Created: 10/21/2013
# 
# Plot fitted gains output from pd_led_pmt_batch file 
# Useful syntax: carp = data[data['Channel'] == 4]

import numpy as np
import matplotlib.pyplot as plt
from pylab import *
from matplotlib.backends.backend_pdf import PdfPages
from math import sqrt
#from ROOT import TCanvas

def read_PD_LED_response():
    imagedir = '/data4/saslutsky/PulserComp/imagesLEDDebug'
    filename = 'GainResults.txt'
    path = imagedir + "/" +filename
    
    outputfilename = 'GainsTogether.pdf'
    outputpath = imagedir + "/" +  outputfilename

    # import data. 
    # TODO: improve this to read the header string from the file
    data = np.genfromtxt(path, skip_header=1, 
                         delimiter = "\t", 
                         names = ['Run','A', 'AErr','Mu',
                                  'MuErr','Sigma','SigmaErr', 'Chi2'])
    data.sort(order = 'Run') 

    return data

if __name__ == "__main__":

    saveBool = 0
    plt.ion() #turn on interactive mode
    rcParams['figure.figsize'] = 10, 10     #Set default fig size
    
    data = read_PD_LED_response()
        
    means = data['Mu']
    meanErrs = data['MuErr']
    average = sum(means[:]/len(means))
    
    # calculate "gain" arbitrarily normalized to 260 
    gains = [m/average for m in means]
    gainsErr = [mErr/average for mErr in meanErrs]

    #fig = plt.figure()
    fig0, (ax0) = plt.subplots()
    fig1, (ax1) = plt.subplots()
    fig2, (ax2) = plt.subplots()
    fig3, (ax3) = plt.subplots()
    figChi, (ax4) = plt.subplots()
    figures = [fig0, fig1, fig2, fig3, figChi]
    
    plt.rc('axes', color_cycle=['r', 'g', 'b', 'y'])

    ax0.set_title("A")
    ax1.set_title("Mean Response")
    ax2.set_title("Relative Gain")    
    ax3.set_title("Sigma")
    ax4.set_title("Chi2")

    ax0.set_xlabel('Run Number')
    ax1.set_xlabel('Run Number')
    ax2.set_xlabel('Run Number')
    ax3.set_xlabel('Run Number')
    ax4.set_xlabel('Run Number')

    marks = 4
    ax0.errorbar(data['Run'], data['A'], yerr=data['AErr'],
                 linestyle='None', marker='o', markersize=marks)
    ax1.errorbar(data['Run'], data['Mu'], yerr=data['MuErr'],
                 linestyle='None', marker='o', markersize=marks)
    ax2.errorbar(data['Run'], gains, gainsErr,
                 linestyle='None', marker='o', markersize=marks)
    ax3.errorbar(data['Run'], data['Sigma'], yerr=data['SigmaErr'],
                 linestyle='None', marker='o', markersize=marks)
    ax4.errorbar(data['Run'], data['Chi2'], 
                 linestyle='None', marker='o', markersize=marks)
    
    ax0.set_xlim([20800, 24000])
    ax1.set_xlim([20800, 24000])
    ax2.set_xlim([20800, 24000])
    ax3.set_xlim([20800, 24000])
    ax4.set_xlim([20800, 24000])

 #   ax0.set_ylim([0, 60])
#    ax1.set_ylim([0, 18])
  #  ax2.set_ylim([0.85, 1.1])
   # ax3.set_ylim([0.85, 1.05])
   # ax4.set_ylim([-0.0005, 0.0])
   # ax5.set_ylim([-0.00015, -0.00008])
   # ax8.set_ylim([0, 8])

    if saveBool:
        outputfile = PdfPages(outputpath)
        for f in range(0, len(figures)):
            outputfile.savefig(figures[f])
            outputfile.close()

    plt.show(block=True)     #block=True keeps the plot window open when in interactive mode 



# too hard to do it manually

#run = data['Run']
#_run0 = np.where(data['Channel'] == 0, run, 0]
#run0 = _run0[_run0.nonzero() #strip zeroes from the array

#p0 = data['p0']
#_p00 = np.where(data['Channel'] == 0, p0, 0]
#p00 = _p00[_p00.nonzero()

#for i in range (0, len(run)):
#    if y[i] > 1e8 or y[i] < -1e2:
#        x[i] = 0
#        y[i] = 0
#        print i
