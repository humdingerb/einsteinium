/*AppAttrFile.h
	Functions for opening, initializing, changing, and writing attribute data
	for applications
*/
#include "AppAttrFile.h"


//AttrEntry is the entry for the new or existing AppAttrFile
//appEntry is the entry for the actual application
AppAttrFile::AppAttrFile(const char *appSig, const BEntry *appEntry)
	:BFile()//create and/or open file
	/*,dirty_data(false)*///when file_data changes
	,new_session(false)
	,appStats()
	,app_entry(*appEntry)//Entry for actual application
	,E_ignore(false)
	,EE_session(0)
	,session(0)
{
	BPath appPath(appEntry), appAttrPath;//create path for application and attribute file
	find_directory(B_USER_SETTINGS_DIRECTORY, &appAttrPath);
	appAttrPath.Append("Einsteinium/Applications");
	appAttrPath.Append(appPath.Leaf());
	appAttrEntry.SetTo(appAttrPath.Path());//convert path to entry
	uint32 openMode = B_READ_WRITE;//We will be reading and writting the attributes
	bool newfile = false;//flag to indicate if a new file must be created
	printf("Finding appInfo file %s\n", appAttrPath.Path());
	if(!appAttrEntry.Exists())//existing AppAttrFile for app not found
	{	// TODO If the filename changed, need to search all attr files for app_sig
		//But for now...
		printf("Creating %s\n", appAttrPath.Path());
		openMode |= B_CREATE_FILE;//set openMode flag to create the file
		newfile = true;//We'll be creating a new file
	}
	SetTo(&appAttrEntry, openMode);

	if(newfile)
	{
		// Initialize starting attributes and stats
		BNode attrNode(&appAttrEntry);
		BNodeInfo attrNodeInfo(&attrNode);
		attrNodeInfo.SetType(e_app_attr_filetype);//Set MIME type
		appStats.app_sig.SetTo(appSig);
		appStats.app_filename.SetTo(appPath.Leaf());//define app filename
		appStats.app_path.SetTo(appPath.Path());//define app path
		writeAttrValues();

		new_session = true;
		BString text("Einsteinium detected a new application, \"");
		text.Append(appPath.Leaf());
		text.Append("\".  Would you like this app to be included in ranking lists?");
		BMessage *msg = new BMessage(E_SET_IGNORE_ATTR);
		msg->AddString("app_signature", appSig);
		(new BAlert("",text.String(), "Ignore", "Include"))
				->Go(new BInvoker(msg, be_app_messenger));
	}
	// TODO if file exists, verify signature matches to catch applications with identical names


	initData(newfile);
}


AppAttrFile::AppAttrFile(const BEntry *attrEntry)//opening an existing AppAttrFile (read only)
	:BFile(attrEntry, B_READ_ONLY)//open file
	/*,dirty_data(false)*///when file_data changes
	,new_session(false)
	,appStats()
	,app_entry()//Entry for actual application
	,appAttrEntry(*attrEntry)
	,E_ignore(false)
	,EE_session(0)
	,session(0)
{
	initData(false);
}


void AppAttrFile::initData(bool newfile)
{
	EE_session = ((einsteinium_engine*)be_app)->GetSession();

	// Database path
	BPath appAttrPath(&appAttrEntry);
	BString dataPathStr(appAttrPath.Path());
	dataPathStr.Append(".db");
	appDataEntry.SetTo(dataPathStr.String());

	// Read attributes from existing file
	if(!newfile) readAttrValues();
	// New attribute file, look for existing data file, if it exists rescan data
	else if(appDataEntry.Exists()) rescanData();
}

AppAttrFile::~AppAttrFile()
{
	Unset();
}

void AppAttrFile::UpdateAppLaunched()
{
	time_t now = time(NULL);//current time
	//update database
	BPath appDataPath(&appDataEntry);
	Edb_Add_Launch_Time(appDataPath.Path(), EE_session, now);

	// Update the attributes
	if(appStats.launch_count == 0) appStats.first_launch = now;
	appStats.launch_count++;//increment launch count
	if(appStats.last_launch > 0){
		appStats.last_interval = now - appStats.last_launch;//update interval between last two launches
	}
	appStats.last_launch = now;//update last launch time

	//calculate score
	calculateScore();
}

void AppAttrFile::UpdateAppQuit()
{
	time_t now = time(NULL);
	//update database
	BPath appDataPath(&appDataEntry);
	Edb_Add_Quit_Time(appDataPath.Path(), EE_session, now);

	// Update the attributes
	if( (!new_session) && (appStats.last_launch!=0) )
	{	appStats.total_run_time += (now - appStats.last_launch); }

	//calculate score
	calculateScore();
}

