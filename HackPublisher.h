#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <string>

class HackPublisher {
  private:
    StaticJsonDocument<200> data;
    bool debug;
    std::string topic;
    const char* broker;
    const int port;
    WiFiClient espClient;
    PubSubClient client;

    char* serializeDocToCString() {
      // Calculate the size needed for serialization
      size_t size = measureJson(data);
  
      // Allocate a buffer to store the JSON string (don't forget the null terminator)
      char* buffer = new char[size + 1];
  
      // Serialize the JSON document to the buffer
      serializeJson(data, buffer, size + 1);
  
      // Store the buffer in a temp var
      char* result = buffer;
  
      // Free the buffer to avoid memory leaks
      delete[] buffer;

      return result;
    }

    void reconnect() {
      // Loop until we're reconnected
      while (!client.connected()) {
        Serial.println("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str())) {
          Serial.println("Connected to MQTT broker");
        }
        else {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println("; trying again in 5 seconds");
          delay(5000);
        }
      }
    }

  public:
    HackPublisher(const char* topic, bool debug = false, const char* broker = "broker.hivemq.com", const int port = 1883)
    : topic("ucla/hack/" + std::string(topic)), debug(debug), broker(broker), port(port), client(espClient) {
      client.setServer(this->broker, this->port);
    }

    void begin() {
      reconnect();
    }
    
    template<typename T>
    void store(const char* sensor, const T& value) {
      if (debug) {
        Serial.print("[STORE DATA] sensor = ");
        Serial.print(sensor);
        Serial.print(", value = ");
        Serial.println(value);
      }
      
      data[sensor] = value;
    }

    void send() {
      if (!client.connected()) {
        reconnect();
      }
      client.loop();

      if (debug) {
        Serial.println("[SEND DATA]");
        serializeJsonPretty(data, Serial);
        Serial.println();
      }
      
      client.publish(topic.c_str(), serializeDocToCString());
      data.clear();
    }

    const char* getTopic() const {
        return topic.c_str();
    }

    const char* getBroker() const {
        return broker;
    }

    const int getPort() const {
        return port;
    }
};
