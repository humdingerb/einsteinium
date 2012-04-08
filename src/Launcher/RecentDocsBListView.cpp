/* RecentDocsBListView.cpp
 * Copyright 2012 Brian Hill
 * All rights reserved. Distributed under the terms of the BSD License.
 */
#include "RecentDocsBListView.h"

RecentDocsBListView::RecentDocsBListView(BRect size)
	:
//	BOutlineListView(size)
	BOutlineListView(size, "Files List", B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES),
	fMenu(NULL),
	fGenericSuperItem(NULL),
	fLastRecentDocRef(),
	isShowing(false)
{
/*	fMenu = new LPopUpMenu(B_EMPTY_STRING);
	fTrackerMI = new BMenuItem("Show in Tracker", new BMessage(SHOW_IN_TRACKER));
	fSettingsMI = new BMenuItem("Settings" B_UTF8_ELLIPSIS, new BMessage(SHOW_SETTINGS));
	fMenu->AddItem(fTrackerMI);
	fMenu->AddSeparatorItem();
	fMenu->AddItem(fSettingsMI);*/
}

/*
void
RecentDocsBListView::AttachedToWindow()
{
	LListView::AttachedToWindow();
//	SetEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS);

	fMenu->SetTargetForItems(this);
	fSettingsMI->SetTarget(be_app);
}*/


void
RecentDocsBListView::MessageReceived(BMessage* msg)
{
	switch(msg->what)
	{
		case B_MOUSE_WHEEL_CHANGED:
		{

		//	printf("Mouse wheel\n");
			// Prevent scrolling while the menu is showing
			if(fMenu && fMenu->IsShowing())
				break;
			// If this list view isn't currently selected redirect message
			// to the main view to handle (fixes bug in Haiku R1A3)
			if(!isShowing)
			{
				msg->what = EL_REDIRECTED_MOUSE_WHEEL_CHANGED;
				Parent()->MessageReceived(msg);
				break;
			}
			HandleMouseWheelChanged(msg);
			break;
		}
		case EL_SHOW_IN_TRACKER:
		{
			int32 selectedIndex = CurrentSelection();
			if(selectedIndex < 0)
				break;
			uint32 modifier = modifiers();
			DocListItem *selectedItem = (DocListItem*)ItemAt(selectedIndex);
			selectedItem->ShowInTracker();
			_HideApp(modifier);
			break;
		}
		default:
			BOutlineListView::MessageReceived(msg);
	}
}


void
RecentDocsBListView::MouseDown(BPoint pos)
{
	int32 button=0;
	Looper()->CurrentMessage()->FindInt32("buttons", &button);
	if ( button & B_PRIMARY_MOUSE_BUTTON )
	{
		// Select list item under mouse pointer, then launch item
		int32 index = IndexOf(pos);
		if(index>=0)
		{
			Select(index);
			_InvokeSelectedItem();
		}
	}
	else if ( button & B_TERTIARY_MOUSE_BUTTON )
	{
		// Launch the currently selected item
		_InvokeSelectedItem();
	}
	else if (button & B_SECONDARY_MOUSE_BUTTON)
	{
		// Select list item under mouse pointer and show menu
		int32 index = IndexOf(pos);
		if(index < 0)
			return;
		Select(index);
		int32 selectedIndex = CurrentSelection();
		if(selectedIndex < 0)
			return;
		// Ignore for super types list items
		BListItem *selectedItem = ItemAt(selectedIndex);
		if(selectedItem->OutlineLevel()==0)
			return;

		_InitPopUpMenu(selectedIndex);

		ConvertToScreen(&pos);
		//fMenu->Go(pos, false, false, true);
		fMenu->Go(pos, true, true, BRect(pos.x - 2, pos.y - 2,
			pos.x + 2, pos.y + 2), true);
	}
}