void AppAttrFile::calculateScore()
{
	const int *scales = ((einsteinium_engine*)be_app)->GetScalesPtr();
	int launch_scale=scales[0],
		first_scale=scales[1],
		last_scale=scales[2],
		interval_scale=scales[3],
		runtime_scale=scales[4];

	const double *Quart = ((einsteinium_engine*)be_app)->GetQuartilesPtr();
	float launch_val(0), first_launch_val(0), last_launch_val(0), last_interval_val(0),
			total_run_time_val(0);
	if( Quart!=NULL )
	{	launch_val = getQuartileVal(Quart+Q_LAUNCHES_INDEX, appStats.launch_count);
		first_launch_val = getQuartileVal(Quart+Q_FIRST_LAUNCH_INDEX, appStats.first_launch);
		last_launch_val = getQuartileVal(Quart+Q_LAST_LAUNCH_INDEX, appStats.last_launch);
		last_interval_val = getQuartileVal(Quart+Q_LAST_INTERVAL_INDEX, appStats.last_interval);
		total_run_time_val = getQuartileVal(Quart+Q_TOTAL_RUN_TIME_INDEX, appStats.total_run_time);

		int max_scale = 100000;
		// If last interval is zero (only one launch) put quartile value at .5 so that this
		// statistic does not adversly effect the score
		if(appStats.last_interval == 0)
		{
			last_interval_val = .5;
		}
		appStats.score =	int(  max_scale*(launch_val * launch_scale)
					+ max_scale*(first_launch_val * first_scale)
					+ max_scale*(last_launch_val * last_scale)
					+ max_scale*((1 - last_interval_val) * interval_scale/* * -1*/)
						// Need to reverse interval scale, because longer intervals decrease the score
						// TODO or do I change the sorting method to sort descending??
					+ max_scale*(total_run_time_val * runtime_scale));

	}
	writeAttrValues();
}

// Calculate the quartile value of where d lies in the quartile range Q
float AppAttrFile::getQuartileVal(const double *Q, double d)
{//	printf("EIN: quartiles\n");
//	printf("Q4: %f, Q3: %f, Q2: %f, Q1: %f, Q0: %f\n", Q[4], Q[3], Q[2], Q[1], Q[0]);
	int index, index_offset = 0;
	if(d >= Q[0] && d<Q[1]) { index = 0; }
	else if(d >= Q[1] && d<Q[2]) { index = 1; }
	else if(d >= Q[2] && d<Q[3]) { index = 2; }
	else if(d >= Q[3] && d<Q[4]) { index = 3; }
	else if(d == Q[4]) { return 1; }
	// For values that lie outside the current quartile range
	else if(d < Q[0]) { index = 0; }
	else if(d > Q[4])
	{	index = 4;
		index_offset = 1;
		// since there is no Q[5], index offset will modify the calculation below to
		// use Q[3] and Q[4] to find the quartile value range
	}
	else { return .5; }

	return (.25*index + .25*((d - Q[index])/(Q[index+1-index_offset] - Q[index-index_offset])));
}


void AppAttrFile::rescanData()
{
	BPath appDataPath(&appDataEntry);
	Edb_Rescan_Data(appDataPath.Path(), &appStats);
	calculateScore();
}

void AppAttrFile::setIgnore(bool ignore)
{
	E_ignore = ignore;
	WriteAttr(ATTR_IGNORE_NAME, B_BOOL_TYPE, 0, &E_ignore, sizeof(E_ignore));
}


