package com.fitocube.backend.mqtt;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fitocube.backend.config.MqttProperties;
import com.fitocube.backend.model.PlantMeasurementsDto;
import com.fitocube.backend.model.PlantStateDto;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.springframework.stereotype.Component;

@Component
@Slf4j
@RequiredArgsConstructor
public class MqttGateway implements MqttCallback {

    private final MqttProperties properties;
    private final ObjectMapper objectMapper;
    
    private MqttClient client;

    @PostConstruct
    void init() {
        connect();
    }

    @PreDestroy
    void shutdown() {
        if (client != null && client.isConnected()) {
            try {
                client.disconnect();
            }
            catch (MqttException e) {
                log.warn("Не удалось корректно отключиться от MQTT", e);
            }
        }
    }

    private void connect() {
        try {
            String clientId = properties.getClientId();
            if (clientId == null || clientId.isBlank()) {
                clientId = "fitocube-backend-" + UUID.randomUUID();
            }

            client = new MqttClient(properties.getBrokerUri(), clientId, new MemoryPersistence());
            client.setCallback(this);

            MqttConnectOptions options = new MqttConnectOptions();
            options.setAutomaticReconnect(true);
            options.setCleanSession(true);
            if (properties.getUsername() != null && !properties.getUsername().isBlank()) {
                options.setUserName(properties.getUsername());
                options.setPassword(properties.getPassword() == null ? new char[0] : properties.getPassword().toCharArray());
            }

            client.connect(options);
            client.subscribe(properties.getDataTopic());
            log.info("Подключились к MQTT {} и подписались на {}",
                     properties.getBrokerUri(),
                     properties.getDataTopic());
        }
        catch (MqttException e) {
            throw new IllegalStateException("Не удалось подключиться к MQTT", e);
        }
    }

    public boolean requestFreshMeasurement() {
        if (client == null || !client.isConnected()) {
            log.warn("MQTT клиент не подключен");
            return false;
        }

        try {
            MqttMessage message = new MqttMessage(properties.getCommandPayload().getBytes(StandardCharsets.UTF_8));
            message.setQos(1);
            client.publish(properties.getCommandTopic(), message);
            log.info("Опубликована команда {} в {}",
                     properties.getCommandPayload(),
                     properties.getCommandTopic());
        }
        catch (Exception e) {
            log.error("Ошибка при отправке команды через MQTT", e);
            return false;
        }
        return true;
    }

    @Override
    public void connectionLost(Throwable cause) {
        log.warn("Связь с MQTT потеряна: {}", cause.getMessage());
    }

    @Override
    public void messageArrived(String topic, MqttMessage message) {
        if (!properties.getDataTopic().equals(topic)) {
            return;
        }

        try {
            PlantStateDto plantState = objectMapper.readValue(message.getPayload(), PlantStateDto.class);
            PlantMeasurementsDto measurements = plantState.getMeasurements();
            if (measurements == null) {
                log.warn("Получено сообщение без блока measurements, пропускаем: {}", message);
                return;
            }


            //TODO сохранение в бд
        }
        catch (Exception e) {
            log.error("Не удалось распарсить MQTT сообщение", e);
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // noop
    }
}
