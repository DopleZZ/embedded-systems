package com.fitocube.backend.services;

import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.repositories.UserRepository;
import jakarta.annotation.PostConstruct;
import java.util.Optional;
import lombok.extern.slf4j.Slf4j;
import org.springframework.lang.NonNull;
import org.springframework.stereotype.Service;

@Service
@Slf4j
public class UserService {

    private final UserRepository userRepository;

    public UserService(UserRepository userRepository) {
        this.userRepository = userRepository;
    }

    @PostConstruct
    public void init() {
        var size = userRepository.count();
        log.info("size is" + size);
    }

    public Optional<UserDto> findByUserName(String userName) {
        return userRepository.findByUserNameIgnoreCase(userName);
    }

    public Optional<UserDto> findById(@NonNull Long userId) {
        return userRepository.findById(userId);
    }
}
