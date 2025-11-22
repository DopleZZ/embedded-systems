package com.fitocube.backend.model;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

@Data
public class SoilMeasurement {

    @JsonProperty("timestamp_ms")
    private long timestampMs;

    @JsonProperty("raw")
    private int raw;

    @JsonProperty("voltage_mv")
    private int voltageMv;
}
