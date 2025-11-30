package com.fitocube.backend.web;

import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.model.request.ClaimRequest;
import com.fitocube.backend.services.PlantService;
import com.fitocube.backend.services.SessionService;
import java.util.Set;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.lang.NonNull;
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

    public PlantsController(PlantService plantService, SessionService sessionService) {
        this.plantService = plantService;
        this.sessionService = sessionService;
    }

    @GetMapping("/{plantId}")
    public ResponseEntity<PlantStateDto> getPlantById(@PathVariable @NonNull Long plantId) {
        return plantService.getPlantById(plantId)
                .map(ResponseEntity::ok)
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    @GetMapping("/by-owner")
    public ResponseEntity<Set<PlantStateDto>> getAllPlantsByOwner(
            @RequestParam(value = "ownerName", required = false) String requestedOwner) {
        var sessionUser = sessionService.requireSessionUser();
        sessionService.ensureSameUser(requestedOwner, sessionUser);

        var set = plantService.getAllPlantsByOwner(sessionUser.userName());
        return set.isEmpty() ? ResponseEntity.notFound().build() : ResponseEntity.ok(set);
    }

    @PostMapping("/claim")
    public ResponseEntity<PlantStateDto> claimPlant(@RequestBody ClaimRequest claimRequest) {
        if (!StringUtils.hasText(claimRequest.getDeviceUid())) {
            throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "deviceUid is required");
        }
        var owner = sessionService.requireSessionUserEntity();
        return plantService.claimPlant(owner, claimRequest)
                .map(ResponseEntity::ok)
                .orElseGet(() -> ResponseEntity.badRequest().build());
    }
}
