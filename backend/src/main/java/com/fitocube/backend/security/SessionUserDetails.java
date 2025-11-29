package com.fitocube.backend.security;

import com.fitocube.backend.model.UserDto;
import com.fitocube.backend.model.session.SessionUser;
import java.util.Collection;
import java.util.List;
import org.springframework.security.core.GrantedAuthority;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.core.userdetails.UserDetails;

public class SessionUserDetails implements UserDetails {

    private final Long id;
    private final String username;
    private final String displayName;
    private final List<GrantedAuthority> authorities;

    public SessionUserDetails(UserDto user) {
        this.id = user.getUserId();
        this.username = user.getUserName();
        this.displayName = user.getDisplayName();
        this.authorities = List.of(new SimpleGrantedAuthority("ROLE_USER"));
    }

    public Long getId() {
        return id;
    }

    public String getDisplayName() {
        return displayName;
    }

    public SessionUser toSessionUser() {
        return new SessionUser(id, username, displayName);
    }

    @Override
    public Collection<? extends GrantedAuthority> getAuthorities() {
        return authorities;
    }

    @Override
    public String getPassword() {
        return "";
    }

    @Override
    public String getUsername() {
        return username;
    }

    @Override
    public boolean isAccountNonExpired() {
        return true;
    }

    @Override
    public boolean isAccountNonLocked() {
        return true;
    }

    @Override
    public boolean isCredentialsNonExpired() {
        return true;
    }

    @Override
    public boolean isEnabled() {
        return true;
    }
}

