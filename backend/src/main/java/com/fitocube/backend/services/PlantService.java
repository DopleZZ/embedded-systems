package com.fitocube.backend.services;


import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.request.ClaimRequest;
import com.fitocube.backend.repositories.PlantStateRepository;
import jakarta.transaction.Transactional;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;
import org.springframework.lang.NonNull;

import java.util.Optional;
import java.util.Set;

@Slf4j
@Service
public class PlantService {

    private final PlantStateRepository plantStateRepository;

    public PlantService(PlantStateRepository plantStateRepository) {
        this.plantStateRepository = plantStateRepository;
    }

    public void savePlant(PlantStateDto dto){
        plantStateRepository.findByDeviceUid(dto.getDeviceUid())
                .ifPresent(existing -> {
                    dto.setPlantId(existing.getPlantId());
                    plantStateRepository.save(dto);
                });
    }

    public Optional<PlantStateDto> getPlantById(@NonNull Long id){
        return plantStateRepository.findById(id);

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
