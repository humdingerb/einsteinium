/* SettingsWindow.h
 * Copyright 2013 Brian Hill
 * All rights reserved. Distributed under the terms of the BSD License.
 */
#ifndef EINSTEINIUM_LAUNCHER_SETTINGSSWINDOW_H
#define EINSTEINIUM_LAUNCHER_SETTINGSSWINDOW_H

#include <InterfaceKit.h>
#include <Application.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <stdio.h>
#include <stdlib.h>
#include "launcher_constants.h"
#include "SettingsView.h"
#include "LauncherExclusionsView.h"
#include "LauncherRankingsView.h"


class SettingsWindow : public BWindow {
public:
							SettingsWindow(AppSettings* settings, ScaleSettings* scales,
											BMessage *appExclusions);
	virtual	bool			QuitRequested();
	virtual void			MessageReceived(BMessage*);
			void			Show(BRect);
			void			SelectTab(uint32 which);
//			AppSettings		GetAppSettings();
			void			PopulateAppSettings(AppSettings *settings);
			ScaleSettings	GetScaleSettings();
			BMessage		GetAppExclusions();
			void			SetAppExclusions(BMessage *exclusionsList);

private:
	BTabView				*fTabView;
	SettingsView			*fLayoutView;
	LauncherRankingsView	*fRankingView;
	LauncherExclusionsView	*fExclusionsView;
};

#endif
