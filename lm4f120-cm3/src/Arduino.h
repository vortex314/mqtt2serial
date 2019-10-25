#include <stdint.h>
#include <string>

uint32_t millis();


class Stream{
    public:
    int available();
    std::string readString();
    void println(std::string s);
    void print(std::string s);
    void print(uint64_t);
};

typedef enum {OUTPUT} Mode;
void pinMode(uint32_t pin,uint32_t mode);
void digitalWrite(uint32_t pin,int v);

Stream Serial;