void
RecentDocsBListView::KeyDown(const char* bytes, int32 numbytes)
{
	if(numbytes == 1) {
		switch(bytes[0]) {
			case B_RETURN:
			case B_SPACE:
			{
				_InvokeSelectedItem();
				break;
			}
			case B_DOWN_ARROW:
			{
				int32 currentSelection = CurrentSelection();
				if(currentSelection==(CountItems()-1))
				{
					Select(0);
					ScrollToSelection();
				}
				else
				{
					BOutlineListView::KeyDown(bytes, numbytes);
				//	Select(currentSelection+1);
				//	ScrollToSelection();
				}
				break;
			}
			case B_UP_ARROW:
			{
				int32 currentSelection = CurrentSelection();
				if(currentSelection==0)
				{
					Select(CountItems()-1);
					ScrollToSelection();
				}
				else
					BOutlineListView::KeyDown(bytes, numbytes);
				break;
			}
			case B_RIGHT_ARROW:
			{
				// Fix a bug in R1A3 where the 0 level list items do not
				// always expand when the right arrow is pressed
				int32 currentSelection = CurrentSelection();
				BListItem *selectedItem = ItemAt(currentSelection);
				if((selectedItem->OutlineLevel()==0) && (!selectedItem->IsExpanded()) )
				{
					Expand(selectedItem);
					break;
				}
				BOutlineListView::KeyDown(bytes, numbytes);
				break;
			}
			default:
				BOutlineListView::KeyDown(bytes, numbytes);
		}
	}
	else
		BOutlineListView::KeyDown(bytes, numbytes);
}


/*
bool
RecentDocsBListView::AddDoc(entry_ref *fileRef, AppSettings *settings)
{
	DocListItem *newItem = new DocListItem(fileRef, settings);
	if(newItem->InitStatus() != B_OK)
		return false;
	SuperTypeListItem *superItem = _GetSuperItem(fileRef, settings);
	if(superItem!=NULL)
		return AddUnder(newItem, superItem);
	else
		return false;
}*/


void
RecentDocsBListView::HandleMouseWheelChanged(BMessage *msg)
{
	if(msg->what!=B_MOUSE_WHEEL_CHANGED
		&& msg->what!=EL_REDIRECTED_MOUSE_WHEEL_CHANGED)
		return;

	float deltaY=0;
//	printf("Mouse wheel changed\n");
	status_t result = msg->FindFloat("be:wheel_delta_y", &deltaY);
	if(result!=B_OK)
		return;
	if(deltaY>0)
	{
		int32 currentSelection = CurrentSelection();
		if(currentSelection==(CountItems()-1))
			Select(0);
		else
			Select(currentSelection+1);
		ScrollToSelection();
	}
	else
	{
		int32 currentSelection = CurrentSelection();
		if(currentSelection==0)
			Select(CountItems()-1);
		else
			Select(currentSelection-1);
		ScrollToSelection();
	}
}


void
RecentDocsBListView::SettingsChanged(uint32 what, AppSettings settings)
{
	Window()->Lock();
	int32 currentSelection = CurrentSelection();
	switch(what)
	{
		case EL_DOC_ICON_OPTION_CHANGED:
		{
			// Update super type items icon size
			int index=0;
			BString nameString;
			void *item;
			while(fSuperListPointers.FindString(EL_SUPERTYPE_NAMES, index, &nameString)==B_OK)
			{
				status_t result = fSuperListPointers.FindPointer(nameString, &item);
				if(result==B_OK)
				{
					((SuperTypeListItem*)item)->SetIconSize(settings.docIconSize);
				}
				index++;
			}
			BuildList(&settings, true);
			Select(currentSelection);
			ScrollToSelection();
			break;
		}
		case EL_FONT_OPTION_CHANGED:
		{
			SetFontSizeForValue(settings.fontSize);
		/*	int count = CountItems();
			for(int i=0; i<count; i++)
			{
				DocListItem* item = (DocListItem*)(ItemAt(0));
				RemoveItem(item);
				AddItem(item);
			}*/
			BuildList(&settings, true);
			Select(currentSelection);
			ScrollToSelection();
			break;
		}
	}
	Window()->Unlock();
}


