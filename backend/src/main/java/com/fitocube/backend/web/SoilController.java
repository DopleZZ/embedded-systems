package com.fitocube.backend.web;

import com.fitocube.backend.model.SoilMeasurement;
import com.fitocube.backend.mqtt.MqttGateway;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
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

    @GetMapping("/last")
    public ResponseEntity<SoilMeasurement> lastMeasurement() {
        return mqttGateway.getLastMeasurement()
            .map(ResponseEntity::ok)
            .orElseGet(() -> ResponseEntity.noContent().build());
    }

    @PostMapping("/request")
    public ResponseEntity<?> requestMeasurement() {
        Optional<SoilMeasurement> measurement = mqttGateway.requestFreshMeasurement();
        return measurement
            .<ResponseEntity<?>>map(ResponseEntity::ok)
            .orElseGet(() -> ResponseEntity.status(HttpStatus.ACCEPTED)
                                            .body("Команда отправлена, но новый ответ не получен"));
    }
}
