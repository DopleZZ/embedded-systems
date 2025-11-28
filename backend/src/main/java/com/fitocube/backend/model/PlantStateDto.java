package com.fitocube.backend.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fitocube.backend.model.enums.Mood;
import lombok.Data;
import lombok.ToString;

@Data
@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonIgnoreProperties(ignoreUnknown = true)
@ToString
public class PlantStateDto {

    private Long plantId;
    private String deviceUid;
    private UserDto owner;
    private String nickname;
    private PlantMeasurementsDto measurements;
    @JsonIgnore
    private Mood mood;
    private Boolean friendVisible;
}
