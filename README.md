# shox
Generates image files for Continuum that have either ball, bomb, or mine proximity boxes.  

## Running
1. Open Continuum.
2. Join the zone and arena with the settings that you want to generate for.
3. Run `shox.exe`, choose the options, then click create.
4. Put the files into the appropriate zone folder.

Running this requires admin permissions because it needs to read Continuum memory for the prox settings.

## Building
Use MSVC to open the solution file and build in x64 release mode.  
If any non-standard build setup is used, a manifest file should be used to create the UAC access level popup.  

### Further development
This could be developed further with support for loading the graphics directly from Continuum memory. This would make it easy to modify images that came from LVZ.  
