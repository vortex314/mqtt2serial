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
