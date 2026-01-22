package com.fitocube.backend.web;

import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.model.PlantMeasurementDto;
import com.fitocube.backend.model.request.AutoWateringRequest;
import com.fitocube.backend.model.request.ClaimRequest;
import com.fitocube.backend.model.request.WateringRequest;
import com.fitocube.backend.model.session.SessionUser;
import com.fitocube.backend.services.PlantMeasurementService;
import com.fitocube.backend.services.PlantService;
import com.fitocube.backend.services.SessionService;
import com.fitocube.backend.services.WateringService;
import java.time.Duration;
import java.time.Instant;
import java.time.format.DateTimeParseException;
import java.util.List;
import java.util.Set;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.lang.NonNull;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.util.StringUtils;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;

@RestController
@RequestMapping("/plants")
public class PlantsController {

    private final PlantService plantService;
    private final SessionService sessionService;
    private final WateringService wateringService;
    private final PlantMeasurementService plantMeasurementService;

    public PlantsController(PlantService plantService,
                            SessionService sessionService,
                            WateringService wateringService,
                            PlantMeasurementService plantMeasurementService) {
        this.plantService = plantService;
        this.sessionService = sessionService;
        this.wateringService = wateringService;
        this.plantMeasurementService = plantMeasurementService;
    }

    @GetMapping("/{plantId}")
    public ResponseEntity<PlantStateDto> getPlantById(@PathVariable @NonNull Long plantId, @AuthenticationPrincipal SessionUser user) {
        return plantService.getPlantById(plantId, user)
                .map(ResponseEntity::ok)
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    @GetMapping("/by-owner")
    public ResponseEntity<Set<PlantStateDto>> getAllPlantsByOwner(
            @RequestParam(value = "ownerName", required = false) String requestedOwner) {
        var sessionUser = sessionService.requireSessionUser();
        System.out.println(sessionUser);
        //sessionService.ensureSameUser(requestedOwner, sessionUser);

        var set = plantService.getAllPlantsByOwner(sessionUser.userName());
        return ResponseEntity.ok(set);
    }

    @GetMapping("/by-friend-name")
    public ResponseEntity<Set<PlantStateDto>> listPlantsByFriendName(@RequestParam("friendName") String friendName) {
        throw new ResponseStatusException(HttpStatus.NOT_IMPLEMENTED, "Not implemented yet");
    }

    @GetMapping("/{plantId}/measurements")
    public ResponseEntity<List<PlantMeasurementDto>> getPlantMeasurements(@PathVariable @NonNull Long plantId,
                                                                           @RequestParam(value = "from", required = false) String from,
                                                                           @RequestParam(value = "to", required = false) String to,
                                                                           @RequestParam(value = "limit", required = false) Integer limit,
                                                                           @AuthenticationPrincipal SessionUser user) {
        var plant = plantService.getPlantById(plantId, user);
        if (plant.isEmpty()) {
            return ResponseEntity.notFound().build();
        }

        Instant fromInstant = null;
        Instant toInstant = null;
        if (from != null) {
            fromInstant = parseInstant(from, "from");
        }
        if (to != null) {
            toInstant = parseInstant(to, "to");
        }

        if (fromInstant == null && toInstant == null && (limit == null || limit <= 0)) {
            toInstant = Instant.now();
            fromInstant = toInstant.minus(Duration.ofHours(24));
        } else if (fromInstant == null && toInstant != null) {
            fromInstant = toInstant.minus(Duration.ofHours(24));
        } else if (fromInstant != null && toInstant == null) {
            toInstant = Instant.now();
        }

        if (fromInstant != null && toInstant != null && fromInstant.isAfter(toInstant)) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "from must be before to");
        }

        var measurements = plantMeasurementService.getMeasurements(plantId, fromInstant, toInstant, limit);
        return ResponseEntity.ok(measurements);
    }

    @PostMapping("/claim")
    public ResponseEntity<PlantStateDto> claimPlant(@RequestBody ClaimRequest claimRequest) {
        if (!StringUtils.hasText(claimRequest.getDeviceUid())) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "deviceUid is required");
        }
        var owner = sessionService.requireSessionUserEntity();
        return plantService.claimPlant(owner, claimRequest)
                .map(plant -> ResponseEntity.status(HttpStatus.CREATED).body(plant))
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.CONFLICT, "device already claimed"));
    }

    @PostMapping("/watering")
    public ResponseEntity<Void> triggerWatering(@RequestParam("plantId") @NonNull Long plantId,
                                                @RequestBody(required = false) WateringRequest request,
                                                @AuthenticationPrincipal SessionUser user) {
        int durationSeconds = request != null && request.getDurationSeconds() != null
                ? request.getDurationSeconds()
                : 5;

        if (durationSeconds < 1 || durationSeconds > 600) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "durationSeconds must be between 1 and 600");
        }

        var result = wateringService.triggerWatering(plantId, user, durationSeconds);
        if (result.isEmpty()) {
            return ResponseEntity.notFound().build();
        }
        if (Boolean.TRUE.equals(result.get())) {
            return ResponseEntity.accepted().build();
        }
        return ResponseEntity.status(HttpStatus.SERVICE_UNAVAILABLE).build();
    }

    @PostMapping("/{plantId}/auto-watering")
    public ResponseEntity<PlantStateDto> updateAutoWatering(@PathVariable @NonNull Long plantId,
                                                            @RequestBody AutoWateringRequest request,
                                                            @AuthenticationPrincipal SessionUser user) {
        if (request == null || request.getEnabled() == null) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "enabled is required");
        }
        if (request.getThresholdPercent() != null && (request.getThresholdPercent() < 0 || request.getThresholdPercent() > 100)) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "thresholdPercent must be between 0 and 100");
        }
        if (request.getDurationSeconds() != null && (request.getDurationSeconds() < 1 || request.getDurationSeconds() > 600)) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "durationSeconds must be between 1 and 600");
        }
        if (request.getCooldownSeconds() != null && (request.getCooldownSeconds() < 0 || request.getCooldownSeconds() > 86400)) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "cooldownSeconds must be between 0 and 86400");
        }

        return plantService.updateAutoWatering(
                        plantId,
                        user,
                        request.getEnabled(),
                        request.getThresholdPercent(),
                        request.getDurationSeconds(),
                        request.getCooldownSeconds())
                .map(ResponseEntity::ok)
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    private Instant parseInstant(String value, String field) {
        try {
            return Instant.parse(value);
        } catch (DateTimeParseException ex) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, field + " must be ISO-8601 instant");
        }
    }
}
