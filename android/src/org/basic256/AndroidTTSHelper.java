package org.basic256;

import android.app.Activity;
import android.content.Intent;
import android.speech.tts.TextToSpeech;
import android.speech.tts.TextToSpeech.OnInitListener;

public class AndroidTTSHelper extends Activity{

	//Copyright (C) 2010 James M. Reneau
	//
	// This program is free software; you can redistribute it and/or modify it
	// under the terms of the GNU General Public License as published by the
	// Free Software Foundation; either version 2 of the License, or (at your option) any later version.
	//
	// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
	// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	//
	// You should have received a copy of the GNU General Public License along with this program;
	// if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	// Boston, MA 02111-1307, USA
	//
	// Originally developed for the mmtag assistive technology project
	//
	// Modification History
	// Date...... Programmer... Description...
	// 2010-02-07 j.m.reneau    original coding
	// 2014-01-01 j.m.reneau    moved to basic256 - changed package and name
	
	private boolean ttsInitialized = false;
	private TextToSpeech ttsEngine;
	
	private final int CHECK_TTSDATA_ACTIVITY = 9999;
	private final int LOAD_TTSDATA_ACTIVITY = 9998;

	
	public TTSHelper() {
		Intent checkIntent = new Intent();
		checkIntent.setAction(TextToSpeech.Engine.ACTION_CHECK_TTS_DATA);
		startActivityForResult(checkIntent, CHECK_TTSDATA_ACTIVITY);
	}
	
	protected void onActivityResult(
	        int requestCode, int resultCode, Intent data) {
	    if (requestCode == CHECK_TTSDATA_ACTIVITY) {
	        if (resultCode == TextToSpeech.Engine.CHECK_VOICE_DATA_PASS) {
	            // success, create the TTS instance
	            ttsEngine = new TextToSpeech(this, onInitListener);
	            ttsInitialized = true;
	        } else {
	            // missing data, install it
	            Intent installIntent = new Intent();
	            installIntent.setAction(
	                TextToSpeech.Engine.ACTION_INSTALL_TTS_DATA);
	            startActivityForResult(installIntent, LOAD_TTSDATA_ACTIVITY);
	        }
	    }
	    if (requestCode == LOAD_TTSDATA_ACTIVITY) {
	    	// re run check
			Intent checkIntent = new Intent();
			checkIntent.setAction(TextToSpeech.Engine.ACTION_CHECK_TTS_DATA);
			startActivityForResult(checkIntent, CHECK_TTSDATA_ACTIVITY);
	    }
	}
	
	private OnInitListener onInitListener = new TextToSpeech.OnInitListener()
	{
	    public void onInit(int stuff)
	    {
	    	ttsInitialized = true;
	    }
	};

	public void say(String text) {
		if (ttsInitialized) {
			ttsEngine.speak(text, TextToSpeech.QUEUE_FLUSH, null);
		}
	}
	
}
