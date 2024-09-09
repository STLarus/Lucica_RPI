#include <stdio.h>
#include "../INCLUDE/EVE.h"
#include "../INCLUDE/mongoose.h"
#include <stdio.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "../PICTURE/Rabac.h"

#define BOT_TOKEN   "jure"
#define CHAT_ID 12345678

#define SPIport	1
#define SPIchannel	2
#define SPIspeed	1000000
#define SPImode	0
#define BITMAP_ADDRESS 0x100000  // Početna adresa u RAM_G memoriji
#define BITMAP_WIDTH   200       // Širina bitmape
#define BITMAP_HEIGHT  200       // Visina bitmape
#define JPEG_ADDRESS 0x100000  // Početna adresa u RAM_G memoriji



static const char* s_url = "https://api.telegram.org/" BOT_TOKEN "/sendMessage";
#define MESSAGE "Oh, someone was able to press that tiny button !"


static void fn(struct mg_connection* c, int ev, void* ev_data) {
	if (ev == MG_EV_OPEN) {
		// Connection created. Store connect expiration time in c->data
		*(uint64_t*)c->data = mg_millis() + 1000 /* ms */;
	}
	else if (ev == MG_EV_POLL) {
		if (mg_millis() > *(uint64_t*)c->data &&
			(c->is_connecting || c->is_resolving)) {
			mg_error(c, "Connect timeout");
		}
	}
	else if (ev == MG_EV_CONNECT) {
		// Connected to server, tell client connection to use TLS
		struct mg_str host = mg_url_host(s_url);
		struct mg_tls_opts opts = { .ca = mg_unpacked("/ca.pem"),
								   .name = mg_url_host(s_url) };
		mg_tls_init(c, &opts);

		// Send request
		char* buffer = mg_mprintf("{%m:%m,%m:%m,%m:%s}", MG_ESC("chat_id"),
			MG_ESC(CHAT_ID), MG_ESC("text"), MG_ESC(MESSAGE),
			MG_ESC("disable_notification"), "false");
		int content_length = strlen(buffer);
		mg_printf(c,
			"%s %s HTTP/1.0\r\n"
			"Host: %.*s\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %d\r\n"
			"\r\n",
			"POST", mg_url_uri(s_url), (int)host.len, host.buf,
			content_length);
		mg_send(c, buffer, content_length);
		free(buffer);
	}
	else if (ev == MG_EV_HTTP_MSG) {
		// Response, print it
		struct mg_http_message* hm = (struct mg_http_message*)ev_data;
		printf("%.*s\n", (int)hm->message.len, hm->message.buf);
		c->is_draining = 1;        // Tell mongoose to close this connection
		*(bool*)c->fn_data = true;  // Re-enable further calls
	}
	else if (ev == MG_EV_ERROR) {
		*(bool*)c->fn_data = true;  // Error, enable further calls
	}
}



int mainxxx(void) {
	int spiChannel = 1;  // SPI1
	int speed = 1000000; // 1 MHz
	int mode = 0;        // SPI mode 0
	int csPin = 16;      // GPIO16 (pin 36), ručno kontroliran CS

	if (wiringPiSetup() == -1)
	{
		printf("WiringPI setup failed! \n");
		return 1;
	}
	// Inicijalizacija WiringPi GPIO sistema
	if (wiringPiSetupGpio() == -1) {
		fprintf(stderr, "Failed to initialize WiringPi.\n");
		return 1;
	}
	
	
	
	// Postavljanje GPIO16 (CS) kao izlaznog pina
	pinMode(csPin, OUTPUT);
	digitalWrite(csPin, HIGH);  // Postavite CS na HIGH

	// Inicijalizacija SPI1 bez hardverskog upravljanja CS-om
	//int fd = wiringPiSPISetupMode(spiChannel, speed, mode);
	//int wiringPiSPIxSetupMode(const int number, const int channel, const int speed, const int mode)

	int fd = wiringPiSPIxSetupMode(SPIport, SPIchannel, SPIspeed, SPImode);
	if (fd < 0) {
		fprintf(stderr, "Failed to initialize SPI1.\n");
		return 1;
	}

	// Priprema podataka za slanje
	unsigned char dataToSend[2] = { 0xAB, 0xCD };

	// Ručno postavljanje CS na LOW prije slanja podataka
	/*digitalWrite(csPin, LOW);

	// Slanje podataka preko SPI1
	if (wiringPiSPIDataRW(spiChannel, dataToSend, sizeof(dataToSend)) == -1) {
		fprintf(stderr, "Failed to send data via SPI1.\n");
		return 1;
	}

	// Ručno postavljanje CS na HIGH nakon slanja podataka
	digitalWrite(csPin, HIGH);

	printf("Data sent: 0x%X 0x%X\n", dataToSend[0], dataToSend[1]);*/
	
	while (1)
	{
		digitalWrite(csPin, LOW);
		wiringPiSPIxDataRW(SPIport, SPIchannel, dataToSend, sizeof(dataToSend));

		//wiringPiSPIDataRW(spiChannel, dataToSend, sizeof(dataToSend));
		digitalWrite(csPin, HIGH);
		usleep(20 * 1000);
	}

	return 0;
}

