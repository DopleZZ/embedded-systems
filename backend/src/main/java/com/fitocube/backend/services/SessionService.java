package com.fitocube.backend.services;

import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.session.SessionUser;
import com.fitocube.backend.security.SessionUserDetails;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpSession;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.security.authentication.AuthenticationManager;
import org.springframework.security.authentication.UsernamePasswordAuthenticationToken;
import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContext;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.security.web.context.HttpSessionSecurityContextRepository;
import org.springframework.stereotype.Service;
import org.springframework.web.server.ResponseStatusException;

@Service
public class SessionService {

    private final AuthenticationManager authenticationManager;
    private final UserService userService;

    public SessionService(AuthenticationManager authenticationManager, UserService userService) {
        this.authenticationManager = authenticationManager;
        this.userService = userService;
    }

    public SessionUser login(String rawUsername, HttpServletRequest request) {
        var authRequest = UsernamePasswordAuthenticationToken.unauthenticated(rawUsername, "");
        var authentication = authenticationManager.authenticate(authRequest);

        SecurityContext context = SecurityContextHolder.createEmptyContext();
        context.setAuthentication(authentication);
        SecurityContextHolder.setContext(context);

        HttpSession session = request.getSession(true);
        session.setAttribute(HttpSessionSecurityContextRepository.SPRING_SECURITY_CONTEXT_KEY, context);

        return extractSessionUser(authentication);
    }

    public Optional<SessionUser> getCurrentUser() {
        return Optional.ofNullable(SecurityContextHolder.getContext().getAuthentication())
                .filter(Authentication::isAuthenticated)
                .filter(authentication -> authentication.getPrincipal() instanceof SessionUserDetails)
                .map(this::extractSessionUser);
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
        if (requestedUserName != null
                && !requestedUserName.isBlank()
                && !sessionUser.userName().equalsIgnoreCase(requestedUserName)) {
            throw new ResponseStatusException(HttpStatus.FORBIDDEN, "Cannot access requested resource");
        }
    }

    public void logout(HttpSession session) {
        SecurityContextHolder.clearContext();
        if (session == null) {
            return;
        }
        try {
            session.invalidate();
        } catch (IllegalStateException ignored) {
            // already invalidated
        }
    }

    private SessionUser extractSessionUser(Authentication authentication) {
        var principal = authentication.getPrincipal();
        if (principal instanceof SessionUserDetails details) {
            return details.toSessionUser();
        }
        throw new ResponseStatusException(HttpStatus.UNAUTHORIZED, "Invalid session state");
    }
}

