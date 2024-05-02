# shox
Generates ship image files for Continuum that have either ball or bomb proximity boxes.  

## Running
1. Open Continuum.
2. Join the zone and arena with the settings that you want to generate for.
3. Run `shox.exe` or `shox.exe --bomb` to generate the ship files. This will dump ship1-ship8.png into the working directory.
4. Put the files into the appropriate zone folder.

Running this requires admin permissions because it needs to read Continuum memory for the ball / bomb prox settings.

## Building
Use MSVC to open the solution file and build in x64 release mode.  
If any non-standard build setup is used, a manifest file should be used to create the UAC access level popup.  

### Further development
This could be developed further with support for loading the ship graphics directly from Continuum memory. This would make it easy to modify ships that came from LVZ.  

Also, it would be easy to find Continuum's working directory and automatically dump the graphics files to the zone folder.
