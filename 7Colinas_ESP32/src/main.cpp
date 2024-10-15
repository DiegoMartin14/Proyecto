#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <ESP32Time.h>
#include <secrets.h>
#include <Firebase_ESP_Client.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Objetos de server NTP
ESP32Time rtc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", -10800, 60000); // Configuración del cliente NTP (congifuro zona horaria UTC-3 y sincronizo cada 1 min el horario)

// Objetos de firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData firebaseData;

// Definicion de variables y constantes
int publicacionNumero = 1;
unsigned long previousMillisPublicar = 0;
unsigned int intervalPublicar = 120000; // Intervalo para publicar (10min) / (AHORA ESTA PUBLICANDO CADA 2 MIN)

int adcPins[] = {34, 35, 32, 33, 39, 36}; // Pines GPIO asociados al ADC1
int valores[6];
int tanqueNumero = 1;
unsigned long previousMillisLeer = 0;
unsigned int intervalLeer = 3000; // Intervalo para leer (3seg)
float tempTanque[6];

unsigned long millisLeerDocs = 0;
unsigned long tiempoLeerDocs = 150000; // para leer documentos cada 2,5 min

// Para el control de temperatura
int histeresis = 1;
int set = 19;
bool fueraDeRango;
int bandaAlta = set + histeresis;
int bandaBaja = set - histeresis;

// Declaraciones de las funciones
void publicarDatos();
void leerTemperaturas();
void leerDocumentosFirestore();
void activarAlarma(int tanque, int temperaturaActual, int tempMin, int tempMax);
float convertir_rango(int valor);
void crearDocumento(float, String, String, int);

// Inicializa la biblioteca de Telegram
// WiFiClientSecure secured_client;
// UniversalTelegramBot bot(TOKEN_BOT, secured_client);

void setup()
{
    Serial.begin(115200);

    // Configuración WiFi
    WiFiManager wm;
    bool res = wm.autoConnect("Microcontrolador");
    if (!res)
    {
        Serial.println("Falló la conexión a WiFi");
        ESP.restart();
    }
    else
    {
        Serial.println("Conectado a WiFi :)");
    }

    // Configuración para el horario
    timeClient.begin();
    configTime(-10800, 0, "europe.pool.ntp.org");
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        rtc.setTimeStruct(timeinfo);
    }

    // Configuración de Firebase
    config.api_key = API_KEY;
    auth.user.email = USUARIO_EMAIL;
    auth.user.password = USUARIO_CONTRA;
    config.database_url = DATABASE_URL;

    Firebase.reconnectWiFi(true);
    Firebase.begin(&config, &auth);
    
    // Configuración del ADC
    for (int tanqueNumero = 0; tanqueNumero < 6; ++tanqueNumero)
    {
        analogSetAttenuation(ADC_11db); // Atenuación para 11 dB (rango de 0-3.3V)
        analogReadResolution(10);       // Resolución de 10 bits
    }
}

void loop()
{
    // Actualizar el cliente NTP
    timeClient.update();

    // Intervalo para lectura de temperatura
    if (millis() - previousMillisLeer >= intervalLeer)
    {
        previousMillisLeer = millis();
        leerTemperaturas();
    }

    // Intervalo para publicación de datos
    if (millis() - previousMillisPublicar >= intervalPublicar)
    {
        publicarDatos();
    }

    // Intervalo para leer documentos
    if (millis() - millisLeerDocs >= tiempoLeerDocs)
    {
        leerDocumentosFirestore();
    }
}

////////////////////////////// FUNCIONES //////////////////////////////

// FUNCION PARA LEER TEMPERATURAS
void leerTemperaturas()
{
    for (int i = 0; i < 6; i++)
    {
        valores[i] = random(876, 935);
        tempTanque[i] = convertir_rango(valores[i]);
    }
}

