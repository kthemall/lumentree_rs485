\# Lumentree SUNT 4K – ESP8266 MQTT Monitor



With this Arduino project, you can read selected Modbus registers from a

\*\*Lumentree SUNT 4K\*\* inverter via RS485 and publish the values to an MQTT server.



Tested with a \*\*Lumentree SUNT 4K\*\*.



\## 📊 Monitored Values



The following values are read from the inverter:



| Value | Modbus Register | MQTT Topic | Unit |

|---|---:|---|---|

| Grid Power | `59` | `lumentree2/grid\_power` | W |

| Battery Power | `61` | `lumentree2/battery\_power` | W |

| Home Load | `67` | `lumentree2/home\_load` | W |



The values are published every \*\*5 seconds\*\*.



\## 🛠️ Hardware



\- ESP8266 ESP-12F

\- RS485 module

\- WiFi connection



\## ⚙️ Modbus Configuration



| Setting | Value |

|---|---|

| Protocol | Modbus RTU |

| Baud rate | `9600` |

| Slave ID | `1` |

| Function | `03 – Read Holding Registers` |



\## 📡 MQTT



Configure your MQTT connection in the Arduino sketch:



```cpp

const char\* mqtt\_server = "";

const int   mqtt\_port   = 1883;

const char\* mqtt\_user   = "";

const char\* mqtt\_pass   = "";

