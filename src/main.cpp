#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SSD1306Wire.h>
#include <OLEDDisplayUi.h>
#include <APRS-IS.h>
#include <APRS-Decoder.h>

#include "settings.h"

SSD1306Wire display(0x3c, D2, D5);
OLEDDisplayUi ui(&display);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, 60*60);
APRS_IS aprs_is(USER, PASS, TOOL, VERS);

int next_update = -1;

void setup_wifi();
void setup_ntp();

void msOverlay(OLEDDisplay * display, OLEDDisplayUiState * state)
{
	display->setFont(ArialMT_Plain_10);
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->drawString(0, 0, "OE5BPA");
	display->setTextAlignment(TEXT_ALIGN_RIGHT);
	display->drawString(128, 0, timeClient.getFormattedTime());
}

String Status;
void drawFrameStatus(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y)
{
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->setFont(ArialMT_Plain_10);
	display->drawString(64 + x, 10 + y, "Status");
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_10);
	//display->drawString(0 + x, 20 + y, Status);
	display->drawStringMaxWidth(0 + x, 20 + y, 128, Status);
}

APRSMessage Messages[10];
uint savedMessages = 0;
void drawFrameTracker(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y)
{
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->setFont(ArialMT_Plain_10);
	display->drawString(64 + x, 10 + y, "Tracker");
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_10);
	uint y_ = 20;
	for(uint i = 0; i < savedMessages; i++)
	{
		String text = Messages[i].getSource() + ": " + Messages[i].getAPRSBody()->getData();
		display->drawString(0 + x, y_ + y, text);
		y_ += 10;
	}
}

void drawFrameiGate(OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y)
{
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->setFont(ArialMT_Plain_10);
	display->drawString(64 + x, 10 + y, "iGate");
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_10);
	display->drawString(0 + x, 20 + y, "blub");
}

const int frameCount = 2;
FrameCallback frames[frameCount] = {drawFrameStatus, drawFrameTracker};
FrameCallback startupFrames[1] = {drawFrameStatus};
const int overlaysCount = 1;
OverlayCallback overlays[overlaysCount] = {msOverlay};

void setup()
{
	Serial.begin(115200);
	Status = "";
	
	ui.setTargetFPS(30);
	//ui.setActiveSymbol(activeSymbol);
	//ui.setInactiveSymbol(inactiveSymbol);
	ui.setIndicatorPosition(BOTTOM);
	ui.setIndicatorDirection(LEFT_RIGHT);
	ui.setFrameAnimation(SLIDE_LEFT);
	ui.setFrames(startupFrames, 1);
	ui.setOverlays(overlays, overlaysCount);
	ui.init();
	
	delay(500);
	Serial.println("[INFO] APRS Monitor by OE5BPA (Peter Buchegger)");
	Status = "APRS Monitor by OE5BPA (Peter Buchegger)";

	ui.update();

	setup_wifi();
	ui.update();
	setup_ntp();
	ui.update();

	delay(500);
}

void loop()
{
	ui.update();
	timeClient.update();
	if(WiFi.status() != WL_CONNECTED)
	{
		Serial.println("[ERROR] WiFi not connected!");
		Status = "ERROR: WiFi not connected!";
		ui.update();
		delay(1000);
		return;
	}
	if(!aprs_is.connected())
	{
		Serial.print("[INFO] connecting to server: ");
		Serial.print(SERVER);
		Serial.print(" on port: ");
		Serial.println(PORT);
		Status = "INFO: Connecting to server...";
		if(!aprs_is.connect(SERVER, PORT, FILTER))
		{
			Serial.println("[ERROR] Connection failed.");
			Serial.println("[INFO] Waiting 5 seconds before retrying...");
			Status = "ERROR: Server connection failed! Waiting 5 sec...";
			ui.update();
			delay(5000);
			return;
		}
		Serial.println("[INFO] Connected to server!");
	}
	static bool setup_new_frames = true;
	if(setup_new_frames)
	{
		Status = "Waiting for data...";
		ui.setFrames(frames, frameCount);
		setup_new_frames = false;
	}
	if(aprs_is.available() > 0)
	{
		String str = aprs_is.getMessage();
		if(!str.startsWith("#"))
		{
			Serial.print("[" + timeClient.getFormattedTime() + "] ");
			Messages[0].decode(str);
			Serial.print("[INFO] ");
			Serial.println(Messages[0].toString());
			Status = str;
			savedMessages = 1;
		}
	}
}

void setup_wifi()
{
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_NAME, WIFI_KEY);
	Serial.print("[INFO] Waiting for WiFi");
	Status = "INFO: Waiting for WiFi";
	while(WiFi.status() != WL_CONNECTED)
	{
		Serial.print(".");
		Status = "INFO: Waiting for WiFi ....";
		ui.update();
		delay(500);
	}
	Serial.println("");
	Serial.println("[INFO] WiFi connected");
	Serial.print("[INFO] IP address: ");
	Serial.println(WiFi.localIP());
	Status = "INFO: WiFi connected\nIP: " + WiFi.localIP().toString();
}

void setup_ntp()
{
	timeClient.begin();
	if(!timeClient.forceUpdate())
	{
		Serial.println("[WARN] NTP Client force update issue!");
		Status = "WARN: NTP Client force update issue!";
	}
	Serial.println("[INFO] NTP Client init done!");
	Status = "INFO: NTP Client init done!";
}
