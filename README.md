# ESP8266_HA_Audio_Node
PUBLIC - ESP8266 I2S implementation of HA TTS node over HTTP and MQTT 

Current State: Proof of concept

Dependencies: 
I have included the used libraries (some of them) in the repo.  I highly recommend looking at the PlatformIO.ini file and using 
```
lib_ldf_mode = deep
```

[ESPAUDIO](https://github.com/earlephilhower/ESP8266Audio)

[WifiManager](https://github.com/tzapu/WiFiManager)

[PubSubClient](https://github.com/knolleary/pubsubclient)



---------------------------------------------------

To get started, clone the repo to platformIO.

You'll need an I2S decoder board.  I have used both with success:

* Max98357 I2S 3W Class D Amplifier
* DAC Module CJMCU-1334 UDA1334A I2S DAC Audio Stereo Decoder

I think at one point I also used the ESP8266 no DAC option but it was not adequate in volume with my test speaker. 

Once cloned, plug in your MQTT broker and wire up the DAC.

I used the following arrangement:
```
WemosD1 Mini    I2S Card

--------------------------------

5V              Vin
Gnd             Gnd
RX              Data/DIN
D4              LRCLK (Left/Right Word Clock I think)
D8              BCLK


```

If you did it right so far, on uploading via USB, you'll be presented with the ubiquitous/awesome WiFiManager Access Point Configuration options.  

Once you connect it to wifi, it should go ahead and start streaming webradio so you'll know it is working. 

Test the MQTT feature by sending it some other Soma radiostations over MQTT to make sure it's picking up messages from the broker. Don't use RETAIN in MQTT.

---------------------------------------------

You're now ready for the tricky part, getting HA to talk to it.  

I already have a media player entity running on the Pi (MPD).  What I want is to send the same TTS MP3 from HA to the Node.  

To do that, I added the following automation:
```
- id: esp_audio_node
  alias: remote esp audio node
  initial_state: true
  trigger:
  - entity_id: media_player.mpd1
    platform: state
    to: "playing"
  action:
    service: mqtt.publish
    data_template:
      topic: 'espaudio/espweb'
      payload_template: "{{ states.media_player.mpd1.attributes['media_content_id']}}"
      
 ```