void AppAttrFile::readAttrValues()
{	//Read each attribute.  If attribute isn't found, initialize to proper default value
	attr_info info;
	//E Session
	if(GetAttrInfo(ATTR_SESSION_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_SESSION_NAME, B_INT32_TYPE, 0, &session, sizeof(time_t)); }
	new_session = ( (EE_session != session) || (session==0) );
	//App path
	if(GetAttrInfo(ATTR_APP_PATH_NAME, &info) == B_NO_ERROR)
	{	ReadAttrString(ATTR_APP_PATH_NAME, &appStats.app_path); }
	else
	{	 }// TODO what??
	//get path from application path
	BPath appPath(appStats.app_path.String());//Path to actual application
	app_entry.SetTo(appPath.Path());
	//Read each attribute.  If attribute isn't found, initialize to proper default value
	//Application signature
	if(GetAttrInfo(ATTR_APP_SIG_NAME, &info) == B_OK)
	{	ReadAttrString(ATTR_APP_SIG_NAME, &appStats.app_sig); }
	else
	{	BNode appNode(appPath.Path());//get signature from application node
		if(appNode.GetAttrInfo("BEOS:APP_SIG", &info) == B_OK)
		{	appNode.ReadAttrString("BEOS:APP_SIG", &appStats.app_sig); }
		else printf("Couldn't find app sig for file %s\n", appPath.Path());
	}
	//Displayed name
	//App filename
	if(GetAttrInfo(ATTR_APP_FILENAME_NAME, &info) == B_NO_ERROR)
	{	ReadAttrString(ATTR_APP_FILENAME_NAME, &appStats.app_filename); }
	else
	{	appStats.app_filename.SetTo(appPath.Leaf()); }//get filename from path
	//Number of launches
	if(GetAttrInfo(ATTR_LAUNCHES_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_LAUNCHES_NAME, B_INT32_TYPE, 0, &appStats.launch_count, sizeof(uint32)); }
	//Score
	if(GetAttrInfo(ATTR_SCORE_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_SCORE_NAME, B_INT32_TYPE, 0, &appStats.score, sizeof(int)); }
	//Checksum
	//Class (app_sig, mimeype)
	//Ignore (default, not app specific)
	if(GetAttrInfo(ATTR_IGNORE_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_IGNORE_NAME, B_BOOL_TYPE, 0, &E_ignore, sizeof(E_ignore)); }
	//Owner?? this might need to be app specific
	//Date last activated
	if(GetAttrInfo(ATTR_LAST_LAUNCH_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_LAST_LAUNCH_NAME, B_INT32_TYPE, 0, &appStats.last_launch, sizeof(time_t)); }
	else// TODO if this isn't found use first launch date???  Undecided yet.
	{	}
	//Date first activated
	if(GetAttrInfo(ATTR_FIRST_LAUNCH_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_FIRST_LAUNCH_NAME, B_INT32_TYPE, 0, &appStats.first_launch, sizeof(time_t)); }
	else//first launch should be the same as file creation date
	{	time_t creation_t;
		if(GetCreationTime(&creation_t)==B_OK)
		{	appStats.first_launch = creation_t; }
	//	else printf("Created stat not found\n");
	}
	//Time between last two activations
	if(GetAttrInfo(ATTR_LAST_INTERVAL_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_LAST_INTERVAL_NAME, B_INT32_TYPE, 0, &appStats.last_interval, sizeof(uint32)); }
	else
	{	}
	//Total running time
	if(GetAttrInfo(ATTR_TOTAL_RUNTIME_NAME, &info) == B_NO_ERROR)
	{	ReadAttr(ATTR_TOTAL_RUNTIME_NAME, B_INT32_TYPE, 0, &appStats.total_run_time, sizeof(uint32)); }
	else
	{	}
	//Average daily interval
	//Int_reg
	//daily_int_reg
}

void AppAttrFile::writeAttrValues()
{	//E session
	if(new_session)
	{	WriteAttr(ATTR_SESSION_NAME, B_INT32_TYPE, 0, &EE_session, sizeof(EE_session)); }
	//Application signature
	if(strcmp(appStats.app_sig.String(),"")!=0)
	{	WriteAttrString(ATTR_APP_SIG_NAME, &appStats.app_sig); }
	//Displayed name
	//App filename
	WriteAttrString(ATTR_APP_FILENAME_NAME, &appStats.app_filename);
	//App path
	WriteAttrString(ATTR_APP_PATH_NAME, &appStats.app_path);
	//Number of launches
	WriteAttr(ATTR_LAUNCHES_NAME, B_INT32_TYPE, 0, &appStats.launch_count, sizeof(appStats.launch_count));
	//Score
	WriteAttr(ATTR_SCORE_NAME, B_INT32_TYPE, 0, &appStats.score, sizeof(appStats.score));
	//Checksum
	//Class (app_sig, mimeype)
	//Ignore (default, not app specific)
	WriteAttr(ATTR_IGNORE_NAME, B_BOOL_TYPE, 0, &E_ignore, sizeof(E_ignore));
	//Owner?? this might need to be app specific
	//Date last activated
	WriteAttr(ATTR_LAST_LAUNCH_NAME, B_INT32_TYPE, 0, &appStats.last_launch, sizeof(appStats.last_launch));
	//Date first activated
	WriteAttr(ATTR_FIRST_LAUNCH_NAME, B_INT32_TYPE, 0, &appStats.first_launch, sizeof(appStats.first_launch));
	//Time between last two activations
	WriteAttr(ATTR_LAST_INTERVAL_NAME, B_INT32_TYPE, 0, &appStats.last_interval, sizeof(appStats.last_interval));
	//Total running time
	WriteAttr(ATTR_TOTAL_RUNTIME_NAME, B_INT32_TYPE, 0, &appStats.total_run_time, sizeof(appStats.total_run_time));

	//Average daily interval
	//Int_reg
	//daily_int_reg
}

// TODO change this to a DuplicateStats function, move duplication code to AppStats class
void AppAttrFile::CopyAppStatsInto(AppStats* targetStats)
{
	targetStats->app_sig.SetTo(appStats.app_sig);
	targetStats->app_path.SetTo(appStats.app_path);
	targetStats->app_filename.SetTo(appStats.app_filename);
	targetStats->score = appStats.score;
	targetStats->launch_count = appStats.launch_count;
	targetStats->last_launch = appStats.last_launch;
	targetStats->first_launch = appStats.first_launch;
	targetStats->last_interval = appStats.last_interval;
	targetStats->total_run_time = appStats.total_run_time;
}

