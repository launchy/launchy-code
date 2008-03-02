/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef PLATFORM_X11
#define PLATFORM_X11


#include "platform_gnome_util.h"
#include "platform_base.h"
#include "platform_base_hotkey.h"
#include "platform_base_hottrigger.h"



class PlatformGnome : public QObject, public PlatformBase 
{
    Q_OBJECT
	Q_INTERFACES(PlatformBase)
private:
    GnomeAlphaBorder * alpha;
    QString alphaFile;
public:
    PlatformGnome();
    ~PlatformGnome();

    QApplication* init(int* argc, char** argv);
	// Mandatory functions
	// Mandatory functions
	void SetHotkey(const QKeySequence& key, QObject* receiver, const char* slot)
	{
		GlobalShortcutManager::disconnect(oldKey, receiver, slot);
		GlobalShortcutManager::connect(key, receiver, slot);
		oldKey = key;
	}

	QString GetSettingsDirectory() { 
	    return "";
	};


	QList<Directory> GetInitialDirs();


	void AddToNotificationArea() {};
	void RemoveFromNotificationArea() {};

	bool isAlreadyRunning() {
	    return false;
	}

	void alterItem(CatItem*);
	bool Execute(QString path, QString args);

	bool SupportsAlphaBorder();
	bool CreateAlphaBorder(QWidget* w, QString ImageName);
	void DestroyAlphaBorder() { delete alpha ; alpha = NULL; return; }
	void MoveAlphaBorder(QPoint pos) { if (alpha) alpha->move(pos); }
	void ShowAlphaBorder() { if (alpha) alpha->show(); }
	void HideAlphaBorder() { if (alpha) alpha->hide(); }
	void SetAlphaOpacity(double trans ) { if (alpha) alpha->SetAlphaOpacity(trans); }
};


#endif
