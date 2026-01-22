package com.fitocube.backend.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.FetchType;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.JoinColumn;
import jakarta.persistence.ManyToOne;
import jakarta.persistence.SequenceGenerator;
import jakarta.persistence.Table;
import java.time.Instant;
import lombok.Data;

@Data
@Entity
@Table(name = "plant_measurements")
public class PlantMeasurementDto {

    @Id
    @GeneratedValue(strategy = GenerationType.SEQUENCE, generator = "plant_measurement_seq")
    @SequenceGenerator(name = "plant_measurement_seq", sequenceName = "plant_measurement_seq", allocationSize = 1)
    private Long measurementId;

    @ManyToOne(fetch = FetchType.LAZY)
    @JoinColumn(name = "plant_id", nullable = false)
    @JsonIgnore
    private PlantStateDto plant;

    @Column(name = "air_temperature_c")
    private Double airTemperatureC;

    @Column(name = "air_humidity_percent")
    private Double airHumidityPercent;

    @Column(name = "soil_moisture_percent")
    private Double soilMoisturePercent;

    @Column(name = "soil_moisture_raw")
    private Integer soilMoistureRaw;

    @Column(name = "measurement_timestamp", nullable = false)
    private Instant timestamp;
}
