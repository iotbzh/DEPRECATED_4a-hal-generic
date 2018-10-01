# 4a-hal-generic for 4A (AGL Advance Audio Architecture).

------------------------------------------------------------

* Object: Use a new api to centralize call to hals, automatically create hal api from configuration files
* Status: In Progress (master)
* Author: Jonathan Aillet jonathan.aillet@iot.bzh
* Date  : June-2018

## Cloning repositories for current version

### Cloning 4a-hal-generic with its submodules

git clone --recurse-submodules https://github.com/iotbzh/4a-hal-generic.git

### Cloning 4a-softmixer hal-rc1 version (needed to make work '4a-hal-generic') with its submodules

git clone --recurse-submodules https://github.com/iotbzh/4a-softmixer.git

### Cloning 4a-alsacore with its submodules

git clone --recurse-submodules https://gerrit.automotivelinux.org/gerrit/src/4a-alsa-core

## Quick introduction to how hal are handled with 4a-hal-generic

* At startup of the 4a-hal binding, a new api called '4a-hal-manger' will be created.
  This api is meant to provide verbs to list the loaded hals in your system and to know their current status.
* The '4a-hal-manager' will also create a new hal for each audio configuration file found.
  These configuration files are used by the controller and are meant to be used with a mixer.
* External hal (e.g. loaded in another binding) can be loaded/unloaded into '4a-hal-manager' by reaching it
  with this hal. It must provide information such as 'api' and 'uid' at loading. It must also provide
  a subscription to an event that the '4a-hal-manager' will use to know this hal status.
  WARNING: not fully implemented yet.

## Preparation

### Install Alsa Loopback (DEPRECATED)

You must have snd-aloop enabled as a module in the running kernel.
Check that this way on the target:

```bash
zcat /proc/config.gz | grep CONFIG_SND_ALOOP
CONFIG_SND_ALOOP=m
```

If it is not the case, run menuconfig and enable it under:
> Device Drivers > Sound card support > Advanced Linux Sound Architecture > Generic sound device

```bash
sudo modprobe snd-aloop
```

### Create a new hal json configuration corresponding to your audio hardware configuration

#### Information about audio json configuration files

* All audio hardware configuration files are a json description of your audio devices.
* They all must be in ./4a-hal-cfg-reference or ./4a-hal-cfg-community and must begin with 'hal-4a'.
* You can find some examples of these configurations in this directory.
* Each configuration file found at startup will create a new hal with its own api.
* At 'init-mixer' hal command, your mixer configuration will be sent.

#### What you need to set in this configuration file to make your audio work

* In `metadata` section:
  * The `uid` field must be the path to your alsa audio device.
  * The `api` field should be changed to the desired application framework api of your hal.
* For `onload`, `controls`, and `events` sections, please look at the controller documentation
  (In ./app-controller/README.md)
* In `halcontrol` section:
  * In this section you put controls corresponding to alsa control calls.
  * If a control is not available, it will be registered in alsa using '4a-alsa-core'.
  * These controls will be available as verbs for your hal.
  * The value passed to these verbs should be a percentage, the hal will do the conversion to the correct alsa value.
    To be recognized by the hal, the value should be associated to the key `val` and should be an integer.
* In `halmixer` section (that it is passed to the mixer):
  * The `uid` field will be the name of the mixer corresponding to your hal.
  * The `mixerapi` field should contain the name of the api to call for reaching the mixer
    (not need to be changed if you use '4a-softmixer').
  * The `prefix` field  is not mandatory, it is where you precise the prefix that will be applied
    when the mixer attach call happens. All the streams the mixer creates will be prefixed with this field.
  * In `ramps` section:
    * Define the ramp that you can use in your mixer (ramps in example files can be used).
    * The `uid` field is where you specify the name of the ramp.
    * The ramp will set the current volume to the targeted volume step by step:
      * The `delay` field is the delay between two volume modification.
      * The `up` field is the volume increase in one step (when increasing volume is requested).
      * The `down` field is the volume decrease in one step (when decreasing volume is requested).
  * The `playbacks` section will contain your output audio information (such as the path to you alsa audio device
    and the configuration of your device):
    * A `uid` field that will be used by the mixer to identify the playback.
    * The `params` field is an optional field where you can specify some parameters
      for your playback (such as rate).
    * The `sink` field is where you describe your playback:
      * The `controls` field must contain the alsa control labels
        that the mixer will use to set/mute volume on your audio device.
      * The `channels` field must contain an object that will link an `uid` to
        a physical output audio `port` that will be used in zone.
  * The `captures` section will contain your input audio information (such as the path to you alsa audio device
    and the configuration of your device):
    * A `uid` field that will be used by the mixer to identify the capture.
    * The `params` field is an optional field where you can specify some parameters
      for your capture (such as rate).
    * The `sink` field is where you describe your capture:
      * The `controls` field must contain the alsa control labels
        that the mixer will use to set/mute volume on your audio device.
      * The `channels` field must contain an object that will link an `uid` to
        a physical input audio `port` that will be used in zone.
  * In `zones` section: (zones in example files can be used):
    * You can define the zones that you want for your mixer.
    * You must define which sink will be used in these zones.
    * These zones will be used to define streams.
    * The `sink` field must contain objects that will link a physical ouput audio port
      (defined in `playbacks` section) using `target` field to a logical output audio `channel`
      that will be used in zone.
    * The `source` field must contain objects that will link a physical input audio port
      (defined in `captures` section) using `target` field to a logical input audio `channel`
      that will be used in zone.
  * In `streams` section: (streams in example files can be used):
    * You can define the streams that you want for your mixer.
    * Mandatory fields:
      * A `uid` field, it is the name used by the mixer to identify streams (e.g. when using `info` verb)
      * The `verb` field is the name used by the mixer to declare the stream verb
        (but prefixing it with `prefix` if defined).
        It is also used by the hal to declare a verb to make it accessible from a hal point of view.
      * The `zone` field must correspond to the wanted zone of the stream.
    * Optional fields:
      * The `source` is where you can precise which loop to use for this stream (not the default one).
        The loop are defined in the mixer configuration file.
      * The `params` is where you can specify some parameters for your stream (such as rate).
      * `volume` and `mute` fields are initiate values of the stream.

