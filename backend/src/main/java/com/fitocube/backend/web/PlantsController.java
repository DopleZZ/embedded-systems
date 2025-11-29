package com.fitocube.backend.web;


import com.fitocube.backend.model.PlantStateDto;
import com.fitocube.backend.model.request.ClaimRequest;
import com.fitocube.backend.services.PlantService;
import jakarta.annotation.PostConstruct;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.service.annotation.GetExchange;

import java.util.List;
import java.util.Set;

@RestController
@RequestMapping("/plants")
@CrossOrigin({"*"})
public class PlantsController {

    private final PlantService plantService;

    public PlantsController(PlantService plantService) {
        this.plantService = plantService;
    }

    @GetMapping("/{plantId}")
    public ResponseEntity<PlantStateDto> getPlantById(@PathVariable Long plantId) {
        return plantService.getPlantById(plantId)
                .map(ResponseEntity::ok)
                .orElseGet(() -> ResponseEntity.notFound().build());
    }

    @GetMapping("/by-owner")
    public ResponseEntity<Set<PlantStateDto>> getAllPlantsByOwner(@RequestParam("ownerName") String username) {
        var set = plantService.getAllPlantsByOwner(username);
        return set.isEmpty() ? ResponseEntity.notFound().build() : ResponseEntity.ok(set);
    }


    //TODO после аутентификации добавить
//    @PostMapping("/claim")
//    public ResponseEntity<PlantStateDto> claimPlant(ClaimRequest claimRequest){
//        return plantService.claimPlant(claimRequest)
//                .map(ResponseEntity::ok)
//                .orElse(ResponseEntity.badRequest().build());
//    }



}
