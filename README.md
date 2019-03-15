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


Source Code Organization
------------------------

`wnnhook.*` implements a global PM hook, which is loaded by the client (below)
when the latter starts up.  This hook is responsible for intercepting keystrokes
in any running PM application, and either passing it on to the application
directly or running it through WnnIM's conversion logic.

`wnnim.*` is the main client (front-end) application.  It activates the PM
hook (above), and presents the UI controls for the user to operate the IME.

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
