#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>

// To run, set your ESP8266 build to 160MHz, update the SSID info, and upload.

// Randomly picked URL
const char *URL = "http://ice1.somafm.com/u80s-128-mp3";
// char *URL = "http://ice4.somafm.com/seventies-128-mp3";

const char topic1[] = "espaudio/espweb";
const char *mqtt_server = "10.10.10.142";
const char user[] = "glamke";
const char passwd[] = "glamke";

// Instantiate objects
WiFiClient espClient;
PubSubClient client(espClient);
AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2S *out;

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
    const char *ptr = reinterpret_cast<const char *>(cbData);
    (void)isUnicode; // Punt this ball for now
    // Note that the type and string may be in PROGMEM, so copy them to RAM for printf
    char s1[32], s2[64];
    strncpy_P(s1, type, sizeof(s1));
    s1[sizeof(s1) - 1] = 0;
    strncpy_P(s2, string, sizeof(s2));
    s2[sizeof(s2) - 1] = 0;
    Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
    Serial.flush();
}

// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
    const char *ptr = reinterpret_cast<const char *>(cbData);
    // Note that the string may be in PROGMEM, so copy it to RAM for printf
    char s1[64];
    strncpy_P(s1, string, sizeof(s1));
    s1[sizeof(s1) - 1] = 0;
    // prevent deluge of debug msgs
    static int lastms = 0;
    if (millis() - lastms > 1000)
    {
        lastms = millis();
        Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
        Serial.flush();
    }
}
void callback(char *topic, byte *payload, int8_t length)
{
    String topicStr = topic;
    // EJ: Note:  the "topic" value gets overwritten everytime it receives confirmation (callback) message from MQTT

    // Print out some debugging info
    //  Serial.printf("Callback update.\n Free Heap: %d\t", ESP.getFreeHeap());
    //  Serial.printf("Free Block Size: %d\t", ESP.getMaxFreeBlockSize());
    //  Serial.printf("Frag Heap: %d\t", ESP.getHeapFragmentation());
    //  mp3->loop(); Doesn't do anything.  Still stops responding.

    if (topicStr == topic1)
    {
        // Serial.println("topic match, payload follows:");

        if (payload)
        {

            mp3->stop();
            // create character buffer with ending null terminator (string)
            int8_t i;
            char message_buff[150];
            for (i = 0; i < length; i++)
            {
                message_buff[i] = payload[i];
            }
            message_buff[i] = '\0';
            // String msgString = String(message_buff);
            Serial.printf("URL = %s\n", message_buff);
            URL = message_buff;
            delete (file);
            file = new AudioFileSourceICYStream(URL);
            file->RegisterMetadataCB(MDCallback, (void *)"ICY");
            delete (buff);
            buff = new AudioFileSourceBuffer(file, 2048);
            buff->RegisterStatusCB(StatusCallback, (void *)"buffer");
            delete (out);
            out = new AudioOutputI2S();
            delete (mp3);
            mp3 = new AudioGeneratorMP3();

            mp3->RegisterStatusCB(StatusCallback, (void *)"mp3");
            // Serial.println(buff->getFillLevel());

            mp3->begin(buff, out);
        }
    }
}
void reconnect()
{
    // attempt to connect to the wifi if connection is lost
    if (WiFi.status() != WL_CONNECTED)
    {
        // loop while we wait for connection
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        // print out some more debug once connected
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
    // make sure we are connected to WIFI before attemping to reconnect to MQTT
    if (WiFi.status() == WL_CONNECTED)
    {
        // Loop until we're reconnected to the MQTT server
        while (!client.connected())
        {
            Serial.print("Attempting MQTT connection...");

            // Generate client name based on MAC address and last 8 bits of microsecond counter
            String clientName;
            clientName += "espmp3";

            // if connected, subscribe to the topic(s) we want to be notified about
            // EJ: Delete "mqtt_username", and "mqtt_password" here if you are not using any
            if (client.connect((char *)clientName.c_str(), user, passwd))
            {
                // EJ: Update accordingly with your MQTT account
                Serial.println("\tMQTT Connected");
                client.subscribe(topic1);
            }
            // otherwise print failed for debugging
            else
            {
                Serial.println("\tFailed.");
                abort();
            }
        }
    }
}
void setup()
{
    Serial.begin(115200);
    delay(1000);
    // Serial.println("Connecting to WiFi");
    // WiFi.disconnect();
    // WiFi.softAPdisconnect(true);
    // WiFi.mode(WIFI_STA);

    // WiFi.begin(SSID, PASSWORD);
    WiFiManager wifiManager;
    // reset saved settings
    //  wifiManager.resetSettings();

    // set custom ip for portal
    // wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
    // fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("AutoConnectAP");
    // or use this for auto generated name ESP + ChipID
    // wifiManager.autoConnect();
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    //   WiFi.disconnect();
    //   WiFi.softAPdisconnect(true);
    //   WiFi.mode(WIFI_STA);
    //   WiFi.begin(SSID, PASSWORD);
    //   // Try forever
    //   while (WiFi.status() != WL_CONNECTED) {
    //     Serial.println("...Connecting to WiFi");
    //     delay(1000);
    //   }
    //   Serial.println("Connected");

    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    audioLogger = &Serial;
    file = new AudioFileSourceICYStream(URL);
    file->RegisterMetadataCB(MDCallback, (void *)"ICY");
    buff = new AudioFileSourceBuffer(file, 2048);
    buff->RegisterStatusCB(StatusCallback, (void *)"buffer");
    out = new AudioOutputI2S();
    mp3 = new AudioGeneratorMP3();
    mp3->RegisterStatusCB(StatusCallback, (void *)"mp3");
    mp3->begin(buff, out);
}

void loop()
{

    static int lastms = 0;
    static int mqtt_timer = 0;

    // Checks to see if the MQTT connection is alive every 5 secs
    if (millis() - mqtt_timer > 5000)
    {
        mqtt_timer = millis();
        // client.publish(topic1, "still connected");  //this causes reset

        if (!client.connected())
        {
            reconnect();
            // client.publish(topic1, "reconnected");  // this will stop the stream playing
        }
    }

    client.loop();

    if (mp3->isRunning())
    {
        if (!mp3->loop())
            mp3->stop();
    }

    if (mp3->isRunning())
    {
        // Serial.println(buff->getFillLevel());
        if (millis() - lastms > 1000)
        {
            lastms = millis();
            // Serial.printf("Running for %d ms...\n", lastms);
            // Serial.flush();
        }
        if (!mp3->loop())
        {
            // Serial.println(buff->getFillLevel());
            mp3->stop();
        }
    }
    else
    {
        if (millis() - lastms > 5000)
        {

            lastms = millis();
            Serial.printf("Listening to MQTT\n");
            // Print out some debugging info
            Serial.printf("Free Heap: %d\t Free Block Size: %d\t Frag Heap: %d\n", ESP.getFreeHeap(),
                          ESP.getMaxFreeBlockSize(), ESP.getHeapFragmentation());
        }
    }
}
