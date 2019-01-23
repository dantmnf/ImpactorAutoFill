# ImpactorAutoFill
This 💩 will automagically fill and commit your Apple ID in Cydia Impactor.

## Usage
Download from [GitHub releases](https://github.com/dantmnf/ImpactorAutoFill/releases) or build from source code, then:
* (Either) rename `autofill.dll` to `version.dll`, put it next to `Impactor.exe`  
* (Or) put `loader.exe` and `autofill.dll` next to `Impactor.exe`, and run `loader.exe` instead

When Cydia Impactor asks you for Apple ID username and password, we will use a system credential (Windows Security) window instead, and optionally save it to system credential store.

## Configuration
If you need to change any default value listed below, put it as `autofill.ini` next to `Inpactor.exe`
```ini
[AutoFill]
# delay to fill after the Impactor username/password window is shown
# values filled immediately after window shown will be overwritten with empty one
Delay=100

# automatically commit filled value
Commit=true

# the name used to store credential
CredTargetName=ImpactorAutoFill_AppleID

# clear saved credential (and set this to false) on next launch
ClearCred=false
```
