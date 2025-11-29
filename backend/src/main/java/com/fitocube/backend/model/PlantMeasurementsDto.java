package com.fitocube.backend.model;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonInclude;
import jakarta.persistence.Column;
import jakarta.persistence.Embeddable;
import java.time.Instant;
import lombok.Data;

/**
 * Measurements snapshot published by the low-level firmware.
 */
@Data
@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonIgnoreProperties(ignoreUnknown = true)
@Embeddable
public class PlantMeasurementsDto {

    @Column(name = "air_temperature_c")
    private Double airTemperatureC;

    @Column(name = "air_humidity_percent")
    private Double airHumidityPercent;

    @Column(name = "soil_moisture_percent")
    private Double soilMoisturePercent;

    @Column(name = "soil_moisture_raw")
    private Integer soilMoistureRaw;

    @Column(name = "measurement_timestamp")
    private Instant timestamp;
}
