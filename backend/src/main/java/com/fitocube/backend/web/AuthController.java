package com.fitocube.backend.web;

import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.request.CreateUserRequest;
import com.fitocube.backend.model.request.LoginRequest;
import com.fitocube.backend.model.session.SessionUser;
import com.fitocube.backend.services.SessionService;
import com.fitocube.backend.services.UserService;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/auth")
public class AuthController {

    private final UserService userService;
    private final SessionService sessionService;

    public AuthController(UserService userService, SessionService sessionService) {
        this.userService = userService;
        this.sessionService = sessionService;
    }

    @PostMapping("/register")
    public ResponseEntity<UserDto> register(@RequestBody CreateUserRequest request,
                                            HttpServletRequest servletRequest,
                                            HttpServletResponse servletResponse) {
        var user = userService.registerUser(request.getUserName(), request.getDisplayName(), request.getPassword());
        sessionService.persistAuthentication(user, servletRequest, servletResponse);
        return ResponseEntity.status(HttpStatus.CREATED).body(user);
    }

    @PostMapping("/login")
    public ResponseEntity<SessionUser> login(@RequestBody LoginRequest request,
                                             HttpServletRequest servletRequest,
                                             HttpServletResponse servletResponse) {
        var sessionUser = sessionService.login(request.getUserName(), request.getPassword(), servletRequest, servletResponse);
        return ResponseEntity.ok(sessionUser);
    }
}
