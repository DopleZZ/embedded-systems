package com.fitocube.backend.model;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fitocube.backend.model.enums.Mood;
import jakarta.persistence.*;

import java.time.Instant;
import lombok.Data;
import lombok.ToString;

@Data
@JsonInclude(JsonInclude.Include.NON_NULL)
@JsonIgnoreProperties(ignoreUnknown = true)
@ToString
@Entity
@Table(name = "plant_states")
public class PlantStateDto {

    @Id
    @GeneratedValue(strategy = GenerationType.SEQUENCE)
    private Long plantId;

    @Column(name = "device_uid", nullable = false, unique = true)
    private String deviceUid;

    @ManyToOne
    @JoinColumn(name = "owner_id")
    private UserDto owner;

    @Column(name = "nickname")
    private String nickname;

    @Embedded
    private PlantMeasurementsDto measurements;

    @Enumerated(EnumType.STRING)
    @Column(name = "mood")
    private Mood mood;

    @Column(name = "friend_visible")
    private Boolean friendVisible;

    @Column(name = "auto_watering_enabled")
    private Boolean autoWateringEnabled;

    @Column(name = "auto_watering_threshold_percent")
    private Integer autoWateringThresholdPercent;

    @Column(name = "auto_watering_duration_seconds")
    private Integer autoWateringDurationSeconds;

    @Column(name = "auto_watering_cooldown_seconds")
    private Integer autoWateringCooldownSeconds;

    @Column(name = "last_watering_at")
    private Instant lastWateringAt;

}