void
RecentDocsBListView::SetFontSizeForValue(float fontSize)
{
	if(fontSize==0)
		fontSize = be_plain_font->Size();
	BListView::SetFontSize(fontSize);
}


void
RecentDocsBListView::BuildList(AppSettings *settings, bool force=false)
{
	// Check if we need to update list
	BMessage refList;
	if(!force)
	{
		entry_ref recentRef;
		be_roster->GetRecentDocuments(&refList, 1);
		status_t result = refList.FindRef("refs", 0, &recentRef);
		if(result==B_OK && recentRef==fLastRecentDocRef)
		{
			Window()->UpdateIfNeeded();
			return;
		}
		refList.MakeEmpty();
	}

	// Remove existing items
	Window()->Lock();
	while(!IsEmpty())
	{
		BListItem *item = RemoveItem(int32(0));
		// Do not delete level 0 list items, they will be reused to preserve
		// their expanded status
		if(item->OutlineLevel())
			delete item;
	}
/*	int32 itemCount = CountItems();
	if(itemCount)
	{
		const BListItem** itemPtr = Items();
		MakeEmpty();
		Window()->UpdateIfNeeded();
		for(int32 i = itemCount; i>0; i--)
		{
			delete *itemPtr;
			*itemPtr++;
		}
	}*/

	// Get recent documents with a buffer of extra in case there are any that
	// no longer exist
	int fileCount = settings->recentDocCount;
	be_roster->GetRecentDocuments(&refList, 2*fileCount);

	// Add any refs found
	if(!refList.IsEmpty())
	{
		int32 refCount = 0, totalCount = 0;
		type_code typeFound;
		refList.GetInfo("refs", &typeFound, &refCount);
		entry_ref newref;
		BEntry newEntry;
		bool needFirstRecentDoc = true;
		BList docListItems, superListItems;
		BMessage superTypesAdded;

		// Create DocListItems and supertypes
		for(int i=0; i<refCount && totalCount<fileCount; i++)
		{
			refList.FindRef("refs", i, &newref);
		//	printf("Found ref: %s\n", newref.name);
			newEntry.SetTo(&newref);
			if(newEntry.Exists())
			{
				// Save first recent doc entry
				if(needFirstRecentDoc)
				{
					fLastRecentDocRef = newref;
					needFirstRecentDoc = false;
				}

				DocListItem *newItem = new DocListItem(&newref, settings);
				if(newItem->InitStatus() == B_OK)
				{
					docListItems.AddItem(newItem);
					const char *superTypeName = newItem->GetSuperTypeName();
					bool foundBool;
					if(superTypesAdded.FindBool(superTypeName, &foundBool)!=B_OK)
					{
						void* superItemPointer;
						if(fSuperListPointers.FindPointer(superTypeName, &superItemPointer)==B_OK)
						{
							AddItem((SuperTypeListItem*)superItemPointer);
						}
						else
						{
							SuperTypeListItem *superItem = _GetSuperItem(superTypeName, settings);
							fSuperListPointers.AddPointer(superTypeName, superItem);
							fSuperListPointers.AddString(EL_SUPERTYPE_NAMES, superTypeName);
							AddItem(superItem);
							// TODO add pointer to doc item?
						}
						superTypesAdded.AddBool(superTypeName, true);
					}
					totalCount++;
				}
				else
					delete newItem;
			}
		}

		// Add items to list
		int refreshCount = 20;
		for(int i=docListItems.CountItems()-1; i>=0; i--)
		{
			DocListItem *docItem = (DocListItem*)docListItems.ItemAt(i);
			const char *superTypeName = docItem->GetSuperTypeName();
			void* superItemPointer;
			status_t result = fSuperListPointers.FindPointer(superTypeName, &superItemPointer);
			if(result!=B_OK)
			{
				superItemPointer = _GetGenericSuperItem(settings);
				if(!HasItem((SuperTypeListItem*)superItemPointer))
					AddItem((SuperTypeListItem*)superItemPointer);
			}
			AddUnder(docItem, (SuperTypeListItem*)superItemPointer);
			refreshCount--;
			if(!refreshCount)
			{
				Select(0);
				Window()->UpdateIfNeeded();
				refreshCount = 20;
			}
		}

		Window()->UpdateIfNeeded();
		if(!IsEmpty())
		{
			Select(0);
		}
	}

	Window()->Unlock();
}


