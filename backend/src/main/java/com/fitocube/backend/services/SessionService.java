package com.fitocube.backend.services;

import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.session.SessionUser;
import jakarta.servlet.http.HttpSession;
import java.util.Optional;
import org.springframework.http.HttpStatus;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;
import org.springframework.web.server.ResponseStatusException;

@Service
public class SessionService {

    private static final String SESSION_ATTRIBUTE = "FITOCUBE_SESSION_USER";

    private final UserService userService;

    public SessionService(UserService userService) {
        this.userService = userService;
    }

    public SessionUser login(String rawUsername, HttpSession session) {
        var username = Optional.ofNullable(rawUsername)
                .map(String::trim)
                .filter(StringUtils::hasText)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.BAD_REQUEST, "Username is required"));

        var user = userService.findByUserName(username)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.UNAUTHORIZED, "Unknown user"));

        var sessionUser = new SessionUser(user.getUserId(), user.getUserName(), user.getDisplayName());
        session.setAttribute(SESSION_ATTRIBUTE, sessionUser);
        return sessionUser;
    }

    public Optional<SessionUser> getCurrentUser(HttpSession session) {
        return Optional.ofNullable(session)
                .map(s -> s.getAttribute(SESSION_ATTRIBUTE))
                .map(SessionUser.class::cast);
    }

    public SessionUser requireSessionUser(HttpSession session) {
        return getCurrentUser(session)
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.UNAUTHORIZED, "No active session"));
    }

    public UserDto requireSessionUserEntity(HttpSession session) {
        var sessionUser = requireSessionUser(session);
        return userService.findById(sessionUser.id())
                .orElseThrow(() -> new ResponseStatusException(HttpStatus.UNAUTHORIZED, "User not found"));
    }

    public void ensureSameUser(String requestedUserName, SessionUser sessionUser) {
        if (StringUtils.hasText(requestedUserName)
                && !sessionUser.userName().equalsIgnoreCase(requestedUserName)) {
            throw new ResponseStatusException(HttpStatus.FORBIDDEN, "Cannot access requested resource");
        }
    }

    public void logout(HttpSession session) {
        if (session == null) {
            return;
        }
        try {
            session.invalidate();
        } catch (IllegalStateException ignored) {
            // session already invalidated
        }
    }
}

