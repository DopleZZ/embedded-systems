package com.fitocube.backend.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonInclude;
import java.time.Instant;
import lombok.Data;

/**
 * Measurements snapshot published by the low-level firmware.
 */
@Data
@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonIgnoreProperties(ignoreUnknown = true)
public class PlantMeasurementsDto {

    private Double airTemperatureC;
    private Double airHumidityPercent;
    private Double soilMoisturePercent;
    private Integer soilMoistureRaw;
    private Instant timestamp;
}