// FUNCION PARA PUBLICAR DATOS
void publicarDatos()
{
    switch (publicacionNumero)
    {
    case 1:
        crearDocumento(tempTanque[0], rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
        publicacionNumero = 2;
        break;
    case 2:
        crearDocumento(tempTanque[1], rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
        publicacionNumero = 3;
        break;
    case 3:
        crearDocumento(tempTanque[2], rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
        publicacionNumero = 4;
        break;
    case 4:
        crearDocumento(tempTanque[3], rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
        publicacionNumero = 5;
        break;
    case 5:
        crearDocumento(tempTanque[4], rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
        publicacionNumero = 6;
        break;
    case 6:
        crearDocumento(tempTanque[5], rtc.getDate(), timeClient.getFormattedTime(), publicacionNumero);
        publicacionNumero = 1;
        previousMillisPublicar = millis();
        break;
    }
}

// FUNCION PARA CONVERTIR VALOR DIGITAL A TEMPERATURA
float convertir_rango(int valor_ADC)
{
    float m = (25.0 - (-10.0)) / 1023.0;
    float b = 25.0 - m * 1023.0;
    return m * valor_ADC + b;
}

// FUNCION PARA CREAR DOCUMENTO, ENVIARLO A FIRESTORE Y A REALTIME DATABASE
void crearDocumento(float temperatura, String fecha, String hora, int tanque)
{
    if (Firebase.ready())
    {
        timeClient.update();
        String documentId = String(timeClient.getEpochTime());
        String documentPath = "Produccion/" + documentId;

        // Redondear temperatura a dos decimales
        float temperaturaRedondeada = roundf(temperatura * 100) / 100.0;

        // Formatear temperatura como cadena
        char tempStr[10];
        dtostrf(temperaturaRedondeada, 6, 2, tempStr);

        // Crear objeto JSON
        FirebaseJson json;
        json.set("fields/temperatura/stringValue", String(temperaturaRedondeada));
        json.set("fields/Tanque/integerValue", tanque);
        json.set("fields/Fecha/stringValue", fecha);
        json.set("fields/Hora/stringValue", hora);

        // Crear el documento en Firestore
        if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROYECTO_ID, "", documentPath.c_str(), json.raw()))
        {
            Serial.printf("Documento creado: %s\n", documentPath.c_str());
        }
        else
        {
            Serial.println("Error al crear el documento: " + String(fbdo.errorReason()));
        }

        // Actualización en Realtime Database
        String path = "/FirebaseComunicacion/Tanque " + String(tanque);
        Firebase.RTDB.setFloat(&fbdo, path.c_str(), temperaturaRedondeada);
    }
}

// FUNCION PARA LEER DOCUMENTOS DE FIRESTORE
void leerDocumentosFirestore()
{
    char tanques[6][10] = {"Tanque1", "Tanque2", "Tanque3", "Tanque4", "Tanque5", "Tanque6"};

    for (int i = 0; i < 6; i++)
    {
        String nombreTanque = tanques[i];
        String docId = "Configuracion/" + nombreTanque;

        if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROYECTO_ID, "", docId.c_str(), "", "", ""))
        {
            FirebaseJson &json = fbdo.jsonObject();
            FirebaseJsonData jsonData;

            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, fbdo.payload());

            if (error)
            {
                Serial.print("Error al deserializar JSON: ");
                Serial.println(error.c_str());
                return;
            }

            // Leer TempMin
            if (doc["fields"]["TempMin"]["stringValue"])
            {
                int tempMin = doc["fields"]["TempMin"]["stringValue"].as<int>();

                // Leer TempMax
                if (doc["fields"]["TempMax"]["stringValue"])
                {
                    int tempMax = doc["fields"]["TempMax"]["stringValue"].as<int>();

                    // Lógica de comparacion de alarma
                    int temperaturaActual = (int)tempTanque[i];
                    if (temperaturaActual < tempMin || temperaturaActual > tempMax)
                    {
                        activarAlarma(i + 1, temperaturaActual, tempMin, tempMax);
                        crearDocumento(tempTanque[i], rtc.getDate(), timeClient.getFormattedTime(), i + 1);
                    }
                }
                else
                {
                    Serial.println("Error al obtener TempMax.stringValue");
                }
            }
            else
            {
                Serial.println("Error al obtener TempMin.stringValue");
            }
        }
        else
        {
            Serial.println("Error al leer el documento: " + String(fbdo.errorReason()));
        }
    }
    millisLeerDocs = millis();
}

// FUNCION MANEJO DE ALARMA (FUNCION PROVISORIA SOLO PARA DEPURACION POR EL MOMENTO)
void activarAlarma(int tanque, int temperaturaActual, int tempMin, int tempMax)
{
    Serial.printf("¡Alarma! Tanque %d - Temperatura Actual: %d, TempMin: %d, TempMax: %d\n", tanque, temperaturaActual, tempMin, tempMax);
    // bot.sendMessage(CHAT_ID, "Hola! hay temperaturas fuera de rango!!");
}