SuperTypeListItem*
RecentDocsBListView::_GetSuperItem(const char *mimeString, AppSettings *settings)
{
/*	BNode node;
	if (node.SetTo(newref) != B_OK)
		return _GetGenericSuperItem(settings);
	BNodeInfo nodeInfo(&node);
	char mimeString[B_MIME_TYPE_LENGTH];
	if(nodeInfo.GetType(mimeString) != B_OK)
		return _GetGenericSuperItem(settings);
	BMimeType nodeType;
	nodeType.SetTo(mimeString);
	BMimeType superType;
	if(nodeType.GetSupertype(&superType)!=B_OK)
		return _GetGenericSuperItem(settings);
	*/
	BMimeType superType(mimeString);
	if(superType.IsValid())
	{
		SuperTypeListItem *item = new SuperTypeListItem(&superType, settings);
		if(item->InitStatus()==B_OK)
		{
//			AddItem(item);
			return item;
		}
	}

	return _GetGenericSuperItem(settings);
}


SuperTypeListItem*
RecentDocsBListView::_GetGenericSuperItem(AppSettings *settings)
{
	if(fGenericSuperItem==NULL)
	{
		BMimeType genericType("application/octet-stream");
		BMimeType superType;
		if(genericType.GetSupertype(&superType)!=B_OK)
			return NULL;
		fGenericSuperItem = new SuperTypeListItem(&superType, settings);
		if(fGenericSuperItem->InitStatus()!=B_OK)
		{
			delete fGenericSuperItem;
			fGenericSuperItem = NULL;
		}
		else
		{
			fGenericSuperItem->SetName("Unknown type");
//			AddItem(fGenericSuperItem);
		}
	}
	return fGenericSuperItem;
}


status_t
RecentDocsBListView::_InvokeSelectedItem()
{
	status_t result = B_OK;
	int32 selectedIndex = CurrentSelection();
	if(selectedIndex >= 0)
	{
		uint32 modifier = modifiers();
		BListItem *selectedItem = ItemAt(selectedIndex);
		// file or folder
		if(selectedItem->OutlineLevel())
		{
			result = ((DocListItem*)selectedItem)->Launch();
			_HideApp(modifier);
		}
		// super type
		else
		{
			SuperTypeListItem *item = (SuperTypeListItem*)selectedItem;
			if(item->IsExpanded())
				Collapse(item);
			else
				Expand(item);
		}
	}

	return result;
}


void
RecentDocsBListView::_InitPopUpMenu(int32 selectedIndex)
{
	if(fMenu==NULL)
	{
		fMenu = new LPopUpMenu(B_EMPTY_STRING);
		fTrackerMI = new BMenuItem("Show in Tracker", new BMessage(EL_SHOW_IN_TRACKER));
		fSettingsMI = new BMenuItem("Settings" B_UTF8_ELLIPSIS, new BMessage(EL_SHOW_SETTINGS));
		fMenu->AddItem(fTrackerMI);
		fMenu->AddSeparatorItem();
		fMenu->AddItem(fSettingsMI);
		fMenu->SetTargetForItems(this);
		fSettingsMI->SetTarget(be_app);
	}
}


void
RecentDocsBListView::_HideApp(uint32 modifier=0)
{
	if(!(modifier & kPreventHideModifier) )
		be_app->PostMessage(EL_HIDE_APP);
}