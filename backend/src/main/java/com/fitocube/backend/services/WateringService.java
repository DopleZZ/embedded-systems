package com.fitocube.backend.services;

import com.fitocube.backend.model.session.SessionUser;
import com.fitocube.backend.mqtt.MqttGateway;
import com.fitocube.backend.repositories.PlantStateRepository;
import java.util.Optional;
import lombok.RequiredArgsConstructor;
import org.springframework.lang.NonNull;
import org.springframework.stereotype.Service;

@Service
@RequiredArgsConstructor
public class WateringService {

    private final PlantStateRepository plantStateRepository;
    private final MqttGateway mqttGateway;

    /**
     * @return empty если растение не найдено/не принадлежит пользователю; иначе статус публикации команды в MQTT.
     */
    public Optional<Boolean> triggerWatering(@NonNull Long plantId,
                                             @NonNull SessionUser user,
                                             int durationSeconds) {
        return plantStateRepository.findById(plantId)
                .filter(plant -> plant.getOwner() != null && plant.getOwner().getUserName().equals(user.userName()))
                .map(plant -> mqttGateway.requestWatering(plant.getDeviceUid(), durationSeconds));
    }
}

