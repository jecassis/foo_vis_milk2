/*
  LICENSE
  -------
  Copyright 2005-2013 Nullsoft, Inc.
  Copyright 2021-2024 Jimmy Cassis
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    * Neither the name of Nullsoft nor the names of its contributors may be used to
      endorse or promote products derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __NULLSOFT_DX_PLUGIN_DEFINES_H__
#define __NULLSOFT_DX_PLUGIN_DEFINES_H__

// APPNAME is the name that will appear in Winamp's list of installed plugins.
// Try to include the version number with the name.
// Note: To change the name of the file (DLL) that the plugin is compiled
//       to, go to the Project -> Properties -> Linker -> Output File.
//       Do not forget to change both Debug and Release build configurations!
#define SHORTNAME L"MilkDrop 2"     // used as window caption for both MilkDrop and the config panel
#define LONGNAME "MilkDrop v2.25k"  // appears at bottom of config panel
#define LONGNAMEW TEXT(LONGNAME)

// INT_VERSION is the major version #, multiplied by 100 (ie. version 1.02
// would be 102). If the app goes to read in the INI file and sees that
// the INI file is from an older version of your plugin, it will ignore
// their old settings and reset them to the defaults for the new version;
// but that only works if you keep this value up-to-date.
// ***To disable this behavior, just always leave this at 100. ***
#define INT_VERSION 225

// INT_SUBVERSION is the minor version #, counting up from 0 as you do
// mini-releases. If the plugin goes to read the old INI file and sees that
// the major version # is the same but the minor version # is not, it will,
// again, ignore their old settings and reset them to the defaults for the
// new version. ***To disable this behavior, just always leave this at 0. ***
#define INT_SUBVERSION 11 // straight=0, a=1, b=2, ...

// SUBDIR puts milkdrop's documentation, INI file, presets folder, etc.
// in a subdir underneath Winamp\Plugins.
#define SUBDIR L"milkdrop2\\"

// INIFILE is the name of the .INI file that will save the user's
// config panel settings. Do not include a path; just give the filename.
// The actual file will be stored in the Winamp\Plugins directory,
// OR POSSIBLY "c:\application data\...(user)...\winamp\plugins"!!! - if
// they have separate settings for each user!
#define INIFILE L"milk2.ini" //*** DO NOT PUT IN A SUBDIR because on save, if directory doesn't already exist, it won't be able to save the INI file.
                             //*** Could be in c:\program files\winamp\plugins, or in c:\application data\...user...\winamp\plugins !!
#define MSG_INIFILE L"milk2_msg.ini"
#define IMG_INIFILE L"milk2_img.ini"
#define ADAPTERSFILE L"milk2_adapters.txt"

// DOCFILE is the name of the documentation file that you'll write
// for your users. Do not include a path; just give the filename.
// When a user clicks the 'View Docs' button on the config panel,
// the plugin will try to display this file, located in the
// Winamp\Plugins directory.
// ***Note that the button will be invisible (on the config panel)
// at runtime if this string is empty.***
#define DOCFILE SUBDIR L"docs\\milkdrop.html"

// PLUGIN_WEB_URL is the web address of the homepage for your plugin.
// It should be a well-formed URL (http://...). When a user clicks
// the 'View Webpage' button on the config panel, the plugin will
// launch their default browser to display this page.
// ***Note that the button will be invisible (on the config panel)
// at runtime if this string is empty.***
#define PLUGIN_WEB_URL "https://www.geisswerks.com/milkdrop/"

// The following two strings - AUTHOR_NAME and COPYRIGHT - will be used
// in a little box in the config panel, to identify the author and copyright
// holder of the plugin. Keep them short so they fit in the box.
#define AUTHOR_NAME L"Ryan Geiss && Jimmy Cassis"
#define COPYRIGHT L"© 2001–2013 Nullsoft, Inc. — © 2022–2024 Jimmy Cassis"

// CLASSNAME is the name of the window class that the plugin will
// use. You don't want this to overlap with any other plugins
// or applications that are running, so change this to something
// that will probably be unique. For example, if your plugin was
// called Libido, then "LibidoClass" would probably be a safe bet.
#define NAMECLASS "MilkDrop2"
#define CLASSNAME TEXT(NAMECLASS)

// Here you can give names to the buttons (~tabs) along the top
// of the config panel. Each button, when clicked, will bring
// up the corresponding 'property page' (embedded dialog),
// IDD_PROPPAGE_1 through IDD_PROPPAGE_8. If you want less than
// 8 buttons to show up, just leave their names as blank. For
// full instructions on how to add a new tab/page, see
// DOCUMENTATION.TXT.
//#define CONFIG_PANEL_BUTTON_1 " Common Settings "   // nPage==1
//#define CONFIG_PANEL_BUTTON_2 "  MORE SETTINGS  "   // nPage==2
//#define CONFIG_PANEL_BUTTON_3 " Artist Tools "      // nPage==3
//#define CONFIG_PANEL_BUTTON_4 " Transitions "       // nPage==4
//#define CONFIG_PANEL_BUTTON_5 ""                    // nPage==5
//#define CONFIG_PANEL_BUTTON_6 ""                    // nPage==6
//#define CONFIG_PANEL_BUTTON_7 ""                    // nPage==7
//#define CONFIG_PANEL_BUTTON_8 ""                    // nPage==8
// As if 2.0e, these strings are defined in the stringtable of the dll
// and otherwise work the same as these header defines.
// (The equivelent of "" in these is a single space now)

// Adjust the defaults for the 4 built-in fonts here.
#define SIMPLE_FONT_DEFAULT_FACE L"Courier New"
#define SIMPLE_FONT_DEFAULT_SIZE 14
#define SIMPLE_FONT_DEFAULT_BOLD 0
#define SIMPLE_FONT_DEFAULT_ITAL 0
#define SIMPLE_FONT_DEFAULT_AA 0
#define DECORATIVE_FONT_DEFAULT_FACE L"Times New Roman"
#define DECORATIVE_FONT_DEFAULT_SIZE 24
#define DECORATIVE_FONT_DEFAULT_BOLD 0
#define DECORATIVE_FONT_DEFAULT_ITAL 1
#define DECORATIVE_FONT_DEFAULT_AA 1
#define HELPSCREEN_FONT_DEFAULT_FACE L"Segoe UI"
#define HELPSCREEN_FONT_DEFAULT_SIZE 14 // NOTE: should fit on 640x480 screen!
#define HELPSCREEN_FONT_DEFAULT_BOLD 1
#define HELPSCREEN_FONT_DEFAULT_ITAL 0
#define HELPSCREEN_FONT_DEFAULT_AA 0
#define PLAYLIST_FONT_DEFAULT_FACE L"Segoe UI" //"Arial"
#define PLAYLIST_FONT_DEFAULT_SIZE 16
#define PLAYLIST_FONT_DEFAULT_BOLD 0
#define PLAYLIST_FONT_DEFAULT_ITAL 0
#define PLAYLIST_FONT_DEFAULT_AA 0

// Automatically add extra fonts to the configuration panel
// by simply `#define`ing them here, UP TO A MAX OF 5 EXTRA FONTS.
// Access the font by calling `GetFont(EXTRA_1)` for extra font #1,
// `GetExtraFont(EXTRA_2)` for extra font #2, and so on.
#define NUM_EXTRA_FONTS 2 // <- do not exceed 5 here!
#define TOOLTIP_FONT EXTRA_1
//#define EXTRA_FONT_1_NAME "Tooltips" <- Defined in the STRINGTABLE  resources
#define EXTRA_FONT_1_DEFAULT_FACE L"Calibri"
#define EXTRA_FONT_1_DEFAULT_SIZE 14
#define EXTRA_FONT_1_DEFAULT_BOLD 0
#define EXTRA_FONT_1_DEFAULT_ITAL 0
#define EXTRA_FONT_1_DEFAULT_AA 0
#define SONGTITLE_FONT EXTRA_2
//#define EXTRA_FONT_2_NAME "Animated Song Titles" <- Defined in the STRINGTABLE resources
#define EXTRA_FONT_2_DEFAULT_FACE L"Georgia"
#define EXTRA_FONT_2_DEFAULT_SIZE 18
#define EXTRA_FONT_2_DEFAULT_BOLD 0
#define EXTRA_FONT_2_DEFAULT_ITAL 1
#define EXTRA_FONT_2_DEFAULT_AA 1

#define WINDOWCAPTION SHORTNAME // caption that will appear on the plugin window
#define DLLDESC LONGNAME        // description of this DLL, as it appears in Winamp's list of visualization plugins
#define MODULEDESC LONGNAME     // description of this visualization module within the DLL (..this framework is set up for just 1 module per DLL)

// Finally, a few parameters that will control how things are done
// inside the plugin shell:
#define NUM_AUDIO_BUFFER_SAMPLES 576 // number of waveform data samples stored in the buffer for analysis
#define NUM_FFT_SAMPLES 512 // number of spectrum analyzer samples
#define NUM_WAVEFORM_SAMPLES 480 // number of waveform data samples available for rendering a frame (RANGE: 32-576)
                                 // Note: if it is less than 576, then VMS will do its best
                                 //       to line up the waveforms from frame to frame for you, using
                                 //       the extra samples as 'squish' space.
                                 // Note: the more 'slush' samples you leave, the better the alignment
                                 //       will be. 512 samples gives you decent alignment; 400 samples
                                 //       leaves room for fantastic alignment.
                                 // Observe that if you specify a value here (say 400) and then only
                                 // render a sub-portion of that in some cases (say, 200 samples),
                                 // make sure you render the *middle* 200 samples (#100-300), because
                                 // the alignment happens *mostly at the center*.
#define NUM_FREQUENCIES 512 // number of frequency samples desired from the FFT, for 0-11kHz range.
                            //   ** this must be a power of 2!
                            //   ** the actual FFT will use twice this many frequencies **

#define TEXT_MARGIN 10             // number of pixels of margin to leave between text and the edge of the screen
#define PLAYLIST_INNER_MARGIN 4.0f // extra margin between the playlist box and the text inside

#define PLAYLIST_COLOR_PLAYING_TRACK 0xFFCCFF00 // alpha|red|green|blue
#define PLAYLIST_COLOR_HILITE_TRACK 0xFFFF5050
#define PLAYLIST_COLOR_BOTH 0xFFFFCC22
#define PLAYLIST_COLOR_NORMAL 0xFFCCCCCC

#define MENU_COLOR 0xFFCCCCCC
#define MENU_HILITE_COLOR 0xFFFF4400
#define DIR_COLOR 0xFF88CCFF
#define TOOLTIP_COLOR 0xFFBBBBCC

#define MAX_PRESETS_PER_PAGE 32

#define PRESS_F1_DUR 3.0f  // in seconds
#define PRESS_F1_EXP 10.0f // exponent for how quickly it accelerates to leave the screen: 1 = linear, >1 = stays and then dashes off at end

#endif
