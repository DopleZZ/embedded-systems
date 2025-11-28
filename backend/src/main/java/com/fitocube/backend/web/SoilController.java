package com.fitocube.backend.web;

import com.fitocube.backend.mqtt.MqttGateway;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/soil")
public class SoilController {

    private final MqttGateway mqttGateway;

    public SoilController(MqttGateway mqttGateway) {
        this.mqttGateway = mqttGateway;
    }

    @PostMapping("/request")
    public ResponseEntity<?> requestMeasurement() {
        boolean published = mqttGateway.requestFreshMeasurement();
        if (!published) {
            return ResponseEntity.status(HttpStatus.SERVICE_UNAVAILABLE)
                                 .body("MQTT недоступен, команда не была отправлена");
        }

        return ResponseEntity.status(HttpStatus.ACCEPTED)
                             .body("Команда передана устройству, ожидайте измерения");
    }
}
