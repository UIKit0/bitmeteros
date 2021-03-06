/*
 * BitMeterOS
 * http://codebox.org.uk/bitmeterOS
 *
 * Copyright (c) 2011 Rob Dawson
 *
 * Licensed under the GNU General Public License
 * http://www.gnu.org/licenses/gpl.txt
 *
 * This file is part of BitMeterOS.
 *
 * BitMeterOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BitMeterOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with BitMeterOS.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef _WIN32
	#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include "common.h"
#include "bmclient.h"
#include "client.h"

/*
Contains the code that handles monitor requests made via the bmclient utility.
*/

static void sigIntHandler();
static int stopNow = FALSE;
extern struct Prefs prefs;
static void printBar(BW_INT dl, BW_INT ul);
static void printText(BW_INT dl, BW_INT ul);

static void setDefaultPrefs(){
 // Use these defaults if nothing else is specified by the user
	if (prefs.monitorType == PREF_NOT_SET){
		prefs.monitorType = PREF_MONITOR_TYPE_NUMS;
	}
	if (prefs.direction == PREF_NOT_SET){
		prefs.direction = PREF_DIRECTION_DL;
	}
	if (prefs.barChars == PREF_NOT_SET){
		prefs.barChars = DEFAULT_BAR_CHARS;
	}
	if (prefs.maxAmount == PREF_NOT_SET){
		prefs.maxAmount = DEFAULT_MAX_AMOUNT;
	}
}

void doMonitor(){
	setDefaultPrefs();

 // We are going to loop forever unless we get an interrupt
	signal(SIGINT, sigIntHandler);

 // Check how often new values get written to the database - this is how often we update the display
	int dbWriteInterval = getConfigInt(CONFIG_DB_WRITE_INTERVAL, TRUE);
	if (dbWriteInterval < 1){
		dbWriteInterval = 1;	
	}

	if (dbWriteInterval == 1){
		printf("Monitoring... (Ctrl-C to abort)\n");
	} else {
		printf("Monitoring [updating every %d secs]... (Ctrl-C to abort)\n", dbWriteInterval);	
	}
	struct Data* values;
	struct Data* currentValue;
	int doBars = (prefs.monitorType == PREF_MONITOR_TYPE_BAR);
	BW_INT dl, ul;
	
	
	while(!stopNow){
		dl = ul = 0;

	 // Get the values for the appropriate time-span
	 	time_t now = getTime();
		values = getMonitorValues(now - dbWriteInterval, prefs.host, prefs.adapter);

		if (values == NULL){
		 // We print out zeroes if there is nothing from the db
            values = allocData();
		}
        currentValue = values;

		while(currentValue != NULL){
		 // If system clock gets put back we don't want to include all the values that now lie in the future
			if (currentValue->ts <= now){
				dl += currentValue->dl;
				ul += currentValue->ul;
			}
			currentValue = currentValue->next;
		}

		if (doBars){
		 // We need to 'draw' a bar to represent the data
			printBar(dl, ul);
		} else {
		 // We just need to display the figures
			printText(dl, ul);
		}

		freeData(values);
	    doSleep(dbWriteInterval);
	}
	printf("monitoring aborted.\n");
}

static void printBar(BW_INT dl, BW_INT ul){
 /* We must draw a bar to represent either the upload/download speed, as well as
 	displaying a numeric value. */

 // This is where we put the numeric part
	char amtTxt[20];
	BW_INT amt = ((prefs.direction == PREF_DIRECTION_DL) ? dl : ul);
	formatAmount(amt, TRUE, TRUE, amtTxt);

 // Work out how many characters this bar will occupy
	int charCount = prefs.barChars * ((float)amt / prefs.maxAmount);
	charCount = (charCount > prefs.barChars) ? prefs.barChars : charCount;

 // Display the numeric value
	printf("%10s|", amtTxt);

 // Draw the bar
	int i;
	for(i=1; i<= charCount; i++){
		printf("#");
	}
	printf("\n");
}

static void printText(BW_INT dl, BW_INT ul){
	printf("DL: %llu UL: %llu\n", dl, ul);
}

static void sigIntHandler(){
	stopNow = TRUE;
}
