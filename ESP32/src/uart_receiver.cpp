// uart_receiver.cpp
#include "uart_receiver.h"

void UartReceiver::begin(uint32_t baud, int rxPin, int txPin)
{
    port->begin(baud, SERIAL_8N1, rxPin, txPin);
}

bool UartReceiver::extractStringValue(const char *json, const char *key, char *out, size_t maxLen)
{
    const char *pos = strstr(json, key);
    if (!pos)
        return false;

    pos += strlen(key);
    while (*pos && *pos != '"')
        pos++;
    if (*pos != '"')
        return false;
    pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && i + 1 < maxLen)
    {
        out[i++] = *pos++;
    }
    out[i] = '\0';
    return true;
}

bool UartReceiver::extractIntValue(const char *json, const char *key, long *out)
{
    const char *pos = strstr(json, key);
    if (!pos)
        return false;

    pos += strlen(key);
    while (*pos && (*pos == ' ' || *pos == ':' || *pos == '"' || *pos == '\'' || *pos == '\t'))
    {
        pos++;
    }

    if (!*pos)
        return false;

    *out = strtol(pos, nullptr, 10);
    return true;
}

void UartReceiver::parseJsonLine(const char *json, TelemetryPacket &out)
{
    out.raw_json = json;
    extractStringValue(json, "\"mode\":", out.mode, sizeof(out.mode));
    extractStringValue(json, "\"state\":", out.state, sizeof(out.state));
    extractIntValue(json, "\"ax\":", &out.ax);
    extractIntValue(json, "\"ay\":", &out.ay);
    extractIntValue(json, "\"az\":", &out.az);
    extractIntValue(json, "\"raw\":", &out.raw);
    extractIntValue(json, "\"filtered\":", &out.filtered);
    extractIntValue(json, "\"squat_count\":", &out.squat_count);
    extractIntValue(json, "\"pushup_count\":", &out.pushup_count);
}

bool UartReceiver::tryReadPacket(TelemetryPacket &out)
{
    while (port->available() > 0)
    {
        char c = (char)port->read();

        if (c == '\r')
            continue;

        if (c == '\n')
        {
            if (lineBuf.length() > 0)
            {
                parseJsonLine(lineBuf.c_str(), out);
                lineBuf = "";
                return true;
            }
        }
        else
        {
            lineBuf += c;
        }
    }
    return false;
}
