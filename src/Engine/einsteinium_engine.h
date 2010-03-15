/*einsteinium_engine.h
	Main engine application object definitions
*/
#ifndef EINSTEINIUM_ENGINE_H
#define EINSTEINIUM_ENGINE_H
#include <AppKit.h>
#include <Deskbar.h>
#include <StorageKit.h>
#include <stdio.h>
#include "engine_constants.h"
#include "methods.h"
#include "AppStats.h"
#include "AppAttrFile.h"
#include "EESettingsFile.h"
#include "EEShelfView.h"

struct Subscriber {
	int16 count;
	BMessenger messenger;
};
//Einsteinium Engine is an inherited subclass of BApplication to
//enabled the use of BMessages to the system and other apps
class einsteinium_engine : public BApplication
{public:
					einsteinium_engine();
//					~einsteinium_engine();
	virtual bool	QuitRequested();//Clean up data
	virtual void	ReadyToRun();//First function to run
	virtual void	ArgvReceived(int32, char**);//command line options
	virtual void	MessageReceived(BMessage*);//received BMessages
	const double*	GetQuartilesPtr() const { return quartiles; }
	const int*		GetScalesPtr() const { return settingsFile->GetScales(); }
	const time_t	GetSession() { return ee_session; }
private:
	BPath			settingsDirPath;
	EESettingsFile	*settingsFile;
	bool			watchingRoster;//will be true when roster is being watched
	BMessageRunner	*quartileRunner;
	const time_t	ee_session;//current session number
	double			quartiles[30];
	BList			appsList, subscribersList;
	BQuery			rankQuery;
	BView			*shelfView;
	int32			shelfViewId;
	void			SendListToSubscriber(BList *appStatsList, Subscriber *subscriber);
	void			PopulateAppRankMessage(BList *appStatsList, BMessage *message, int count);
//	void			DoRankQuery();
	void			forEachAttrFile(int);
	void			rescanAllAttrFiles();
	void			rescanAttrFile(BEntry*);
	void			updateAllAttrScores();
	void			updateAttrScore(BEntry*);
	void			createAppList();//Created ordered list of apps
	void			EmptyAppStatsList(BList &list);
	void			updateQuartiles();
	template < class itemType >
	void			getQuartiles(itemType (*)(AppStats*), BList&, double*);
	void			writeQuartiles(BFile*, double*);
	void			writeQuartilesNamed(BFile*, double*, const char*);
//	bool			readQuartiles(BFile*, double*);
//	bool			readQuartilesNamed(BFile*, double*, const char*);
};


template < class itemType >
void deleteList(BList&, itemType*);

#endif