### Note about using a USB device

Dynamic handling of USB devices is not yet implemented, so you need to modify the 'hal-4a-2ch-generic-usb.json'
to change the `uid` value in `metadata` section and `path` values in `playbacks` and `captures` sections.
All these values should be replaced by the alsa entry path of your usb device, these entry are listed
under directory '/dev/snd/by-id/...'.

## Compile (for each repositories)

```bash
mkdir build
cd build
cmake ..
make
```

## Using '4a-hal' binder

### Selection of the hal loaded at binding launch

Currently, the 4a-hal binding will try to launch a hal for each json audio configuration
files found in path specified with CONTROL_CONFIG_PATH (an audio configuration file should begin with 4a-hal-*).
If you don't want a hal to be launch with the binding, you can just rename/remove the corresponding
audio json configuration file.

Be aware that every hal which will find its associated audio device will attach to the mixer.
So, if you work on M3 and want to use an external usb audio device, be sure to rename the m3 configuration file.

### Run your binder from shell

```bash
afb-daemon --name=afb-4a --workdir=$PATH_TO_4a-softmixer/build --binding=$PATH_TO_4a-alsa-core/build/alsa-binding/afb-alsa-4a.so --binding=$$PATH_TO_4a-softmixer/build/package/lib/softmixer-binding.so --binding=$PATH_TO_4a-hal-generic/build/4a-hal/4a-hal.so  --roothttp=$PATH_TO_4a-softmixer/build/package/htdocs --no-ldpaths --port=1234 --token= -vvv
```

### Connect your binder

Connect to your 4a binder using afb-client-demo

```bash
afb-client-demo ws://localhost:1234/api?token=
```

### List the ready hal

In the connected client, try to list the ready hal:

```4a-hal-manager loaded```

And now with more information:

```4a-hal-manager loaded { "verbose" : true }```

### Play with an 'internal' hal (described in a json configuration file)

#### Get streams information

Now, you can obtain streams information of your initialized internal hal:

```*halapiname* info```

All the streams listed are available as a verb of the hal api using `name` field.
You can also get the corresponding card id of the stream in `cardId` field.
The card id stream format is `hw:X,X,X` and can be used to play music.

#### Play some music into a stream

WARNING: Current version does not handle audio rate conversion, using gstreamer
or equivalent with audio hardware params is mandatory.

Use the previously obtain card id to play audio in the selected stream:

`gst123 --audio-output alsa=hw:X,X,X your_audio_file.mp3`

#### During playing, try the stream commands to change/ramp volume

Now you can use your hal api to send commands to mixer. This way, you can change/ramp volume:

`*halapiname* *selected_stream* { "volume" : "+10" }`

`*halapiname* *selected_stream* { "volume" : 70 }`

`*halapiname* *selected_stream* { "ramp" : "-10" }`

`*halapiname* *selected_stream* { "ramp" : 100 }`

#### Warning

Alsa try to automatically store current state into `/var/lib/alsa/asound.state`
that may result to odd situation during development/testing. In order to clean
up your Alsa snd-aloop config, a simple `rmmod` might not be enough in some case
you may have to delete `/var/lib/alsa/asound.state` before applying `modprobe`.

In case of doubt, check with following command that you start from a clear green field

```bash
rmmod snd-aloop && modprobe --first-time snd-aloop && amixer -D hw:Loopback controls | grep vol
```

### Load an 'external' hal

To load an external to '4a-hal-manger', you need to you use an 'api_call' from you hal binding.
With this 'api_call' you must sent a json description of your api:

```json
"metadata" : {
  "api" : "mandatory, string that is your hal binding api",
  "uid" : "mandatory, string that specify your hal uid (usually the device used by your hal)",
  "info" : "optional, string that describes your hal",
  "author" : "optional, string that says who is the author of your hal",
  "version" : "optional, string that says what is the version of your hal",
  "date" : "optional, string that says the date of your hal",
  "snd-dev-id": "optional, integer that specify you alsa device id (hw:X), -1 if not available"
}
```

Your hal must also have a 'subscribe' verb available and event name 'hal_status'.

At external hal loading, the '4a-hal-manager' will subscribe to this event.
Within your hal, you must generate an event each time the status of your hal changes.

### Known issues

#### Fail to find json configuration files

When compiling and executing 4a-hal-generic and 4a-softmixer for a specific target using an sdk,
some bindings won't find there json configuration files.
This issue does not appears when compiling and deploying the 4a-hal-generic/4a-softmixer
with yocto because a recipe its taking charge of these issues.

Nevertheless, to make the bindings find their configuration files, you can export the `CONTROL_CONFIG_PATH`
environment variable to the directories where the configuration files are stored.
Example on a target where the 4a-hal and 4a-softmixer are deployed into `/home/root/4a` directory :
`export CONTROL_CONFIG_PATH=/home/root/4a/smixer/etc:/home/root/4a/4a-hal/etc`.

## What is missing in this version

* Check that external hal really exist at loading
* Handling external hal status events.
* Generation of an '4a-hal-manager' event when a hal status change.
* Dynamic handling of USB devices.
