WnnIM for OS/2
==============

WnnIM for OS/2 (a.k.a. 'WnnIM/2' or just 'WnnIM') is an Input Method Editor for
typing East Asian (CJK) text under Presentation Manager.

Specifically, WnnIM itself is the front-end processor component, and is designed
to work in conjunction with the open source FreeWnn IME engine on the back-end.
As such, it will (when finished) require the FreeWnn server for the desired
language to be installed and running (not necessarily on the same computer, but
accessible via TCP/IP).

All the FreeWnn-specific code will be isolated in its own source files.  In
principle, the rest of the WnnIM code could probably be used to implement an IME
based on a different engine such as Canna or Anthy (although I have no plans to
try doing such a thing myself).

The included Makefile requires the IBM C Compiler (preferably version 3.65).


Concepts
--------

WnnIM's operation is based around two 'mode' settings which work in conjunction
with each other: _input mode_ and _CJK conversion mode_.

**Note:** Not all of the following has actually been implemented yet.

The input mode controls how typed characters are converted into basic phonetic
characters for the language in question.  The number of modes supported varies
according to language, although 'none' (i.e. no conversion) is always available.
(Japanese, for example, will have 'hiragana', 'katakana', 'halfwidth katakana' 
and 'fullwidth ASCII' modes; Korean, by contrast, only has 'hangul'.)  

If the target language uses phonetic characters, then how it works is that all 
typed characters are saved into a buffer (known internally as the 'romaji 
buffer') until the buffer contents can be successfully converted into phonetic 
characters (the results of which are placed in the 'kana buffer').  Once 
conversion is successful and unambiguous, the converted characters are inserted
into the application window where the original characters were typed -- unless 
CJK conversion is also active, in which case see below.  If conversion is 
unsuccessful (i.e. the buffer contents do not represent a valid character in the
target language), then the conversion attempt is abandoned and the current 
buffer contents are inserted into the application window unchanged.  

[TODO 1: If conversion is successful but ambiguous (that is, the buffer 
represents a valid character as-is, but could potentially become a different
character if more letters were added, as may be the case with Korean), then the
current conversion is offered as a 'candidate' for the user to either accept or
continue typing.]

[TODO 2: If the target language does not use phonetic characters, then all of
the above is bypassed: typed characters are simply added to the clause buffer 
in place of phonetic characters, and the logic otherwise proceeds as below.  
This will probably be the case with Chinese (both types?).]

The other mode is CJK (or 'clause') conversion mode.  This mode is a simple
on-or-off toggle.  When active, the phonetic characters which have been 
confirmed for entry (per above) will not be sent directly to the application,
but rather added to a 'clause buffer' which is displayed on-screen as an overlay
at the cursor position.  When the user hits a 'convert' key, the current clause
buffer will be converted (if possible) into the Chinese-origin CJK glyphs 
(kanji/hanzi/hanja) applicable to the current language.  Since there are usually
multiple possible candidates for conversion, the candidates are displayed in a
pop-up list on screen so the user can accept the correct one.  Once a candidate
is accepted, the full converted clause is inserted in the application window.

In either case, once a conversion (phonetic or clause) is accepted and the 
result inserted in the application, all buffers are cleared and the process 
starts over.

If the input mode is set to 'none', then all conversion (input and CJK) is
disabled, and typed characters are simply sent to the application as they
are.


Source Code Organization
------------------------

`wnnclient.*` implements the interface to the FreeWnn engine.  If WnnIM were
to be adapted to use a different engine, this is the module that would need to
be replaced.

`wnnhook.*` implements a global PM hook, which is loaded by the client (below)
when the latter starts up.  This hook is responsible for intercepting keystrokes
in any running PM application, and either passing it on to the application
directly or running it through WnnIM's conversion logic.

`wnnim.*` is the main client (front-end) application.  It activates the PM
hook (above), and presents the UI controls for the user to operate the IME.  It
also implements the functions to perform the actual input conversion, which are
called by the PM hook as needed.

`codepage.*` contains various useful functions for dealing with text
encodings.

`ids.h` defines various resource and message IDs used by both the client and the
PM hook.


Notices
-------

WnnIM for OS/2 is (C) 2019 Alexander Taylor

Some of the PM hook logic was derived from `xray` by Michael Shillingford, and
his [EDM/2 programming article](http://www.edm2.com/0501/hooks.html) on the
subject.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307  USA

WnnIM program source: https://github.com/altsan/os2-wnnim
