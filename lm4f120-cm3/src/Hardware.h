#ifndef HARDWARE_H
#define HARDWARE_H
#include <Bytes.h>
#include <Erc.h>

class Bytes;

typedef void (*FunctionPointer)(void*);

typedef uint32_t Erc;
typedef uint32_t PhysicalPin;
typedef enum {
	LP_TXD = 0,
	LP_RXD,
	LP_SCL,
	LP_SDA,
	LP_MISO,
	LP_MOSI,
	LP_SCK,
	LP_CS
} LogicalPin;

class Driver {
	public:
		virtual Erc init() = 0;
		virtual Erc deInit() = 0;
};

class UART : public Driver {
	public:
		static UART& create(uint32_t module,PhysicalPin txd, PhysicalPin rxd);
		virtual Erc mode(const char*)=0;
		virtual Erc init() = 0;
		virtual Erc deInit() = 0;
		virtual Erc setClock(uint32_t clock) = 0;

		virtual Erc write(const uint8_t* data, uint32_t length) = 0;
		virtual Erc write(uint8_t b) = 0;
		virtual Erc read(Bytes& bytes) = 0;
		virtual uint8_t read() = 0;
		virtual void onRxd(FunctionPointer, void*) = 0;
		virtual void onTxd(FunctionPointer, void*) = 0;
		virtual uint32_t hasSpace() = 0;
		virtual uint32_t hasData() = 0;
};

//===================================================== GPIO DigitalIn ========

class DigitalIn : public Driver {
	public:
		typedef enum { DIN_NONE, DIN_RAISE, DIN_FALL, DIN_CHANGE } PinChange;

		typedef enum { DIN_PULL_UP = 1, DIN_PULL_DOWN = 2 } Mode;
		static DigitalIn& create(PhysicalPin pin);
		virtual int read() = 0;
		virtual Erc init() = 0;
		virtual Erc deInit() = 0;
		virtual Erc onChange(PinChange pinChange, FunctionPointer fp,
		                     void* object) = 0;
		virtual Erc setMode(Mode m) = 0;
		virtual PhysicalPin getPin() = 0;
};
//===================================================== GPIO DigitalOut
class DigitalOut : public Driver {
	public:
		static DigitalOut& create(PhysicalPin pin);
		virtual Erc init() = 0;
		virtual Erc deInit() = 0;
		virtual Erc write(int) = 0;
		virtual PhysicalPin getPin() = 0;
};
//===================================================== I2C ===

#define I2C_WRITE_BIT
#define I2C_READ_BIT 0

class I2C : public Driver {
	public:
		static I2C& create(PhysicalPin scl, PhysicalPin sda);
		virtual Erc init() = 0;
		virtual Erc deInit() = 0;
		virtual Erc setClock(uint32_t) = 0;
		virtual Erc setSlaveAddress(uint8_t address) = 0;
		virtual Erc write(uint8_t* data, uint32_t size) = 0;
		virtual Erc write(uint8_t data) = 0;
		virtual Erc read(uint8_t* data, uint32_t size) = 0;
};

class Spi : public Driver {
	public:
	public:
		typedef enum {
			SPI_MODE_PHASE0_POL0 = 0,
			SPI_MODE_PHASE1_POL0 = 1,
			SPI_MODE_PHASE0_POL1 = 2,
			SPI_MODE_PHASE1_POL1 = 3
		} SpiMode;
		typedef enum {
			SPI_CLOCK_125K = 125000,
			SPI_CLOCK_250K = 250000,
			SPI_CLOCK_500K = 500000,
			SPI_CLOCK_1M = 1000000,
			SPI_CLOCK_2M = 2000000,
			SPI_CLOCK_4M = 4000000,
			SPI_CLOCK_8M = 8000000,
			SPI_CLOCK_10M = 10000000,
			SPI_CLOCK_20M = 20000000
		} SpiClock;

		static Spi& create(PhysicalPin miso, PhysicalPin mosi, PhysicalPin sck,
		                   PhysicalPin cs);
		virtual ~Spi();
		virtual Erc init() = 0;
		virtual Erc deInit() = 0;
		virtual Erc exchange(Bytes& in, Bytes& out) = 0;
		virtual Erc onExchange(FunctionPointer, void*) = 0;
		virtual Erc setClock(uint32_t) = 0;
		virtual Erc setMode(SpiMode) = 0;
		virtual Erc setLsbFirst(bool) = 0;
		virtual Erc setHwSelect(bool) = 0;
};

class ADC {

	public:
		static ADC& create(PhysicalPin pin);

		virtual Erc init() = 0;
		virtual int getValue() = 0;
};

class Connector {
		uint32_t _pinsUsed;
		uint32_t _connectorIdx;
		uint32_t _physicalPins[8];
		UART* _uart;
		Spi* _spi;
		I2C* _i2c;
		ADC* _adc;

	private:
		void lockPin(LogicalPin);
		bool isUsedPin(LogicalPin lp) { return _pinsUsed & lp; }
		void freePin(LogicalPin);

	public:
		uint32_t toPin(uint32_t logicalPin);
		static const char* uextPin(uint32_t logicalPin);
		Connector(uint32_t idx);
		UART& getUART();
		Spi& getSPI();
		I2C& getI2C();
		DigitalIn& getDigitalIn(LogicalPin);
		DigitalOut& getDigitalOut(LogicalPin);
		ADC& getADC(LogicalPin);
		uint32_t index() {return _connectorIdx;};
		// PWM& getPWM();
};

#endif
