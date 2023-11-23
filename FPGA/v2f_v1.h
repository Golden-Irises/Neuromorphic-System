#ifndef V2F_V1
#define V2F_V1

#define A2      0x02
#define A3      0x03

#define INPUT   false
#define OUTPUT  true

#define LOW     false
#define HIGH    true

void *pinMode(int, int);

void *digitalWrite(int, int);

int analogRead(int);

void *delay(int);

void *COM_HARD_BEGIN(int);

void *COM_HARD_PRINTLN(int);

struct COM_HARD {
    void *(*begin)(int) = COM_HARD_BEGIN;

    void *(*println)(int) = COM_HARD_PRINTLN;
} Serial;

#endif