void krug(void)
{
	EVE_LIB_BeginCoProList(); // Početak CoPro liste komandi
	EVE_CMD_DLSTART(); // Početak display liste
	EVE_CLEAR_COLOR_RGB(50, 100, 50); // Postavljanje crne pozadine
	EVE_CLEAR(1, 1, 1); // Čišćenje ekrana

	EVE_COLOR_RGB(255, 255, 255); // Postavljanje boje za crtanje (bela)
	EVE_BEGIN(EVE_BEGIN_POINTS); // Počinje crtanje tačaka (koje se mogu koristiti kao krugovi)
	EVE_POINT_SIZE(60 * 16); // Postavljanje veličine kruga (60 piksela, veličina se daje u 1/16 piksela)
	EVE_VERTEX2F(240 * 16, 136 * 16); // Postavljanje koordinata kruga (240, 136) u 1/16 piksela
	EVE_END(); // Završava crtanje tačaka

	EVE_DISPLAY(); // Prikaz display liste
	EVE_CMD_SWAP(); // Zamena frame buffer-a
	EVE_LIB_EndCoProList(); // Kraj CoPro liste komandi
	EVE_LIB_AwaitCoProEmpty(); // Čekanje dok se sve komande ne izvrše
	
}



// Funkcija za učitavanje bitmape u RAM_G memoriju
void loadBitmapToFT813(const uint8_t* bitmapData, uint32_t size) {
	//EVE_LIB_WriteDataToRAMG(BITMAP_ADDRESS, bitmapData, size); // Učitavanje bitmape
	 EVE_LIB_WriteDataToRAMG(bitmapData, size,BITMAP_ADDRESS); // Učitavanje bitmape
}
void drawButton() {
	EVE_LIB_BeginCoProList(); // Početak CoPro liste komandi
	EVE_CMD_DLSTART(); // Početak display liste
	EVE_CLEAR_COLOR_RGB(100, 100,100); // Postavljanje crne pozadine
	EVE_CLEAR(1, 1, 1); // Čišćenje ekrana

	// Crtanje dugmeta
	EVE_CMD_BUTTON(150, 100, 100, 40, 28, 110, "Broj 1!"); // (x, y, širina, visina, font, opcije, tekst)
	EVE_CMD_BUTTON(150, 200, 100, 40, 28, 0, "Broj 2!"); // (x, y, širina, visina, font, opcije, tekst)


	EVE_DISPLAY(); // Prikaz display liste
	EVE_CMD_SWAP(); // Zamena frame buffer-a
	EVE_LIB_EndCoProList(); // Kraj CoPro liste komandi
	EVE_LIB_AwaitCoProEmpty(); // Čekanje dok se komande izvrše
}

void drawButtonsAndTime(const char* timeString) {
	EVE_LIB_BeginCoProList(); // Početak CoPro liste komandi
	EVE_CMD_DLSTART(); // Početak nove display liste
	EVE_CLEAR_COLOR_RGB(100, 100, 100); // Postavljanje pozadine
	EVE_CLEAR(1, 1, 1); // Čišćenje ekrana

	// Crtanje 6 dugmadi
	EVE_CMD_BUTTON(50, 50, 100, 40, 28, 0, "Dugme 1");
	EVE_CMD_BUTTON(50, 100, 100, 40, 28, 0, "Dugme 2");
	EVE_CMD_BUTTON(50, 150, 100, 40, 28, 0, "Dugme 3");
	EVE_CMD_BUTTON(200, 50, 100, 40, 28, 0, "Dugme 4");
	EVE_CMD_BUTTON(200, 100, 100, 40, 28, 0, "Dugme 5");
	EVE_CMD_BUTTON(200, 150, 100, 40, 28, 0, "Dugme 6");

	// Dodavanje vremena (sat) na ekran
	EVE_CMD_TEXT(150, 250, 28, EVE_OPT_CENTER, timeString); // Prikaz sata

	EVE_DISPLAY(); // Prikaz display liste
	EVE_CMD_SWAP(); // Zamena frame buffer-a
	EVE_LIB_EndCoProList(); // Kraj CoPro liste komandi
	EVE_LIB_AwaitCoProEmpty(); // Čekanje dok se komande izvrše
}




