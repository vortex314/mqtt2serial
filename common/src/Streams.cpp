#include "Streams.h"

namespace std {
void __throw_length_error(char const *) {
  Serial.println("__throw_length_error");
  while (1)
    ;
}
void __throw_bad_alloc() {
  Serial.println("__throw_bad_alloc");
  while (1)
    ;
}
void __throw_bad_function_call() {
  Serial.println("__throw_bad_function_call");
  while (1)
    ;
}
}  // namespace std
#ifdef ARDUINO


Thread::Thread() {};

void Thread::awakeRequestable(Requestable* rq) {};
void Thread::awakeRequestableFromIsr(Requestable* rq) {};

void Thread::run() { // ARDUINO single thread version ==> continuous polling
	for(auto timer : _timers) timer->request();
}


void* Thread::id() {
	return (void*)1;
}

void* Thread::currentId() {
	return (void*)1;
}

#endif

#ifdef ESP32_IDF
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
nvs_handle _nvs=0;
class ConfigStore
{
    static nvs_handle _nvs;

  public:
    static void init()
    {
	if(_nvs == 0) return;
	esp_err_t err = nvs_flash_init();
	if(err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	    ESP_ERROR_CHECK(nvs_flash_erase());
	    err = nvs_flash_init();
	}
	err = nvs_open("storage", NVS_READWRITE, &_nvs);
	if(err != ESP_OK) WARN(" non-volatile storage open fails.");
    }
    bool load(const char* name, void* value, uint32_t length)
    {
	size_t required_size = length;
	esp_err_t err = nvs_get_blob(_nvs, name, value, &required_size);
	if(err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return false;
	return true;
    }

    bool save(const char* name, void* value, uint32_t length)
    {
	INFO(" Config saved : %s ", name);
	esp_err_t err = nvs_set_blob(_nvs, name, value, length);
	if(err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return false;
	nvs_commit(_nvs);
	return true;
    }
    bool load(const char* name, std::string& value)
    {
	char buffer[256];
	size_t required_size = sizeof(buffer);
	esp_err_t err = nvs_get_str(_nvs, name, buffer, &required_size);
	if(err == ESP_OK) {
	    INFO("found %s", name);
	    value = buffer;
	    return true;
	}
	return false;
    }

    bool save(const char* name, std::string& value)
    {
	char buffer[256];
	strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
	esp_err_t err = nvs_set_str(_nvs, name, buffer);
	if(err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return false;
	nvs_commit(_nvs);
	return true;
    }
};

#endif