package com.fitocube.backend.services;

import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.repositories.UserRepository;
import jakarta.annotation.PostConstruct;
import java.util.Optional;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.HttpStatus;
import org.springframework.lang.NonNull;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.StringUtils;
import org.springframework.web.server.ResponseStatusException;

@Service
@Slf4j
public class UserService {

    private final UserRepository userRepository;
    private final PasswordEncoder passwordEncoder;

    public UserService(UserRepository userRepository, PasswordEncoder passwordEncoder) {
        this.userRepository = userRepository;
        this.passwordEncoder = passwordEncoder;
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

    @Transactional
    public UserDto registerUser(String rawUserName, String rawDisplayName, String rawPassword) {
        var userName = Optional.ofNullable(rawUserName)
                .map(String::trim)
                .filter(StringUtils::hasText)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.BAD_REQUEST, "userName is required"));

        var password = Optional.ofNullable(rawPassword)
                .map(String::trim)
                .filter(StringUtils::hasText)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.BAD_REQUEST, "password is required"));

        var displayName = Optional.ofNullable(rawDisplayName)
                .map(String::trim)
                .filter(StringUtils::hasText)
                .orElse(userName);

        var existing = userRepository.findByUserNameIgnoreCase(userName);
        if (existing.isPresent()) {
            throw new ResponseStatusException(HttpStatus.CONFLICT, "User already exists");
        }

        var user = new UserDto();
        user.setUserName(userName);
        user.setDisplayName(displayName);
        user.setPasswordHash(passwordEncoder.encode(password));
        return userRepository.save(user);
    }
}
