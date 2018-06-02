# 4a-hal-generic for 4A (AGL Advance Audio Architecture).

------------------------------------------------------------

* Object: Use a new api to centralize call to hals, automatically create hal api from configuration files
* Status: In Progress (RC1)
* Author: Jonathan Aillet jonathan.aillet@iot.bzh
* Date  : June-2018


## Cloning repositories for RC1 version

### Cloning 4a-hal-generic rc1 version with its submodules

git clone --recurse-submodules -b rc1 https://github.com/iotbzh/4a-hal-generic.git

### Cloning 4a-softmixer hal-rc1 version (needed to make work '4a-hal-generic') with its submodules

git clone --recurse-submodules -b hal-rc1-sandbox https://github.com/iotbzh/4a-softmixer.git


## Quick introduction to how hal are handled with 4a-hal-generic

* At startup of the 4a-hal binding, a new api called '4a-hal-manger' will be created.
  This api is meant to provide verbs to list the loaded hal in your system and to now there current status.
* The '4a-hal-manager' will also create a new hal for each audio configuration files found.
  These configration files are used by the controller and are meant to be used with a mixer.
* External hal (e.g. loaded in another binding) can be loaded/unloaded into '4a-hal-manger' by reaching it
  with this hal. It must provides information such as 'api' and 'uid' at loading. It must also provides
  a subscription to an event that the '4a-hal-manager' will use to known this hal status.
  WARNING: not fully implemented yet.


## Preparation

### Install Alsa Loopback

You must have snd-aloop enabled as a module in the running kernel.
Check that this way on the target:

```
	zcat /proc/config.gz | grep CONFIG_SND_ALOOP
	CONFIG_SND_ALOOP=m
```

If it not the case, run menuconfig and enable it under:
> Device Drivers > Sound card support > Advanced Linux Sound Architecture > Generic sound device


```
	sudo modprobe snd-aloop
```

### Create a new hal json configuration correponding to your audio hardware configuration

#### Information about audio json configuration files

* All audio hardware configuration files are a json description of you audio device.
* They all must be in ./4a-hal-references-boards-cfg/ and must begin with 'hal-4a'.
* You can found some examples of these configurations in this directory.
* Each configuration file found at startup will create a new hal with its own api.
* At 'init-mixer' command of your hal, your

#### What you need to set in this configuration file to make you audio work

* In 'metadata' section:
  * The 'uid' field must be the path to your alsa audio device.
  * The 'api' field should be changed to the desired application framework api of you hal
* For 'onload', 'controls', and 'events' sections, please look at the controller documentation
  (In ./app-controller/README.md)
* In 'halcontrol' section:
  * This section is where you put controls which are alsa control calls
  * If a control is not available, it will be registered in alsa using '4a-alsa-core'
  * Thsese controls will be available as verb for your hal api using 'card/' prefix
  * WARNING: use of this section is not yet implemented
* In 'halmixer' section (what it is passed to the mixer):.
  * The 'uid' field will be the name of the mixer correponding to your hal
  * The 'mixerapi' field should contain the name of the api to call for reaching the mixer
    (not need to be changed if you use '4a-softmixer').
  * The 'backend' section will contain your audio information (such as the path to you alsa audio device
    and the configration of you device).
  * In 'frontend' section:
    * In 'ramps' section: will be defined the ramp that you can use in your mixer (ramps in example files can be used).
  * In 'zones' section: (zones in example files can be used)
    * You can defined the zones that you want for your mixer.
    * You must defined which sink will be used in these zones.
    * These zones will be used to defined streams.
  * In 'streams' section: (streams in example files can be used)
    * You can defined the streams that you want for your mixer.
    * It must contain:
      * A 'uid' field (which will be used to reach the stream).
      * The 'zone' field must correspond to the wanted zone of the stream.
      * The 'ramp' field must correspond to the wanted ramp of the stream.
    * Other fields are optionals


