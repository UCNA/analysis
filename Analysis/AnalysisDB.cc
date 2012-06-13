#include "AnalysisDB.hh"
#include <time.h>

AnaResult::AnaResult(const std::string& auth): arid(0), author(auth),
timestamp(time(NULL)), startRun(0), endRun(0), s(BOTH), value(0), err(0), csid(0) { }

Stringmap AnaResult::toStringmap() const {
	Stringmap m;
	m.insert("author",author);
	m.insert("timestamp",timestamp);
	m.insert("startRun",startRun);
	m.insert("endRun",endRun);
	m.insert("evtps",typeSetString());
	m.insert("side",sideWords(s));
	m.insert("value",value);
	m.insert("err",err);
	m.insert("csid",csid);
	return m;
}

std::string AnaResult::typeSetString() const {
	std::string ts;
	for(std::set<EventType>::const_iterator it = etypes.begin(); it != etypes.end(); it++) {
		if(it != etypes.begin()) ts += ",";
		if((*it)<=TYPE_III_EVENT)
			ts += (*it)==TYPE_0_EVENT?"0":(*it)==TYPE_I_EVENT?"I":(*it)==TYPE_II_EVENT?"II":"III";
	}
	return ts;
}


AnalysisDB* AnalysisDB::getADB() {
	static AnalysisDB* ADB = NULL;
	if(!ADB) ADB = new AnalysisDB(getEnvSafe("UCNADBADDRESS"),getEnvSafe("UCNADBUSER"),getEnvSafe("UCNADBPASS"));
	return ADB;
}

unsigned int AnalysisDB::uploadCutSpec(AnaCutSpec& c) {
	sprintf(query,"INSERT INTO cut_spec(energy_min,energy_max,radius,positioning) VALUES (%f,%f,%f,'%s')",
			c.emin,c.emax,c.radius,c.postp == AnaCutSpec::POS_PLAIN?"plain":"rotated");
	execute();
	c.csid = getInsertID();
	return c.csid;
}

unsigned int AnalysisDB::uploadAnaResult(AnaResult& r) {
	sprintf(query,"INSERT INTO analysis_results(author,date,type,source,start_run,end_run,event_type,ana_choice,side,value,err,cut_spec_id) \
			VALUES ('%s',%i,'%s','%s',%i,%i,'%s','%c',%s,%f,%f,%i)",
			r.author.c_str(),
			r.timestamp,
			r.anatp==AnaResult::ANA_ASYM?"Asymmetry":"Counts",
			r.datp==AnaResult::REAL_DATA?"Data":r.datp==AnaResult::G4_DATA?"G4":"Pen",
			r.startRun,
			r.endRun,
			r.typeSetString().c_str(),
			choiceLetter(r.anach),
			dbSideName(r.s),
			r.value,
			r.err,
			r.csid);
	execute();
	r.arid = getInsertID();
	return r.arid;
}

void AnalysisDB::deleteAnaResult(unsigned int arid) {
	sprintf(query,"SELECT cut_spec_id FROM analysis_results WHERE analsysis_results_id = %i",arid);
	execute();
	TSQLRow* r = getFirst();
	if(!r) return;
	int csid = fieldAsInt(r); 
	delete(r);
	
	sprintf(query,"DELETE FROM analysis_results WHERE analsysis_results_id = %i",arid);
	execute();
	
	sprintf(query,"SELECT COUNT(*) FROM analysis_results WHERE cut_spec_id = %i",csid);
	execute();
	assert(r);
	if(!fieldAsInt(r))
		deleteCutSpec(csid);
	delete(r);
}

void AnalysisDB::deleteCutSpec(unsigned int csid) {
	sprintf(query,"DELETE FROM cut_spec WHERE cut_spec_id = %i",csid);
	execute();
}

AnaResult AnalysisDB::getAnaResult(unsigned int arid) {
	//                    0      1    2    3      4         5       6          7          8    9     10  11
	sprintf(query,"SELECT author,date,type,source,start_run,end_run,event_type,ana_choice,side,value,err,cut_spec_id \
			FROM analysis_results WHERE analsysis_results_id = %i",arid);
	TSQLRow* r = getFirst();
	if(!r) {
		SMExcept e("MissingAnaResult");
		e.insert("analysis_results_id",arid);
		throw(e);
	}
	AnaResult a(fieldAsString(r,0));
	a.timestamp = fieldAsInt(r,1);
	a.anatp = fieldAsString(r,2)=="Asymmetry"?AnaResult::ANA_ASYM:AnaResult::ANA_COUNTS;
	a.datp = fieldAsString(r,3)=="Data"?AnaResult::REAL_DATA:fieldAsString(r,4)=="G4"?AnaResult::G4_DATA:AnaResult::PEN_DATA;
	a.startRun = fieldAsInt(r,4);
	a.endRun = fieldAsInt(r,5);
	std::vector<std::string> tps = split(fieldAsString(r,6));
	for(std::vector<std::string>::iterator it = tps.begin(); it != tps.end(); it++)
		a.etypes.insert((*it)=="0"?TYPE_0_EVENT:EventType(TYPE_0_EVENT+it->size()));
	a.anach = AnalysisChoice(fieldAsString(r,7)[0]-'A'+1);
	a.s = fieldAsString(r,8)=="East"?EAST:WEST;
	a.value = fieldAsFloat(r,9);
	a.err = fieldAsFloat(r,10);
	a.csid = fieldAsInt(r,11);
	delete(r);
	return a;
}

AnaCutSpec AnalysisDB::getCutSpec(unsigned int csid) {
	AnaCutSpec c;
	sprintf(query,"SELECT energy_min,energy_max,radius,positioning FROM cut_spec WHERE cut_spec_id = %i",csid);
	TSQLRow* r = getFirst();
	if(!r) {
		SMExcept e("MissingCutSpec");
		e.insert("cut_spec_id",csid);
		throw(e);
	}
	c.emin = fieldAsFloat(r,0);
	c.emax = fieldAsFloat(r,1);
	c.radius = fieldAsFloat(r,2);
	c.postp = fieldAsString(r,3)=="plain"?AnaCutSpec::POS_PLAIN:AnaCutSpec::POS_ROTATED;
	delete(r);
	return c;
}

std::vector<AnaResult> AnalysisDB::findMatching(const AnaResult& A) {
	std::string qry = "SELECT analsysis_results_id FROM analysis_results WHERE author = '"+A.author+"'";
	qry += " AND type = "; qry += (A.anatp==AnaResult::ANA_ASYM?"'Asymmetry'":"'Counts'");
	qry += " AND ana_choice = '"+ctos(choiceLetter(A.anach))+"'";
	qry += " AND side = "; qry += dbSideName(A.s);
	qry += " AND event_type = '"+A.typeSetString()+"'";
	if(A.startRun)
		qry += " AND start_run = "+itos(A.startRun);
	if(A.endRun)
		qry += " AND end_run = "+itos(A.endRun);
	sprintf(query,"%s",qry.c_str());
	execute();
	std::vector<unsigned int> arids;
	while(TSQLRow* r = res->Next()) {
		arids.push_back(fieldAsInt(r,0));
		delete(r);
	}
	std::vector<AnaResult> v;
	for(unsigned int i=0; i<arids.size(); i++)
		v.push_back(getAnaResult(arids[i]));
	return v;
}
