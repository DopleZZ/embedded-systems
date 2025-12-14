package com.fitocube.backend.services;


import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.request.ClaimRequest;
import com.fitocube.backend.model.session.SessionUser;
import com.fitocube.backend.repositories.PlantStateRepository;
import com.fitocube.backend.repositories.UserRepository;
import jakarta.transaction.Transactional;
import lombok.extern.slf4j.Slf4j;
import org.springframework.security.core.annotation.AuthenticationPrincipal;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;
import org.springframework.lang.NonNull;

import java.util.Optional;
import java.util.Set;

@Slf4j
@Service
public class PlantService {

    private final PlantStateRepository plantStateRepository;
    private final UserRepository userRepository;

    public PlantService(PlantStateRepository plantStateRepository,
                        SessionService sessionService,
                        UserRepository userRepository) {
        this.plantStateRepository = plantStateRepository;
        this.userRepository = userRepository;
    }

    public Optional<PlantStateDto> savePlant(PlantStateDto dto){
        return plantStateRepository.findByDeviceUid(dto.getDeviceUid())
                .map(existing -> {
                    existing.setMeasurements(dto.getMeasurements());
                    if (dto.getMood() != null) {
                        existing.setMood(dto.getMood());
                    }
                    if (dto.getFriendVisible() != null) {
                        existing.setFriendVisible(dto.getFriendVisible());
                    }
                    return plantStateRepository.save(existing);
                });
    }

    public Optional<PlantStateDto> getPlantById(@NonNull Long id, @AuthenticationPrincipal SessionUser user){

        return plantStateRepository.findById(id)
                .filter(plant -> plant.getOwner() != null && plant.getOwner().getUserName().equals(user.userName()));


    }

    @Transactional
    public Optional<PlantStateDto> updateAutoWatering(@NonNull Long plantId,
                                                      @NonNull SessionUser user,
                                                      boolean enabled,
                                                      Integer thresholdPercent,
                                                      Integer durationSeconds,
                                                      Integer cooldownSeconds) {
        return plantStateRepository.findById(plantId)
                .filter(plant -> plant.getOwner() != null && plant.getOwner().getUserName().equals(user.userName()))
                .map(plant -> {
                    plant.setAutoWateringEnabled(enabled);
                    if (thresholdPercent != null) {
                        plant.setAutoWateringThresholdPercent(thresholdPercent);
                    }
                    if (durationSeconds != null) {
                        plant.setAutoWateringDurationSeconds(durationSeconds);
                    }
                    if (cooldownSeconds != null) {
                        plant.setAutoWateringCooldownSeconds(cooldownSeconds);
                    }
                    return plantStateRepository.save(plant);
                });
    }

    public Set<PlantStateDto> getAllPlantsByOwner(String username) {
        return plantStateRepository.findAllByOwner_UserName(username);
    }


    @Transactional
    public Optional<PlantStateDto> claimPlant(UserDto owner, ClaimRequest req) {
        var existing = plantStateRepository.findByDeviceUid(req.getDeviceUid());
        if (existing.isPresent()) {
            var plant = existing.get();
            if (plant.getOwner() != null
                    && !plant.getOwner().getUserId().equals(owner.getUserId())) {
                return Optional.empty();
            }
            plant.setOwner(owner);
            if (StringUtils.hasText(req.getNickname())) {
                plant.setNickname(req.getNickname());
            }
            return Optional.of(plantStateRepository.save(plant));
        }

        PlantStateDto plant = new PlantStateDto();
        plant.setDeviceUid(req.getDeviceUid());
        plant.setNickname(req.getNickname());
        plant.setOwner(owner);
        return Optional.of(plantStateRepository.save(plant));
    }

}
