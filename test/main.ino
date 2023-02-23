#include "micro_printf.h"

void setup() {
  Serial.begin(115200);
}

void loop() {
  char buf[100];
  delay(1000);
  m_snprintf(buf, sizeof(buf), "hi, %g", 1.234);
  Serial.println(buf);
}
