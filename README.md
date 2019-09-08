# indi_microstepper

This is an indi driver for the Milkyway Microstepper. 

# Binaries

The current binaries can be downloaded under releases. 

# Build

## Prerequisites

Install libindi on your system as source and build it or use a binary distribution.

To build the microstepper driver you need the source tar ball and unpack it on your system

See building at https://www.indilib.org/develop/developer-manual/163-setting-development-environment.html

# Build

1. Unpack the source archive and copy the whole directory to indi/3rdParty
2. change directory to indi/3rdParty/indi-microstepper
3. execute "cmake -DCMAKE_INSTALL_PREFIX=/usr .
4. execute "make"
5. execute "sudo make install"

# Install

Extract the binary archive. There are 3 files, the driver binary, the xml and a txt file.

You have to copy the driver binary named indi-microstepper to /usr/bin/ and the xml to /usr/share/indi/

Then indi can find the driver under its name.

# Known issues
Driver is under development, so there are a few issues

1. Focuser can't stop while movement
2. Kstars is stalled while Focuser movement
3. At the start sometime the position isn't shown correct.
4. Reverse motion switch is not running correctly in all dialogs.
5. others haven't found until now

