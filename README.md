# FTE: The Folding Text Editor

This is a fork of the "20010819" source release [available here](http://fte.sourceforge.net).

**This is NOT based on the newest FTE sources available. Please check the actual SourceForge
project page for newer source updates if you're not interested in the DOS-specific changes
that this Git repo is about.**

Changes:

* Built against Watcom C/C++ 11.0 and DOS/4GW
* Fixed the `ScreenSizeX` and `ScreenSizeY` global config properties so that they are actually used under DOS.

Existing `.CNF` files built for the 20010819 FTE release should all work the same and not need
to be rebuilt.

From the "Release" page, the ZIP file contains two pre-built CNF config files that are otherwise
identical except that `FTE.CNF` sets 80x25 text mode while `FTE80x50.CNF` sets 80x50 text mode.
The `config/` directory contains everything you'll need to build your own custom config via the
included `CFTE.EXE`. Just make any changes you like and then run:

```
CFTE config/main.fte
```

Which will result in a new file `FTE-NEW.CNF` being built (assuming no errors).

### Background

The motivation for these changes comes from the fact that I wanted a DOS build of FTE that I could
use on my "retro" DOS-based PCs for writing code using Watcom C/C++ and DOS/4GW. The only DOS
release of FTE that I'm aware of is compiled with DJGPP and hence uses CWSDPMI. Different DOS
extenders usually don't mix well together and as I found out for myself, when trying to use FTE's
Run/Compile menu options to run external Watcom tools (like wlink, wd, etc) most anything that
uses DOS/4GW would crash likely due to the fact that FTE was running under CWSDPMI.

After looking at the source code, it seemed that only minor changes would be needed to get a 
DOS/4GW-compatible build going (and indeed, _some_ of the Watcom-specific work had been done
already ... I guess in the past someone else had this building with Watcom?).

After I got it building I noticed that it didn't support DOS 80x50 text mode, even though from
looking at the code it was clear that support for customizable screen sizes was planned from the
start. I guess whoever did the original DOS porting work didn't hook up different DOS text mode
support where it needed to be hooked up.

For my modified source release of FTE I've cleaned out any sources and build scripts that are
NOT related to Watcom/DOS support. Since this fork is based off an older version of FTE, I feel
like if you're looking for support for some other OS, you're not going to be best served by
starting off from here anyway. Go to the more up to date SourceForge project page instead.
