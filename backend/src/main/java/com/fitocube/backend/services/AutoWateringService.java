package com.fitocube.backend.services;

import com.fitocube.backend.model.PlantMeasurementsDto;
import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.model.enums.Mood;
import com.fitocube.backend.repositories.PlantStateRepository;
import java.time.Duration;
import java.time.Instant;
import java.util.Optional;
import lombok.RequiredArgsConstructor;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

@Service
@RequiredArgsConstructor
public class AutoWateringService {

    public record Decision(int durationSeconds, int thresholdPercent, int cooldownSeconds) {}

    private static final int DEFAULT_THRESHOLD_PERCENT = 25;
    private static final int DEFAULT_DURATION_SECONDS = 5;
    private static final int DEFAULT_COOLDOWN_SECONDS = 3600;

    private final PlantStateRepository plantStateRepository;

    public Optional<Decision> evaluate(PlantStateDto plant, Instant now) {
        if (plant == null || plant.getPlantId() == null) {
            return Optional.empty();
        }

        Boolean enabled = Optional.ofNullable(plant.getAutoWateringEnabled()).orElse(false);
        if (!enabled) {
            return Optional.empty();
        }

        PlantMeasurementsDto measurements = plant.getMeasurements();
        if (measurements == null) {
            return Optional.empty();
        }

        int threshold = Optional.ofNullable(plant.getAutoWateringThresholdPercent())
                .filter(v -> v >= 0 && v <= 100)
                .orElse(DEFAULT_THRESHOLD_PERCENT);
        int durationSeconds = Optional.ofNullable(plant.getAutoWateringDurationSeconds())
                .filter(v -> v >= 1 && v <= 600)
                .orElse(DEFAULT_DURATION_SECONDS);
        int cooldownSeconds = Optional.ofNullable(plant.getAutoWateringCooldownSeconds())
                .filter(v -> v >= 0 && v <= 86400)
                .orElse(DEFAULT_COOLDOWN_SECONDS);

        Instant last = plant.getLastWateringAt();
        if (last != null && cooldownSeconds > 0) {
            if (Duration.between(last, now).getSeconds() < cooldownSeconds) {
                return Optional.empty();
            }
        }

        boolean needsWater = false;
        Double soil = measurements.getSoilMoisturePercent();
        if (soil != null && soil < (double) threshold) {
            needsWater = true;
        }

        Mood mood = plant.getMood();
        if (mood == Mood.DRY || mood == Mood.THIRSTY) {
            needsWater = true;
        }

        if (!needsWater) {
            return Optional.empty();
        }

        return Optional.of(new Decision(durationSeconds, threshold, cooldownSeconds));
    }

    @Transactional
    public void markWatered(Long plantId, Instant wateredAt, Decision decision) {
        if (plantId == null || wateredAt == null || decision == null) {
            return;
        }

        plantStateRepository.findById(plantId).ifPresent(plant -> {
            plant.setLastWateringAt(wateredAt);
            plant.setAutoWateringThresholdPercent(decision.thresholdPercent());
            plant.setAutoWateringDurationSeconds(decision.durationSeconds());
            plant.setAutoWateringCooldownSeconds(decision.cooldownSeconds());
            plantStateRepository.save(plant);
        });
    }
}

