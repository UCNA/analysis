# Some helper functions for parsing the run log that might
# have general applicability

def identifySourceSegments(runlogname = "/home/saslutsky/UCNA/UCNAReplay_052214/UCNA/Aux/UCNA Run Log 2012.txt"):
    newcycle = 0
    runlist = list()
    f = open(runlogname, "r")
    for line in f:
        # Reset cycle if found first run of cycle
        if newcycle:
            if line.find("*2") > -1:
                print "First Run: " + line[1:6]
                print "----"
                newcycle = 0
                runlist.append(int(line[1:6]))
                
        # identify new cycle
        if line.find("@cycle") > -1:
            if line.find("#") < 0:
                newcycle = 1
                print "----"
                print line
                
            
    return runlist

def makeVLinesforSegments(axisIn, startrun = 0):
    segmentlist = identifySourceSegments()
    vlinelist = [run - 1 for run in segmentlist if run > startrun]  # otherwise vlines overlap markers :-p                                       
    axisIn.vlines(vlinelist, 0.0, 2.0, linestyles = 'dashed')

    return axisIn

#def identifySegmentofRun(runnum):
#    print "test"
#    return 0
