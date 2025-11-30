package com.fitocube.backend.services;

import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.session.SessionUser;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import jakarta.servlet.http.HttpSession;
import java.util.List;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.security.authentication.UsernamePasswordAuthenticationToken;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.core.context.SecurityContext;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.security.web.context.SecurityContextRepository;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;
import org.springframework.web.server.ResponseStatusException;

@Service
public class SessionService {

    private final UserService userService;
    private final SecurityContextRepository securityContextRepository;
    private final PasswordEncoder passwordEncoder;

    public SessionService(UserService userService,
                          SecurityContextRepository securityContextRepository,
                          PasswordEncoder passwordEncoder) {
        this.userService = userService;
        this.securityContextRepository = securityContextRepository;
        this.passwordEncoder = passwordEncoder;
    }

    public SessionUser login(String rawUsername,
                             String rawPassword,
                             HttpServletRequest request,
                             HttpServletResponse response) {
        var username = Optional.ofNullable(rawUsername)
                .map(String::trim)
                .filter(StringUtils::hasText)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.BAD_REQUEST, "Username is required"));

        var password = Optional.ofNullable(rawPassword)
                .filter(StringUtils::hasText)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.BAD_REQUEST, "Password is required"));

        var user = userService.findByUserName(username)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.UNAUTHORIZED, "Unknown user"));

        if (!passwordEncoder.matches(password, user.getPasswordHash())) {
            throw new ResponseStatusException(HttpStatus.UNAUTHORIZED, "Invalid credentials");
        }

        return persistAuthentication(user, request, response);
    }

    public Optional<SessionUser> getCurrentUser() {
        return Optional.ofNullable(SecurityContextHolder.getContext())
                .map(SecurityContext::getAuthentication)
                .filter(Authentication::isAuthenticated)
                .map(Authentication::getPrincipal)
                .filter(SessionUser.class::isInstance)
                .map(SessionUser.class::cast);
    }

    public SessionUser requireSessionUser() {
        return getCurrentUser()
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.UNAUTHORIZED, "No active session"));
    }

    public UserDto requireSessionUserEntity() {
        var sessionUser = requireSessionUser();
        return userService.findById(sessionUser.id())
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.UNAUTHORIZED, "User not found"));
    }

    public void ensureSameUser(String requestedUserName, SessionUser sessionUser) {
        if (StringUtils.hasText(requestedUserName)
                && !sessionUser.userName().equalsIgnoreCase(requestedUserName)) {
            throw new ResponseStatusException(HttpStatus.FORBIDDEN, "Cannot access requested resource");
        }
    }


    public SessionUser persistAuthentication(UserDto user,
                                             HttpServletRequest request,
                                             HttpServletResponse response) {
        var sessionUser = new SessionUser(user.getUserId(), user.getUserName(), user.getDisplayName());
        var authentication = UsernamePasswordAuthenticationToken.authenticated(
                sessionUser,
                null,
                List.of(new SimpleGrantedAuthority("ROLE_USER")));
        var context = SecurityContextHolder.createEmptyContext();
        context.setAuthentication(authentication);
        SecurityContextHolder.setContext(context);
        securityContextRepository.saveContext(context, request, response);
        return sessionUser;
    }
}
