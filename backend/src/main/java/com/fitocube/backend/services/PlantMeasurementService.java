package com.fitocube.backend.services;

import com.fitocube.backend.model.PlantMeasurementDto;
import com.fitocube.backend.model.PlantMeasurementsDto;
import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.repositories.PlantMeasurementRepository;
import java.time.Instant;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import lombok.RequiredArgsConstructor;
import org.springframework.data.domain.PageRequest;
import org.springframework.stereotype.Service;

@Service
@RequiredArgsConstructor
public class PlantMeasurementService {

    private final PlantMeasurementRepository plantMeasurementRepository;

    public void recordMeasurement(PlantStateDto plant, PlantMeasurementsDto measurements) {
        if (plant == null || measurements == null || measurements.getTimestamp() == null) {
            return;
        }

        PlantMeasurementDto entry = new PlantMeasurementDto();
        entry.setPlant(plant);
        entry.setAirTemperatureC(measurements.getAirTemperatureC());
        entry.setAirHumidityPercent(measurements.getAirHumidityPercent());
        entry.setSoilMoisturePercent(measurements.getSoilMoisturePercent());
        entry.setSoilMoistureRaw(measurements.getSoilMoistureRaw());
        entry.setTimestamp(measurements.getTimestamp());
        plantMeasurementRepository.save(entry);
    }

    public List<PlantMeasurementDto> getMeasurements(Long plantId,
                                                     Instant from,
                                                     Instant to,
                                                     Integer limit) {
        if (plantId == null) {
            return Collections.emptyList();
        }
        if (limit != null && limit > 0 && (from == null && to == null)) {
            List<PlantMeasurementDto> recent = plantMeasurementRepository.findByPlant_PlantIdOrderByTimestampDesc(
                    plantId,
                    PageRequest.of(0, limit));
            recent.sort(Comparator.comparing(PlantMeasurementDto::getTimestamp));
            return recent;
        }
        if (from != null && to != null) {
            return plantMeasurementRepository.findByPlant_PlantIdAndTimestampBetweenOrderByTimestampAsc(plantId, from, to);
        }
        return plantMeasurementRepository.findByPlant_PlantIdOrderByTimestampAsc(plantId);
    }
}
