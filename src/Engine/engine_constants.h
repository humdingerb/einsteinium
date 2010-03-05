/*constants.h
	Any global constants should be declared here
*/
#ifndef EE_CONSTANTS_H
#define EE_CONSTANTS_H

//Einsteinium_engine application signature
#define e_engine_sig "application/x-vnd.Einsteinium_Engine"
#define e_app_attr_filetype "document/x-E-app-attr"
//Main directory for all Einsteinium settings
#define e_settings_dir "Einsteinium"
#define e_settings_app_dir "Applications"
//settings file
#define ee_settings_file "engine_settings"

//XML Text
#define EE_XMLTEXT_ROOT_NAME "einsteinium_engine"
#define EE_XMLTEXT_CHILD_NAME_INCLUSION "list_inclusion"
#define EE_XMLTEXT_PROPERTY_INCLUSION_DEFAULT "default"
#define EE_XMLTEXT_VALUE_INCLUDE "include"
#define EE_XMLTEXT_VALUE_PROMPT "prompt"
#define EE_XMLTEXT_VALUE_EXCLUDE "exclude"
#define EE_XMLTEXT_CHILD_NAME_RANK "rank_scales"
#define EE_XMLTEXT_PROPERTY_LAUNCHES "launches"
#define EE_XMLTEXT_PROPERTY_FIRSTLAUNCH "first_launch"
#define EE_XMLTEXT_PROPERTY_LASTLAUNCH "last_launch"
#define EE_XMLTEXT_PROPERTY_INTERVAL "interval"
#define EE_XMLTEXT_PROPERTY_RUNTIME "run_time"

//Attribute names
#define ATTR_SESSION_NAME "EIN:SESSION"
#define ATTR_APP_PATH_NAME "EIN:APP_PATH"
#define ATTR_APP_SIG_NAME "EIN:APP_SIG"
#define ATTR_APP_FILENAME_NAME "EIN:APP_FILENAME"
#define ATTR_LAUNCHES_NAME "EIN:LAUNCHES"
#define ATTR_SCORE_NAME "EIN:SCORE"
#define ATTR_IGNORE_NAME "EIN:IGNORE"
#define ATTR_LAST_LAUNCH_NAME "EIN:LAST_LAUNCH"
#define ATTR_FIRST_LAUNCH_NAME "EIN:FIRST_LAUNCH"
#define ATTR_LAST_INTERVAL_NAME "EIN:LAST_INTERVAL"
#define ATTR_TOTAL_RUNTIME_NAME "EIN:TOTAL_RUN_TIME"

//Defaults
#define DEFAULTVALUE_LAUNCHES 5

// Other text
#define EE_SHELFVIEW_NAME "ee_shelfview"

//Indexes
enum scales_index
{	LAUNCH_INDEX=0,
	FIRST_INDEX,
	LAST_INDEX,
	INTERVAL_INDEX,
	RUNTIME_INDEX
};

//Messages
enum engine_messages
{	E_PRINT_RANKING_APPS='Epra',
	E_REPLY_RANKING_APPS='Erra',
	E_RANKING_APPS_REPLY='Erar',
	E_PRINT_RECENT_APPS='Eprc',
	E_REPLY_RECENT_APPS='Errc',
	E_RECENT_APPS_REPLY='Ercr',
	E_UPDATE_QUARTILES='Eupq',
	E_RESCAN_DATA_FILES,
	E_UPDATE_SCORES,
	E_SET_IGNORE_ATTR,
	E_SHELFVIEW_OPENPREFS,
	E_SHELFVIEW_OPEN,
	// Subscriber messages
	E_SUBSCRIBE_RANKED_APPS,
	E_SUBSCRIBER_UPDATE_RANKED_APPS
};
enum attr_file_methods
{	UPDATE_ATTR_SCORE=0,
	RESCAN_ATTR_DATA,
	CREATE_APP_LIST
};
enum quartile_indexes
{	Q_SCORE_INDEX=0,
	Q_FIRST_LAUNCH_INDEX=5,
	Q_LAST_LAUNCH_INDEX=10,
	Q_LAST_INTERVAL_INDEX=15,
	Q_LAUNCHES_INDEX=20,
	Q_TOTAL_RUN_TIME_INDEX=25
};

#endif
