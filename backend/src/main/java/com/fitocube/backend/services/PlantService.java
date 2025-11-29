package com.fitocube.backend.services;


import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.model.request.ClaimRequest;
import com.fitocube.backend.repositories.PlantStateRepository;
import com.fitocube.backend.repositories.UserRepository;
import jakarta.annotation.PostConstruct;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.util.List;
import java.util.Optional;
import java.util.Set;

@Slf4j
@Service
public class PlantService {

    private final PlantStateRepository plantStateRepository;
    private final UserRepository userRepository;

    public PlantService(PlantStateRepository plantStateRepository, UserRepository userRepository) {
        this.plantStateRepository = plantStateRepository;
        this.userRepository = userRepository;
    }

    public void savePlant(PlantStateDto dto){
        plantStateRepository.findByDeviceUid(dto.getDeviceUid())
                .ifPresent(existing -> {
                    dto.setPlantId(existing.getPlantId());
                    plantStateRepository.save(dto);
                });
    }

    public Optional<PlantStateDto> getPlantById(Long id){
        return plantStateRepository.findById(id);

    }

    public Set<PlantStateDto> getAllPlantsByOwner(String username) {
        return plantStateRepository.findAllByOwner_UserName(username);
    }


    // TODO после аутентификации уже сделать
//    public Optional<PlantStateDto> claimPlant(ClaimRequest req) {
//        return plantStateRepository.findByDeviceUid(req.getDeviceUid())
//                .or(() -> userRepository.findByUserNameIgnoreCase(req.getNickname())
//                        .map(owner -> {
//                            PlantStateDto plant = new PlantStateDto();
//                            plant.setDeviceUid(req.getDeviceUid());
//                            plant.setNickname(req.getNickname());
//                            plant.setOwner(owner);
//                            plantStateRepository.save(plant);
//                            return plant;
//                        }));
//    }

}
