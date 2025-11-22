package com.fitocube.backend.config;

import java.time.Duration;

import lombok.Data;
import org.springframework.boot.context.properties.ConfigurationProperties;

@Data
@ConfigurationProperties(prefix = "mqtt")
public class MqttProperties {

    private String brokerUri = "tcp://localhost:1883";
    private String clientId = "fitocube-backend";
    private String username;
    private String password;
    private String commandTopic = "soil/cmd";
    private String dataTopic = "soil/data";
    private String commandPayload = "get_info";
    private Duration responseTimeout = Duration.ofSeconds(5);
}
