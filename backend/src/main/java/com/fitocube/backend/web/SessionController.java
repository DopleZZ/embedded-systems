package com.fitocube.backend.web;

import com.fitocube.backend.model.request.LoginRequest;
import com.fitocube.backend.model.session.SessionUser;
import com.fitocube.backend.services.SessionService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpSession;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/session")
public class SessionController {

    private final SessionService sessionService;

    public SessionController(SessionService sessionService) {
        this.sessionService = sessionService;
    }

    @PostMapping("/login")
    public ResponseEntity<SessionUser> login(@RequestBody LoginRequest loginRequest, HttpServletRequest request) {
        var sessionUser = sessionService.login(loginRequest.getUsername(), request);
        return ResponseEntity.ok(sessionUser);
    }

    @GetMapping("/me")
    public ResponseEntity<SessionUser> currentSession() {
        var sessionUser = sessionService.requireSessionUser();
        return ResponseEntity.ok(sessionUser);
    }

    @PostMapping("/logout")
    public ResponseEntity<Void> logout(HttpSession session) {
        sessionService.logout(session);
        return ResponseEntity.noContent().build();
    }
}

