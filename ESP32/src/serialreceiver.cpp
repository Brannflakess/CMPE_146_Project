#include <Arduino.h>

static const int ESP32_UART_RX = 16;
static const int ESP32_UART_TX = 17;
static const uint32_t UART_BAUD = 115200;
static const uint32_t USB_BAUD  = 115200;

HardwareSerial stmSerial(2);

char lineBuf[256];
size_t linePos = 0;

bool extractStringValue(const char *json, const char *key, char *out, size_t maxLen)
{
    const char *pos = strstr(json, key);
    if (!pos) return false;

    pos += strlen(key);
    while (*pos && *pos != '"') pos++;
    if (*pos != '"') return false;
    pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && i + 1 < maxLen)
    {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return true;
}

bool extractIntValue(const char *json, const char *key, long *out)
{
    const char *pos = strstr(json, key);
    if (!pos) return false;

    pos += strlen(key);
    while (*pos && (*pos == ' ' || *pos == ':' || *pos == '"' || *pos == '\'' || *pos == '\t'))
    {
        pos++;
    }

    if (!*pos) return false;

    *out = strtol(pos, nullptr, 10);
    return true;
}

void processJsonLine(const char *json)
{
    char mode[32] = "UNKNOWN";
    char state[32] = "UNKNOWN";
    long ax = 0, ay = 0, az = 0;
    long raw = 0, filtered = 0;
    long squat_count = 0, pushup_count = 0;

    extractStringValue(json, "\"mode\":", mode, sizeof(mode));
    extractStringValue(json, "\"state\":", state, sizeof(state));
    extractIntValue(json, "\"ax\":", &ax);
    extractIntValue(json, "\"ay\":", &ay);
    extractIntValue(json, "\"az\":", &az);
    extractIntValue(json, "\"raw\":", &raw);
    extractIntValue(json, "\"filtered\":", &filtered);
    extractIntValue(json, "\"squat_count\":", &squat_count);
    extractIntValue(json, "\"pushup_count\":", &pushup_count);

    Serial.println();
    Serial.println("===== JSON RECEIVED FROM STM32 =====");
    Serial.print("Raw JSON     : ");
    Serial.println(json);
    Serial.print("Mode         : ");
    Serial.println(mode);
    Serial.print("AX           : ");
    Serial.println(ax);
    Serial.print("AY           : ");
    Serial.println(ay);
    Serial.print("AZ           : ");
    Serial.println(az);
    Serial.print("Raw axis     : ");
    Serial.println(raw);
    Serial.print("Filtered     : ");
    Serial.println(filtered);
    Serial.print("State        : ");
    Serial.println(state);
    Serial.print("Squat count  : ");
    Serial.println(squat_count);
    Serial.print("Pushup count : ");
    Serial.println(pushup_count);
    Serial.println("====================================");
    Serial.println();
}

void setup()
{
    Serial.begin(USB_BAUD);
    delay(1000);

    Serial.println("ESP32 receiver starting...");
    Serial.printf("UART2 RX=%d TX=%d baud=%lu\n",
                  ESP32_UART_RX, ESP32_UART_TX, (unsigned long)UART_BAUD);

    stmSerial.begin(UART_BAUD, SERIAL_8N1, ESP32_UART_RX, ESP32_UART_TX);
}

void loop()
{
    while (stmSerial.available() > 0)
    {
        char c = (char)stmSerial.read();

        if (c == '\r') continue;

        if (c == '\n')
        {
            if (linePos > 0)
            {
                lineBuf[linePos] = '\0';
                processJsonLine(lineBuf);
                linePos = 0;
            }
        }
        else
        {
            if (linePos < sizeof(lineBuf) - 1)
            {
                lineBuf[linePos++] = c;
            }
            else
            {
                // overflow protection
                linePos = 0;
            }
        }
    }
}