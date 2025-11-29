package com.fitocube.backend.repositories;

import com.fitocube.backend.model.PlantStateDto;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.List;
import java.util.Optional;
import java.util.Set;

public interface PlantStateRepository extends JpaRepository<PlantStateDto, Long> {

    Optional<PlantStateDto> findByDeviceUid(String deviceUid);

    Set<PlantStateDto> findAllByOwner_UserName(String username);

    boolean existsByDeviceUid(String deviceUid);
}
