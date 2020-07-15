#include <CircBuf.h>
#include <Hardware.h>
#include <Log.h>
#include <Streams.h>
#include <string>
#include <deque>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/lm4f/gpio.h>
#include <libopencm3/lm4f/nvic.h>
#include <libopencm3/lm4f/rcc.h>
#include <libopencm3/lm4f/systemcontrol.h>
#include <libopencm3/lm4f/uart.h>

class UART_TIVA : public UART
{
  FunctionPointer _onRxd;
  FunctionPointer _onTxd;
  void *_onRxdVoid = 0;
  void *_onTxdVoid = 0;
  uint32_t clock = 9600;
  uint32_t _pinTxd;
  uint32_t _pinRxd;
  uint32_t _baudrate;
  std::deque<uint8_t> _rxdBuf;
  std::deque<uint8_t> _txdBuf;

  uint32_t _driver;
  uint32_t _dataBits = 8;
  uart_parity _parity;
  uint32_t _stopBits;
  uint32_t _module = 0;

public:
  static UART &create(uint32_t module) { return *new UART_TIVA(module); };

  UART_TIVA(uint32_t module) { _module = module; }

  Erc mode(const char *m)
  {
    if (m[0] == '8')
      _dataBits = 8;
    else if (m[0] == '7')
      _dataBits = 7;
    else if (m[0] == '6')
      _dataBits = 6;
    else if (m[0] == '5')
      _dataBits = 5;
    else
      return EINVAL;
    if (m[1] == 'E')
      _parity = UART_PARITY_EVEN;
    else if (m[1] == 'O')
      _parity = UART_PARITY_ODD;
    else if (m[1] == 'N')
      _parity = UART_PARITY_NONE;
    else
      return EINVAL;
    if (m[2] == '1')
      _stopBits = 1;
    else if (m[2] == '2')
      _stopBits = 2;
    else
      return EINVAL;
    return E_OK;
  }
  Erc init()
  {
    periph_clock_enable(RCC_GPIOA);
    /* Mux PA0 and PA1 to UART0 (alternate function 1) */
    gpio_set_af(GPIOA, 1, GPIO0 | GPIO1);
    /* Enable the UART clock */
    periph_clock_enable(RCC_UART0);
    /* We need a brief delay before we can access UART config registers */
    __asm__("nop");
    /* Disable the UART while we mess with its setings */
    uart_disable(UART0);
    /* Configure the UART clock source as precision internal oscillator */
    uart_clock_from_piosc(UART0);
    uart_set_baudrate(UART0, _baudrate);
    uart_set_databits(UART0, _dataBits);
    uart_set_parity(UART0, _parity);
    uart_set_stopbits(UART0, _stopBits);
    /* Now that we're done messing with the settings, enable the UART */
    uart_enable(UART0);
    return E_OK;
  }
  Erc deInit(){return E_OK;};
  Erc setClock(uint32_t clock) { _baudrate = clock; return E_OK;};

  Erc write(const uint8_t *data, uint32_t length)
  {
    for (uint32_t idx = 0; idx < length; idx++)
      write(*(data + idx));
      return E_OK;
  };
  Erc write(uint8_t b)
  {
    _txdBuf.push_back(b);
    return E_OK;
  };
  Erc read(Bytes &bytes)
  {
    while (_rxdBuf.size())
    {
      auto b = _rxdBuf.front();
      _rxdBuf.pop_front();
      bytes.write(b);
    }
    return E_OK;
  };
  uint8_t read()
  {
    auto b = _rxdBuf.front();
    _rxdBuf.pop_front();
    return b;
  };
  void onRxd(FunctionPointer fp, void *context)
  {
    _onRxd = fp;
    _onRxdVoid = context;
  };
  void onTxd(FunctionPointer fp, void *context)
  {
    _onTxd = fp;
    _onTxdVoid = context;
  };
  uint32_t hasSpace() { return 5; };
  uint32_t hasData() { return _rxdBuf.size(); };
};

class Serial : public AsyncFlow<std::string>
{
};

void app_main()
{
  UART &uart0 = UART_TIVA::create(0);
  std::string hw = "Hello World";
  uart0.write('A');
}