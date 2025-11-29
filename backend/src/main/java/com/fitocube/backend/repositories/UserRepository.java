package com.fitocube.backend.repositories;

import java.util.Optional;

import com.fitocube.backend.model.UserDto;
import org.springframework.data.jpa.repository.JpaRepository;

public interface UserRepository extends JpaRepository<UserDto, Long> {

    Optional<UserDto> findByUserNameIgnoreCase(String userName);
}
