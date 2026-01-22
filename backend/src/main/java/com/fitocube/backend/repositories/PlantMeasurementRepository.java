package com.fitocube.backend.repositories;

import com.fitocube.backend.model.PlantMeasurementDto;
import java.time.Instant;
import java.util.List;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;

public interface PlantMeasurementRepository extends JpaRepository<PlantMeasurementDto, Long> {

    List<PlantMeasurementDto> findByPlant_PlantIdOrderByTimestampAsc(Long plantId);

    List<PlantMeasurementDto> findByPlant_PlantIdAndTimestampBetweenOrderByTimestampAsc(Long plantId,
                                                                                         Instant from,
                                                                                         Instant to);

    List<PlantMeasurementDto> findByPlant_PlantIdOrderByTimestampDesc(Long plantId, Pageable pageable);
}