## Compile (for each repositories)

```
    mkdir build
    cd build
    cmake ..
    make
```


## Using '4a-hal' binder

### Run your binder from shell

```
afb-daemon --name=afb-4a --workdir=/home/jon/work/agl-audio/4a-softmixer/build   --binding=/home/jon/work/agl-audio/4a-softmixer/build/package/lib/softmixer-binding.so --binding=/home/jon/work/agl-audio/4a-hal-generic/build/4a-hal/4a-hal.so  --roothttp=/home/jon/work/agl-audio/4a-softmixer/build/package/htdocs --no-ldpaths --port=1234 --token= -vvv
```

### Connect your binder

Connect to your 4a binder using afb-client-demo

```
afb-client-demo ws://localhost:1234/api?token=
```

### List the loaded hal

In the connected client, try to list the loaded hal:

```4a-hal-manager loaded```

And now with more informations:

```4a-hal-manager loaded { "verbose" : 1 }```

### Play with an 'internal' hal (described in a json configuration file)

#### Initalize an internal hal

Use an api name obtain in the previous command to initialize mixer of the wanted hal:

```4a-hal-*halapiname* init-mixer```

#### Get streams informations

Now, you can obtain streams information of your initalized internal hal:

```4a-hal-*halapiname* list```

All the streams listed are available as a verb of the hal api using 'name' field.
You can also get the corresponding card id of the stream in 'cardId' field.
The card id of a stream is 'hw:X,X,X' format and can be used to play music.

#### Play some music into a stream

WARNING: Current version does not handle audio rate conversion, using gstreamer or equivalent to match with audio hardware params is mandatory.

Use the previously obtain card if to play audio in the selected stream:

```gst123 --audio-output alsa=hw:X,X,X your_audio_file.mp3```

#### During playing, try the stream commands to change/ramp volume

Now you can use your hal api to send command to mixer. This way, you can change/ramp volume :

``` 4a-hal-*halapiname* *selected_stream* { "volume" : "+10" }```

``` 4a-hal-*halapiname* *selected_stream* { "volume" : 70 }```

``` 4a-hal-*halapiname* *selected_stream* { "ramp" : "-10" }```

``` 4a-hal-*halapiname* *selected_stream* { "ramp" : 100 }```


#### Warning

Alsa try top automatically store current state into /var/lib/alsa/asound.state when developing/testing this may create impossible
situation. In order to clean up your Alsa snd-aloop config, a simple "rmmod" might not be enough in some case you may have to delete
/var/lib/alsa/asound.state before applying "modprobe".

In case of doubt check with folling command that you start from a clear green field

```
rmmod snd-aloop && modprobe --first-time  snd-aloop && amixer -D hw:Loopback controls | grep vol
```


### Load an 'external' hal

To load an external to '4a-hal-manger', you need to you use an 'api_call' from you hal binding.
With this 'api_call' you must sent a json descition of your api:

```
{
  "api" : mandatory, string that is your hal binding api
  "uid" : mandatory, string that specify your hal uid (usually the device used by your hal)
  "info" : optional, string that describe your hal
  "author" : optional, string that says who is the author of your hal
  "version" : optional, string that says what is the version of your hal
  "date" : optional, string that says the date of your hal
}
```

Your hal must also have a 'subscribe' verb available and event name 'hal_status'.

At external hal loading, the '4a-hal-manager' will subscibe to this event.
Within your hal, you must generate an event each time the status of your hal changes.


## What is missing in RC1 version

* Check that external really exist at loading
* Handling external hal status events.
* Handling 'halcontrol' section in configuration files.
* Generation of an event of '4a-hal-manager' when a hal status change.
* Checking that the specified device in configuration file is present ('4a-alsa-core' binding will be needed).
* Actualization of the internal hal status after mixer initialization.
* At mixer initialization, check that the specified device is not already used by another hal.
* Dynamic handling of USB devices.