#define JPEG_ADDRESS 0x100000  // Početna adresa u RAM_G memoriji

// Funkcija za učitavanje JPEG slike u RAM_G memoriju
void loadJPEGToFT813(const uint8_t* jpegData, uint32_t size) {
	EVE_LIB_WriteDataToRAMG( jpegData, size,JPEG_ADDRESS);  // Učitavanje JPEG podataka
}

void drawText() {
	EVE_LIB_BeginCoProList(); // Početak CoPro liste komandi
	EVE_CMD_DLSTART(); // Početak display liste
	EVE_CLEAR_COLOR_RGB(0, 0, 0); // Postavljanje crne pozadine
	EVE_CLEAR(1, 1, 1); // Čišćenje ekrana

	// Ispisivanje teksta
	EVE_CMD_TEXT(50, 50, 20, 0, "Ormar 12345"); // (x, y, font, options, tekst)

	EVE_DISPLAY(); // Prikaz display liste
	EVE_CMD_SWAP(); // Zamena frame buffer-a
	EVE_LIB_EndCoProList(); // Kraj CoPro liste komandi
	EVE_LIB_AwaitCoProEmpty(); // Čekanje dok se komande izvrše
}

void addTextToExistingImage() {
	EVE_LIB_BeginCoProList(); // Početak CoPro liste komandi
	EVE_CMD_DLSTART(); // Početak display liste
	// **Ne koristimo EVE_CLEAR da ne bismo obrisali prethodne elemente**

	// Dodaj tekst na postojeću sliku
	EVE_CMD_TEXT(150, 100, 28, 0, "Ovo je tekst!"); // (x, y, font, options, tekst)

	EVE_DISPLAY(); // Prikaz display liste
	EVE_CMD_SWAP(); // Zamena frame buffer-a
	EVE_LIB_EndCoProList(); // Kraj CoPro liste komandi
	EVE_LIB_AwaitCoProEmpty(); // Čekanje dok se komande izvrše
}

void main(void)
{
//const uint8_t* bitmapData; // Ovdje bi bila tvoja bitmapa u formatu niza podataka
	uint32_t bitmapSize = BITMAP_WIDTH * BITMAP_HEIGHT * 2; // Veličina bitmape (RGB565 format koristi 2 bajta po pikselu)
 	EVE_Init();
	
	char timeString[16];
	uint16_t time=100;
    
	// Ovo je samo simulacija: Pretpostavljamo da funkcija getTimeString vraća trenutno vreme u formatu "HH:MM:SS"
	while (1) {
		strcpy(timeString,"13.09.2024 12:23:32"); // Ažuriraj vreme svake sekunde
		drawButtonsAndTime(timeString); // Ponovo iscrtaj dugmad i novo vreme
		sleep(1); // Pauza od 1 sekunde
		
		strcpy(timeString, "13.09.2024 12:23:33"); // Ažuriraj vreme svake sekunde
		drawButtonsAndTime(timeString); // Ponovo iscrtaj dugmad i novo vreme
		sleep(1); // Pauza od 1 sekunde
		
		strcpy(timeString, "13.09.2024 12:23:34"); // Ažuriraj vreme svake sekunde
		drawButtonsAndTime(timeString); // Ponovo iscrtaj dugmad i novo vreme
		sleep(1); // Pauza od 1 sekunde
		
		strcpy(timeString, "13.09.2024 12:23:35"); // Ažuriraj vreme svake sekunde
		drawButtonsAndTime(timeString); // Ponovo iscrtaj dugmad i novo vreme
		sleep(1); // Pauza od 1 sekunde
		
		
	}
	
	
	
	
	drawText();
	drawButton(); // Prikaz dugmeta na ekranu
	addTextToExistingImage();	//drawText(); // Prikazivanje teksta na ekranu
	
	
	int jure = 5;


/*	struct mg_mgr mgr;
	mg_log_set(MG_LL_INFO);
	mg_mgr_init(&mgr);
	bool enable = true;
	for (;;) {
		mg_mgr_poll(&mgr, 50);
		if (enable && !gpio_get_level(39)) {
			enable = false;
			mg_http_connect(&mgr, s_url, fn, &enable);
		}
	}*/

}