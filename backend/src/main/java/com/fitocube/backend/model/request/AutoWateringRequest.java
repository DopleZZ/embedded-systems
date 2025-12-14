package com.fitocube.backend.model.request;

import lombok.Data;

@Data
public class AutoWateringRequest {

    private Boolean enabled;

    private Integer thresholdPercent;

    private Integer durationSeconds;

    private Integer cooldownSeconds;
}

